// PostScript-like 2D graphics on top of Cairo + SDL2.
//
// Each operator follows the sli3 operator authoring contract
// (validate first, peek operands, pop on success, pop self from
// estack on success path). The currently-targeted GraphicsContext
// lives in globaldict under /currentpage so drawing ops can read
// PS-style (no explicit context threaded through every call).
//
// Coordinate system: PostScript's bottom-left origin. The Y flip
// is set once on GraphicsContext construction; ops just hand x/y
// straight to Cairo.

#include "sli_graphics_module.h"

#include "sli_dictionary.h"
#include "sli_function.h"
#include "sli_graphics.h"
#include "sli_graphicscontexttype.h"
#include "sli_interpreter.h"
#include "sli_patterntype.h"
#include "sli_name.h"
#include "sli_string.h"
#include "sli_token.h"
#include "sli_type.h"

#include <cairo/cairo.h>
#include <SDL2/SDL.h>
#include <fontconfig/fontconfig.h>

#include <cmath>
#include <exception>
#include <set>
#include <string>
#include <vector>

namespace sli3
{
namespace
{

//------------------------------------------------------------------------
// Helpers
//------------------------------------------------------------------------

// Stored in /gfxstatus -- a dedicated dictionary the graphics
// module installs in systemdict at init time. Holding internal
// runtime state in its own namespace keeps it out of globaldict
// and lets it be inspected as a single unit (`gfxstatus pstack`,
// `gfxstatus keys`).
Name const& current_page_slot()
{
    static Name n("currentpage");
    return n;
}

bool is_numeric(Token const& t)
{
    return t.is_of_type(sli3::integertype) || t.is_of_type(sli3::doubletype);
}

double as_double(Token const& t)
{
    if (t.is_of_type(sli3::doubletype))   return t.data_.double_val;
    if (t.is_of_type(sli3::integertype))  return static_cast<double>(t.data_.long_val);
    return 0.0;  // unreachable after is_numeric checks
}

// Resolve /gfxstatus via dictstack lookup. The dict is installed
// in systemdict by install_gfxstatus_dict_ at init time, so the
// lookup always succeeds in a healthy interpreter.
Dictionary* get_gfxstatus(SLIInterpreter* i)
{
    Token g;
    if (!i->lookup(Name("gfxstatus"), g)) return nullptr;
    if (!g.is_of_type(sli3::dictionarytype)) return nullptr;
    return g.data_.dict_val;
}

// Read /currentpage from globaldict, returning nullptr if absent or
// the entry has been invalidated by /closepage. Does NOT raise; the
// caller decides what to do.
GraphicsContext* peek_current_gc(SLIInterpreter* i)
{
    Dictionary* gd = get_gfxstatus(i);
    if (!gd) return nullptr;
    TokenMap::iterator it = gd->find(current_page_slot());
    if (it == gd->end()) return nullptr;
    Token const& t = it->second;
    if (!t.is_of_type(sli3::graphicscontexttype)) return nullptr;
    GraphicsContext* g = t.data_.graphics_val;
    if (g == nullptr || !g->valid()) return nullptr;
    return g;
}

// Lookup or raise. Used by every drawing op that targets the current
// page. On miss / invalid, raises and returns nullptr — the caller
// must check and return immediately so it doesn't pop the estack twice.
GraphicsContext* require_current_gc(SLIInterpreter* i, Name op_name)
{
    GraphicsContext* g = peek_current_gc(i);
    if (g) return g;
    i->raiseerror(op_name, Name("NoCurrentPageError"));
    return nullptr;
}

// Save / clear the /currentpage slot.
void set_current_page(SLIInterpreter* i, Token const& gc_tok)
{
    Dictionary* gd = get_gfxstatus(i);
    if (gd) gd->insert(current_page_slot(), gc_tok);
}

// Stored in /gfxstatus alongside /currentpage. Mirrors PS's currentfont.
Name const& current_font_slot()
{
    static Name n("currentfont");
    return n;
}

// String coercion from a stringtype / literaltype / nametype Token.
// Returns false (without raising) if the operand is something else;
// the caller decides which error to raise.
bool token_to_string(Token const& t, std::string& out)
{
    if (t.is_of_type(sli3::stringtype))
    {
        out = t.data_.string_val->str();
        return true;
    }
    if (t.is_of_type(sli3::literaltype) || t.is_of_type(sli3::nametype))
    {
        out = Name(t.data_.name_val).toString();
        return true;
    }
    return false;
}

// HSB -> RGB (PostScript convention, components in [0,1]). Matches the
// algorithm in GS's gs_setrgbcolor.ps. Saturation/value clamped on input.
void hsb_to_rgb(double h, double s, double v, double& r, double& g, double& b)
{
    if (s < 0.0) s = 0.0; else if (s > 1.0) s = 1.0;
    if (v < 0.0) v = 0.0; else if (v > 1.0) v = 1.0;
    h -= std::floor(h);  // wrap to [0,1)
    double const h6 = h * 6.0;
    int const sector = static_cast<int>(h6) % 6;
    double const f = h6 - std::floor(h6);
    double const p = v * (1.0 - s);
    double const q = v * (1.0 - s * f);
    double const t = v * (1.0 - s * (1.0 - f));
    switch (sector)
    {
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        default: r = v; g = p; b = q; break;
    }
}

// Dict accessors used by /setfont. Each returns false on miss or wrong
// type so the caller can fall back to defaults.
bool dict_get_string(Dictionary* d, Name n, std::string& out)
{
    Token t;
    if (!d->lookup(n, t)) return false;
    return token_to_string(t, out);
}

bool dict_get_name(Dictionary* d, Name n, Name& out)
{
    Token t;
    if (!d->lookup(n, t)) return false;
    if (t.is_of_type(sli3::literaltype) || t.is_of_type(sli3::nametype))
    {
        out = Name(t.data_.name_val);
        return true;
    }
    return false;
}

bool dict_get_double(Dictionary* d, Name n, double& out)
{
    Token t;
    if (!d->lookup(n, t)) return false;
    if (t.is_of_type(sli3::doubletype))  { out = t.data_.double_val; return true; }
    if (t.is_of_type(sli3::integertype)) { out = static_cast<double>(t.data_.long_val); return true; }
    return false;
}

void clear_current_page(SLIInterpreter* i, GraphicsContext const* expected)
{
    Dictionary* gd = get_gfxstatus(i);
    if (!gd) return;
    TokenMap::iterator it = gd->find(current_page_slot());
    if (it == gd->end()) return;
    Token const& t = it->second;
    if (t.is_of_type(sli3::graphicscontexttype)
        && t.data_.graphics_val == expected)
        gd->erase(it);
}

//------------------------------------------------------------------------
// Surface lifecycle
//------------------------------------------------------------------------

// `w h newpage -> gc`
class NewPageFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(2);
        if (!is_numeric(i->pick(1)) || !is_numeric(i->pick(0)))
        {
            i->raiseerror(i->ArgumentTypeError);
            return;
        }
        long w = static_cast<long>(as_double(i->pick(1)));
        long h = static_cast<long>(as_double(i->pick(0)));
        if (w <= 0 || h <= 0)
        {
            i->raiseerror(i->RangeCheckError);
            return;
        }

        GraphicsContext* g = nullptr;
        try
        {
            g = GraphicsContext::open_window(static_cast<int>(w), static_cast<int>(h),
                                             std::string("sli3"));
        }
        catch (std::exception const& e)
        {
            i->message(sli3::M_ERROR, "newpage", e.what());
            i->raiseerror(Name("newpage"), Name("GraphicsBackendError"));
            return;
        }

        Token tok(i->get_type(sli3::graphicscontexttype));
        tok.data_.graphics_val = g;
        // Set /currentpage BEFORE the operand stack is touched so a
        // subsequent exception leaves a consistent /currentpage <-> ostack
        // pairing. Token copy bumps the refcount; the original `tok`
        // (still refcount 1 from `new`) plus the dict copy (refcount 2)
        // plus the eventual stack push (refcount 3) wash out as ops run.
        set_current_page(i, tok);
        i->pop(2);
        i->push(tok);
    }
};

// `w h newoffscreen -> gc` -- create a windowless Cairo image surface.
// Same semantics as /newpage but no SDL window opens, so it works in
// CI / headless contexts and can be paired with /writepng for batch
// PNG generation.
class NewOffscreenFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(2);
        if (!is_numeric(i->pick(1)) || !is_numeric(i->pick(0)))
        {
            i->raiseerror(i->ArgumentTypeError);
            return;
        }
        long w = static_cast<long>(as_double(i->pick(1)));
        long h = static_cast<long>(as_double(i->pick(0)));
        if (w <= 0 || h <= 0) { i->raiseerror(i->RangeCheckError); return; }
        GraphicsContext* g = nullptr;
        try
        {
            g = GraphicsContext::open_offscreen(static_cast<int>(w),
                                                static_cast<int>(h));
        }
        catch (std::exception const& e)
        {
            i->message(sli3::M_ERROR, "newoffscreen", e.what());
            i->raiseerror(Name("newoffscreen"), Name("GraphicsBackendError"));
            return;
        }
        Token tok(i->get_type(sli3::graphicscontexttype));
        tok.data_.graphics_val = g;
        set_current_page(i, tok);
        i->pop(2);
        i->push(tok);
    }
};

// Shared body for `(path) w h newpdf|newsvg -> gc`.
template <GraphicsContext* (*Open)(std::string const&, int, int)>
class NewFileBackendFunction : public SLIFunction
{
public:
    NewFileBackendFunction(Name op_name) : op_name_(op_name) {}
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(3);
        if (!i->pick(2).is_of_type(sli3::stringtype)
            || !is_numeric(i->pick(1))
            || !is_numeric(i->pick(0)))
        {
            i->raiseerror(i->ArgumentTypeError);
            return;
        }
        std::string path = i->pick(2).data_.string_val->str();
        long w = static_cast<long>(as_double(i->pick(1)));
        long h = static_cast<long>(as_double(i->pick(0)));
        if (w <= 0 || h <= 0) { i->raiseerror(i->RangeCheckError); return; }
        GraphicsContext* g = nullptr;
        try
        {
            g = Open(path, static_cast<int>(w), static_cast<int>(h));
        }
        catch (std::exception const& e)
        {
            i->message(sli3::M_ERROR, op_name_.toString().c_str(), e.what());
            i->raiseerror(op_name_, Name("GraphicsBackendError"));
            return;
        }
        Token tok(i->get_type(sli3::graphicscontexttype));
        tok.data_.graphics_val = g;
        set_current_page(i, tok);
        i->pop(3);
        i->push(tok);
    }
private:
    Name op_name_;
};

// `(path) gc writepng -> -`. Only valid for image-backed surfaces;
// raises UnsupportedSurfaceError on PDF/SVG.
class WritePngFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(2);
        if (!i->pick(1).is_of_type(sli3::stringtype)
            || !i->pick(0).is_of_type(sli3::graphicscontexttype))
        {
            i->raiseerror(i->ArgumentTypeError);
            return;
        }
        std::string path = i->pick(1).data_.string_val->str();
        GraphicsContext* g = i->pick(0).data_.graphics_val;
        if (!g || !g->valid())
        {
            i->raiseerror(Name("writepng"), Name("NoCurrentPageError"));
            return;
        }
        if (!g->is_image_surface())
        {
            i->raiseerror(Name("writepng"), Name("UnsupportedSurfaceError"));
            return;
        }
        if (!g->write_png(path))
        {
            i->raiseerror(Name("writepng"), Name("WritePngError"));
            return;
        }
        i->pop(2);
    }
};

