#include "sli_graphics.h"

#include <SDL2/SDL.h>
#include <cairo/cairo.h>
#include <cairo/cairo-pdf.h>
#include <cairo/cairo-svg.h>

#include <memory>

// IMPORTANT: do NOT call TransformProcessType / NSApp setActivationPolicy
// / SDL_RaiseWindow here. On macOS, elevating a terminal-launched sli3
// to a foreground GUI app steals keyboard + mouse focus from the
// hosting Terminal, and the process survives the terminal's close --
// the user is left with an unresponsive REPL and no way to ^C out.
// We accept that the SDL window opens *behind* the terminal and that
// its close button may not reach us cleanly; the safe escape is
// always the bounded /wait pump (no /interact blocking loop).

namespace sli3
{

namespace
{

// SDL's video subsystem stays initialized for the process lifetime
// once we've brought it up -- repeated SDL_InitSubSystem(VIDEO) /
// SDL_QuitSubSystem(VIDEO) churn around individual /newpage /
// /closepage calls would tear down the OS connection mid-program and
// stall on macOS. We init once on the first WINDOW context and let
// process exit reclaim it. Non-WINDOW backends don't touch SDL at all.
bool ensure_sdl_video_()
{
    if (SDL_WasInit(SDL_INIT_VIDEO)) return true;
    return SDL_InitSubSystem(SDL_INIT_VIDEO) == 0;
}

void throw_cairo_status_(cairo_surface_t* s, char const* what)
{
    cairo_status_t st = cairo_surface_status(s);
    throw std::runtime_error(std::string(what) + ": "
                             + cairo_status_to_string(st));
}

}  // namespace

GraphicsContext::GraphicsContext(Backend b, int width, int height)
    : backend_(b), width_(width), height_(height)
{
    if (width <= 0 || height <= 0)
        throw std::runtime_error("GraphicsContext: width and height must be positive");
}

GraphicsContext::~GraphicsContext()
{
    // close() is idempotent and handles PDF/SVG flush + WINDOW hide.
    // Calling it here matches the header's lifecycle contract so a
    // refcount-driven destruction (no explicit /closepage) still
    // finalises the file backend gracefully.
    close();
}

void GraphicsContext::finish_cairo_setup_(bool paint_background)
{
    cr_ = cairo_create(surface_);
    if (cairo_status(cr_) != CAIRO_STATUS_SUCCESS)
        throw std::runtime_error("cairo_create failed");

    // Opaque white background -- only meaningful for image surfaces,
    // and skipped when the surface was preloaded with content (e.g.
    // /loadpng) so we don't clobber the loaded pixels.
    if (is_image_surface() && paint_background)
    {
        cairo_save(cr_);
        cairo_set_source_rgb(cr_, 1.0, 1.0, 1.0);
        cairo_paint(cr_);
        cairo_restore(cr_);
    }

    // Hi-DPI: scale all logical user-space units by device_scale_
    // before the Y-flip baseline. Surface is allocated at
    // (width_ * device_scale_, height_ * device_scale_) pixels, but
    // user code keeps drawing in (width_, height_) coordinates.
    if (device_scale_ != 1.0)
        cairo_scale(cr_, device_scale_, device_scale_);

    // PostScript bottom-left origin. Applied once before any user
    // operator touches the context; cairo_save / cairo_restore via
    // /gsave / /grestore preserve the base CTM.
    cairo_translate(cr_, 0.0, static_cast<double>(height_));
    cairo_scale(cr_, 1.0, -1.0);
}

int GraphicsContext::pixel_width() const
{
    if (surface_ && is_image_surface())
        return cairo_image_surface_get_width(surface_);
    return static_cast<int>(width_ * device_scale_);
}

int GraphicsContext::pixel_height() const
{
    if (surface_ && is_image_surface())
        return cairo_image_surface_get_height(surface_);
    return static_cast<int>(height_ * device_scale_);
}

GraphicsContext* GraphicsContext::open_window(int width, int height, std::string const& title)
{
    std::unique_ptr<GraphicsContext> g(new GraphicsContext(Backend::WINDOW, width, height));

    if (!ensure_sdl_video_())
        throw std::runtime_error(std::string("SDL_InitSubSystem(VIDEO) failed: ") + SDL_GetError());

    g->window_ = SDL_CreateWindow(title.c_str(),
                                  SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                  width, height,
                                  SDL_WINDOW_SHOWN);
    if (!g->window_)
        throw std::runtime_error(std::string("SDL_CreateWindow failed: ") + SDL_GetError());

    g->renderer_ = SDL_CreateRenderer(g->window_, -1, SDL_RENDERER_ACCELERATED);
    if (!g->renderer_)
        g->renderer_ = SDL_CreateRenderer(g->window_, -1, SDL_RENDERER_SOFTWARE);
    if (!g->renderer_)
        throw std::runtime_error(std::string("SDL_CreateRenderer failed: ") + SDL_GetError());

    g->texture_ = SDL_CreateTexture(g->renderer_, SDL_PIXELFORMAT_ARGB8888,
                                    SDL_TEXTUREACCESS_STREAMING, width, height);
    if (!g->texture_)
        throw std::runtime_error(std::string("SDL_CreateTexture failed: ") + SDL_GetError());

    g->surface_ = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    if (cairo_surface_status(g->surface_) != CAIRO_STATUS_SUCCESS)
        throw_cairo_status_(g->surface_, "cairo_image_surface_create failed");

    g->finish_cairo_setup_();
    return g.release();
}

GraphicsContext* GraphicsContext::open_offscreen(int width, int height)
{
    std::unique_ptr<GraphicsContext> g(new GraphicsContext(Backend::OFFSCREEN, width, height));
    g->surface_ = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    if (cairo_surface_status(g->surface_) != CAIRO_STATUS_SUCCESS)
        throw_cairo_status_(g->surface_, "cairo_image_surface_create failed");
    g->finish_cairo_setup_();
    return g.release();
}

GraphicsContext* GraphicsContext::open_hidpi_offscreen(int width, int height, double scale)
{
    if (scale <= 0.0)
        throw std::runtime_error("hidpi_offscreen: scale must be positive");
    int pw = static_cast<int>(width  * scale);
    int ph = static_cast<int>(height * scale);
    if (pw <= 0 || ph <= 0)
        throw std::runtime_error("hidpi_offscreen: resulting pixel size is zero");
    std::unique_ptr<GraphicsContext> g(new GraphicsContext(Backend::OFFSCREEN, width, height));
    g->device_scale_ = scale;
    g->surface_ = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, pw, ph);
    if (cairo_surface_status(g->surface_) != CAIRO_STATUS_SUCCESS)
        throw_cairo_status_(g->surface_, "cairo_image_surface_create failed");
    g->finish_cairo_setup_();
    return g.release();
}

