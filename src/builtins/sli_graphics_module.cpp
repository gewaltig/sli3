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

#include <cmath>
#include <exception>
#include <string>

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
            g = new GraphicsContext(static_cast<int>(w), static_cast<int>(h),
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

class SetRgbColorFunction : public SLIFunction
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
ClosePageFunction    closepage_fn;
CurrentPageFunction  currentpage_fn;
NewPathFunction      newpath_fn;
ClosePathFunction    closepath_fn;
XYFunction<cairo_move_to>     moveto_fn   {Name("moveto")};
XYFunction<cairo_line_to>     lineto_fn   {Name("lineto")};
XYFunction<cairo_rel_line_to> rlineto_fn  {Name("rlineto")};
XYFunction<cairo_translate>   translate_fn{Name("translate")};
XYFunction<cairo_scale>       scale_fn    {Name("scale")};
RectFunction         rect_fn;
ArcFunction          arc_fn;
CircleFunction       circle_fn;
StrokeFunction       stroke_fn;
FillFunction         fill_fn;
ErasePageFunction    erasepage_fn;
SetRgbColorFunction  setrgbcolor_fn;
SetGrayFunction      setgray_fn;
SetLineWidthFunction setlinewidth_fn;
GSaveFunction        gsave_fn;
GRestoreFunction     grestore_fn;
RotateFunction       rotate_fn;
ShowPageFunction     showpage_fn;
FlushPageFunction    flushpage_fn;
WaitFunction         wait_fn;
WindowClosedFunction windowclosed_fn;

}  // namespace

void init_sligraphics(SLIInterpreter* i)
{
    i->createcommand("newpage",      &newpage_fn);
    i->createcommand("closepage",    &closepage_fn);
    i->createcommand("currentpage",  &currentpage_fn);

    i->createcommand("newpath",      &newpath_fn);
    i->createcommand("closepath",    &closepath_fn);
    i->createcommand("moveto",       &moveto_fn);
    i->createcommand("lineto",       &lineto_fn);
    i->createcommand("rlineto",      &rlineto_fn);

    i->createcommand("rect",         &rect_fn);
    i->createcommand("arc",          &arc_fn);
    i->createcommand("circle",       &circle_fn);

    i->createcommand("stroke",       &stroke_fn);
    i->createcommand("fill",         &fill_fn);
    i->createcommand("erasepage",    &erasepage_fn);

    i->createcommand("setrgbcolor",  &setrgbcolor_fn);
    i->createcommand("setgray",      &setgray_fn);
    i->createcommand("setlinewidth", &setlinewidth_fn);

    i->createcommand("gsave",        &gsave_fn);
    i->createcommand("grestore",     &grestore_fn);

    i->createcommand("translate",    &translate_fn);
    i->createcommand("scale",        &scale_fn);
    i->createcommand("rotate",       &rotate_fn);

    i->createcommand("showpage",     &showpage_fn);
    i->createcommand("flushpage",    &flushpage_fn);
    i->createcommand("wait",         &wait_fn);
    i->createcommand("windowclosed", &windowclosed_fn);
}

}  // namespace sli3