// `w h scale newhidpioffscreen -> gc`. Same as /newoffscreen but with
// a backing pixel buffer of (w*scale, h*scale). Drawing happens in
// the original logical (w, h) frame; the larger buffer is invisible
// to user code until /writepng dumps it. Useful for crisp rendering
// on Retina / high-DPI displays where 1:1 PNGs look soft.
class NewHidpiOffscreenFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(3);
        if (!is_numeric(i->pick(2)) || !is_numeric(i->pick(1))
            || !is_numeric(i->pick(0)))
        {
            i->raiseerror(i->ArgumentTypeError);
            return;
        }
        long w = static_cast<long>(as_double(i->pick(2)));
        long h = static_cast<long>(as_double(i->pick(1)));
        double s = as_double(i->pick(0));
        if (w <= 0 || h <= 0 || s <= 0.0)
        {
            i->raiseerror(i->RangeCheckError);
            return;
        }
        GraphicsContext* g = nullptr;
        try
        {
            g = GraphicsContext::open_hidpi_offscreen(
                    static_cast<int>(w), static_cast<int>(h), s);
        }
        catch (std::exception const& e)
        {
            i->message(sli3::M_ERROR, "newhidpioffscreen", e.what());
            i->raiseerror(Name("newhidpioffscreen"),
                          Name("GraphicsBackendError"));
            return;
        }
        Token tok(i->get_type(sli3::graphicscontexttype));
        tok.data_.graphics_val = g;
        set_current_page(i, tok);
        i->pop(3);
        i->push(tok);
    }
};

// `(file.png) loadpng -> gc`. Build an OFFSCREEN-backed context whose
// initial pixels are the PNG file's content; width/height come from
// the file. The result does NOT replace /:currentpage -- caller
// chooses (drawimage onto current page, setpage to draw on top, or
// just writepng to round-trip).
class LoadPngFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        if (!i->pick(0).is_of_type(sli3::stringtype))
        {
            i->raiseerror(i->ArgumentTypeError);
            return;
        }
        std::string path = i->pick(0).data_.string_val->str();
        GraphicsContext* g = nullptr;
        try { g = GraphicsContext::open_image(path); }
        catch (std::exception const& e)
        {
            i->message(sli3::M_ERROR, "loadpng", e.what());
            i->raiseerror(Name("loadpng"), Name("LoadPngError"));
            return;
        }
        Token tok(i->get_type(sli3::graphicscontexttype));
        tok.data_.graphics_val = g;
        i->pop(1);
        i->push(tok);
    }
};

// `gc cx cy w h drawimage -> -`. Paint `gc`'s image surface onto the
// current page at user-space rectangle (cx, cy, w, h). Compensates
// for Cairo's Y-down pixel layout so the image lands right-side-up in
// our PS-style Y-up frame.
class DrawImageFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(5);
        if (!i->pick(4).is_of_type(sli3::graphicscontexttype))
        {
            i->raiseerror(i->ArgumentTypeError);
            return;
        }
        for (int k = 0; k < 4; ++k)
            if (!is_numeric(i->pick(k)))
            {
                i->raiseerror(i->ArgumentTypeError);
                return;
            }
        GraphicsContext* src = i->pick(4).data_.graphics_val;
        if (!src || !src->valid() || !src->is_image_surface())
        {
            i->raiseerror(Name("drawimage"), Name("UnsupportedSurfaceError"));
            return;
        }
        GraphicsContext* tgt = require_current_gc(i, Name("drawimage"));
        if (!tgt) return;
        double cx = as_double(i->pick(3));
        double cy = as_double(i->pick(2));
        double w  = as_double(i->pick(1));
        double h  = as_double(i->pick(0));
        if (w <= 0.0 || h <= 0.0)
        {
            i->raiseerror(i->RangeCheckError);
            return;
        }
        int iw = cairo_image_surface_get_width(src->surface());
        int ih = cairo_image_surface_get_height(src->surface());

        cairo_t* cr = tgt->cr();
        cairo_save(cr);
        // PS user-space: (cx, cy) is the bottom-left of the destination
        // rectangle; (cx + w, cy + h) is the top-right. Cairo's source
        // surface has pixel (0,0) at its top-left. To paint the image
        // upright at (cx, cy) sized (w, h):
        //   1) translate to the destination's top-left in user space
        //      (cx, cy + h), since user-Y grows up
        //   2) scale (w/iw, -h/ih) so the image extends rightward (+X)
        //      and downward (in user-Y, which is screen-up) into the
        //      destination rectangle
        //   3) set the source surface at the (now-flipped) origin
        cairo_translate(cr, cx, cy + h);
        cairo_scale(cr, w / static_cast<double>(iw), -h / static_cast<double>(ih));
        cairo_set_source_surface(cr, src->surface(), 0.0, 0.0);
        cairo_paint(cr);
        cairo_restore(cr);
        i->pop(5);
    }
};

//------------------------------------------------------------------------
// Gradients and source patterns. The wrapper CairoPattern lives in
// sli_graphics.h alongside GraphicsContext; PatternType drives the
// Token refcount, see sli_patterntype.{h,cpp}.
//------------------------------------------------------------------------

namespace
{

// Wrap a freshly-built cairo_pattern_t as a Token. Steals the
// reference (the wrapper takes ownership; on the cairo side the
// refcount starts at 1 and we don't touch it).
Token wrap_pattern_(SLIInterpreter* i, cairo_pattern_t* raw)
{
    Token t(i->get_type(sli3::patterntype));
    t.data_.pattern_val = new CairoPattern(raw);
    return t;
}

}  // namespace

// `x0 y0 x1 y1 linearpattern -> pattern`. Builds a linear gradient
// from (x0, y0) to (x1, y1). Add color stops with /addcolorstop
// before /setpattern.
class LinearPatternFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(4);
        for (int k = 0; k < 4; ++k)
            if (!is_numeric(i->pick(k)))
            {
                i->raiseerror(i->ArgumentTypeError);
                return;
            }
        double x0 = as_double(i->pick(3));
        double y0 = as_double(i->pick(2));
        double x1 = as_double(i->pick(1));
        double y1 = as_double(i->pick(0));
        cairo_pattern_t* p = cairo_pattern_create_linear(x0, y0, x1, y1);
        if (cairo_pattern_status(p) != CAIRO_STATUS_SUCCESS)
        {
            cairo_pattern_destroy(p);
            i->raiseerror(Name("linearpattern"), Name("PatternError"));
            return;
        }
        i->pop(4);
        i->push(wrap_pattern_(i, p));
    }
};

// `cx0 cy0 r0 cx1 cy1 r1 radialpattern -> pattern`. Builds a radial
// gradient between two circles (inner and outer). Same color-stop
// rule as /linearpattern.
class RadialPatternFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(6);
        for (int k = 0; k < 6; ++k)
            if (!is_numeric(i->pick(k)))
            {
                i->raiseerror(i->ArgumentTypeError);
                return;
            }
        double cx0 = as_double(i->pick(5));
        double cy0 = as_double(i->pick(4));
        double r0  = as_double(i->pick(3));
        double cx1 = as_double(i->pick(2));
        double cy1 = as_double(i->pick(1));
        double r1  = as_double(i->pick(0));
        cairo_pattern_t* p = cairo_pattern_create_radial(cx0, cy0, r0, cx1, cy1, r1);
        if (cairo_pattern_status(p) != CAIRO_STATUS_SUCCESS)
        {
            cairo_pattern_destroy(p);
            i->raiseerror(Name("radialpattern"), Name("PatternError"));
            return;
        }
        i->pop(6);
        i->push(wrap_pattern_(i, p));
    }
};

// `pattern offset r g b addcolorstop -> -` or
// `pattern offset r g b a addcolorstop -> -`. Mutates the pattern
// in-place (Cairo pattern objects are mutable up until the first
// time they're used as a source). offset is in [0, 1].
class AddColorStopFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        // Try the 6-operand alpha form first.
        if (i->load() >= 6
            && i->pick(5).is_of_type(sli3::patterntype)
            && is_numeric(i->pick(4))
            && is_numeric(i->pick(3))
            && is_numeric(i->pick(2))
            && is_numeric(i->pick(1))
            && is_numeric(i->pick(0)))
        {
            CairoPattern* p = i->pick(5).data_.pattern_val;
            if (!p || !p->valid())
            {
                i->raiseerror(Name("addcolorstop"), Name("PatternError"));
                return;
            }
            double off = as_double(i->pick(4));
            double r   = as_double(i->pick(3));
            double g   = as_double(i->pick(2));
            double b   = as_double(i->pick(1));
            double a   = as_double(i->pick(0));
            cairo_pattern_add_color_stop_rgba(p->get(), off, r, g, b, a);
            i->pop(6);
            return;
        }
        // 5-operand (no alpha) form. Alpha defaults to 1.
        i->require_stack_load(5);
        if (!i->pick(4).is_of_type(sli3::patterntype)
            || !is_numeric(i->pick(3))
            || !is_numeric(i->pick(2))
            || !is_numeric(i->pick(1))
            || !is_numeric(i->pick(0)))
        {
            i->raiseerror(i->ArgumentTypeError);
            return;
        }
        CairoPattern* p = i->pick(4).data_.pattern_val;
        if (!p || !p->valid())
        {
            i->raiseerror(Name("addcolorstop"), Name("PatternError"));
            return;
        }
        double off = as_double(i->pick(3));
        double r   = as_double(i->pick(2));
        double g   = as_double(i->pick(1));
        double b   = as_double(i->pick(0));
        cairo_pattern_add_color_stop_rgb(p->get(), off, r, g, b);
        i->pop(5);
    }
};

// `pattern setpattern -> -`. Installs the pattern as the current
// source for subsequent /stroke and /fill ops. Cairo retains its own
// reference; our wrapper keeps the pattern alive across the call.
class SetPatternFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        if (!i->pick(0).is_of_type(sli3::patterntype))
        {
            i->raiseerror(i->ArgumentTypeError);
            return;
        }
        CairoPattern* p = i->pick(0).data_.pattern_val;
        if (!p || !p->valid())
        {
            i->raiseerror(Name("setpattern"), Name("PatternError"));
            return;
        }
        GraphicsContext* g = require_current_gc(i, Name("setpattern"));
        if (!g) return;
        cairo_set_source(g->cr(), p->get());
        i->pop(1);
    }
};

// `gc setpage -> -`. Make the operand the active drawing target.
// Lets a program juggle multiple pages by saving each /newpage result
// and switching with /setpage between them.
class SetPageFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        if (!i->pick(0).is_of_type(sli3::graphicscontexttype))
        {
            i->raiseerror(i->ArgumentTypeError);
            return;
        }
        set_current_page(i, i->pick(0));
        i->pop(1);
    }
};

// `gc closepage -> -`
class ClosePageFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        if (!i->pick(0).is_of_type(sli3::graphicscontexttype))
        {
            i->raiseerror(i->ArgumentTypeError);
            return;
        }
        GraphicsContext* g = i->pick(0).data_.graphics_val;
        if (g)
        {
            g->close();
            clear_current_page(i, g);
        }
        i->pop(1);
    }
};

// `-> gc`
class CurrentPageFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        GraphicsContext* g = require_current_gc(i, Name("currentpage"));
        if (!g) return;
        Token tok(i->get_type(sli3::graphicscontexttype));
        tok.data_.graphics_val = g;
        // Manually bump refcount: raw assignment didn't go through
        // Token copy, and once we push() it the destructor of the
        // popped operand would otherwise drop us to zero. (Same
        // pattern as DictionaryStack::toArray, called out in the
        // operator-authoring contract.)
        g->add_reference();
        i->push(tok);
    }
};

//------------------------------------------------------------------------
// Path construction
//------------------------------------------------------------------------

class NewPathFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        GraphicsContext* g = require_current_gc(i, Name("newpath"));
        if (!g) return;
        cairo_new_path(g->cr());
    }
};

class ClosePathFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        GraphicsContext* g = require_current_gc(i, Name("closepath"));
        if (!g) return;
        cairo_close_path(g->cr());
    }
};

// Shared logic for ops that take two numeric coords and call into
// one Cairo function.
template <void (*Fn)(cairo_t*, double, double)>
class XYFunction : public SLIFunction
{
public:
    explicit XYFunction(Name op_name) : op_name_(op_name) {}

    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(2);
        if (!is_numeric(i->pick(1)) || !is_numeric(i->pick(0)))
        {
            i->raiseerror(i->ArgumentTypeError);
            return;
        }
        GraphicsContext* g = require_current_gc(i, op_name_);
        if (!g) return;
        double x = as_double(i->pick(1));
        double y = as_double(i->pick(0));
        Fn(g->cr(), x, y);
        i->pop(2);
    }

private:
    Name op_name_;
};

//------------------------------------------------------------------------
// Shape shortcuts
//------------------------------------------------------------------------

