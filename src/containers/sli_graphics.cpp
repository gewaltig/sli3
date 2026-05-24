#include "sli_graphics.h"

#include <SDL2/SDL.h>
#include <cairo/cairo.h>

// IMPORTANT: do NOT call TransformProcessType / NSApp setActivationPolicy
// / SDL_RaiseWindow here. On macOS, elevating a terminal-launched sli3
// to a foreground GUI app steals keyboard + mouse focus from the
// hosting Terminal, and the process survives the terminal's close —
// the user is left with an unresponsive REPL and no way to ^C out.
// We accept that the SDL window opens *behind* the terminal and that
// its close button may not reach us cleanly; the safe escape is
// always the bounded /wait pump (no /interact blocking loop).

namespace sli3
{

namespace
{

// SDL's video subsystem stays initialized for the process lifetime
// once we've brought it up — repeated SDL_InitSubSystem(VIDEO) /
// SDL_QuitSubSystem(VIDEO) churn around individual /newpage /
// /closepage calls would tear down the OS connection mid-program and
// stall on macOS. We init once on the first GraphicsContext and let
// process exit reclaim it.
bool ensure_sdl_video_()
{
    if (SDL_WasInit(SDL_INIT_VIDEO)) return true;
    return SDL_InitSubSystem(SDL_INIT_VIDEO) == 0;
}

}  // namespace

GraphicsContext::GraphicsContext(int width, int height, std::string const& title)
    : width_(width), height_(height)
{
    if (width <= 0 || height <= 0)
        throw std::runtime_error("GraphicsContext: width and height must be positive");

    if (!ensure_sdl_video_())
        throw std::runtime_error(std::string("SDL_InitSubSystem(VIDEO) failed: ") + SDL_GetError());

    window_ = SDL_CreateWindow(title.c_str(),
                               SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                               width, height,
                               SDL_WINDOW_SHOWN);
    if (!window_)
    {
        std::string err = SDL_GetError();
        release_resources_();
        throw std::runtime_error("SDL_CreateWindow failed: " + err);
    }

    // Intentionally NOT calling SDL_RaiseWindow / activating focus:
    // see the comment at the top of this file. The window opens
    // wherever the WM decides, which is fine — it's discoverable in
    // Mission Control / the Dock, and not raising avoids any chance
    // of stealing input from the parent terminal.

    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer_)
    {
        // Fall back to software renderer — useful under SDL_VIDEODRIVER=dummy
        // (CI / headless) where the accelerated path isn't available.
        renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_SOFTWARE);
    }
    if (!renderer_)
    {
        std::string err = SDL_GetError();
        release_resources_();
        throw std::runtime_error("SDL_CreateRenderer failed: " + err);
    }

    texture_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_ARGB8888,
                                 SDL_TEXTUREACCESS_STREAMING, width, height);
    if (!texture_)
    {
        std::string err = SDL_GetError();
        release_resources_();
        throw std::runtime_error("SDL_CreateTexture failed: " + err);
    }

    // Cairo allocates the pixel buffer itself; we hand it off to SDL
    // each present() via SDL_UpdateTexture. ARGB32 (Cairo) and
    // SDL_PIXELFORMAT_ARGB8888 (SDL) are the same on little-endian:
    // BGRA byte order in memory, alpha in the high byte of the word.
    surface_ = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    if (cairo_surface_status(surface_) != CAIRO_STATUS_SUCCESS)
    {
        release_resources_();
        throw std::runtime_error("cairo_image_surface_create failed");
    }

    cr_ = cairo_create(surface_);
    if (cairo_status(cr_) != CAIRO_STATUS_SUCCESS)
    {
        release_resources_();
        throw std::runtime_error("cairo_create failed");
    }

    // Opaque white background — what an empty PostScript page would show.
    cairo_save(cr_);
    cairo_set_source_rgb(cr_, 1.0, 1.0, 1.0);
    cairo_paint(cr_);
    cairo_restore(cr_);

    // PostScript-style bottom-left origin. Applied once before any
    // user operator touches the context. Drawing ops can still use
    // cairo_save/cairo_restore via /gsave /grestore; the base CTM
    // we set here is preserved by Cairo's CTM stack.
    cairo_translate(cr_, 0.0, static_cast<double>(height));
    cairo_scale(cr_, 1.0, -1.0);
}

GraphicsContext::~GraphicsContext()
{
    release_resources_();
}

void GraphicsContext::close()
{
    // SDL_DestroyWindow only QUEUES the platform tear-down — on macOS
    // the NSWindow actually disappears the next time the Cocoa
    // event source runs. If no further SDL call happens (the user
    // just types `currentpage closepage` and gets the prompt back)
    // the window stays visible until something else wakes the event
    // loop. Hide first + pump so it disappears immediately, then
    // destroy, then pump again to drain the teardown.
    if (window_)
    {
        SDL_HideWindow(window_);
        SDL_PumpEvents();
    }
    release_resources_();
    SDL_PumpEvents();
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

    cairo_surface_flush(surface_);
    unsigned char* pixels = cairo_image_surface_get_data(surface_);
    int stride = cairo_image_surface_get_stride(surface_);
    SDL_UpdateTexture(texture_, nullptr, pixels, stride);
    SDL_RenderClear(renderer_);
    SDL_RenderCopy(renderer_, texture_, nullptr, nullptr);
    SDL_RenderPresent(renderer_);

    // Cooperative event pump so the window can be dragged, brought to
    // front, etc. between drawing turns. SDL_QUIT or a close button
    // flips window_closed_ so subsequent ops can check.
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

void GraphicsContext::pump_for(double max_seconds)
{
    if (!cr_) return;

    present();

    if (max_seconds <= 0.0) return;

    // Hard deadline. Even if events never arrive (a real risk on
    // macOS for non-app processes), we always return after this.
    Uint64 freq = SDL_GetPerformanceFrequency();
    Uint64 start = SDL_GetPerformanceCounter();
    Uint64 deadline_ticks = static_cast<Uint64>(max_seconds * static_cast<double>(freq));

    while (!window_closed_)
    {
        Uint64 now = SDL_GetPerformanceCounter() - start;
        if (now >= deadline_ticks) break;

        // ~30 ms slice keeps the loop responsive without busy-waiting.
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
            // some macOS configurations; treating any of them as
            // "close the window" would surprise the user.
        }
        while (SDL_PollEvent(&ev));
    }
}

}  // namespace sli3