GraphicsContext* GraphicsContext::open_image(std::string const& path)
{
    // Load the PNG first so we can derive width/height from it.
    cairo_surface_t* surf = cairo_image_surface_create_from_png(path.c_str());
    if (cairo_surface_status(surf) != CAIRO_STATUS_SUCCESS)
    {
        cairo_status_t st = cairo_surface_status(surf);
        std::string msg = std::string("cairo_image_surface_create_from_png failed: ")
                          + cairo_status_to_string(st);
        cairo_surface_destroy(surf);
        throw std::runtime_error(msg);
    }
    int w = cairo_image_surface_get_width(surf);
    int h = cairo_image_surface_get_height(surf);
    // Validate dims BEFORE handing the surface to GraphicsContext --
    // the ctor throws on non-positive dims, and at that point `surf`
    // would leak because no owner exists yet.
    if (w <= 0 || h <= 0)
    {
        cairo_surface_destroy(surf);
        throw std::runtime_error("open_image: PNG has zero width or height");
    }
    std::unique_ptr<GraphicsContext> g(new GraphicsContext(Backend::OFFSCREEN, w, h));
    g->surface_ = surf;
    // false: do NOT paint the white background -- we want the PNG's
    // pixels to survive.
    g->finish_cairo_setup_(false);
    return g.release();
}