// `x y w h rect -> -` (adds to current path)
class RectFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(4);
        for (int k = 0; k < 4; ++k)
            if (!is_numeric(i->pick(k)))
            {
                i->raiseerror(i->ArgumentTypeError);
                return;
            }
        GraphicsContext* g = require_current_gc(i, Name("rect"));
        if (!g) return;
        double x = as_double(i->pick(3));
        double y = as_double(i->pick(2));
        double w = as_double(i->pick(1));
        double h = as_double(i->pick(0));
        cairo_rectangle(g->cr(), x, y, w, h);
        i->pop(4);
    }
};

// `cx cy r a1 a2 arc -> -` (angles in degrees, like PS)
class ArcFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(5);
        for (int k = 0; k < 5; ++k)
            if (!is_numeric(i->pick(k)))
            {
                i->raiseerror(i->ArgumentTypeError);
                return;
            }
        GraphicsContext* g = require_current_gc(i, Name("arc"));
        if (!g) return;
        double cx = as_double(i->pick(4));
        double cy = as_double(i->pick(3));
        double r  = as_double(i->pick(2));
        double a1 = as_double(i->pick(1)) * M_PI / 180.0;
        double a2 = as_double(i->pick(0)) * M_PI / 180.0;
        cairo_arc(g->cr(), cx, cy, r, a1, a2);
        i->pop(5);
    }
};

// Shared logic for ops taking 6 numeric coords (cubic curves).
template <void (*Fn)(cairo_t*, double, double, double, double, double, double)>
class CurveFunction : public SLIFunction
{
public:
    explicit CurveFunction(Name op_name) : op_name_(op_name) {}
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(6);
        for (int k = 0; k < 6; ++k)
            if (!is_numeric(i->pick(k)))
            {
                i->raiseerror(i->ArgumentTypeError);
                return;
            }
        GraphicsContext* g = require_current_gc(i, op_name_);
        if (!g) return;
        double x1 = as_double(i->pick(5));
        double y1 = as_double(i->pick(4));
        double x2 = as_double(i->pick(3));
        double y2 = as_double(i->pick(2));
        double x3 = as_double(i->pick(1));
        double y3 = as_double(i->pick(0));
        Fn(g->cr(), x1, y1, x2, y2, x3, y3);
        i->pop(6);
    }
private:
    Name op_name_;
};

// `x1 y1 x2 y2 r arct -> -` -- arc-tangent fillet (PS arct).
//
// Round the corner of the polyline (currentpoint) -> (x1,y1) -> (x2,y2)
// with a circular arc of radius r tangent to both segments. Equivalent
// to a /lineto to the first tangent point on segment 1, then an arc
// from there to the tangent point on segment 2, leaving the current
// point on segment 2.
//
// Degenerate cases per PS spec: no current point, zero-length segments,
// r <= 0, or collinear/anti-parallel segments all degenerate to a
// straight /lineto P1. NoCurrentPointError is raised only for the
// missing-current-point case (the others are PS-legal).
class ArctFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(5);
        for (int k = 0; k < 5; ++k)
            if (!is_numeric(i->pick(k)))
            {
                i->raiseerror(i->ArgumentTypeError);
                return;
            }
        GraphicsContext* g = require_current_gc(i, Name("arct"));
        if (!g) return;
        cairo_t* cr = g->cr();
        if (!cairo_has_current_point(cr))
        {
            i->raiseerror(Name("arct"), Name("NoCurrentPointError"));
            return;
        }
        double x1 = as_double(i->pick(4));
        double y1 = as_double(i->pick(3));
        double x2 = as_double(i->pick(2));
        double y2 = as_double(i->pick(1));
        double r  = as_double(i->pick(0));

        double x0 = 0.0, y0 = 0.0;
        cairo_get_current_point(cr, &x0, &y0);

        // v1: from corner back toward current point. v2: from corner
        // forward to the next vertex. Both used for the tangent-point
        // and arc-center geometry; the cross product of the FORWARD
        // directions (-v1, v2) carries the turn sign.
        double v1x = x0 - x1, v1y = y0 - y1;
        double v2x = x2 - x1, v2y = y2 - y1;
        double len1 = std::sqrt(v1x * v1x + v1y * v1y);
        double len2 = std::sqrt(v2x * v2x + v2y * v2y);
        if (len1 == 0.0 || len2 == 0.0 || r <= 0.0)
        {
            cairo_line_to(cr, x1, y1);
            i->pop(5);
            return;
        }
        v1x /= len1; v1y /= len1;
        v2x /= len2; v2y /= len2;

        double cos_alpha = v1x * v2x + v1y * v2y;
        // sin²(alpha) ~ 0 means alpha is 0 or 180 -- segments are
        // collinear (or anti-parallel). No arc fits; fall back to a
        // straight line at the corner, matching PS.
        double sin_alpha_sq = 1.0 - cos_alpha * cos_alpha;
        if (sin_alpha_sq < 1e-12)
        {
            cairo_line_to(cr, x1, y1);
            i->pop(5);
            return;
        }

        // tan(alpha/2) via half-angle identity. Both sides are >= 0
        // because 0 < alpha < pi here (collinear cases filtered above).
        double tan_half  = std::sqrt((1.0 - cos_alpha) / (1.0 + cos_alpha));
        double d         = r / tan_half;            // tangent-point offset from P1
        double sin_half  = std::sqrt((1.0 - cos_alpha) * 0.5);
        // Bisector direction (v1+v2 normalized). Length is 2*cos(alpha/2),
        // which is non-zero because sin²(alpha) is non-degenerate.
        double bisx = v1x + v2x, bisy = v1y + v2y;
        double bisl = std::sqrt(bisx * bisx + bisy * bisy);
        bisx /= bisl; bisy /= bisl;

        double t1x = x1 + v1x * d, t1y = y1 + v1y * d;
        double t2x = x1 + v2x * d, t2y = y1 + v2y * d;
        double cx  = x1 + bisx * (r / sin_half);
        double cy  = y1 + bisy * (r / sin_half);

        cairo_line_to(cr, t1x, t1y);
        double a1 = std::atan2(t1y - cy, t1x - cx);
        double a2 = std::atan2(t2y - cy, t2x - cx);
        // Forward-direction cross product: positive => CCW user turn,
        // matching the same convention /arc draws under our Y-flipped
        // CTM (cairo_arc visually sweeps CCW when angles increase in
        // user space). Use cairo_arc_negative for CW turns.
        double cross = (-v1x) * v2y - (-v1y) * v2x;  // = v1y*v2x - v1x*v2y
        if (cross >= 0.0)
            cairo_arc(cr, cx, cy, r, a1, a2);
        else
            cairo_arc_negative(cr, cx, cy, r, a1, a2);

        i->pop(5);
    }
};

// `cx cy r a1 a2 arcn -> -` -- negative-direction arc (PS arcn).
class ArcNFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(5);
        for (int k = 0; k < 5; ++k)
            if (!is_numeric(i->pick(k)))
            {
                i->raiseerror(i->ArgumentTypeError);
                return;
            }
        GraphicsContext* g = require_current_gc(i, Name("arcn"));
        if (!g) return;
        double cx = as_double(i->pick(4));
        double cy = as_double(i->pick(3));
        double r  = as_double(i->pick(2));
        double a1 = as_double(i->pick(1)) * M_PI / 180.0;
        double a2 = as_double(i->pick(0)) * M_PI / 180.0;
        cairo_arc_negative(g->cr(), cx, cy, r, a1, a2);
        i->pop(5);
    }
};

// `cx cy r circle -> -` (full circle, doesn't take angles)
class CircleFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(3);
        for (int k = 0; k < 3; ++k)
            if (!is_numeric(i->pick(k)))
            {
                i->raiseerror(i->ArgumentTypeError);
                return;
            }
        GraphicsContext* g = require_current_gc(i, Name("circle"));
        if (!g) return;
        double cx = as_double(i->pick(2));
        double cy = as_double(i->pick(1));
        double r  = as_double(i->pick(0));
        cairo_new_sub_path(g->cr());  // detach from any open subpath
        cairo_arc(g->cr(), cx, cy, r, 0.0, 2.0 * M_PI);
        cairo_close_path(g->cr());
        i->pop(3);
    }
};

//------------------------------------------------------------------------
// Painting
//------------------------------------------------------------------------

class StrokeFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        GraphicsContext* g = require_current_gc(i, Name("stroke"));
        if (!g) return;
        cairo_stroke(g->cr());
    }
};

class FillFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        GraphicsContext* g = require_current_gc(i, Name("fill"));
        if (!g) return;
        cairo_fill(g->cr());
    }
};

// Repaints the current page in opaque white, preserving the current
// color / line width / CTM. Named like PS's /erasepage to avoid the
// stack-clearing /clear already bound by sli_stack.cpp.
class ErasePageFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        GraphicsContext* g = require_current_gc(i, Name("erasepage"));
        if (!g) return;
        cairo_t* cr = g->cr();
        cairo_save(cr);
        // White over the whole image surface — done in device space,
        // not user space, so it doesn't depend on the current CTM.
        cairo_identity_matrix(cr);
        cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
        cairo_paint(cr);
        cairo_restore(cr);
    }
};

// Like /erasepage but paints with CAIRO_OPERATOR_CLEAR -- writes
// fully transparent pixels (alpha = 0) over the whole surface
// instead of opaque white. Useful when the result will be
// composited over something else (e.g. an HTML page) or when you
// want a transparent background in writepng output.
class ClearTransparentFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        GraphicsContext* g = require_current_gc(i, Name("cleartransparent"));
        if (!g) return;
        cairo_t* cr = g->cr();
        cairo_save(cr);
        cairo_identity_matrix(cr);
        cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
        cairo_paint(cr);
        cairo_restore(cr);
    }
};

//------------------------------------------------------------------------
// State
//------------------------------------------------------------------------

namespace
{

// Shared array-form parser for /setrgbcolor and /setrgbacolor: accepts
// a 3-element [r g b] array (opaque) or a 4-element [r g b a] array.
// On success, calls cairo_set_source_rgba and pops; on shape mismatch,
// raises and returns. Returns true if handled (array form was taken),
// false if the operand isn't an array (caller falls through to the
// scalar form).
bool try_color_array_(SLIInterpreter* i, Name op_name)
{
    if (!i->pick(0).is_of_type(sli3::arraytype)) return false;
    TokenArray const* a = i->pick(0).data_.array_val;
    if (a->size() != 3 && a->size() != 4)
    {
        i->raiseerror(op_name, Name("ColorShapeError"));
        return true;
    }
    for (size_t k = 0; k < a->size(); ++k)
        if (!is_numeric(a->get(k)))
        {
            i->raiseerror(op_name, Name("ColorShapeError"));
            return true;
        }
    GraphicsContext* g = require_current_gc(i, op_name);
    if (!g) return true;  // raise already issued
    double alpha = a->size() == 4 ? as_double(a->get(3)) : 1.0;
    cairo_set_source_rgba(g->cr(),
                          as_double(a->get(0)),
                          as_double(a->get(1)),
                          as_double(a->get(2)),
                          alpha);
    i->pop(1);
    return true;
}

}  // namespace

// `r g b setrgbcolor -> -` | `[r g b] setrgbcolor -> -` |
// `[r g b a] setrgbcolor -> -`. The array forms pair naturally with the
// /color dictionary (`color /red get setrgbcolor`); 4-element arrays
// carry alpha, 3-element arrays are opaque. The 3-scalar form remains
// opaque -- explicit alpha via /setrgbacolor or the 4-element array.
class SetRgbColorFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        if (try_color_array_(i, Name("setrgbcolor"))) return;
        // Three-scalar form, opaque alpha=1.
        i->require_stack_load(3);
        for (int k = 0; k < 3; ++k)
            if (!is_numeric(i->pick(k)))
            {
                i->raiseerror(i->ArgumentTypeError);
                return;
            }
        GraphicsContext* g = require_current_gc(i, Name("setrgbcolor"));
        if (!g) return;
        cairo_set_source_rgba(g->cr(),
                              as_double(i->pick(2)),
                              as_double(i->pick(1)),
                              as_double(i->pick(0)),
                              1.0);
        i->pop(3);
    }
};

class SetGrayFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        if (!is_numeric(i->pick(0)))
        {
            i->raiseerror(i->ArgumentTypeError);
            return;
        }
        GraphicsContext* g = require_current_gc(i, Name("setgray"));
        if (!g) return;
        double gv = as_double(i->pick(0));
        cairo_set_source_rgb(g->cr(), gv, gv, gv);
        i->pop(1);
    }
};

class SetLineWidthFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        if (!is_numeric(i->pick(0)))
        {
            i->raiseerror(i->ArgumentTypeError);
            return;
        }
        GraphicsContext* g = require_current_gc(i, Name("setlinewidth"));
        if (!g) return;
        double w = as_double(i->pick(0));
        cairo_set_line_width(g->cr(), w);
        i->pop(1);
    }
};

// `h s b sethsbcolor -> -`
class SetHsbColorFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(3);
        for (int k = 0; k < 3; ++k)
            if (!is_numeric(i->pick(k)))
            {
                i->raiseerror(i->ArgumentTypeError);
                return;
            }
        GraphicsContext* g = require_current_gc(i, Name("sethsbcolor"));
        if (!g) return;
        double h = as_double(i->pick(2));
        double s = as_double(i->pick(1));
        double v = as_double(i->pick(0));
        double r, gn, b;
        hsb_to_rgb(h, s, v, r, gn, b);
        cairo_set_source_rgb(g->cr(), r, gn, b);
        i->pop(3);
    }
};

// `r g b a setrgbacolor -> -` | `[r g b] setrgbacolor -> -` |
// `[r g b a] setrgbacolor -> -`. The array forms match /setrgbcolor's;
// 3-element arrays are opaque. The 4-scalar form is the explicit-RGBA
// variant.
class SetRgbaColorFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        if (try_color_array_(i, Name("setrgbacolor"))) return;
        // Four-scalar form.
        i->require_stack_load(4);
        for (int k = 0; k < 4; ++k)
            if (!is_numeric(i->pick(k)))
            {
                i->raiseerror(i->ArgumentTypeError);
                return;
            }
        GraphicsContext* g = require_current_gc(i, Name("setrgbacolor"));
        if (!g) return;
        cairo_set_source_rgba(g->cr(),
                              as_double(i->pick(3)),
                              as_double(i->pick(2)),
                              as_double(i->pick(1)),
                              as_double(i->pick(0)));
        i->pop(4);
    }
};

// `currentrgbacolor -> r g b a`. Same source-pattern restriction as
// /currentrgbcolor (raises NonSolidSourceError on gradients / images).
class CurrentRgbaColorFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        GraphicsContext* g = require_current_gc(i, Name("currentrgbacolor"));
        if (!g) return;
        cairo_pattern_t* p = cairo_get_source(g->cr());
        if (!p || cairo_pattern_get_type(p) != CAIRO_PATTERN_TYPE_SOLID)
        {
            i->raiseerror(Name("currentrgbacolor"), Name("NonSolidSourceError"));
            return;
        }
        double r = 0, gn = 0, b = 0, a = 0;
        cairo_pattern_get_rgba(p, &r, &gn, &b, &a);
        i->push<double>(r);
        i->push<double>(gn);
        i->push<double>(b);
        i->push<double>(a);
    }
};

// One-int Cairo state-setter (line cap, line join). Cairo's enums
// happen to use the same numbering as PS: 0=butt/miter, 1=round/round,
// 2=square/bevel.
class SetLineCapFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        if (!i->pick(0).is_of_type(sli3::integertype))
        {
            i->raiseerror(i->ArgumentTypeError);
            return;
        }
        GraphicsContext* g = require_current_gc(i, Name("setlinecap"));
        if (!g) return;
        long n = i->pick(0).data_.long_val;
        if (n < 0 || n > 2) { i->raiseerror(i->RangeCheckError); return; }
        cairo_set_line_cap(g->cr(), static_cast<cairo_line_cap_t>(n));
        i->pop(1);
    }
};

class SetLineJoinFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        if (!i->pick(0).is_of_type(sli3::integertype))
        {
            i->raiseerror(i->ArgumentTypeError);
            return;
        }
        GraphicsContext* g = require_current_gc(i, Name("setlinejoin"));
        if (!g) return;
        long n = i->pick(0).data_.long_val;
        if (n < 0 || n > 2) { i->raiseerror(i->RangeCheckError); return; }
        cairo_set_line_join(g->cr(), static_cast<cairo_line_join_t>(n));
        i->pop(1);
    }
};

class SetMiterLimitFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        if (!is_numeric(i->pick(0)))
        {
            i->raiseerror(i->ArgumentTypeError);
            return;
        }
        GraphicsContext* g = require_current_gc(i, Name("setmiterlimit"));
        if (!g) return;
        cairo_set_miter_limit(g->cr(), as_double(i->pick(0)));
        i->pop(1);
    }
};

// `[d0 d1 ...] offset setdash -> -`. Empty array -> solid line.
class SetDashFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(2);
        if (!i->pick(1).is_of_type(sli3::arraytype) || !is_numeric(i->pick(0)))
        {
            i->raiseerror(i->ArgumentTypeError);
            return;
        }
        TokenArray const* arr = i->pick(1).data_.array_val;
        for (Token const* t = arr->begin(); t != arr->end(); ++t)
            if (!is_numeric(*t))
            {
                i->raiseerror(i->ArgumentTypeError);
                return;
            }
        GraphicsContext* g = require_current_gc(i, Name("setdash"));
        if (!g) return;
        std::vector<double> dashes;
        dashes.reserve(arr->size());
        for (Token const* t = arr->begin(); t != arr->end(); ++t)
            dashes.push_back(as_double(*t));
        double offset = as_double(i->pick(0));
        cairo_set_dash(g->cr(),
                       dashes.empty() ? nullptr : dashes.data(),
                       static_cast<int>(dashes.size()),
                       offset);
        i->pop(2);
    }
};

class GSaveFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        GraphicsContext* g = require_current_gc(i, Name("gsave"));
        if (!g) return;
        cairo_save(g->cr());
    }
};

class GRestoreFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        GraphicsContext* g = require_current_gc(i, Name("grestore"));
        if (!g) return;
        cairo_restore(g->cr());
    }
};

// `clip -> -` -- non-zero winding clip with the current path.
class ClipFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        GraphicsContext* g = require_current_gc(i, Name("clip"));
        if (!g) return;
        cairo_set_fill_rule(g->cr(), CAIRO_FILL_RULE_WINDING);
        cairo_clip(g->cr());
    }
};

// `eoclip -> -` -- even/odd rule clip; restores fill rule afterwards.
class EoClipFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        GraphicsContext* g = require_current_gc(i, Name("eoclip"));
        if (!g) return;
        cairo_t* cr = g->cr();
        cairo_fill_rule_t saved = cairo_get_fill_rule(cr);
        cairo_set_fill_rule(cr, CAIRO_FILL_RULE_EVEN_ODD);
        cairo_clip(cr);
        cairo_set_fill_rule(cr, saved);
    }
};

// `currentpoint -> x y` -- PS-style query.
class CurrentPointFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        GraphicsContext* g = require_current_gc(i, Name("currentpoint"));
        if (!g) return;
        if (!cairo_has_current_point(g->cr()))
        {
            i->raiseerror(Name("currentpoint"), Name("NoCurrentPointError"));
            return;
        }
        double x = 0, y = 0;
        cairo_get_current_point(g->cr(), &x, &y);
        i->push<double>(x);
        i->push<double>(y);
    }
};

// `pathbbox -> llx lly urx ury` -- current path's bounding box in
// user-space coordinates.
class PathBBoxFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        GraphicsContext* g = require_current_gc(i, Name("pathbbox"));
        if (!g) return;
        double x1 = 0, y1 = 0, x2 = 0, y2 = 0;
        cairo_path_extents(g->cr(), &x1, &y1, &x2, &y2);
        i->push<double>(x1);
        i->push<double>(y1);
        i->push<double>(x2);
        i->push<double>(y2);
    }
};

// `clippath -> -`. Replace the current path with a rectangle covering
// the current clip extents. Cairo doesn't expose the actual clip
// region as a path -- we approximate with the bounding box, which
// matches the common case (rectangular clips). For curve-shaped clips
// the user loses precision, but PS programs typically only need the
// bounding box for layout anyway.
class ClipPathFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        GraphicsContext* g = require_current_gc(i, Name("clippath"));
        if (!g) return;
        cairo_t* cr = g->cr();
        double x1 = 0, y1 = 0, x2 = 0, y2 = 0;
        cairo_clip_extents(cr, &x1, &y1, &x2, &y2);
        cairo_new_path(cr);
        cairo_rectangle(cr, x1, y1, x2 - x1, y2 - y1);
    }
};

// `flattenpath -> -`. Replace the current path with a curve-free
// approximation. Walks cairo_copy_path_flat and re-emits the segments
// via cairo_move_to / cairo_line_to / cairo_close_path; curves are
// already broken down into line segments by the flat-copy call.
class FlattenPathFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        GraphicsContext* g = require_current_gc(i, Name("flattenpath"));
        if (!g) return;
        cairo_t* cr = g->cr();
        cairo_path_t* flat = cairo_copy_path_flat(cr);
        if (flat->status != CAIRO_STATUS_SUCCESS)
        {
            cairo_status_t st = flat->status;
            cairo_path_destroy(flat);
            i->message(sli3::M_ERROR, "flattenpath", cairo_status_to_string(st));
            i->raiseerror(Name("flattenpath"), Name("PathError"));
            return;
        }
        cairo_new_path(cr);
        for (int k = 0; k < flat->num_data; k += flat->data[k].header.length)
        {
            cairo_path_data_t* d = &flat->data[k];
            switch (d->header.type)
            {
                case CAIRO_PATH_MOVE_TO:
                    cairo_move_to(cr, d[1].point.x, d[1].point.y); break;
                case CAIRO_PATH_LINE_TO:
                    cairo_line_to(cr, d[1].point.x, d[1].point.y); break;
                case CAIRO_PATH_CLOSE_PATH:
                    cairo_close_path(cr); break;
                case CAIRO_PATH_CURVE_TO:
                    // cairo_copy_path_flat guarantees no curves remain,
                    // but the switch needs the case to silence warnings.
                    break;
            }
        }
        cairo_path_destroy(flat);
    }
};

// `move_proc line_proc curve_proc close_proc pathforall -> -`.
//
// Synthesize a procedure whose body is a flattened sequence of
// "<args> <proc>" runs -- one per path segment -- and hand it to the
// dispatcher via the e-stack. We don't need a new iterator type or
// dispatcher hook; the standard procedure-execution path naturally
// runs each segment's args + proc in order.
class PathForAllFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(4);
        for (int k = 0; k < 4; ++k)
        {
            unsigned tag = i->pick(k).tag();
            if (tag != sli3::proceduretype && tag != sli3::litproceduretype)
            {
                i->raiseerror(i->ArgumentTypeError);
                return;
            }
        }
        GraphicsContext* g = require_current_gc(i, Name("pathforall"));
        if (!g) return;

        // Copy the user procs before we touch the stack -- the
        // tokens get embedded into the synthesized body and shared
        // by every segment (Token copy bumps the array refcount).
        Token move_p  = i->pick(3);
        Token line_p  = i->pick(2);
        Token curve_p = i->pick(1);
        Token close_p = i->pick(0);

        cairo_path_t* path = cairo_copy_path(g->cr());
        if (path->status != CAIRO_STATUS_SUCCESS)
        {
            cairo_status_t st = path->status;
            cairo_path_destroy(path);
            i->message(sli3::M_ERROR, "pathforall", cairo_status_to_string(st));
            i->raiseerror(Name("pathforall"), Name("PathError"));
            return;
        }

        TokenArray* body = new TokenArray();
        // Reserve a guess: 3 tokens per moveto/lineto, 7 per curveto,
        // 1 per close. num_data is "header + points" packed, so this
        // is a loose upper bound.
        body->reserve(static_cast<size_t>(path->num_data));

        auto push_d = [&](double v) {
            body->push_back(i->new_token<sli3::doubletype, double>(v));
        };
        for (int k = 0; k < path->num_data; k += path->data[k].header.length)
        {
            cairo_path_data_t* d = &path->data[k];
            switch (d->header.type)
            {
                case CAIRO_PATH_MOVE_TO:
                    push_d(d[1].point.x); push_d(d[1].point.y);
                    body->push_back(move_p);
                    break;
                case CAIRO_PATH_LINE_TO:
                    push_d(d[1].point.x); push_d(d[1].point.y);
                    body->push_back(line_p);
                    break;
                case CAIRO_PATH_CURVE_TO:
                    push_d(d[1].point.x); push_d(d[1].point.y);
                    push_d(d[2].point.x); push_d(d[2].point.y);
                    push_d(d[3].point.x); push_d(d[3].point.y);
                    body->push_back(curve_p);
                    break;
                case CAIRO_PATH_CLOSE_PATH:
                    body->push_back(close_p);
                    break;
            }
        }
        cairo_path_destroy(path);

        i->pop(4);
        // Hand the synthesized procedure to the dispatcher. It pushes
        // an iiterate marker and walks the body tokens, which runs
        // each segment's args + proc in order.
        i->EStack().push(i->new_token<sli3::proceduretype, TokenArray*>(body));
    }
};

