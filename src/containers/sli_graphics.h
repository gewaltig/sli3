#ifndef SLI_GRAPHICS_H
#define SLI_GRAPHICS_H

#include <cstdint>
#include <stdexcept>
#include <string>

// Forward declarations to keep Cairo + SDL out of headers. Both are
// pulled in only by the .cpp and by the graphics module that needs to
// drive them. Everyone else dealing with a GraphicsContext sees only
// the opaque type + refcount surface.
struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;
typedef struct _cairo cairo_t;
typedef struct _cairo_surface cairo_surface_t;

namespace sli3
{

/**
 * Reference-counted wrapper around a Cairo drawing surface, routed to
 * one of four backends:
 *
 *   - WINDOW    : live SDL2 window + streaming texture (the /newpage
 *                 default; what the original MVP was). present()
 *                 uploads the surface to the texture and pumps Cocoa
 *                 events; pump_for blocks bounded.
 *   - OFFSCREEN : pure Cairo image surface, no SDL. present() is a
 *                 no-op. Backs /newoffscreen, useful for batch /
 *                 CI workflows where there is no display.
 *   - PDF, SVG  : file-backed Cairo surface (cairo_pdf_surface_create /
 *                 cairo_svg_surface_create). present() calls
 *                 cairo_show_page (starts a new page in the file).
 *                 close() flushes via cairo_surface_finish.
 *
 * All four backends share the same drawing-context interface (cr()),
 * so every drawing operator works uniformly regardless of where the
 * output goes. Only the lifecycle ops (/newpage variants, /showpage,
 * /closepage, /writepng) branch on backend.
 *
 * The Cairo CTM is configured for PostScript bottom-left origin
 * (cairo_translate(0,h); cairo_scale(1,-1)) once at construction;
 * SLI code uses page coordinates directly.
 *
 * Single-threaded: every method must run on the same thread that
 * called the factory (matches sli3's single-threaded model). Refcount
 * is intrusive (non-atomic). remove_reference returns the new count
 * and self-deletes at zero.
 *
 * Lifecycle: factories throw std::runtime_error if any backend call
 * fails (partially-constructed state is cleaned up by ~GraphicsContext
 * via the unique_ptr the factory holds before release). close() is
 * idempotent; the destructor calls close() if it hasn't run yet.
 *
 * Graphics contexts are not serializable -- OS resources / file
 * handles don't survive a snapshot. GraphicsContextType::serialize
 * warns and writes no payload; deserialize produces a Token whose
 * backing context has valid() == false.
 */
class GraphicsContext
{
public:
    enum class Backend { WINDOW, OFFSCREEN, PDF, SVG };

    static GraphicsContext* open_window   (int width, int height, std::string const& title);
    static GraphicsContext* open_offscreen(int width, int height);
    static GraphicsContext* open_pdf      (std::string const& path, int width, int height);
    static GraphicsContext* open_svg      (std::string const& path, int width, int height);

    /**
     * Load a PNG into an OFFSCREEN-backed context. Width/height come
     * from the PNG. The background paint is skipped so the loaded
     * pixels survive. The PS-style Y-flip is applied as usual, so
     * additional drawing into this context uses page coordinates
     * (and the file-as-loaded shows in its original orientation when
     * written back with /writepng).
     */
    static GraphicsContext* open_image(std::string const& path);

    GraphicsContext(GraphicsContext const&) = delete;
    GraphicsContext& operator=(GraphicsContext const&) = delete;

    ~GraphicsContext();

    std::uint32_t add_reference()    { return ++refs_; }
    std::uint32_t remove_reference()
    {
        if (--refs_ == 0)
        {
            delete this;
            return 0;
        }
        return refs_;
    }
    std::uint32_t references() const { return refs_; }

    Backend backend() const { return backend_; }
    int width()  const { return width_; }
    int height() const { return height_; }

    /** True until close() (or destructor) runs. */
    bool valid() const { return cr_ != nullptr; }

    /** True once the user closed the window. Only meaningful for WINDOW. */
    bool window_closed() const { return window_closed_; }

    /**
     * True if the backing surface is a pixel buffer (WINDOW or
     * OFFSCREEN). PDF/SVG surfaces have no readable pixel data, so
     * /writepng raises rather than trying to dump them.
     */
    bool is_image_surface() const
    {
        return backend_ == Backend::WINDOW || backend_ == Backend::OFFSCREEN;
    }

    /**
     * Drawing context -- already configured with PostScript-style
     * bottom-left origin. Caller must not destroy it. Returns nullptr
     * after close().
     */
    cairo_t* cr() const { return cr_; }

    /**
     * Backend-aware "present this page":
     *   WINDOW    -- upload the surface to the SDL texture, present,
     *                pump pending Cocoa events.
     *   PDF / SVG -- cairo_show_page (start a new page in the file).
     *   OFFSCREEN -- no-op.
     * No-op when closed.
     */
    void present();

    /**
     * Pump events for at most `max_seconds`, presenting on redraw
     * signals. WINDOW only -- the other backends have no event source.
     * Bounded by design: an unbounded blocking call is dangerous on
     * macOS where the SDL window may not deliver its own close events
     * back to a terminal-launched process.
     */
    void pump_for(double max_seconds);

    /**
     * Tear down the backend. Safe to call multiple times. After this
     * the wrapper is "invalid" (valid() == false) but the wrapper
     * memory survives until the refcount reaches zero so existing
     * Tokens don't dangle.
     */
    void close();

    /**
     * Write the current image surface to `path` as a PNG. Only valid
     * for image surfaces (WINDOW / OFFSCREEN); returns false otherwise
     * or on any Cairo error.
     */
    bool write_png(std::string const& path);

    /**
     * Borrow accessors for the underlying Cairo surface (every
     * backend) and the SDL window (WINDOW only; nullptr otherwise).
     * Used by ops like /drawimage (cairo_set_source_surface needs
     * the source) and /setwindowtitle (SDL_SetWindowTitle on the
     * underlying window).
     */
    cairo_surface_t* surface() const { return surface_; }
    SDL_Window*      sdl_window() const { return window_; }

    /**
     * WINDOW-only: resize the underlying SDL window + reallocate the
     * texture + reallocate the Cairo image surface at the new size.
     * The drawing context's CTM is reset to the new size's Y-flip
     * baseline (so subsequent drawing stays oriented correctly).
     * Throws on any backend failure. Raises std::runtime_error if the
     * backend isn't WINDOW.
     */
    void resize_window(int width, int height);

private:
    explicit GraphicsContext(Backend b, int width, int height);
    void finish_cairo_setup_(bool paint_background = true);
    void release_resources_();

    Backend       backend_;
    int           width_  = 0;
    int           height_ = 0;

    SDL_Window*      window_   = nullptr;
    SDL_Renderer*    renderer_ = nullptr;
    SDL_Texture*     texture_  = nullptr;
    cairo_surface_t* surface_  = nullptr;
    cairo_t*         cr_       = nullptr;

    bool window_closed_ = false;

    std::uint32_t refs_ = 1;
};

}  // namespace sli3

#endif