GraphicsContext* GraphicsContext::open_pdf(std::string const& path, int width, int height)
{
    std::unique_ptr<GraphicsContext> g(new GraphicsContext(Backend::PDF, width, height));
    g->surface_ = cairo_pdf_surface_create(path.c_str(),
                                           static_cast<double>(width),
                                           static_cast<double>(height));
    if (cairo_surface_status(g->surface_) != CAIRO_STATUS_SUCCESS)
        throw_cairo_status_(g->surface_, "cairo_pdf_surface_create failed");
    g->finish_cairo_setup_();
    return g.release();
}

GraphicsContext* GraphicsContext::open_svg(std::string const& path, int width, int height)
{
    std::unique_ptr<GraphicsContext> g(new GraphicsContext(Backend::SVG, width, height));
    g->surface_ = cairo_svg_surface_create(path.c_str(),
                                           static_cast<double>(width),
                                           static_cast<double>(height));
    if (cairo_surface_status(g->surface_) != CAIRO_STATUS_SUCCESS)
        throw_cairo_status_(g->surface_, "cairo_svg_surface_create failed");
    g->finish_cairo_setup_();
    return g.release();
}

void GraphicsContext::close()
{
    if (backend_ == Backend::WINDOW && window_)
    {
        // SDL_DestroyWindow only QUEUES the platform tear-down -- on
        // macOS the NSWindow actually disappears the next time the
        // Cocoa event source runs. Hide + pump + flush so it
        // disappears immediately and the events generated by the hide
        // don't pile up in the SDL queue across many open/close
        // cycles. Same flush after destroy to drain the destroy-side
        // events.
        SDL_HideWindow(window_);
        SDL_PumpEvents();
        SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);
    }
    if (surface_ && (backend_ == Backend::PDF || backend_ == Backend::SVG))
    {
        // Flush the file. Must happen BEFORE cairo_surface_destroy so
        // partially-written PDF/SVG state is committed; cairo_destroy
        // alone leaves the file unwritten.
        cairo_surface_finish(surface_);
    }
    release_resources_();
    if (backend_ == Backend::WINDOW)
    {
        SDL_PumpEvents();
        SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);
    }
}

void GraphicsContext::release_resources_()
{
    if (cr_)        { cairo_destroy(cr_); cr_ = nullptr; }
    if (surface_)   { cairo_surface_destroy(surface_); surface_ = nullptr; }
    if (texture_)   { SDL_DestroyTexture(texture_); texture_ = nullptr; }
    if (renderer_)  { SDL_DestroyRenderer(renderer_); renderer_ = nullptr; }
    if (window_)    { SDL_DestroyWindow(window_); window_ = nullptr; }
}

void GraphicsContext::present()
{
    if (!cr_) return;

    if (backend_ == Backend::WINDOW)
    {
        cairo_surface_flush(surface_);
        unsigned char* pixels = cairo_image_surface_get_data(surface_);
        int stride = cairo_image_surface_get_stride(surface_);
        SDL_UpdateTexture(texture_, nullptr, pixels, stride);
        SDL_RenderClear(renderer_);
        SDL_RenderCopy(renderer_, texture_, nullptr, nullptr);
        SDL_RenderPresent(renderer_);

        SDL_Event ev;
        while (SDL_PollEvent(&ev))
        {
            if (ev.type == SDL_QUIT)
                window_closed_ = true;
            else if (ev.type == SDL_WINDOWEVENT
                     && ev.window.event == SDL_WINDOWEVENT_CLOSE)
                window_closed_ = true;
        }
    }
    else if (backend_ == Backend::PDF || backend_ == Backend::SVG)
    {
        // PostScript /showpage on a file backend ends the current page
        // and starts a fresh one. Multi-page output is then just
        // repeated draw + showpage; the file is finalized by
        // /closepage.
        cairo_show_page(cr_);
    }
    // OFFSCREEN: no-op.
}