//------------------------------------------------------------------------
// Matrix / CTM ops. PostScript represents a 2D affine as a 6-element
// array [xx yx xy yy x0 y0] matching cairo_matrix_t's layout.
//------------------------------------------------------------------------

namespace
{

Token matrix_to_array_(SLIInterpreter* i, cairo_matrix_t const& m)
{
    TokenArray* a = new TokenArray();
    a->reserve(6);
    a->push_back(i->new_token<sli3::doubletype, double>(m.xx));
    a->push_back(i->new_token<sli3::doubletype, double>(m.yx));
    a->push_back(i->new_token<sli3::doubletype, double>(m.xy));
    a->push_back(i->new_token<sli3::doubletype, double>(m.yy));
    a->push_back(i->new_token<sli3::doubletype, double>(m.x0));
    a->push_back(i->new_token<sli3::doubletype, double>(m.y0));
    return i->new_token<sli3::arraytype, TokenArray*>(a);
}

bool array_to_matrix_(TokenArray const* a, cairo_matrix_t& m)
{
    if (a->size() != 6) return false;
    for (size_t k = 0; k < 6; ++k)
        if (!is_numeric(a->get(k))) return false;
    m.xx = as_double(a->get(0));
    m.yx = as_double(a->get(1));
    m.xy = as_double(a->get(2));
    m.yy = as_double(a->get(3));
    m.x0 = as_double(a->get(4));
    m.y0 = as_double(a->get(5));
    return true;
}

}  // namespace

// `currentmatrix -> [xx yx xy yy x0 y0]`
class CurrentMatrixFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        GraphicsContext* g = require_current_gc(i, Name("currentmatrix"));
        if (!g) return;
        cairo_matrix_t m;
        cairo_get_matrix(g->cr(), &m);
        i->push(matrix_to_array_(i, m));
    }
};

// `[a b c d tx ty] setmatrix -> -`
class SetMatrixFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        if (!i->pick(0).is_of_type(sli3::arraytype))
        {
            i->raiseerror(i->ArgumentTypeError);
            return;
        }
        cairo_matrix_t m;
        if (!array_to_matrix_(i->pick(0).data_.array_val, m))
        {
            i->raiseerror(Name("setmatrix"), Name("MatrixShapeError"));
            return;
        }
        GraphicsContext* g = require_current_gc(i, Name("setmatrix"));
        if (!g) return;
        cairo_set_matrix(g->cr(), &m);
        i->pop(1);
    }
};

// `[a b c d tx ty] concat -> -` -- post-multiply: CTM := matrix * CTM.
class ConcatFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        if (!i->pick(0).is_of_type(sli3::arraytype))
        {
            i->raiseerror(i->ArgumentTypeError);
            return;
        }
        cairo_matrix_t m;
        if (!array_to_matrix_(i->pick(0).data_.array_val, m))
        {
            i->raiseerror(Name("concat"), Name("MatrixShapeError"));
            return;
        }
        GraphicsContext* g = require_current_gc(i, Name("concat"));
        if (!g) return;
        cairo_transform(g->cr(), &m);
        i->pop(1);
    }
};

// `initmatrix -> -` -- reset CTM to the PS-style default (Y-flip
// baseline that finish_cairo_setup_ applied at /newpage time).
class InitMatrixFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        GraphicsContext* g = require_current_gc(i, Name("initmatrix"));
        if (!g) return;
        cairo_t* cr = g->cr();
        cairo_identity_matrix(cr);
        cairo_translate(cr, 0.0, static_cast<double>(g->height()));
        cairo_scale(cr, 1.0, -1.0);
    }
};

// `x y transform -> tx ty` -- user-space to device-space point.
class TransformFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(2);
        if (!is_numeric(i->pick(1)) || !is_numeric(i->pick(0)))
        {
            i->raiseerror(i->ArgumentTypeError);
            return;
        }
        GraphicsContext* g = require_current_gc(i, Name("transform"));
        if (!g) return;
        double x = as_double(i->pick(1));
        double y = as_double(i->pick(0));
        cairo_user_to_device(g->cr(), &x, &y);
        i->pop(2);
        i->push<double>(x);
        i->push<double>(y);
    }
};

// `tx ty itransform -> x y` -- device-space to user-space point.
class ITransformFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(2);
        if (!is_numeric(i->pick(1)) || !is_numeric(i->pick(0)))
        {
            i->raiseerror(i->ArgumentTypeError);
            return;
        }
        GraphicsContext* g = require_current_gc(i, Name("itransform"));
        if (!g) return;
        double x = as_double(i->pick(1));
        double y = as_double(i->pick(0));
        cairo_device_to_user(g->cr(), &x, &y);
        i->pop(2);
        i->push<double>(x);
        i->push<double>(y);
    }
};

//------------------------------------------------------------------------
// State queries. The get-side of every setter we already shipped, in
// PS's `current*` naming convention.
//------------------------------------------------------------------------

class CurrentLineWidthFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        GraphicsContext* g = require_current_gc(i, Name("currentlinewidth"));
        if (!g) return;
        i->push<double>(cairo_get_line_width(g->cr()));
    }
};

class CurrentLineCapFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        GraphicsContext* g = require_current_gc(i, Name("currentlinecap"));
        if (!g) return;
        i->push<long>(static_cast<long>(cairo_get_line_cap(g->cr())));
    }
};

class CurrentLineJoinFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        GraphicsContext* g = require_current_gc(i, Name("currentlinejoin"));
        if (!g) return;
        i->push<long>(static_cast<long>(cairo_get_line_join(g->cr())));
    }
};

// `currentrgbcolor -> r g b`. Raises if the current source isn't a
// solid color pattern (gradients / images can't be unpacked to RGB).
class CurrentRgbColorFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        GraphicsContext* g = require_current_gc(i, Name("currentrgbcolor"));
        if (!g) return;
        cairo_pattern_t* p = cairo_get_source(g->cr());
        if (!p || cairo_pattern_get_type(p) != CAIRO_PATTERN_TYPE_SOLID)
        {
            i->raiseerror(Name("currentrgbcolor"), Name("NonSolidSourceError"));
            return;
        }
        double r = 0, gn = 0, b = 0, a = 0;
        cairo_pattern_get_rgba(p, &r, &gn, &b, &a);
        i->push<double>(r);
        i->push<double>(gn);
        i->push<double>(b);
    }
};

// `currentdash -> [d0 d1 ...] offset`
class CurrentDashFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        GraphicsContext* g = require_current_gc(i, Name("currentdash"));
        if (!g) return;
        int n = cairo_get_dash_count(g->cr());
        TokenArray* arr = new TokenArray();
        arr->reserve(static_cast<size_t>(n));
        double offset = 0.0;
        if (n > 0)
        {
            std::vector<double> dashes(static_cast<size_t>(n));
            cairo_get_dash(g->cr(), dashes.data(), &offset);
            for (double d : dashes)
                arr->push_back(i->new_token<sli3::doubletype, double>(d));
        }
        i->push(i->new_token<sli3::arraytype, TokenArray*>(arr));
        i->push<double>(offset);
    }
};

//------------------------------------------------------------------------
// Window / page ergonomics.
//------------------------------------------------------------------------

// `(title) setwindowtitle -> -`. WINDOW backend only.
class SetWindowTitleFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        if (!i->pick(0).is_of_type(sli3::stringtype))
        {
            i->raiseerror(i->ArgumentTypeError);
            return;
        }
        GraphicsContext* g = require_current_gc(i, Name("setwindowtitle"));
        if (!g) return;
        if (g->backend() != GraphicsContext::Backend::WINDOW
            || !g->sdl_window())
        {
            i->raiseerror(Name("setwindowtitle"), Name("UnsupportedSurfaceError"));
            return;
        }
        SDL_SetWindowTitle(g->sdl_window(), i->pick(0).data_.string_val->c_str());
        i->pop(1);
    }
};

// `w h resize -> -`. WINDOW backend only -- changes the SDL window
// size and reallocates the backing Cairo image surface. The CTM is
// reset to the new size's Y-flip baseline (otherwise scaled drawing
// would silently spill outside the visible area).
class ResizeFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(2);
        if (!is_numeric(i->pick(1)) || !is_numeric(i->pick(0)))
        {
            i->raiseerror(i->ArgumentTypeError);
            return;
        }
        long w = static_cast<long>(as_double(i->pick(1)));
        long h = static_cast<long>(as_double(i->pick(0)));
        if (w <= 0 || h <= 0) { i->raiseerror(i->RangeCheckError); return; }
        GraphicsContext* g = require_current_gc(i, Name("resize"));
        if (!g) return;
        if (g->backend() != GraphicsContext::Backend::WINDOW)
        {
            i->raiseerror(Name("resize"), Name("UnsupportedSurfaceError"));
            return;
        }
        try
        {
            g->resize_window(static_cast<int>(w), static_cast<int>(h));
        }
        catch (std::exception const& e)
        {
            i->message(sli3::M_ERROR, "resize", e.what());
            i->raiseerror(Name("resize"), Name("GraphicsBackendError"));
            return;
        }
        i->pop(2);
    }
};

// `x y setwindowpos -> -`. WINDOW only; moves the OS window.
class SetWindowPosFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(2);
        if (!is_numeric(i->pick(1)) || !is_numeric(i->pick(0)))
        {
            i->raiseerror(i->ArgumentTypeError);
            return;
        }
        GraphicsContext* g = require_current_gc(i, Name("setwindowpos"));
        if (!g) return;
        if (g->backend() != GraphicsContext::Backend::WINDOW
            || !g->sdl_window())
        {
            i->raiseerror(Name("setwindowpos"),
                          Name("UnsupportedSurfaceError"));
            return;
        }
        long x = static_cast<long>(as_double(i->pick(1)));
        long y = static_cast<long>(as_double(i->pick(0)));
        SDL_SetWindowPosition(g->sdl_window(),
                              static_cast<int>(x), static_cast<int>(y));
        i->pop(2);
    }
};

// `bool setfullscreen -> -`. true = fullscreen-desktop, false =
// windowed. WINDOW only.
class SetFullscreenFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        if (!i->pick(0).is_of_type(sli3::booltype))
        {
            i->raiseerror(i->ArgumentTypeError);
            return;
        }
        GraphicsContext* g = require_current_gc(i, Name("setfullscreen"));
        if (!g) return;
        if (g->backend() != GraphicsContext::Backend::WINDOW
            || !g->sdl_window())
        {
            i->raiseerror(Name("setfullscreen"),
                          Name("UnsupportedSurfaceError"));
            return;
        }
        bool on = i->pick(0).data_.bool_val;
        Uint32 flags = on ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0;
        SDL_SetWindowFullscreen(g->sdl_window(), flags);
        i->pop(1);
    }
};

// `pagesize -> w h` -- width and height of the current page in user
// units. Works for every backend.
class PageSizeFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        GraphicsContext* g = require_current_gc(i, Name("pagesize"));
        if (!g) return;
        i->push<long>(g->width());
        i->push<long>(g->height());
    }
};

