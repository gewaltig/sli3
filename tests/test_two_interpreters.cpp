// Two-interpreter lifetime test.
//
// Several operator execute() bodies cache the SLIType* of their
// result type as a function-local `static`, e.g.
//
//     static SLIType *bool_tid = i->get_type(sli3::booltype);
//
// This binds the cached pointer to the FIRST SLIInterpreter that
// ever called the op. Constructing a second interpreter (even
// sequentially, after the first has been destroyed) and invoking the
// same op reads through a dangling pointer into the freed
// interpreter's type table.
//
// Stage 7 of fix-plan.md retires those caches. Until then, the
// xfail-marked subtest below is expected to crash. Run with
// `--no-xfail` (default) to keep CI green; run with `--xfail` to
// trigger the canary explicitly.

#include "sli_interpreter.h"
#include "sli_iostream.h"
#include "sli_token.h"
#include "sli_tokenstack.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <memory>
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

void prime_eval(SLIInterpreter& i, std::string const& src)
{
    auto* iss = new std::istringstream(src);
    auto* wrap = new SLIistream(iss);

    i.OStack().clear();
    i.EStack().clear();
    i.set_call_depth(0);

    i.EStack().push(i.new_token<sli3::quittype>());
    Token x(i.get_type(sli3::xistreamtype));
    x.data_.istream_val = wrap;
    i.EStack().push(x);
    i.EStack().push(i.new_token<sli3::nametype, Name>(i.iparse_name));
}

// ---------- baseline: literals only ------------------------------------
//
// No operator dispatch — exercises only the parser + dispatcher's
// literal-push paths. Should work today regardless of static caches.

void test_baseline_two_interpreters()
{
    {
        SLIInterpreter a;
        prime_eval(a, "1 2 3");
        a.execute_dispatch_(0);
        CHECK(a.OStack().load() == 3);
    }
    // a destroyed.
    {
        SLIInterpreter b;
        prime_eval(b, "4 5 6");
        b.execute_dispatch_(0);
        CHECK(b.OStack().load() == 3);
        CHECK(b.OStack().pick(0).data_.long_val == 6);
    }
}

// ---------- XFAIL canary: op with static SLIType* cache -----------------
//
// `leq_ii` (sli_math.cpp:841) caches `static SLIType *bool_tid =
// i->get_type(...)`. The first interpreter to invoke it pins
// bool_tid; the second sees a dangling pointer.

void test_xfail_static_cache_after_destroy()
{
    {
        SLIInterpreter a;
        prime_eval(a, "1 2 leq_ii");
        a.execute_dispatch_(0);
        CHECK(a.OStack().load() == 1);
        CHECK(a.OStack().pick(0).is_of_type(sli3::booltype));
    }
    // Interpreter A destroyed; bool_tid in leq_iifunction now dangles.
    {
        SLIInterpreter b;
        prime_eval(b, "1 2 leq_ii");
        b.execute_dispatch_(0);                  // <-- expected to crash
        CHECK(b.OStack().load() == 1);
        CHECK(b.OStack().pick(0).is_of_type(sli3::booltype));
    }
}

}  // namespace

int main(int argc, char** argv)
{
    bool run_xfail = false;
    for (int k = 1; k < argc; ++k)
    {
        if (std::string(argv[k]) == "--xfail")
            run_xfail = true;
    }

    test_baseline_two_interpreters();
    test_xfail_static_cache_after_destroy();
    (void) run_xfail;

    std::cout << "test_two_interpreters: ok\n";
    return 0;
}