void GraphicsContext::pump_for(double max_seconds)
{
    if (!cr_) return;

    present();

    // Non-WINDOW backends have no event source -- nothing to pump.
    if (backend_ != Backend::WINDOW) return;

    if (max_seconds <= 0.0) return;

    Uint64 freq = SDL_GetPerformanceFrequency();
    Uint64 start = SDL_GetPerformanceCounter();
    Uint64 deadline_ticks = static_cast<Uint64>(max_seconds * static_cast<double>(freq));

    while (!window_closed_)
    {
        Uint64 now = SDL_GetPerformanceCounter() - start;
        if (now >= deadline_ticks) break;

        SDL_Event ev;
        int got = SDL_WaitEventTimeout(&ev, 30);
        if (got == 0) continue;
        do
        {
            if (ev.type == SDL_QUIT)
                window_closed_ = true;
            else if (ev.type == SDL_WINDOWEVENT
                     && ev.window.event == SDL_WINDOWEVENT_CLOSE)
                window_closed_ = true;
            else if (ev.type == SDL_WINDOWEVENT
                     && (ev.window.event == SDL_WINDOWEVENT_EXPOSED
                         || ev.window.event == SDL_WINDOWEVENT_SHOWN))
            {
                SDL_RenderClear(renderer_);
                SDL_RenderCopy(renderer_, texture_, nullptr, nullptr);
                SDL_RenderPresent(renderer_);
            }
            // Intentionally NO keyboard dismissal: keys typed into
            // the parent terminal may bleed into our event queue on
            // some macOS configurations.
        }
        while (SDL_PollEvent(&ev));
    }
}

CairoPattern::~CairoPattern()
{
    if (p_) cairo_pattern_destroy(p_);
}

bool GraphicsContext::write_png(std::string const& path)
{
    if (!cr_ || !is_image_surface()) return false;
    cairo_surface_flush(surface_);
    cairo_status_t st = cairo_surface_write_to_png(surface_, path.c_str());
    return st == CAIRO_STATUS_SUCCESS;
}

void GraphicsContext::resize_window(int width, int height)
{
    if (backend_ != Backend::WINDOW)
        throw std::runtime_error("resize_window: not a WINDOW-backed context");
    if (!window_)
        throw std::runtime_error("resize_window: window is closed");
    if (width <= 0 || height <= 0)
        throw std::runtime_error("resize_window: width and height must be positive");

    // Build the new texture + surface + cairo_t first, swap atomically
    // only once everything has succeeded. On any failure mid-build the
    // old context is left intact and valid() stays true, so the SDL
    // window doesn't become a zombie.
    SDL_Texture* new_texture = SDL_CreateTexture(renderer_,
                                                 SDL_PIXELFORMAT_ARGB8888,
                                                 SDL_TEXTUREACCESS_STREAMING,
                                                 width, height);
    if (!new_texture)
        throw std::runtime_error(std::string("SDL_CreateTexture failed: ") + SDL_GetError());

    cairo_surface_t* new_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
                                                              width, height);
    if (cairo_surface_status(new_surface) != CAIRO_STATUS_SUCCESS)
    {
        std::string msg = std::string("cairo_image_surface_create failed: ")
                          + cairo_status_to_string(cairo_surface_status(new_surface));
        cairo_surface_destroy(new_surface);
        SDL_DestroyTexture(new_texture);
        throw std::runtime_error(msg);
    }

    cairo_t* new_cr = cairo_create(new_surface);
    if (cairo_status(new_cr) != CAIRO_STATUS_SUCCESS)
    {
        cairo_destroy(new_cr);
        cairo_surface_destroy(new_surface);
        SDL_DestroyTexture(new_texture);
        throw std::runtime_error("cairo_create failed");
    }

    // Commit: tear the OS down to the new size, drop the old resources,
    // install the new ones, re-apply the PS Y-flip baseline.
    SDL_SetWindowSize(window_, width, height);

    if (cr_)      cairo_destroy(cr_);
    if (surface_) cairo_surface_destroy(surface_);
    if (texture_) SDL_DestroyTexture(texture_);

    cr_      = new_cr;
    surface_ = new_surface;
    texture_ = new_texture;
    width_   = width;
    height_  = height;

    // Re-apply baseline directly (don't re-call finish_cairo_setup_
    // because that creates a fresh cr_ and would leak new_cr).
    if (device_scale_ != 1.0)
        cairo_scale(cr_, device_scale_, device_scale_);
    cairo_translate(cr_, 0.0, static_cast<double>(height_));
    cairo_scale(cr_, 1.0, -1.0);
}

}  // namespace sli3