// `(text) textextents -> dict { /XBearing /YBearing /Width /Height
//                               /XAdvance /YAdvance }`
class TextExtentsFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        if (!i->pick(0).is_of_type(sli3::stringtype))
        {
            i->raiseerror(i->ArgumentTypeError);
            return;
        }
        GraphicsContext* g = require_current_gc(i, Name("textextents"));
        if (!g) return;
        cairo_text_extents_t ext;
        cairo_text_extents(g->cr(), i->pick(0).data_.string_val->c_str(), &ext);
        Dictionary* d = new Dictionary();
        d->insert(Name("XBearing"), i->new_token<sli3::doubletype, double>(ext.x_bearing));
        d->insert(Name("YBearing"), i->new_token<sli3::doubletype, double>(ext.y_bearing));
        d->insert(Name("Width"),    i->new_token<sli3::doubletype, double>(ext.width));
        d->insert(Name("Height"),   i->new_token<sli3::doubletype, double>(ext.height));
        d->insert(Name("XAdvance"), i->new_token<sli3::doubletype, double>(ext.x_advance));
        d->insert(Name("YAdvance"), i->new_token<sli3::doubletype, double>(ext.y_advance));
        i->pop(1);
        i->push(i->new_token<sli3::dictionarytype, Dictionary*>(d));
    }
};

class RotateFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        if (!is_numeric(i->pick(0)))
        {
            i->raiseerror(i->ArgumentTypeError);
            return;
        }
        GraphicsContext* g = require_current_gc(i, Name("rotate"));
        if (!g) return;
        double deg = as_double(i->pick(0));
        cairo_rotate(g->cr(), deg * M_PI / 180.0);
        i->pop(1);
    }
};

//------------------------------------------------------------------------
// Text -- PS-style font dicts driving the Cairo "toy" text API.
// A font is a Dictionary with /Family /Slant /Weight /Size. setfont
// applies the dict to the cairo_t and records it under
// globaldict /:currentfont so currentfont can recover it.
//------------------------------------------------------------------------

namespace
{

cairo_font_slant_t slant_from_name_(Name n)
{
    if (n == Name("italic"))  return CAIRO_FONT_SLANT_ITALIC;
    if (n == Name("oblique")) return CAIRO_FONT_SLANT_OBLIQUE;
    return CAIRO_FONT_SLANT_NORMAL;
}

cairo_font_weight_t weight_from_name_(Name n)
{
    if (n == Name("bold")) return CAIRO_FONT_WEIGHT_BOLD;
    return CAIRO_FONT_WEIGHT_NORMAL;
}

// Build a fresh font dict with the given family + defaults.
Token make_font_dict_(SLIInterpreter* i, std::string family)
{
    Dictionary* d = new Dictionary();
    d->insert(Name("Family"), i->new_token<sli3::stringtype, std::string>(std::move(family)));
    d->insert(Name("Slant"),  i->new_token<sli3::literaltype, Name>(Name("normal")));
    d->insert(Name("Weight"), i->new_token<sli3::literaltype, Name>(Name("normal")));
    d->insert(Name("Size"),   i->new_token<sli3::doubletype, double>(12.0));
    return i->new_token<sli3::dictionarytype, Dictionary*>(d);
}

}  // namespace

// `name|string findfont -> font-dict`
class FindFontFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        std::string family;
        if (!token_to_string(i->pick(0), family))
        {
            i->raiseerror(i->ArgumentTypeError);
            return;
        }
        Token font = make_font_dict_(i, std::move(family));
        i->pop(1);
        i->push(font);
    }
};

// `font size scalefont -> font'` (returns a fresh dict with /Size set)
class ScaleFontFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(2);
        if (!i->pick(1).is_of_type(sli3::dictionarytype)
            || !is_numeric(i->pick(0)))
        {
            i->raiseerror(i->ArgumentTypeError);
            return;
        }
        Dictionary* src = i->pick(1).data_.dict_val;
        double size = as_double(i->pick(0));
        // Clone so the caller can keep using the original dict
        // unchanged (PS scalefont is non-destructive).
        Dictionary* out = new Dictionary(*src);
        out->insert(Name("Size"), i->new_token<sli3::doubletype, double>(size));
        i->pop(2);
        i->push(i->new_token<sli3::dictionarytype, Dictionary*>(out));
    }
};

// `font setfont -> -`. Applies family/slant/weight via
// cairo_select_font_face and /Size via cairo_set_font_size. Stores
// the dict in globaldict /:currentfont so currentfont can recover it.
class SetFontFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        if (!i->pick(0).is_of_type(sli3::dictionarytype))
        {
            i->raiseerror(i->ArgumentTypeError);
            return;
        }
        GraphicsContext* g = require_current_gc(i, Name("setfont"));
        if (!g) return;
        Dictionary* d = i->pick(0).data_.dict_val;
        std::string family;
        if (!dict_get_string(d, Name("Family"), family))
        {
            i->raiseerror(Name("setfont"), Name("InvalidFontDictError"));
            return;
        }
        Name slant_n("normal"), weight_n("normal");
        dict_get_name(d, Name("Slant"),  slant_n);
        dict_get_name(d, Name("Weight"), weight_n);
        double size = 12.0;
        dict_get_double(d, Name("Size"), size);
        cairo_select_font_face(g->cr(), family.c_str(),
                               slant_from_name_(slant_n),
                               weight_from_name_(weight_n));
        cairo_set_font_size(g->cr(), size);
        Dictionary* gd = get_gfxstatus(i);
        if (gd) gd->insert(current_font_slot(), i->pick(0));
        i->pop(1);
    }
};

// `currentfont -> font-dict`. Raises if /setfont hasn't run.
class CurrentFontFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        Dictionary* gd = get_gfxstatus(i);
        Token t;
        if (!gd || !gd->lookup(current_font_slot(), t)
            || !t.is_of_type(sli3::dictionarytype))
        {
            i->raiseerror(Name("currentfont"), Name("NoCurrentFontError"));
            return;
        }
        i->push(t);
    }
};

// `(text) show -> -`. Draws at the current point, advances it.
class ShowFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        if (!i->pick(0).is_of_type(sli3::stringtype))
        {
            i->raiseerror(i->ArgumentTypeError);
            return;
        }
        GraphicsContext* g = require_current_gc(i, Name("show"));
        if (!g) return;
        // Cairo's bottom-left Y-flip would render text upside-down in
        // our PS-style coordinate frame. Stash the CTM, restore the
        // device-space (top-left) Y so text glyphs come out right way
        // up at the user's current point, then put the CTM back.
        cairo_t* cr = g->cr();
        double x = 0, y = 0;
        if (cairo_has_current_point(cr))
            cairo_get_current_point(cr, &x, &y);
        std::string const& s = i->pick(0).data_.string_val->str();
        cairo_save(cr);
        cairo_translate(cr, x, y);
        cairo_scale(cr, 1.0, -1.0);
        cairo_move_to(cr, 0.0, 0.0);
        cairo_show_text(cr, s.c_str());
        cairo_restore(cr);
        // Cairo's show_text advances the point in the now-restored
        // frame; recompute a sensible current point from text_extents.
        cairo_text_extents_t ext;
        cairo_text_extents(cr, s.c_str(), &ext);
        cairo_move_to(cr, x + ext.x_advance, y + ext.y_advance);
        i->pop(1);
    }
};

// `(text) stringwidth -> dx dy`
class StringWidthFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        if (!i->pick(0).is_of_type(sli3::stringtype))
        {
            i->raiseerror(i->ArgumentTypeError);
            return;
        }
        GraphicsContext* g = require_current_gc(i, Name("stringwidth"));
        if (!g) return;
        cairo_text_extents_t ext;
        cairo_text_extents(g->cr(), i->pick(0).data_.string_val->str().c_str(), &ext);
        i->pop(1);
        i->push<double>(ext.x_advance);
        i->push<double>(ext.y_advance);
    }
};

//------------------------------------------------------------------------
// Font / rendering quality options. Cairo has three knobs that affect
// how crisp text and shapes come out:
//
//   antialias    -- /default /none /gray /subpixel /fast /good /best
//                   set on both the cairo_t (shape AA) and the font
//                   options (text AA) so users get a single switch.
//   hinting      -- /default /none /slight /medium /full
//                   text only; snaps glyph stems to the pixel grid.
//   hintmetrics  -- /default /on /off
//                   text only; rounds glyph metrics for crisper kerning.
//
// Helpers below build cairo_font_options_t, mutate one field, push
// it back to the context, and destroy. Idiomatic, but verbose enough
// to factor.
//------------------------------------------------------------------------

namespace
{

struct AAEntry { char const* name; cairo_antialias_t v; };
constexpr AAEntry k_antialias[] = {
    {"default",  CAIRO_ANTIALIAS_DEFAULT},
    {"none",     CAIRO_ANTIALIAS_NONE},
    {"gray",     CAIRO_ANTIALIAS_GRAY},
    {"subpixel", CAIRO_ANTIALIAS_SUBPIXEL},
    {"fast",     CAIRO_ANTIALIAS_FAST},
    {"good",     CAIRO_ANTIALIAS_GOOD},
    {"best",     CAIRO_ANTIALIAS_BEST},
};
bool aa_from_name_(Name n, cairo_antialias_t& out)
{
    std::string const& s = n.toString();
    for (auto const& e : k_antialias)
        if (s == e.name) { out = e.v; return true; }
    return false;
}
Name aa_to_name_(cairo_antialias_t v)
{
    for (auto const& e : k_antialias)
        if (e.v == v) return Name(e.name);
    return Name("default");
}

struct HintStyleEntry { char const* name; cairo_hint_style_t v; };
constexpr HintStyleEntry k_hint_style[] = {
    {"default", CAIRO_HINT_STYLE_DEFAULT},
    {"none",    CAIRO_HINT_STYLE_NONE},
    {"slight",  CAIRO_HINT_STYLE_SLIGHT},
    {"medium",  CAIRO_HINT_STYLE_MEDIUM},
    {"full",    CAIRO_HINT_STYLE_FULL},
};
bool hint_style_from_name_(Name n, cairo_hint_style_t& out)
{
    std::string const& s = n.toString();
    for (auto const& e : k_hint_style)
        if (s == e.name) { out = e.v; return true; }
    return false;
}
Name hint_style_to_name_(cairo_hint_style_t v)
{
    for (auto const& e : k_hint_style)
        if (e.v == v) return Name(e.name);
    return Name("default");
}

struct HintMetricsEntry { char const* name; cairo_hint_metrics_t v; };
constexpr HintMetricsEntry k_hint_metrics[] = {
    {"default", CAIRO_HINT_METRICS_DEFAULT},
    {"off",     CAIRO_HINT_METRICS_OFF},
    {"on",      CAIRO_HINT_METRICS_ON},
};
bool hint_metrics_from_name_(Name n, cairo_hint_metrics_t& out)
{
    std::string const& s = n.toString();
    for (auto const& e : k_hint_metrics)
        if (s == e.name) { out = e.v; return true; }
    return false;
}
Name hint_metrics_to_name_(cairo_hint_metrics_t v)
{
    for (auto const& e : k_hint_metrics)
        if (e.v == v) return Name(e.name);
    return Name("default");
}

// Read-modify-write helper for font options. The fn callback modifies
// the options struct in place; the surrounding code handles get / set
// / destroy.
template <class Fn>
void mutate_font_options_(cairo_t* cr, Fn&& fn)
{
    cairo_font_options_t* opts = cairo_font_options_create();
    cairo_get_font_options(cr, opts);
    fn(opts);
    cairo_set_font_options(cr, opts);
    cairo_font_options_destroy(opts);
}

}  // namespace

// `/name setantialias -> -`. Affects BOTH cairo_set_antialias (shape
// AA) and cairo_font_options_set_antialias (text AA) so a single op
// upgrades the whole page.
class SetAntialiasFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        std::string s;
        if (!token_to_string(i->pick(0), s))
        {
            i->raiseerror(i->ArgumentTypeError);
            return;
        }
        cairo_antialias_t aa;
        if (!aa_from_name_(Name(s), aa))
        {
            i->raiseerror(Name("setantialias"), Name("UnknownAntialiasError"));
            return;
        }
        GraphicsContext* g = require_current_gc(i, Name("setantialias"));
        if (!g) return;
        cairo_set_antialias(g->cr(), aa);
        mutate_font_options_(g->cr(), [&](cairo_font_options_t* o) {
            cairo_font_options_set_antialias(o, aa);
        });
        i->pop(1);
    }
};

// `currentantialias -> /name`. Reports the cairo_t's shape AA mode;
// font and shape are kept in sync by /setantialias, so this is the
// authoritative read-back.
class CurrentAntialiasFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        GraphicsContext* g = require_current_gc(i, Name("currentantialias"));
        if (!g) return;
        cairo_antialias_t aa = cairo_get_antialias(g->cr());
        i->push(i->new_token<sli3::literaltype, Name>(aa_to_name_(aa)));
    }
};

