// Tests for sli_graphics_module.cpp -- the PostScript-flavored 2D
// drawing surface backed by Cairo + SDL2. Runs under
// SDL_VIDEODRIVER=dummy so a window manager is not required (CI / SSH
// sessions). The dummy backend exercises the full code path -- window
// creation, software renderer, texture upload, present, event pump --
// just without putting pixels on a real screen.
//
// Three layers of coverage:
//   1. Type registration: graphicscontexttype is in the SLIInterpreter.
//   2. Direct .execute(&i) on the C++ ops (newpage / setrgbcolor /
//      moveto / lineto / stroke / fill / closepage), inspecting the
//      operand stack and the underlying Cairo state.
//   3. Refcount + close-state invariants: a closed context is invalid
//      and subsequent ops raise; a serialize round-trip emits the
//      documented warning and yields an invalid context.

#include "sli_array.h"
#include "sli_dictionary.h"
#include "sli_function.h"
#include "sli_graphics.h"
#include "sli_graphicscontexttype.h"
#include "sli_interpreter.h"
#include "sli_name.h"
#include "sli_serialize.h"
#include "sli_string.h"
#include "sli_token.h"
#include "sli_tokenstack.h"
#include "sli_type.h"
#include "test_harness.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

using namespace sli3;

namespace
{

#define CHECK(cond)                                                    \
    do {                                                               \
        if (!(cond)) {                                                 \
            std::cerr << "FAIL @" << __LINE__ << ": " #cond "\n";      \
            std::exit(1);                                              \
        }                                                              \
    } while (0)

// Ops follow the uniform new-ABI: the dispatcher pre-pops the fn
// slot before calling fn->execute, and the body must NOT pop the
// estack itself. Direct .execute(&i) invocations from the test skip
// the dispatcher, so we simply don't push a placeholder either.

Token int_token(SLIInterpreter& i, long n)
{
    return i.new_token<sli3::integertype, long>(n);
}

Token double_token(SLIInterpreter& i, double d)
{
    return i.new_token<sli3::doubletype, double>(d);
}

void run_op(SLIInterpreter& i, char const* name)
{
    Token& fn = i.lookup(Name(name));
    if (!fn.is_of_type(sli3::functiontype))
    {
        std::cerr << "lookup of " << name << " returned tag=" << fn.tag()
                  << ", expected functiontype=" << sli3::functiontype << "\n";
        std::exit(1);
    }
    fn.data_.func_val->execute(&i);
}

void test_type_registered(SLIInterpreter& i)
{
    SLIType* t = i.get_type(sli3::graphicscontexttype);
    CHECK(t != 0);
    CHECK(t->get_typename() == Name("graphicscontexttype"));
    CHECK(t->is_executable() == false);
}

// Open a page, sanity-check that newpage left a graphicscontexttype
// on the operand stack and registered it under /currentpage in
// globaldict. The wrapper carries the requested dimensions and a
// valid drawing context.
GraphicsContext* test_newpage(SLIInterpreter& i)
{
    i.push(int_token(i, 120));
    i.push(int_token(i, 80));
    run_op(i, "newpage");
    CHECK(i.load() == 1);
    CHECK(i.top().is_of_type(sli3::graphicscontexttype));
    GraphicsContext* g = i.top().data_.graphics_val;
    CHECK(g != 0);
    CHECK(g->valid());
    CHECK(g->width() == 120);
    CHECK(g->height() == 80);

    // /currentpage in globaldict points at the same wrapper.
    Token gd;
    CHECK(i.lookup(Name("globaldict"), gd));
    CHECK(gd.is_of_type(sli3::dictionarytype));
    Dictionary* d = gd.data_.dict_val;
    CHECK(d != 0);
    Token cp;
    CHECK(d->lookup(Name(":currentpage"), cp));
    CHECK(cp.is_of_type(sli3::graphicscontexttype));
    CHECK(cp.data_.graphics_val == g);

    return g;
}

// Exercise the drawing path. The dummy SDL backend doesn't render
// pixels we can inspect, but the ops still execute the full Cairo
// chain; what we verify is that they don't crash, the stack effects
// are correct, and the underlying surface remains valid.
void test_draw_ops(SLIInterpreter& i, GraphicsContext* g)
{
    // setrgbcolor: 3 args, no return.
    i.push(double_token(i, 1.0));
    i.push(double_token(i, 0.0));
    i.push(double_token(i, 0.0));
    run_op(i, "setrgbcolor");
    CHECK(i.load() == 1);  // unchanged (the page is still on top)

    // moveto / lineto / closepath / fill -- triangle path.
    i.push(int_token(i, 10));
    i.push(int_token(i, 10));
    run_op(i, "moveto");
    CHECK(i.load() == 1);

    i.push(int_token(i, 50));
    i.push(int_token(i, 10));
    run_op(i, "lineto");
    CHECK(i.load() == 1);

    i.push(int_token(i, 30));
    i.push(int_token(i, 60));
    run_op(i, "lineto");
    CHECK(i.load() == 1);

    run_op(i, "closepath");
    CHECK(i.load() == 1);

    run_op(i, "fill");
    CHECK(i.load() == 1);

    // gsave / grestore -- counts must balance, Cairo asserts on stack
    // underflow otherwise.
    run_op(i, "gsave");
    run_op(i, "grestore");

    // present -- under the dummy backend this is a no-op render but
    // still walks SDL_UpdateTexture / SDL_RenderPresent.
    run_op(i, "showpage");

    // wait 0 -- the bounded pump should be a no-op at 0 seconds, but
    // it still has to clean up its operand correctly.
    i.push(double_token(i, 0.0));
    run_op(i, "wait");
    CHECK(i.load() == 1);

    // windowclosed predicate -- under the dummy backend nobody clicks
    // close, so this should be false.
    run_op(i, "windowclosed");
    CHECK(i.load() == 2);
    CHECK(i.top().is_of_type(sli3::booltype));
    CHECK(i.top().data_.bool_val == false);
    i.pop(1);

    CHECK(g->valid());  // surface still alive after the draw
}

// Verify currentpage retrieves the same context that newpage left in
// globaldict and bumps the refcount (so the wrapper isn't released
// when the original stack entry is dropped).
void test_currentpage(SLIInterpreter& i, GraphicsContext* g)
{
    std::uint32_t before = g->references();
    run_op(i, "currentpage");
    CHECK(i.load() == 2);
    CHECK(i.top().is_of_type(sli3::graphicscontexttype));
    CHECK(i.top().data_.graphics_val == g);
    CHECK(g->references() == before + 1);
    i.pop(1);
    CHECK(g->references() == before);
}

// closepage releases the SDL window + Cairo surface, marks the
// wrapper invalid, and clears /currentpage. Subsequent ops that need
// the current page should raise NoCurrentPageError.
void test_closepage_invalidates(SLIInterpreter& i)
{
    // Operand stack at this point: [gc]. closepage consumes it.
    CHECK(i.load() == 1);
    CHECK(i.top().is_of_type(sli3::graphicscontexttype));
    GraphicsContext* g = i.top().data_.graphics_val;

    // Take an extra reference so we can inspect the wrapper after the
    // operand stack drops it -- closepage tears the window down but
    // the wrapper memory survives until refcount hits zero.
    g->add_reference();

    run_op(i, "closepage");
    CHECK(i.load() == 0);
    CHECK(!g->valid());  // window torn down

    // /:currentpage should be gone from globaldict.
    Token gd;
    CHECK(i.lookup(Name("globaldict"), gd));
    Dictionary* d = gd.data_.dict_val;
    Token cp;
    CHECK(!d->lookup(Name(":currentpage"), cp));

    // A draw op now should raise -- we run it directly and watch
    // errordict /newerror flip on.
    Dictionary& ed = i.error_dict();
    ed.insert(Name("newerror"), i.new_token<sli3::booltype, bool>(false));
    Token& fn = i.lookup(Name("stroke"));
    fn.data_.func_val->execute(&i);
    Token ne;
    CHECK(ed.lookup(Name("newerror"), ne));
    CHECK(ne.is_of_type(sli3::booltype));
    CHECK(ne.data_.bool_val == true);
    // Clean up the error state so subsequent tests aren't poisoned.
    ed.insert(Name("newerror"), i.new_token<sli3::booltype, bool>(false));
    i.EStack().clear();
    i.OStack().clear();

    // Drop our extra reference; this triggers the wrapper delete.
    g->remove_reference();
}

void test_serialize_round_trip(SLIInterpreter& i)
{
    // Snapshot the warning to stderr without polluting the test
    // output. We don't assert on the exact text -- just that the
    // round-trip yields a Token of the right type whose backing
    // context is invalid (per the streams contract).
    Token src(i.get_type(sli3::graphicscontexttype));
    src.data_.graphics_val = GraphicsContext::open_offscreen(40, 40);

    std::stringbuf buf;
    {
        std::ostream out(&buf);
        BinaryWriter w(out);
        w.write_header();
        std::cerr.setstate(std::ios::failbit);  // suppress the warning
        write_token(src, w);
        std::cerr.clear();
    }

    {
        std::istream in(&buf);
        BinaryReader r(in);
        r.read_header();
        Token dst = read_token(r, i);
        CHECK(dst.is_of_type(sli3::graphicscontexttype));
        GraphicsContext* g = dst.data_.graphics_val;
        // Either an invalid context (close() ran in deserialize) or
        // null (no SDL available in the test environment). Both are
        // "not usable for drawing" -- the contract we promise.
        CHECK(g == nullptr || !g->valid());
    }
}

// Offscreen + curve + clip + writepng. No SDL involvement at any
// point -- exercises the extension-pass code path that batch/CI
// workflows would actually use.
void test_offscreen_curve_clip_png(SLIInterpreter& i)
{
    // Build a 60x40 offscreen surface as the current page.
    i.push(int_token(i, 60));
    i.push(int_token(i, 40));
    run_op(i, "newoffscreen");
    CHECK(i.load() == 1);
    CHECK(i.top().is_of_type(sli3::graphicscontexttype));
    GraphicsContext* g = i.top().data_.graphics_val;
    CHECK(g != nullptr && g->valid());
    CHECK(g->backend() == GraphicsContext::Backend::OFFSCREEN);
    CHECK(g->is_image_surface());

    // Path with a cubic curve, then clip to a rectangle, then fill.
    run_op(i, "newpath");
    i.push(int_token(i, 5)); i.push(int_token(i, 5)); run_op(i, "moveto");
    i.push(int_token(i, 10)); i.push(int_token(i, 35));
    i.push(int_token(i, 50)); i.push(int_token(i, 35));
    i.push(int_token(i, 55)); i.push(int_token(i, 5));
    run_op(i, "curveto");
    run_op(i, "closepath");

    // Clip with a smaller rect; the subsequent fill should be inside it.
    run_op(i, "gsave");
    run_op(i, "newpath");
    i.push(int_token(i, 10)); i.push(int_token(i, 10));
    i.push(int_token(i, 40)); i.push(int_token(i, 20));
    run_op(i, "rect");
    run_op(i, "clip");
    i.push(double_token(i, 0.3));
    i.push(double_token(i, 0.6));
    i.push(double_token(i, 0.9));
    run_op(i, "setrgbcolor");
    // Big circle that will be cropped by the clip.
    i.push(int_token(i, 30)); i.push(int_token(i, 20)); i.push(int_token(i, 50));
    run_op(i, "circle");
    run_op(i, "fill");
    run_op(i, "grestore");

    // Write PNG: (path) gc writepng
    std::string path = "/tmp/sli3-test-offscreen.png";
    std::remove(path.c_str());
    i.push(i.new_token<sli3::stringtype, std::string>(path));
    run_op(i, "currentpage");
    run_op(i, "writepng");
    CHECK(i.load() == 1);  // gc still on stack (currentpage was popped by writepng)
    // File should exist and be non-empty.
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    CHECK(f.good());
    CHECK(f.tellg() > 0);
    f.close();
    std::remove(path.c_str());

    run_op(i, "closepage");
    CHECK(i.load() == 0);
}

// arct exercise on an offscreen surface. Builds a rounded rectangle
// path (4 corner fillets via arct), fills it, and writes a PNG to
// confirm nothing crashes on the analytic-geometry path. Also asserts
// that calling arct without a current point raises NoCurrentPointError.
void test_arct(SLIInterpreter& i)
{
    // Open a small offscreen page.
    i.push(int_token(i, 80));
    i.push(int_token(i, 60));
    run_op(i, "newoffscreen");
    CHECK(i.load() == 1);
    i.pop(1);

    // Missing current point -> NoCurrentPointError. Newpath clears any
    // current point left by the white-background fill in finish_cairo_setup_.
    run_op(i, "newpath");
    Dictionary& ed = i.error_dict();
    ed.insert(Name("newerror"), i.new_token<sli3::booltype, bool>(false));
    i.push(int_token(i, 10)); i.push(int_token(i, 10));
    i.push(int_token(i, 70)); i.push(int_token(i, 10));
    i.push(int_token(i, 5));
    Token& arct_fn = i.lookup(Name("arct"));
    arct_fn.data_.func_val->execute(&i);
    Token ne;
    CHECK(ed.lookup(Name("newerror"), ne));
    CHECK(ne.is_of_type(sli3::booltype));
    CHECK(ne.data_.bool_val == true);
    ed.insert(Name("newerror"), i.new_token<sli3::booltype, bool>(false));
    i.OStack().clear();
    i.EStack().clear();

    // Rounded rectangle via 4 arct fillets (corners at (10,10),
    // (70,10), (70,50), (10,50), radius 8). Start with moveto on the
    // top edge so each arct rounds the next corner.
    i.push(int_token(i, 18)); i.push(int_token(i, 10));
    run_op(i, "moveto");
    i.push(int_token(i, 70)); i.push(int_token(i, 10));
    i.push(int_token(i, 70)); i.push(int_token(i, 50));
    i.push(int_token(i, 8));
    run_op(i, "arct");
    i.push(int_token(i, 70)); i.push(int_token(i, 50));
    i.push(int_token(i, 10)); i.push(int_token(i, 50));
    i.push(int_token(i, 8));
    run_op(i, "arct");
    i.push(int_token(i, 10)); i.push(int_token(i, 50));
    i.push(int_token(i, 10)); i.push(int_token(i, 10));
    i.push(int_token(i, 8));
    run_op(i, "arct");
    i.push(int_token(i, 10)); i.push(int_token(i, 10));
    i.push(int_token(i, 70)); i.push(int_token(i, 10));
    i.push(int_token(i, 8));
    run_op(i, "arct");
    run_op(i, "closepath");
    i.push(double_token(i, 0.0)); i.push(double_token(i, 0.5));
    i.push(double_token(i, 0.8)); run_op(i, "setrgbcolor");
    run_op(i, "fill");

    std::string path = "/tmp/sli3-test-arct.png";
    std::remove(path.c_str());
    i.push(i.new_token<sli3::stringtype, std::string>(path));
    run_op(i, "currentpage");
    run_op(i, "writepng");
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    CHECK(f.good());
    CHECK(f.tellg() > 0);
    f.close();
    std::remove(path.c_str());

    // Drain (writepng raises on PDF reject elsewhere; on success it pops
    // both operands itself, so just close the page).
    i.OStack().clear();
    run_op(i, "currentpage");
    run_op(i, "closepage");
}

// Text round-trip: findfont returns a dict with the expected slots,
// setfont mutates globaldict /:currentfont, currentfont can recover it.
void test_text_findfont_setfont(SLIInterpreter& i)
{
    // Need a current page so setfont has something to apply to.
    i.push(int_token(i, 40));
    i.push(int_token(i, 40));
    run_op(i, "newoffscreen");
    CHECK(i.load() == 1);
    i.pop(1);  // we'll use currentpage implicitly

    // findfont (sans)
    i.push(i.new_token<sli3::stringtype, std::string>(std::string("sans")));
    run_op(i, "findfont");
    CHECK(i.load() == 1);
    CHECK(i.top().is_of_type(sli3::dictionarytype));
    Dictionary* fd = i.top().data_.dict_val;
    CHECK(fd != nullptr);
    Token fam, slant, weight, size;
    CHECK(fd->lookup(Name("Family"), fam));
    CHECK(fam.is_of_type(sli3::stringtype));
    CHECK(fam.data_.string_val->str() == "sans");
    CHECK(fd->lookup(Name("Slant"), slant));
    CHECK(slant.is_of_type(sli3::literaltype));
    CHECK(fd->lookup(Name("Weight"), weight));
    CHECK(fd->lookup(Name("Size"), size));
    CHECK(size.is_of_type(sli3::doubletype));
    CHECK(size.data_.double_val == 12.0);

    // scalefont (24) -> new dict with /Size 24
    i.push(double_token(i, 24.0));
    run_op(i, "scalefont");
    CHECK(i.load() == 1);
    Dictionary* fd24 = i.top().data_.dict_val;
    Token sz;
    CHECK(fd24->lookup(Name("Size"), sz));
    CHECK(sz.data_.double_val == 24.0);

    // setfont mutates globaldict /:currentfont
    run_op(i, "setfont");
    CHECK(i.load() == 0);
    Token gd;
    CHECK(i.lookup(Name("globaldict"), gd));
    Dictionary* gdp = gd.data_.dict_val;
    Token cf;
    CHECK(gdp->lookup(Name(":currentfont"), cf));
    CHECK(cf.is_of_type(sli3::dictionarytype));
    Dictionary* cfp = cf.data_.dict_val;
    Token cfsz;
    CHECK(cfp->lookup(Name("Size"), cfsz));
    CHECK(cfsz.data_.double_val == 24.0);

    // currentfont pushes the dict back
    run_op(i, "currentfont");
    CHECK(i.load() == 1);
    CHECK(i.top().is_of_type(sli3::dictionarytype));
    i.pop(1);

    // Tear down the offscreen surface and clean up /:currentfont so
    // later tests don't see stale state.
    run_op(i, "currentpage");
    run_op(i, "closepage");
    gdp->erase(Name(":currentfont"));
}

// PDF backend: open a PDF, draw, close, confirm the file exists
// and starts with the PDF magic. Multi-page is exercised via showpage.
void test_pdf_backend(SLIInterpreter& i)
{
    std::string path = "/tmp/sli3-test.pdf";
    std::remove(path.c_str());
    i.push(i.new_token<sli3::stringtype, std::string>(path));
    i.push(int_token(i, 100));
    i.push(int_token(i, 100));
    run_op(i, "newpdf");
    CHECK(i.load() == 1);
    GraphicsContext* g = i.top().data_.graphics_val;
    CHECK(g != nullptr && g->valid());
    CHECK(g->backend() == GraphicsContext::Backend::PDF);
    CHECK(!g->is_image_surface());

    // Draw, present (= cairo_show_page for PDF), draw again.
    i.push(double_token(i, 1.0));
    i.push(double_token(i, 0.0));
    i.push(double_token(i, 0.0));
    run_op(i, "setrgbcolor");
    i.push(int_token(i, 10)); i.push(int_token(i, 10));
    i.push(int_token(i, 80)); i.push(int_token(i, 80));
    run_op(i, "rect");
    run_op(i, "fill");
    run_op(i, "showpage");

    run_op(i, "closepage");
    CHECK(i.load() == 0);

    std::ifstream f(path, std::ios::binary);
    CHECK(f.good());
    char header[4] = {0,0,0,0};
    f.read(header, 4);
    // PDF files start with "%PDF".
    CHECK(header[0] == '%' && header[1] == 'P' && header[2] == 'D' && header[3] == 'F');
    f.close();
    std::remove(path.c_str());
}

// CTM round-trip: read currentmatrix, identity-multiply it via concat,
// confirm the matrix is unchanged (modulo floating-point noise).
// transform + itransform are inverses on any user point.
void test_ctm(SLIInterpreter& i)
{
    i.push(int_token(i, 60));
    i.push(int_token(i, 40));
    run_op(i, "newoffscreen");
    CHECK(i.load() == 1);
    i.pop(1);

    run_op(i, "currentmatrix");
    CHECK(i.load() == 1);
    CHECK(i.top().is_of_type(sli3::arraytype));
    TokenArray* m0 = i.top().data_.array_val;
    CHECK(m0->size() == 6);
    // Save the matrix elements for the comparison.
    double e0[6];
    for (size_t k = 0; k < 6; ++k) e0[k] = m0->get(k).data_.double_val;

    // Build [1 0 0 1 0 0] (identity) and concat. CTM should be unchanged.
    TokenArray* id = new TokenArray();
    id->reserve(6);
    for (int k = 0; k < 6; ++k)
    {
        double v = (k == 0 || k == 3) ? 1.0 : 0.0;
        id->push_back(i.new_token<sli3::doubletype, double>(v));
    }
    i.push(i.new_token<sli3::arraytype, TokenArray*>(id));
    run_op(i, "concat");
    CHECK(i.load() == 1);  // original CTM array still on stack
    i.pop(1);

    run_op(i, "currentmatrix");
    TokenArray* m1 = i.top().data_.array_val;
    for (size_t k = 0; k < 6; ++k)
    {
        double v = m1->get(k).data_.double_val;
        CHECK(std::fabs(v - e0[k]) < 1e-9);
    }
    i.pop(1);

    // transform + itransform round-trip on (10, 20).
    i.push(double_token(i, 10.0));
    i.push(double_token(i, 20.0));
    run_op(i, "transform");
    CHECK(i.load() == 2);
    run_op(i, "itransform");
    CHECK(i.load() == 2);
    CHECK(std::fabs(i.pick(1).data_.double_val - 10.0) < 1e-9);
    CHECK(std::fabs(i.pick(0).data_.double_val - 20.0) < 1e-9);
    i.pop(2);

    run_op(i, "currentpage");
    run_op(i, "closepage");
}

// State queries: round-trip each setter with its current* peer.
void test_state_queries(SLIInterpreter& i)
{
    i.push(int_token(i, 40));
    i.push(int_token(i, 40));
    run_op(i, "newoffscreen");
    i.pop(1);

    // line width
    i.push(double_token(i, 3.5));
    run_op(i, "setlinewidth");
    run_op(i, "currentlinewidth");
    CHECK(i.load() == 1);
    CHECK(std::fabs(i.top().data_.double_val - 3.5) < 1e-9);
    i.pop(1);

    // line cap (1 = round)
    i.push(int_token(i, 1));
    run_op(i, "setlinecap");
    run_op(i, "currentlinecap");
    CHECK(i.load() == 1);
    CHECK(i.top().data_.long_val == 1);
    i.pop(1);

    // line join (2 = bevel)
    i.push(int_token(i, 2));
    run_op(i, "setlinejoin");
    run_op(i, "currentlinejoin");
    CHECK(i.top().data_.long_val == 2);
    i.pop(1);

    // RGB color (solid source path)
    i.push(double_token(i, 0.25));
    i.push(double_token(i, 0.5));
    i.push(double_token(i, 0.75));
    run_op(i, "setrgbcolor");
    run_op(i, "currentrgbcolor");
    CHECK(i.load() == 3);
    CHECK(std::fabs(i.pick(2).data_.double_val - 0.25) < 1e-9);
    CHECK(std::fabs(i.pick(1).data_.double_val - 0.5)  < 1e-9);
    CHECK(std::fabs(i.pick(0).data_.double_val - 0.75) < 1e-9);
    i.pop(3);

    // dash
    TokenArray* dashes = new TokenArray();
    dashes->push_back(i.new_token<sli3::doubletype, double>(4.0));
    dashes->push_back(i.new_token<sli3::doubletype, double>(2.0));
    i.push(i.new_token<sli3::arraytype, TokenArray*>(dashes));
    i.push(double_token(i, 1.0));
    run_op(i, "setdash");
    run_op(i, "currentdash");
    CHECK(i.load() == 2);
    CHECK(i.pick(1).is_of_type(sli3::arraytype));
    TokenArray* out = i.pick(1).data_.array_val;
    CHECK(out->size() == 2);
    CHECK(std::fabs(out->get(0).data_.double_val - 4.0) < 1e-9);
    CHECK(std::fabs(out->get(1).data_.double_val - 2.0) < 1e-9);
    CHECK(std::fabs(i.pick(0).data_.double_val - 1.0) < 1e-9);
    i.pop(2);

    run_op(i, "currentpage");
    run_op(i, "closepage");
}

// pagesize reports the OFFSCREEN context's dimensions.
void test_pagesize(SLIInterpreter& i)
{
    i.push(int_token(i, 123));
    i.push(int_token(i, 45));
    run_op(i, "newoffscreen");
    i.pop(1);
    run_op(i, "pagesize");
    CHECK(i.load() == 2);
    CHECK(i.pick(1).data_.long_val == 123);
    CHECK(i.pick(0).data_.long_val == 45);
    i.pop(2);
    run_op(i, "currentpage");
    run_op(i, "closepage");
}

// textextents returns a dict with all six metric keys.
void test_textextents(SLIInterpreter& i)
{
    i.push(int_token(i, 40));
    i.push(int_token(i, 40));
    run_op(i, "newoffscreen");
    i.pop(1);

    // Need a font set before measuring.
    i.push(i.new_token<sli3::stringtype, std::string>(std::string("sans")));
    run_op(i, "findfont");
    i.push(double_token(i, 18.0));
    run_op(i, "scalefont");
    run_op(i, "setfont");

    i.push(i.new_token<sli3::stringtype, std::string>(std::string("hi")));
    run_op(i, "textextents");
    CHECK(i.load() == 1);
    CHECK(i.top().is_of_type(sli3::dictionarytype));
    Dictionary* d = i.top().data_.dict_val;
    for (char const* k : {"XBearing", "YBearing", "Width", "Height", "XAdvance", "YAdvance"})
    {
        Token t;
        CHECK(d->lookup(Name(k), t));
        CHECK(t.is_of_type(sli3::doubletype));
    }
    i.pop(1);

    // Cleanup currentfont so the rest of the suite isn't polluted.
    Token gd;
    CHECK(i.lookup(Name("globaldict"), gd));
    gd.data_.dict_val->erase(Name(":currentfont"));
    run_op(i, "currentpage");
    run_op(i, "closepage");
}

// loadpng + drawimage round-trip: write a PNG, load it, paint onto a
// fresh offscreen, write a new PNG, confirm the new file exists.
void test_loadpng_drawimage(SLIInterpreter& i)
{
    std::string src_path = "/tmp/sli3-test-src.png";
    std::string out_path = "/tmp/sli3-test-out.png";
    std::remove(src_path.c_str());
    std::remove(out_path.c_str());

    // Build a source PNG: 50x30 red rectangle.
    i.push(int_token(i, 50));
    i.push(int_token(i, 30));
    run_op(i, "newoffscreen");
    i.pop(1);
    i.push(double_token(i, 1.0));
    i.push(double_token(i, 0.0));
    i.push(double_token(i, 0.0));
    run_op(i, "setrgbcolor");
    i.push(int_token(i, 0)); i.push(int_token(i, 0));
    i.push(int_token(i, 50)); i.push(int_token(i, 30));
    run_op(i, "rect"); run_op(i, "fill");
    i.push(i.new_token<sli3::stringtype, std::string>(src_path));
    run_op(i, "currentpage");
    run_op(i, "writepng");
    std::ifstream src_f(src_path, std::ios::binary | std::ios::ate);
    CHECK(src_f.good());
    src_f.close();
    run_op(i, "currentpage");
    run_op(i, "closepage");

    // Now: fresh page, loadpng, drawimage at offset (10, 5) sized (50, 30).
    i.push(int_token(i, 80));
    i.push(int_token(i, 60));
    run_op(i, "newoffscreen");
    i.pop(1);

    i.push(i.new_token<sli3::stringtype, std::string>(src_path));
    run_op(i, "loadpng");
    CHECK(i.load() == 1);
    CHECK(i.top().is_of_type(sli3::graphicscontexttype));
    GraphicsContext* img = i.top().data_.graphics_val;
    CHECK(img && img->valid());
    CHECK(img->is_image_surface());
    CHECK(img->width() == 50);
    CHECK(img->height() == 30);
    // drawimage takes [gc cx cy w h] and leaves the stack empty of
    // its operands. Pass-through: dup the gc since we want to close it.
    i.index(0);  // dup -- but we want to keep one for closepage
    // Easier: push the args then call drawimage. The gc on top will be popped.
    i.push(int_token(i, 10));
    i.push(int_token(i, 5));
    i.push(int_token(i, 50));
    i.push(int_token(i, 30));
    run_op(i, "drawimage");
    // After drawimage, stack still has the dup'd loaded gc.
    CHECK(i.load() == 1);
    CHECK(i.top().is_of_type(sli3::graphicscontexttype));
    run_op(i, "closepage");  // close the loaded image
    CHECK(i.load() == 0);

    // Write the composed result.
    i.push(i.new_token<sli3::stringtype, std::string>(out_path));
    run_op(i, "currentpage");
    run_op(i, "writepng");
    std::ifstream out_f(out_path, std::ios::binary | std::ios::ate);
    CHECK(out_f.good());
    CHECK(out_f.tellg() > 0);
    out_f.close();

    std::remove(src_path.c_str());
    std::remove(out_path.c_str());

    run_op(i, "currentpage");
    run_op(i, "closepage");
}

// Color dict + polymorphic setrgbcolor: `color /red get setrgbcolor`
// must round-trip through currentrgbcolor as (1, 0, 0).
void test_color_dict(SLIInterpreter& i)
{
    i.push(int_token(i, 30));
    i.push(int_token(i, 30));
    run_op(i, "newoffscreen");
    i.pop(1);

    // Resolve `color` dict, look up /red, push as the array operand.
    Token color_tok;
    CHECK(i.lookup(Name("color"), color_tok));
    CHECK(color_tok.is_of_type(sli3::dictionarytype));
    Dictionary* cd = color_tok.data_.dict_val;
    Token red_arr;
    CHECK(cd->lookup(Name("red"), red_arr));
    CHECK(red_arr.is_of_type(sli3::arraytype));
    TokenArray const* a = red_arr.data_.array_val;
    CHECK(a->size() == 3);
    CHECK(std::fabs(a->get(0).data_.double_val - 1.0) < 1e-9);
    CHECK(std::fabs(a->get(1).data_.double_val - 0.0) < 1e-9);
    CHECK(std::fabs(a->get(2).data_.double_val - 0.0) < 1e-9);

    // The polymorphic setrgbcolor path: push the array, call setrgbcolor,
    // confirm currentrgbcolor reads back (1, 0, 0).
    i.push(red_arr);
    run_op(i, "setrgbcolor");
    CHECK(i.load() == 0);
    run_op(i, "currentrgbcolor");
    CHECK(i.load() == 3);
    CHECK(std::fabs(i.pick(2).data_.double_val - 1.0) < 1e-9);
    CHECK(std::fabs(i.pick(1).data_.double_val - 0.0) < 1e-9);
    CHECK(std::fabs(i.pick(0).data_.double_val - 0.0) < 1e-9);
    i.pop(3);

    // The scalar form must still work after polymorphism.
    i.push(double_token(i, 0.0));
    i.push(double_token(i, 1.0));
    i.push(double_token(i, 0.0));
    run_op(i, "setrgbcolor");
    run_op(i, "currentrgbcolor");
    CHECK(std::fabs(i.pick(1).data_.double_val - 1.0) < 1e-9);
    i.pop(3);

    run_op(i, "currentpage");
    run_op(i, "closepage");
}

// clippath: build a clip, then verify clippath replaces the current
// path with a rect covering the clip extents.
void test_clippath(SLIInterpreter& i)
{
    i.push(int_token(i, 100));
    i.push(int_token(i, 100));
    run_op(i, "newoffscreen");
    i.pop(1);

    // Establish a rectangular clip at (20, 30, 40, 50).
    run_op(i, "newpath");
    i.push(int_token(i, 20)); i.push(int_token(i, 30));
    i.push(int_token(i, 40)); i.push(int_token(i, 50));
    run_op(i, "rect");
    run_op(i, "clip");

    // After clippath, pathbbox should report the clip rect's extents.
    run_op(i, "clippath");
    run_op(i, "pathbbox");
    CHECK(i.load() == 4);
    CHECK(std::fabs(i.pick(3).data_.double_val - 20.0) < 1e-9);
    CHECK(std::fabs(i.pick(2).data_.double_val - 30.0) < 1e-9);
    CHECK(std::fabs(i.pick(1).data_.double_val - 60.0) < 1e-9);
    CHECK(std::fabs(i.pick(0).data_.double_val - 80.0) < 1e-9);
    i.pop(4);

    run_op(i, "currentpage");
    run_op(i, "closepage");
}

// flattenpath: build a path with curves, count segments via pathforall,
// then flattenpath, count again. The flattened version has strictly
// more segments and no curves.
void test_flattenpath_and_pathforall(SLIInterpreter& i)
{
    i.push(int_token(i, 100));
    i.push(int_token(i, 100));
    run_op(i, "newoffscreen");
    i.pop(1);

    // Path: M, curveto, lineto, closepath.
    run_op(i, "newpath");
    i.push(int_token(i, 10)); i.push(int_token(i, 10));
    run_op(i, "moveto");
    i.push(int_token(i, 30)); i.push(int_token(i, 10));
    i.push(int_token(i, 50)); i.push(int_token(i, 50));
    i.push(int_token(i, 60)); i.push(int_token(i, 30));
    run_op(i, "curveto");
    i.push(int_token(i, 70)); i.push(int_token(i, 30));
    run_op(i, "lineto");
    run_op(i, "closepath");

    // pathforall with counter procs. Bottom-of-stack counter accumulates.
    // We feed 4 procs that each push the proc-type identifier; not
    // for performance, just to verify each kind is reached.
    //
    // SLI source: 0 { pop pop 1 add } { pop pop 1 add } { pop pop pop
    // pop pop pop 1 add } { 1 add } pathforall
    using sli_test::eval;
    sli_test::clear_stacks(i);
    eval(i,
         "0 "
         "{ pop pop 1 add } "      // move
         "{ pop pop 1 add } "      // line
         "{ pop pop pop pop pop pop 1 add } "  // curve
         "{ 1 add } "              // close
         "pathforall");
    CHECK(i.load() == 1);
    CHECK(i.top().is_of_type(sli3::integertype));
    long pre_count = i.top().data_.long_val;
    CHECK(pre_count >= 4);   // at least moveto + curveto + lineto + closepath
    i.pop(1);

    // Flatten and recount: must have strictly more (curve becomes many lines).
    run_op(i, "flattenpath");
    eval(i,
         "0 "
         "{ pop pop 1 add } "
         "{ pop pop 1 add } "
         "{ pop pop pop pop pop pop 1 add } "
         "{ 1 add } "
         "pathforall");
    CHECK(i.load() == 1);
    long post_count = i.top().data_.long_val;
    CHECK(post_count > pre_count);
    i.pop(1);

    run_op(i, "currentpage");
    run_op(i, "closepage");
}

// Alpha / transparency round-trips through every entry point:
//   - 3-scalar setrgbcolor      -> alpha defaults to 1
//   - 3-array  setrgbcolor      -> alpha defaults to 1
//   - 4-array  setrgbcolor      -> alpha is the 4th element
//   - 4-scalar setrgbacolor     -> alpha is the 4th element
//   - 4-array  setrgbacolor     -> alpha is the 4th element
// each verified via /currentrgbacolor.
void test_transparency(SLIInterpreter& i)
{
    i.push(int_token(i, 30));
    i.push(int_token(i, 30));
    run_op(i, "newoffscreen");
    i.pop(1);

    auto expect_rgba = [&](double r, double g, double b, double a) {
        run_op(i, "currentrgbacolor");
        CHECK(i.load() == 4);
        CHECK(std::fabs(i.pick(3).data_.double_val - r) < 1e-9);
        CHECK(std::fabs(i.pick(2).data_.double_val - g) < 1e-9);
        CHECK(std::fabs(i.pick(1).data_.double_val - b) < 1e-9);
        CHECK(std::fabs(i.pick(0).data_.double_val - a) < 1e-9);
        i.pop(4);
    };

    // 3-scalar setrgbcolor: implicit alpha=1
    i.push(double_token(i, 0.2));
    i.push(double_token(i, 0.4));
    i.push(double_token(i, 0.6));
    run_op(i, "setrgbcolor");
    expect_rgba(0.2, 0.4, 0.6, 1.0);

    // 3-array setrgbcolor: implicit alpha=1
    {
        TokenArray* a = new TokenArray;
        a->push_back(i.new_token<sli3::doubletype, double>(0.3));
        a->push_back(i.new_token<sli3::doubletype, double>(0.5));
        a->push_back(i.new_token<sli3::doubletype, double>(0.7));
        i.push(i.new_token<sli3::arraytype, TokenArray*>(a));
    }
    run_op(i, "setrgbcolor");
    expect_rgba(0.3, 0.5, 0.7, 1.0);

    // 4-array setrgbcolor: alpha is 4th element
    {
        TokenArray* a = new TokenArray;
        a->push_back(i.new_token<sli3::doubletype, double>(1.0));
        a->push_back(i.new_token<sli3::doubletype, double>(0.0));
        a->push_back(i.new_token<sli3::doubletype, double>(0.0));
        a->push_back(i.new_token<sli3::doubletype, double>(0.5));
        i.push(i.new_token<sli3::arraytype, TokenArray*>(a));
    }
    run_op(i, "setrgbcolor");
    expect_rgba(1.0, 0.0, 0.0, 0.5);

    // 4-scalar setrgbacolor
    i.push(double_token(i, 0.1));
    i.push(double_token(i, 0.2));
    i.push(double_token(i, 0.3));
    i.push(double_token(i, 0.25));
    run_op(i, "setrgbacolor");
    expect_rgba(0.1, 0.2, 0.3, 0.25);

    // 4-array setrgbacolor (same shape rule as setrgbcolor's array form)
    {
        TokenArray* a = new TokenArray;
        a->push_back(i.new_token<sli3::doubletype, double>(0.9));
        a->push_back(i.new_token<sli3::doubletype, double>(0.8));
        a->push_back(i.new_token<sli3::doubletype, double>(0.7));
        a->push_back(i.new_token<sli3::doubletype, double>(0.6));
        i.push(i.new_token<sli3::arraytype, TokenArray*>(a));
    }
    run_op(i, "setrgbacolor");
    expect_rgba(0.9, 0.8, 0.7, 0.6);

    // ColorShapeError: array of wrong length
    {
        Dictionary& ed = i.error_dict();
        ed.insert(Name("newerror"), i.new_token<sli3::booltype, bool>(false));
        TokenArray* a = new TokenArray;
        a->push_back(i.new_token<sli3::doubletype, double>(1.0));
        a->push_back(i.new_token<sli3::doubletype, double>(0.0));
        i.push(i.new_token<sli3::arraytype, TokenArray*>(a));
        Token& fn = i.lookup(Name("setrgbcolor"));
        fn.data_.func_val->execute(&i);
        Token ne;
        CHECK(ed.lookup(Name("newerror"), ne));
        CHECK(ne.data_.bool_val == true);
        ed.insert(Name("newerror"), i.new_token<sli3::booltype, bool>(false));
        i.OStack().clear();
        i.EStack().clear();
    }

    // currentrgbcolor still drops alpha and returns 3 values.
    i.push(double_token(i, 0.4));
    i.push(double_token(i, 0.5));
    i.push(double_token(i, 0.6));
    i.push(double_token(i, 0.7));
    run_op(i, "setrgbacolor");
    run_op(i, "currentrgbcolor");
    CHECK(i.load() == 3);
    CHECK(std::fabs(i.pick(2).data_.double_val - 0.4) < 1e-9);
    CHECK(std::fabs(i.pick(0).data_.double_val - 0.6) < 1e-9);
    i.pop(3);

    run_op(i, "currentpage");
    run_op(i, "closepage");
}

// Gradient/pattern round-trip: linear + radial constructors, add a
// color stop, install via setpattern, fill a rect. The actual pixel
// content isn't checked, just that the chain doesn't crash or leak
// and writepng produces a valid PNG.
void test_patterns(SLIInterpreter& i)
{
    i.push(int_token(i, 60));
    i.push(int_token(i, 40));
    run_op(i, "newoffscreen");
    i.pop(1);

    // Linear gradient
    i.push(double_token(i, 0.0));
    i.push(double_token(i, 0.0));
    i.push(double_token(i, 60.0));
    i.push(double_token(i, 0.0));
    run_op(i, "linearpattern");
    CHECK(i.load() == 1);
    CHECK(i.top().is_of_type(sli3::patterntype));
    CairoPattern* lp = i.top().data_.pattern_val;
    CHECK(lp != nullptr && lp->valid());

    // Add two color stops (one with alpha, one without).
    i.index(0);  // dup
    i.push(double_token(i, 0.0));
    i.push(double_token(i, 1.0));
    i.push(double_token(i, 0.0));
    i.push(double_token(i, 0.0));
    run_op(i, "addcolorstop");          // 5-arg: pattern off r g b
    i.index(0);
    i.push(double_token(i, 1.0));
    i.push(double_token(i, 0.0));
    i.push(double_token(i, 0.0));
    i.push(double_token(i, 1.0));
    i.push(double_token(i, 0.5));
    run_op(i, "addcolorstop");          // 6-arg: pattern off r g b a

    // setpattern consumes the pattern.
    run_op(i, "setpattern");
    CHECK(i.load() == 0);

    // Paint with it.
    i.push(int_token(i, 0));
    i.push(int_token(i, 0));
    i.push(int_token(i, 60));
    i.push(int_token(i, 40));
    run_op(i, "rect");
    run_op(i, "fill");

    // Radial gradient round-trip
    i.push(double_token(i, 30.0));
    i.push(double_token(i, 20.0));
    i.push(double_token(i, 0.0));
    i.push(double_token(i, 30.0));
    i.push(double_token(i, 20.0));
    i.push(double_token(i, 25.0));
    run_op(i, "radialpattern");
    CHECK(i.load() == 1);
    CHECK(i.top().is_of_type(sli3::patterntype));
    i.index(0);
    i.push(double_token(i, 0.0));
    i.push(double_token(i, 1.0));
    i.push(double_token(i, 1.0));
    i.push(double_token(i, 1.0));
    run_op(i, "addcolorstop");
    i.index(0);
    i.push(double_token(i, 1.0));
    i.push(double_token(i, 0.2));
    i.push(double_token(i, 0.2));
    i.push(double_token(i, 0.8));
    run_op(i, "addcolorstop");
    run_op(i, "setpattern");
    i.push(int_token(i, 30));
    i.push(int_token(i, 20));
    i.push(int_token(i, 20));
    run_op(i, "circle");
    run_op(i, "fill");

    // Final round-trip: writepng must succeed.
    std::string path = "/tmp/sli3-test-pattern.png";
    std::remove(path.c_str());
    i.push(i.new_token<sli3::stringtype, std::string>(path));
    run_op(i, "currentpage");
    run_op(i, "writepng");
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    CHECK(f.good());
    CHECK(f.tellg() > 0);
    f.close();
    std::remove(path.c_str());

    i.OStack().clear();
    run_op(i, "currentpage");
    run_op(i, "closepage");
}

// listfonts must return a non-empty array of string-type entries.
void test_listfonts(SLIInterpreter& i)
{
    run_op(i, "listfonts");
    CHECK(i.load() == 1);
    CHECK(i.top().is_of_type(sli3::arraytype));
    TokenArray const* a = i.top().data_.array_val;
    // FontConfig on any reasonable system reports at least a few
    // families. We don't pin a specific count, just non-emptiness
    // and "the entries are strings."
    CHECK(a->size() > 0);
    CHECK(a->get(0).is_of_type(sli3::stringtype));
    i.pop(1);
}

// writepng on a PDF surface must raise UnsupportedSurfaceError.
void test_writepng_rejects_pdf(SLIInterpreter& i)
{
    std::string path = "/tmp/sli3-test-reject.pdf";
    std::remove(path.c_str());
    i.push(i.new_token<sli3::stringtype, std::string>(path));
    i.push(int_token(i, 40)); i.push(int_token(i, 40));
    run_op(i, "newpdf");
    CHECK(i.load() == 1);

    // (path.png) currentpage writepng -> should raise
    Dictionary& ed = i.error_dict();
    ed.insert(Name("newerror"), i.new_token<sli3::booltype, bool>(false));
    i.push(i.new_token<sli3::stringtype, std::string>(std::string("/tmp/sli3-should-not-exist.png")));
    run_op(i, "currentpage");
    run_op(i, "writepng");
    Token ne;
    CHECK(ed.lookup(Name("newerror"), ne));
    CHECK(ne.data_.bool_val == true);
    ed.insert(Name("newerror"), i.new_token<sli3::booltype, bool>(false));
    // writepng pre-raises before popping, so its operands plus the pdf
    // gc are still on the stack. Drain.
    i.OStack().clear();
    i.EStack().clear();

    // Clean up the pdf context (re-read it from globaldict).
    run_op(i, "currentpage");
    run_op(i, "closepage");
    std::remove(path.c_str());
}

}  // namespace

int main()
{
    // SDL's dummy video driver lets newpage open a window object
    // without a real display -- works in CI / headless environments
    // and lets the operator path execute end-to-end.
    setenv("SDL_VIDEODRIVER", "dummy", 1);

    SLIInterpreter i;
    i.startup();  // load sli-init.sli so globaldict / errordict / etc. exist

    test_type_registered(i);
    GraphicsContext* g = test_newpage(i);
    test_draw_ops(i, g);
    test_currentpage(i, g);
    test_closepage_invalidates(i);
    test_serialize_round_trip(i);

    // Extension-pass coverage.
    test_offscreen_curve_clip_png(i);
    test_arct(i);
    test_text_findfont_setfont(i);
    test_pdf_backend(i);
    test_writepng_rejects_pdf(i);
    test_ctm(i);
    test_state_queries(i);
    test_pagesize(i);
    test_textextents(i);
    test_loadpng_drawimage(i);
    test_color_dict(i);
    test_clippath(i);
    test_flattenpath_and_pathforall(i);
    test_transparency(i);
    test_patterns(i);
    test_listfonts(i);

    std::cout << "test_graphics_module: OK\n";
    return 0;
}
