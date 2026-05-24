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

#include <cstdlib>
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
    src.data_.graphics_val = new GraphicsContext(40, 40, std::string("sli3-test"));

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

    std::cout << "test_graphics_module: OK\n";
    return 0;
}
