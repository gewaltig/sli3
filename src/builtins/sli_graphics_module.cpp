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
#include "sli_name.h"
#include "sli_string.h"
#include "sli_token.h"
#include "sli_type.h"

#include <cairo/cairo.h>
#include <SDL2/SDL.h>

#include <cmath>
#include <exception>
#include <string>
#include <vector>

namespace sli3
{
namespace
{

//------------------------------------------------------------------------
// Helpers
//------------------------------------------------------------------------

// Stored in globaldict, NOT systemdict, so it isn't shadowed by the
// /currentpage operator. The leading colon follows the NEST 2.x
// convention for runtime-state names that live in globaldict
// (e.g. /:tictime, /:cliccycles in lib/sli/sli-init.sli).
Name const& current_page_slot()
{
    static Name n(":currentpage");
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

// Resolve globaldict via the dictstack lookup (no private accessor
// required). globaldict was inserted into systemdict by
// init_dictionaries, so the lookup always succeeds in a healthy
// interpreter.
Dictionary* get_globaldict(SLIInterpreter* i)
{
    Token g;
    if (!i->lookup(Name("globaldict"), g)) return nullptr;
    if (!g.is_of_type(sli3::dictionarytype)) return nullptr;
    return g.data_.dict_val;
}

// Read /currentpage from globaldict, returning nullptr if absent or
// the entry has been invalidated by /closepage. Does NOT raise; the
// caller decides what to do.
GraphicsContext* peek_current_gc(SLIInterpreter* i)
{
    Dictionary* gd = get_globaldict(i);
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
    Dictionary* gd = get_globaldict(i);
    if (gd) gd->insert(current_page_slot(), gc_tok);
}

// Stored in globaldict alongside /:currentpage. Mirrors PS's currentfont.
Name const& current_font_slot()
{
    static Name n(":currentfont");
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
    Dictionary* gd = get_globaldict(i);
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

//------------------------------------------------------------------------
// State
//------------------------------------------------------------------------

// `r g b setrgbcolor -> -` or `[r g b] setrgbcolor -> -`.
// The array form pairs naturally with the /color dictionary
// (`color /red get setrgbcolor`).
class SetRgbColorFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        // Array form: a single 3-element numeric array on top.
        if (i->pick(0).is_of_type(sli3::arraytype))
        {
            TokenArray const* a = i->pick(0).data_.array_val;
            if (a->size() != 3
                || !is_numeric(a->get(0))
                || !is_numeric(a->get(1))
                || !is_numeric(a->get(2)))
            {
                i->raiseerror(Name("setrgbcolor"), Name("ColorShapeError"));
                return;
            }
            GraphicsContext* g = require_current_gc(i, Name("setrgbcolor"));
            if (!g) return;
            cairo_set_source_rgb(g->cr(),
                                 as_double(a->get(0)),
                                 as_double(a->get(1)),
                                 as_double(a->get(2)));
            i->pop(1);
            return;
        }
        // Three-scalar form (current behavior).
        i->require_stack_load(3);
        for (int k = 0; k < 3; ++k)
            if (!is_numeric(i->pick(k)))
            {
                i->raiseerror(i->ArgumentTypeError);
                return;
            }
        GraphicsContext* g = require_current_gc(i, Name("setrgbcolor"));
        if (!g) return;
        double r = as_double(i->pick(2));
        double gr = as_double(i->pick(1));
        double b = as_double(i->pick(0));
        cairo_set_source_rgb(g->cr(), r, gr, b);
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

// `r g b a setrgbacolor -> -` (PS extension matching GS; sets alpha).
class SetRgbaColorFunction : public SLIFunction
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
        GraphicsContext* g = require_current_gc(i, Name("setrgbacolor"));
        if (!g) return;
        double r = as_double(i->pick(3));
        double gn = as_double(i->pick(2));
        double b = as_double(i->pick(1));
        double a = as_double(i->pick(0));
        cairo_set_source_rgba(g->cr(), r, gn, b, a);
        i->pop(4);
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
        Dictionary* gd = get_globaldict(i);
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
        Dictionary* gd = get_globaldict(i);
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
NewFileBackendFunction<&GraphicsContext::open_pdf> newpdf_fn{Name("newpdf")};
NewFileBackendFunction<&GraphicsContext::open_svg> newsvg_fn{Name("newsvg")};
ClosePageFunction    closepage_fn;
CurrentPageFunction  currentpage_fn;
SetPageFunction      setpage_fn;
WritePngFunction     writepng_fn;
LoadPngFunction      loadpng_fn;
DrawImageFunction    drawimage_fn;
NewPathFunction      newpath_fn;
ClosePathFunction    closepath_fn;
XYFunction<cairo_move_to>     moveto_fn   {Name("moveto")};
XYFunction<cairo_line_to>     lineto_fn   {Name("lineto")};
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
CurrentDashFunction      currentdash_fn;
SetWindowTitleFunction   setwindowtitle_fn;
ResizeFunction           resize_fn;
PageSizeFunction         pagesize_fn;
TextExtentsFunction      textextents_fn;
RectFunction         rect_fn;
ArcFunction          arc_fn;
CircleFunction       circle_fn;
StrokeFunction       stroke_fn;
FillFunction         fill_fn;
ErasePageFunction    erasepage_fn;
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
ShowPageFunction     showpage_fn;
FlushPageFunction    flushpage_fn;
WaitFunction         wait_fn;
WindowClosedFunction windowclosed_fn;

}  // namespace

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
    install_color_dict_(i);

    // Surface lifecycle
    i->createcommand("newpage",      &newpage_fn);
    i->createcommand("newoffscreen", &newoffscreen_fn);
    i->createcommand("newpdf",       &newpdf_fn);
    i->createcommand("newsvg",       &newsvg_fn);
    i->createcommand("closepage",    &closepage_fn);
    i->createcommand("currentpage",  &currentpage_fn);
    i->createcommand("setpage",      &setpage_fn);
    i->createcommand("writepng",     &writepng_fn);
    i->createcommand("loadpng",      &loadpng_fn);
    i->createcommand("drawimage",    &drawimage_fn);
    i->createcommand("pagesize",     &pagesize_fn);
    i->createcommand("setwindowtitle", &setwindowtitle_fn);
    i->createcommand("resize",       &resize_fn);

    // Path construction
    i->createcommand("newpath",      &newpath_fn);
    i->createcommand("closepath",    &closepath_fn);
    i->createcommand("moveto",       &moveto_fn);
    i->createcommand("lineto",       &lineto_fn);
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

    // Display
    i->createcommand("showpage",     &showpage_fn);
    i->createcommand("flushpage",    &flushpage_fn);
    i->createcommand("wait",         &wait_fn);
    i->createcommand("windowclosed", &windowclosed_fn);
}

}  // namespace sli3