// `/name sethinting -> -`. Text-only (cairo_font_options_set_hint_style).
class SetHintingFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        std::string s;
        if (!token_to_string(i->pick(0), s))
        {
            i->raiseerror(i->ArgumentTypeError);
            return;
        }
        cairo_hint_style_t hs;
        if (!hint_style_from_name_(Name(s), hs))
        {
            i->raiseerror(Name("sethinting"), Name("UnknownHintingError"));
            return;
        }
        GraphicsContext* g = require_current_gc(i, Name("sethinting"));
        if (!g) return;
        mutate_font_options_(g->cr(), [&](cairo_font_options_t* o) {
            cairo_font_options_set_hint_style(o, hs);
        });
        i->pop(1);
    }
};

class CurrentHintingFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        GraphicsContext* g = require_current_gc(i, Name("currenthinting"));
        if (!g) return;
        cairo_font_options_t* opts = cairo_font_options_create();
        cairo_get_font_options(g->cr(), opts);
        cairo_hint_style_t hs = cairo_font_options_get_hint_style(opts);
        cairo_font_options_destroy(opts);
        i->push(i->new_token<sli3::literaltype, Name>(hint_style_to_name_(hs)));
    }
};

// `/name sethintmetrics -> -`. Text-only.
class SetHintMetricsFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        std::string s;
        if (!token_to_string(i->pick(0), s))
        {
            i->raiseerror(i->ArgumentTypeError);
            return;
        }
        cairo_hint_metrics_t hm;
        if (!hint_metrics_from_name_(Name(s), hm))
        {
            i->raiseerror(Name("sethintmetrics"),
                          Name("UnknownHintMetricsError"));
            return;
        }
        GraphicsContext* g = require_current_gc(i, Name("sethintmetrics"));
        if (!g) return;
        mutate_font_options_(g->cr(), [&](cairo_font_options_t* o) {
            cairo_font_options_set_hint_metrics(o, hm);
        });
        i->pop(1);
    }
};

class CurrentHintMetricsFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        GraphicsContext* g = require_current_gc(i, Name("currenthintmetrics"));
        if (!g) return;
        cairo_font_options_t* opts = cairo_font_options_create();
        cairo_get_font_options(g->cr(), opts);
        cairo_hint_metrics_t hm = cairo_font_options_get_hint_metrics(opts);
        cairo_font_options_destroy(opts);
        i->push(i->new_token<sli3::literaltype, Name>(hint_metrics_to_name_(hm)));
    }
};

//------------------------------------------------------------------------
// Compositing operators. Maps a small set of literal names to Cairo's
// cairo_operator_t enum so SLI code can say `/multiply setoperator`
// instead of remembering an integer.
//------------------------------------------------------------------------

namespace
{

struct CompositorEntry { char const* name; cairo_operator_t op; };

constexpr CompositorEntry k_compositors[] = {
    {"clear",          CAIRO_OPERATOR_CLEAR},
    {"source",         CAIRO_OPERATOR_SOURCE},
    {"over",           CAIRO_OPERATOR_OVER},
    {"in",             CAIRO_OPERATOR_IN},
    {"out",            CAIRO_OPERATOR_OUT},
    {"atop",           CAIRO_OPERATOR_ATOP},
    {"dest",           CAIRO_OPERATOR_DEST},
    {"dest_over",      CAIRO_OPERATOR_DEST_OVER},
    {"dest_in",        CAIRO_OPERATOR_DEST_IN},
    {"dest_out",       CAIRO_OPERATOR_DEST_OUT},
    {"dest_atop",      CAIRO_OPERATOR_DEST_ATOP},
    {"xor",            CAIRO_OPERATOR_XOR},
    {"add",            CAIRO_OPERATOR_ADD},
    {"saturate",       CAIRO_OPERATOR_SATURATE},
    {"multiply",       CAIRO_OPERATOR_MULTIPLY},
    {"screen",         CAIRO_OPERATOR_SCREEN},
    {"overlay",        CAIRO_OPERATOR_OVERLAY},
    {"darken",         CAIRO_OPERATOR_DARKEN},
    {"lighten",        CAIRO_OPERATOR_LIGHTEN},
    {"color_dodge",    CAIRO_OPERATOR_COLOR_DODGE},
    {"color_burn",     CAIRO_OPERATOR_COLOR_BURN},
    {"hard_light",     CAIRO_OPERATOR_HARD_LIGHT},
    {"soft_light",     CAIRO_OPERATOR_SOFT_LIGHT},
    {"difference",     CAIRO_OPERATOR_DIFFERENCE},
    {"exclusion",      CAIRO_OPERATOR_EXCLUSION},
    {"hsl_hue",        CAIRO_OPERATOR_HSL_HUE},
    {"hsl_saturation", CAIRO_OPERATOR_HSL_SATURATION},
    {"hsl_color",      CAIRO_OPERATOR_HSL_COLOR},
    {"hsl_luminosity", CAIRO_OPERATOR_HSL_LUMINOSITY},
};

bool name_to_operator_(Name n, cairo_operator_t& out)
{
    std::string const& s = n.toString();
    for (auto const& e : k_compositors)
        if (s == e.name) { out = e.op; return true; }
    return false;
}

Name operator_to_name_(cairo_operator_t op)
{
    for (auto const& e : k_compositors)
        if (e.op == op) return Name(e.name);
    return Name("unknown");
}

}  // namespace

// `name setoperator -> -`. Accepts either a literal name or a string.
class SetOperatorFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        std::string s;
        if (!token_to_string(i->pick(0), s))
        {
            i->raiseerror(i->ArgumentTypeError);
            return;
        }
        cairo_operator_t op;
        if (!name_to_operator_(Name(s), op))
        {
            i->raiseerror(Name("setoperator"), Name("UnknownOperatorError"));
            return;
        }
        GraphicsContext* g = require_current_gc(i, Name("setoperator"));
        if (!g) return;
        cairo_set_operator(g->cr(), op);
        i->pop(1);
    }
};

// `currentoperator -> name` (literal name).
class CurrentOperatorFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        GraphicsContext* g = require_current_gc(i, Name("currentoperator"));
        if (!g) return;
        cairo_operator_t op = cairo_get_operator(g->cr());
        i->push(i->new_token<sli3::literaltype, Name>(operator_to_name_(op)));
    }
};

// `listfonts -> [array of strings]`. Returns every installed font
// family known to FontConfig, deduplicated and lexicographically
// sorted. On macOS where Cairo's toy text API actually goes through
// CoreText, the FontConfig list is usually the same set, but caveat
// emptor: not every name reported here will necessarily resolve via
// /findfont without falling back to a default.
class ListFontsFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        if (!FcInit())
        {
            i->raiseerror(Name("listfonts"), Name("FontConfigError"));
            return;
        }
        FcPattern* pat   = FcPatternCreate();
        FcObjectSet* os  = FcObjectSetBuild(FC_FAMILY, (char*) nullptr);
        FcFontSet* fs    = FcFontList(nullptr, pat, os);

        std::set<std::string> families;
        if (fs)
        {
            for (int k = 0; k < fs->nfont; ++k)
            {
                FcChar8* name = nullptr;
                if (FcPatternGetString(fs->fonts[k], FC_FAMILY, 0, &name)
                    == FcResultMatch && name)
                    families.emplace(reinterpret_cast<char const*>(name));
            }
            FcFontSetDestroy(fs);
        }
        FcObjectSetDestroy(os);
        FcPatternDestroy(pat);

        TokenArray* out = new TokenArray;
        out->reserve(families.size());
        for (auto const& f : families)
            out->push_back(i->new_token<sli3::stringtype, std::string>(f));
        i->push(i->new_token<sli3::arraytype, TokenArray*>(out));
    }
};

// `(text) charpath -> -`. Appends the text's glyphs to the current
// path (cairo_text_path). PS's optional bool arg controls fill vs
// stroke variant; cairo's toy API has no such distinction so we
// ignore the bool if present.
class CharPathFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        // Accept either `(text) charpath` or `(text) bool charpath`.
        int sidx = 0;
        if (i->load() >= 2 && i->pick(0).is_of_type(sli3::booltype)) sidx = 1;
        if (!i->pick(sidx).is_of_type(sli3::stringtype))
        {
            i->raiseerror(i->ArgumentTypeError);
            return;
        }
        GraphicsContext* g = require_current_gc(i, Name("charpath"));
        if (!g) return;
        cairo_t* cr = g->cr();
        double x = 0, y = 0;
        if (cairo_has_current_point(cr))
            cairo_get_current_point(cr, &x, &y);
        std::string const& s = i->pick(sidx).data_.string_val->str();
        cairo_save(cr);
        cairo_translate(cr, x, y);
        cairo_scale(cr, 1.0, -1.0);
        cairo_move_to(cr, 0.0, 0.0);
        cairo_text_path(cr, s.c_str());
        cairo_restore(cr);
        i->pop(sidx + 1);
    }
};

//------------------------------------------------------------------------
// Display
//------------------------------------------------------------------------

class ShowPageFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        GraphicsContext* g = require_current_gc(i, Name("showpage"));
        if (!g) return;
        g->present();
    }
};

class FlushPageFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        GraphicsContext* g = require_current_gc(i, Name("flushpage"));
        if (!g) return;
        g->present();
    }
};

// `seconds wait -> -` — pump events for at most `seconds`, return
// early if the user closes the window. Always bounded; never blocks
// the REPL the way the now-removed /interact did.
class WaitFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        if (!is_numeric(i->pick(0)))
        {
            i->raiseerror(i->ArgumentTypeError);
            return;
        }
        GraphicsContext* g = require_current_gc(i, Name("wait"));
        if (!g) return;
        double secs = as_double(i->pick(0));
        g->pump_for(secs);
        i->pop(1);
    }
};

// `-> bool` — true if the user has closed the window.
class WindowClosedFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        GraphicsContext* g = require_current_gc(i, Name("windowclosed"));
        if (!g) return;
        i->push<bool>(g->window_closed());
    }
};

//------------------------------------------------------------------------
// Static operator instances
//------------------------------------------------------------------------

