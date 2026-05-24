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
 * Reference-counted wrapper around the SDL window + Cairo surface that
 * a /newpage call brings up. Owns: an SDL window, its renderer, a
 * streaming ARGB texture sized to the requested page, a Cairo image
 * surface whose pixel buffer is the texture's backing memory, and a
 * cairo_t drawing context with PostScript's bottom-left origin
 * (`cairo_translate(cr, 0, h); cairo_scale(cr, 1, -1);` applied once).
 *
 * Single-threaded: every method must run on the same thread that
 * called the constructor (matches sli3's single-threaded model). All
 * SDL + Cairo calls happen here so the operator layer never sees the
 * underlying APIs.
 *
 * Refcount is intrusive (non-atomic). GraphicsContextType drives
 * add_reference / remove_reference on Token copy and destruction.
 * remove_reference returns the new count and self-deletes at zero,
 * matching SLIistream / SLIostream.
 *
 * Lifecycle of the underlying window:
 *   - construction opens the window and primes the Cairo surface;
 *     throws std::runtime_error on any backend failure (SDL_Init,
 *     window/renderer/texture creation, Cairo surface allocation).
 *   - close() tears the window down early but keeps the wrapper alive
 *     so any lingering Token references can fail cleanly via valid().
 *     Idempotent.
 *   - the destructor calls close() if it hasn't run yet.
 *
 * Graphics contexts are not serializable — OS resources don't survive
 * a snapshot. GraphicsContextType::serialize warns and writes no
 * payload; deserialize produces a Token whose backing context has
 * valid() == false.
 */
class GraphicsContext
{
public:
    /**
     * Create a window of the given size with the given title and
     * prime the Cairo drawing context. Background is cleared to
     * opaque white. Throws std::runtime_error if any backend call
     * fails (the partially-constructed wrapper releases what it
     * already acquired before propagating).
     */
    GraphicsContext(int width, int height, std::string const& title);

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

    int width()  const { return width_; }
    int height() const { return height_; }

    /** True until close() (or destructor) runs. */
    bool valid() const { return cr_ != nullptr; }

    /** True once the window has been asked to close by the user. */
    bool window_closed() const { return window_closed_; }

    /**
     * Drawing context — already configured with PostScript-style
     * bottom-left origin. Caller must not destroy it. Returns nullptr
     * after close().
     */
    cairo_t* cr() const { return cr_; }

    /**
     * Blit the current surface to the SDL texture and present.
     * Cooperatively pumps pending events so the window stays
     * responsive between drawing turns. No-op when closed.
     */
    void present();

    /**
     * Pump events for at most `max_seconds`, presenting on redraw
     * signals. Returns early if the window is closed. Bounded by
     * design — sli3 is a REPL, and an unbounded blocking call
     * (the previous /interact) is dangerous on macOS where the SDL
     * window may not deliver its own close events back to a
     * terminal-launched process.
     */
    void pump_for(double max_seconds);

    /**
     * Tear down the window and Cairo surface. Safe to call multiple
     * times. After this the wrapper is "invalid" (valid() == false)
     * but the wrapper memory survives until the refcount reaches
     * zero so existing Tokens don't dangle.
     */
    void close();

private:
    void release_resources_();

    int width_  = 0;
    int height_ = 0;

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
