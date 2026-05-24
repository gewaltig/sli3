#include "sli_graphicscontexttype.h"

#include "sli_serialize.h"

#include <iostream>
#include <string>

namespace sli3
{

bool GraphicsContextType::compare(Token const& t1, Token const& t2) const
{
    if (t1.type_ != t2.type_) return false;
    return t1.data_.graphics_val == t2.data_.graphics_val;
}

// Graphics contexts hold live SDL + Cairo resources — they can't
// survive a snapshot. Same shape as IstreamType / OstreamType: warn
// on serialize so the dropped slot is visible, and on deserialize
// produce a wrapper whose valid() is false so any subsequent
// drawing op fails cleanly. Note: this construction path is the only
// one where a context can exist without an open SDL window — every
// other path comes through /newpage which always opens one.
void GraphicsContextType::serialize(Token const&, Writer&) const
{
    std::cerr << "sli3: graphics context is not serializable; "
                 "replaced with closed context\n";
}

void GraphicsContextType::deserialize(Reader&, Token& t) const
{
    t.type_ = const_cast<GraphicsContextType*>(this);
    // No GraphicsContext ctor for "invalid" state — synthesize one
    // by opening a 1x1 window and immediately closing it. SDL_VIDEO
    // must be available for this; under a snapshot replay where SDL
    // can't init, we fall back to a null payload (any subsequent op
    // raises through the standard "missing context" guard).
    try
    {
        auto* g = new GraphicsContext(1, 1, std::string("sli3:invalid"));
        g->close();
        t.data_.graphics_val = g;
    }
    catch (std::exception const&)
    {
        t.data_.graphics_val = nullptr;
    }
}

}  // namespace sli3