NewPageFunction      newpage_fn;
NewOffscreenFunction newoffscreen_fn;
NewHidpiOffscreenFunction newhidpioffscreen_fn;
NewFileBackendFunction<&GraphicsContext::open_pdf> newpdf_fn{Name("newpdf")};
NewFileBackendFunction<&GraphicsContext::open_svg> newsvg_fn{Name("newsvg")};
ClosePageFunction    closepage_fn;
CurrentPageFunction  currentpage_fn;
SetPageFunction      setpage_fn;
WritePngFunction     writepng_fn;
LoadPngFunction      loadpng_fn;
DrawImageFunction    drawimage_fn;
LinearPatternFunction  linearpattern_fn;
RadialPatternFunction  radialpattern_fn;
AddColorStopFunction   addcolorstop_fn;
SetPatternFunction     setpattern_fn;
NewPathFunction      newpath_fn;
ClosePathFunction    closepath_fn;
XYFunction<cairo_move_to>     moveto_fn   {Name("moveto")};
XYFunction<cairo_line_to>     lineto_fn   {Name("lineto")};
XYFunction<cairo_rel_move_to> rmoveto_fn  {Name("rmoveto")};
XYFunction<cairo_rel_line_to> rlineto_fn  {Name("rlineto")};
XYFunction<cairo_translate>   translate_fn{Name("translate")};
XYFunction<cairo_scale>       scale_fn    {Name("scale")};
CurveFunction<cairo_curve_to>     curveto_fn  {Name("curveto")};
CurveFunction<cairo_rel_curve_to> rcurveto_fn {Name("rcurveto")};
ArcNFunction         arcn_fn;
ArctFunction         arct_fn;
CurrentMatrixFunction currentmatrix_fn;
SetMatrixFunction    setmatrix_fn;
ConcatFunction       concat_fn;
InitMatrixFunction   initmatrix_fn;
TransformFunction    transform_fn;
ITransformFunction   itransform_fn;
CurrentLineWidthFunction currentlinewidth_fn;
CurrentLineCapFunction   currentlinecap_fn;
CurrentLineJoinFunction  currentlinejoin_fn;
CurrentRgbColorFunction  currentrgbcolor_fn;
CurrentRgbaColorFunction currentrgbacolor_fn;
CurrentDashFunction      currentdash_fn;
SetWindowTitleFunction   setwindowtitle_fn;
ResizeFunction           resize_fn;
PageSizeFunction         pagesize_fn;
SetWindowPosFunction     setwindowpos_fn;
SetFullscreenFunction    setfullscreen_fn;
TextExtentsFunction      textextents_fn;
RectFunction         rect_fn;
ArcFunction          arc_fn;
CircleFunction       circle_fn;
StrokeFunction       stroke_fn;
FillFunction         fill_fn;
ErasePageFunction    erasepage_fn;
ClearTransparentFunction cleartransparent_fn;
SetRgbColorFunction  setrgbcolor_fn;
SetGrayFunction      setgray_fn;
SetHsbColorFunction  sethsbcolor_fn;
SetRgbaColorFunction setrgbacolor_fn;
SetLineWidthFunction setlinewidth_fn;
SetLineCapFunction   setlinecap_fn;
SetLineJoinFunction  setlinejoin_fn;
SetMiterLimitFunction setmiterlimit_fn;
SetDashFunction      setdash_fn;
ClipFunction         clip_fn;
EoClipFunction       eoclip_fn;
CurrentPointFunction currentpoint_fn;
PathBBoxFunction     pathbbox_fn;
ClipPathFunction     clippath_fn;
FlattenPathFunction  flattenpath_fn;
PathForAllFunction   pathforall_fn;
GSaveFunction        gsave_fn;
GRestoreFunction     grestore_fn;
RotateFunction       rotate_fn;
FindFontFunction     findfont_fn;
ScaleFontFunction    scalefont_fn;
SetFontFunction      setfont_fn;
CurrentFontFunction  currentfont_fn;
ShowFunction         show_fn;
StringWidthFunction  stringwidth_fn;
CharPathFunction     charpath_fn;
ListFontsFunction    listfonts_fn;
SetOperatorFunction     setoperator_fn;
CurrentOperatorFunction currentoperator_fn;
SetAntialiasFunction       setantialias_fn;
CurrentAntialiasFunction   currentantialias_fn;
SetHintingFunction         sethinting_fn;
CurrentHintingFunction     currenthinting_fn;
SetHintMetricsFunction     sethintmetrics_fn;
CurrentHintMetricsFunction currenthintmetrics_fn;
ShowPageFunction     showpage_fn;
FlushPageFunction    flushpage_fn;
WaitFunction         wait_fn;
WindowClosedFunction windowclosed_fn;

}  // namespace

// Build /gfxstatus and bind it in systemdict. This is the home for
// runtime state the module needs to thread through user code without
// requiring the user to plumb a context object: /currentpage (the
// active drawing target), /currentfont (the active font dict),
// /gfxoutdir (the output directory for demo files). Mutable -- ops
// write into it during execution. graphicslib.sli sets /gfxoutdir
// at module-load time; /currentpage and /currentfont start unset and
// appear the first time their respective op writes them.
void install_gfxstatus_dict_(SLIInterpreter* i)
{
    Dictionary* d = new Dictionary;
    i->def(Name("gfxstatus"),
           i->new_token<sli3::dictionarytype, Dictionary*>(d));
}

// Build the read-only /compositors dictionary in systemdict, mapping
// each operator name to its underlying cairo_operator_t value. Lets
// SLI code introspect (compositors keys, compositors /multiply known)
// without needing to remember the table. The integer values are an
// implementation detail; user code passes /name to /setoperator, not
// the integer.
void install_compositors_dict_(SLIInterpreter* i)
{
    Dictionary* d = new Dictionary;
    for (auto const& e : k_compositors)
        d->insert(Name(e.name),
                  i->new_token<sli3::integertype, long>(
                      static_cast<long>(e.op)));
    d->set_access(sli3::ACCESS_READONLY);
    i->def(Name("compositors"),
           i->new_token<sli3::dictionarytype, Dictionary*>(d));
}

// Build the `color` dictionary and bind it in systemdict. Each entry
// is a 3-element double array [r g b] in [0,1]; the polymorphic
// /setrgbcolor accepts the array form, so SLI code can write
// `color /red get setrgbcolor` instead of remembering RGB triples.
//
// Names follow CSS Color Module Level 3 spelling where they overlap;
// `gray` and `grey` are both bound so neither dialect surprises a
// user. This is not a full CSS palette -- just the obvious 20 or so
// that come up in demos.
void install_color_dict_(SLIInterpreter* i)
{
    Dictionary* d = new Dictionary;
    auto add = [&](char const* name, double r, double g, double b)
    {
        TokenArray* a = new TokenArray;
        a->reserve(3);
        a->push_back(i->new_token<sli3::doubletype, double>(r));
        a->push_back(i->new_token<sli3::doubletype, double>(g));
        a->push_back(i->new_token<sli3::doubletype, double>(b));
        a->set_access(sli3::ACCESS_READONLY);
        d->insert(Name(name),
                  i->new_token<sli3::arraytype, TokenArray*>(a));
    };
    add("black",     0.00, 0.00, 0.00);
    add("white",     1.00, 1.00, 1.00);
    add("gray",      0.50, 0.50, 0.50);
    add("grey",      0.50, 0.50, 0.50);
    add("darkgray",  0.25, 0.25, 0.25);
    add("lightgray", 0.75, 0.75, 0.75);
    add("red",       1.00, 0.00, 0.00);
    add("green",     0.00, 0.50, 0.00);   // CSS 'green' is half-bright
    add("blue",      0.00, 0.00, 1.00);
    add("lime",      0.00, 1.00, 0.00);   // pure green
    add("cyan",      0.00, 1.00, 1.00);
    add("magenta",   1.00, 0.00, 1.00);
    add("yellow",    1.00, 1.00, 0.00);
    add("orange",    1.00, 0.65, 0.00);
    add("purple",    0.50, 0.00, 0.50);
    add("brown",     0.65, 0.16, 0.16);
    add("pink",      1.00, 0.75, 0.80);
    add("navy",      0.00, 0.00, 0.50);
    add("olive",     0.50, 0.50, 0.00);
    add("teal",      0.00, 0.50, 0.50);
    add("silver",    0.75, 0.75, 0.75);
    d->set_access(sli3::ACCESS_READONLY);
    i->def(Name("color"),
           i->new_token<sli3::dictionarytype, Dictionary*>(d));
}

void init_sligraphics(SLIInterpreter* i)
{
    install_gfxstatus_dict_(i);
    install_color_dict_(i);
    install_compositors_dict_(i);

    // Surface lifecycle
    i->createcommand("newpage",      &newpage_fn);
    i->createcommand("newoffscreen", &newoffscreen_fn);
    i->createcommand("newhidpioffscreen", &newhidpioffscreen_fn);
    i->createcommand("newpdf",       &newpdf_fn);
    i->createcommand("newsvg",       &newsvg_fn);
    i->createcommand("closepage",    &closepage_fn);
    i->createcommand("currentpage",  &currentpage_fn);
    i->createcommand("setpage",      &setpage_fn);
    i->createcommand("writepng",     &writepng_fn);
    i->createcommand("loadpng",      &loadpng_fn);
    i->createcommand("drawimage",    &drawimage_fn);

    // Gradients / patterns
    i->createcommand("linearpattern", &linearpattern_fn);
    i->createcommand("radialpattern", &radialpattern_fn);
    i->createcommand("addcolorstop",  &addcolorstop_fn);
    i->createcommand("setpattern",    &setpattern_fn);
    i->createcommand("pagesize",     &pagesize_fn);
    i->createcommand("setwindowtitle", &setwindowtitle_fn);
    i->createcommand("resize",         &resize_fn);
    i->createcommand("setwindowpos",   &setwindowpos_fn);
    i->createcommand("setfullscreen",  &setfullscreen_fn);

    // Path construction
    i->createcommand("newpath",      &newpath_fn);
    i->createcommand("closepath",    &closepath_fn);
    i->createcommand("moveto",       &moveto_fn);
    i->createcommand("lineto",       &lineto_fn);
    i->createcommand("rmoveto",      &rmoveto_fn);
    i->createcommand("rlineto",      &rlineto_fn);
    i->createcommand("curveto",      &curveto_fn);
    i->createcommand("rcurveto",     &rcurveto_fn);
    i->createcommand("arcn",         &arcn_fn);
    i->createcommand("arct",         &arct_fn);

    // Shape shortcuts
    i->createcommand("rect",         &rect_fn);
    i->createcommand("arc",          &arc_fn);
    i->createcommand("circle",       &circle_fn);

    // Painting + clipping
    i->createcommand("stroke",       &stroke_fn);
    i->createcommand("fill",         &fill_fn);
    i->createcommand("erasepage",    &erasepage_fn);
    i->createcommand("cleartransparent", &cleartransparent_fn);
    i->createcommand("clip",         &clip_fn);
    i->createcommand("eoclip",       &eoclip_fn);
    i->createcommand("currentpoint", &currentpoint_fn);
    i->createcommand("pathbbox",     &pathbbox_fn);
    i->createcommand("clippath",     &clippath_fn);
    i->createcommand("flattenpath",  &flattenpath_fn);
    i->createcommand("pathforall",   &pathforall_fn);

    // CTM / matrix
    i->createcommand("currentmatrix", &currentmatrix_fn);
    i->createcommand("setmatrix",     &setmatrix_fn);
    i->createcommand("concat",        &concat_fn);
    i->createcommand("initmatrix",    &initmatrix_fn);
    i->createcommand("transform",     &transform_fn);
    i->createcommand("itransform",    &itransform_fn);

    // State queries
    i->createcommand("currentlinewidth", &currentlinewidth_fn);
    i->createcommand("currentlinecap",   &currentlinecap_fn);
    i->createcommand("currentlinejoin",  &currentlinejoin_fn);
    i->createcommand("currentrgbcolor",  &currentrgbcolor_fn);
    i->createcommand("currentrgbacolor", &currentrgbacolor_fn);
    i->createcommand("currentdash",      &currentdash_fn);

    // Text metrics
    i->createcommand("textextents",  &textextents_fn);

    // Color
    i->createcommand("setrgbcolor",  &setrgbcolor_fn);
    i->createcommand("setgray",      &setgray_fn);
    i->createcommand("sethsbcolor",  &sethsbcolor_fn);
    i->createcommand("setrgbacolor", &setrgbacolor_fn);

    // Line state
    i->createcommand("setlinewidth",  &setlinewidth_fn);
    i->createcommand("setlinecap",    &setlinecap_fn);
    i->createcommand("setlinejoin",   &setlinejoin_fn);
    i->createcommand("setmiterlimit", &setmiterlimit_fn);
    i->createcommand("setdash",       &setdash_fn);

    // State stack
    i->createcommand("gsave",        &gsave_fn);
    i->createcommand("grestore",     &grestore_fn);

    // Transforms
    i->createcommand("translate",    &translate_fn);
    i->createcommand("scale",        &scale_fn);
    i->createcommand("rotate",       &rotate_fn);

    // Text
    i->createcommand("findfont",     &findfont_fn);
    i->createcommand("scalefont",    &scalefont_fn);
    i->createcommand("setfont",      &setfont_fn);
    i->createcommand("currentfont",  &currentfont_fn);
    i->createcommand("show",         &show_fn);
    i->createcommand("stringwidth",  &stringwidth_fn);
    i->createcommand("charpath",     &charpath_fn);
    i->createcommand("listfonts",    &listfonts_fn);

    // Compositing
    i->createcommand("setoperator",     &setoperator_fn);
    i->createcommand("currentoperator", &currentoperator_fn);

    // Rendering-quality controls
    i->createcommand("setantialias",       &setantialias_fn);
    i->createcommand("currentantialias",   &currentantialias_fn);
    i->createcommand("sethinting",         &sethinting_fn);
    i->createcommand("currenthinting",     &currenthinting_fn);
    i->createcommand("sethintmetrics",     &sethintmetrics_fn);
    i->createcommand("currenthintmetrics", &currenthintmetrics_fn);

    // Display
    i->createcommand("showpage",     &showpage_fn);
    i->createcommand("flushpage",    &flushpage_fn);
    i->createcommand("wait",         &wait_fn);
    i->createcommand("windowclosed", &windowclosed_fn);
}

}  // namespace sli3
