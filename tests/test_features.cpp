// Feature-completion regression test.
//
// Locks in representative coverage for the operator groups the
// project considers MUST-WORK at this milestone:
//
//   - stack ops          (pop / dup / exch / roll / count / clear / mark)
//   - dictstack ops      (begin / end / def / lookup / countdictstack /
//                          dictstack / cleardictstack / known / undef)
//   - container ops      (length, get, put for arrays / strings / dicts;
//                          conversions cvi_s / cvd_s / cvs)
//   - file ops           (ofstream / ifstream / getline_is / close /
//                          ostrstream + str)
//   - time ops           (realtime / ptimes / clock / tic / toc / sleep_d)
//
// Drives the binary's full bootstrap (sli-init.sli loaded) so the
// trie wrappers and SLI-defined helpers are in scope.

#include "sli_dictionary.h"
#include "sli_exceptions.h"
#include "sli_interpreter.h"
#include "sli_iostream.h"
#include "sli_name.h"
#include "sli_token.h"
#include "sli_tokenstack.h"

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>

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

// Run a snippet through a freshly-bootstrapped interpreter and
// return its operand stack. Each call pays the sli-init.sli load
// cost (~2-3 dispatches' worth) but keeps tests independent.
struct Result
{
    long ostack_depth;
    bool ok;
    std::string errorname;
};

// (eval_through_startup helper retired in favour of the Harness;
// kept as Result struct in case a future test needs it.)

// Most of the tests only care that "the snippet ran without
// raising and left the right thing on the stack." Driving through
// `execute(string)` is heavy; for fine-grained ostack inspection
// drive through prime_eval + execute_dispatch_ on a lazily
// constructed shared interpreter (sli-init loaded once).
struct Harness
{
    SLIInterpreter i;
    Harness()
    {
        // Force startup so sli-init / typeinit / etc. load.
        i.startup();
    }

    void prime(std::string const& src)
    {
        auto* iss = new std::istringstream(src);
        auto* wrap = new SLIistream(iss);
        i.OStack().clear();
        // Don't clear EStack here -- startup pushed sli-init's
        // wind-down frames that the dispatcher will drain on
        // exit. Just push the new source on top.
        i.error_dict().clear();
        Token x(i.get_type(sli3::xistreamtype));
        x.data_.istream_val = wrap;
        i.EStack().push(x);
        i.EStack().push(i.new_token<sli3::nametype, Name>(i.iparse_name));
    }

    void run() { i.execute_dispatch_(0); }

    std::string errorname()
    {
        if (!i.error_dict().known(Name("errorname"))) return "";
        Token const& t = i.error_dict().lookup(Name("errorname"));
        if (t.is_of_type(sli3::literaltype) ||
            t.is_of_type(sli3::nametype) ||
            t.is_of_type(sli3::symboltype))
            return Name(t.data_.name_val).toString();
        return "<not-name>";
    }
};

// ---------- stack -------------------------------------------------------

void test_stack(Harness& h)
{
    h.prime("1 2 3 pop");
    h.run();
    CHECK(h.i.OStack().load() == 2);
    CHECK(h.i.OStack().pick(0).data_.long_val == 2);

    h.prime("1 2 exch");
    h.run();
    CHECK(h.i.OStack().load() == 2);
    CHECK(h.i.OStack().pick(0).data_.long_val == 1);
    CHECK(h.i.OStack().pick(1).data_.long_val == 2);

    h.prime("1 2 3 3 1 roll");
    h.run();
    // [1 2 3] rolled by 1 -> [3 1 2] (top is 2)
    CHECK(h.i.OStack().load() == 3);
    CHECK(h.i.OStack().pick(0).data_.long_val == 2);
    CHECK(h.i.OStack().pick(2).data_.long_val == 3);

    h.prime("1 2 3 count");
    h.run();
    // count pushes the depth before count itself.
    CHECK(h.i.OStack().load() == 4);
    CHECK(h.i.OStack().pick(0).data_.long_val == 3);
}

// ---------- dictstack ---------------------------------------------------

void test_dictstack(Harness& h)
{
    h.prime("countdictstack");
    h.run();
    long const baseline = h.i.OStack().pick(0).data_.long_val;
    CHECK(baseline >= 2);   // at least systemdict + userdict

    h.prime("3 dict begin countdictstack");
    h.run();
    CHECK(h.i.OStack().pick(0).data_.long_val == baseline + 1);

    h.prime("3 dict begin /k 42 def k");
    h.run();
    CHECK(h.i.OStack().pick(0).data_.long_val == 42);

    h.prime("3 dict begin /k 42 def cleardictstack countdictstack");
    h.run();
    // cleardictstack pops every dict above systemdict + userdict.
    CHECK(h.i.OStack().pick(0).data_.long_val == 2);
}

// ---------- containers --------------------------------------------------

void test_containers(Harness& h)
{
    // arrays
    h.prime("[10 20 30] length");
    h.run();
    CHECK(h.i.OStack().pick(0).data_.long_val == 3);

    h.prime("[10 20 30] 1 get");
    h.run();
    CHECK(h.i.OStack().pick(0).data_.long_val == 20);

    // strings
    h.prime("(hello) length");
    h.run();
    CHECK(h.i.OStack().pick(0).data_.long_val == 5);

    h.prime("(foo) (bar) join_s");
    h.run();
    CHECK(h.i.OStack().pick(0).is_of_type(sli3::stringtype));
    CHECK(h.i.OStack().pick(0).data_.string_val->str() == "foobar");

    // string -> int / double
    h.prime("(42) cvi_s");
    h.run();
    CHECK(h.i.OStack().pick(0).is_of_type(sli3::integertype));
    CHECK(h.i.OStack().pick(0).data_.long_val == 42);

    h.prime("(3.5) cvd_s");
    h.run();
    CHECK(h.i.OStack().pick(0).is_of_type(sli3::doubletype));
    CHECK(h.i.OStack().pick(0).data_.double_val == 3.5);

    // any -> string  (via /cvs in sli-init.sli)
    h.prime("42 cvs");
    h.run();
    CHECK(h.i.OStack().pick(0).is_of_type(sli3::stringtype));
    CHECK(h.i.OStack().pick(0).data_.string_val->str().find("42")
          != std::string::npos);

    // dicts: bind a dict to /d, put k=7, then look it up.
    h.prime("/d 3 dict def d /k 7 put d /k get");
    h.run();
    CHECK(h.i.OStack().pick(0).data_.long_val == 7);
}

// ---------- file ops ----------------------------------------------------

void test_file_ops(Harness& h)
{
    // Write a tmp file via ofstream, read it back via ifstream + getline_is.
    char tmpbuf[256];
    std::snprintf(tmpbuf, sizeof(tmpbuf),
                  "/tmp/sli3-features-%d.txt",
                  static_cast<int>(::getpid()));
    std::string const tmppath = tmpbuf;

    {
        // ofstream leaves [stream, true] on success. Drop the bool,
        // write the line, drop the stream (closes the file).
        std::string const src =
            "(" + tmppath + ") ofstream pop "
            "(hello sli3) <- endl pop";
        h.prime(src);
        h.run();
    }
    {
        // ifstream leaves [istream, true] on success.
        std::string const src =
            "(" + tmppath + ") ifstream pop "
            "getline_is";
        h.prime(src);
        h.run();
        // Stack after: istream string true
        CHECK(h.i.OStack().load() == 3);
        CHECK(h.i.OStack().pick(0).is_of_type(sli3::booltype));
        CHECK(h.i.OStack().pick(0).data_.bool_val == true);
        CHECK(h.i.OStack().pick(1).is_of_type(sli3::stringtype));
        CHECK(h.i.OStack().pick(1).data_.string_val->str() == "hello sli3");
    }
    std::remove(tmppath.c_str());

    // ostrstream + str
    h.prime("ostrstream pop (hello) <- str");
    h.run();
    CHECK(h.i.OStack().load() == 1);
    CHECK(h.i.OStack().pick(0).is_of_type(sli3::stringtype));
    CHECK(h.i.OStack().pick(0).data_.string_val->str() == "hello");
}

// ---------- time --------------------------------------------------------

void test_time(Harness& h)
{
    // realtime returns a positive double.
    h.prime("realtime");
    h.run();
    CHECK(h.i.OStack().load() == 1);
    CHECK(h.i.OStack().pick(0).is_of_type(sli3::doubletype));
    CHECK(h.i.OStack().pick(0).data_.double_val > 0.0);

    // ptimes returns a 5-element double array.
    h.prime("ptimes");
    h.run();
    CHECK(h.i.OStack().load() == 1);
    CHECK(h.i.OStack().pick(0).is_of_type(sli3::arraytype));
    CHECK(h.i.OStack().pick(0).data_.array_val->size() == 5);

    // clock returns the same as realtime.
    h.prime("clock");
    h.run();
    CHECK(h.i.OStack().pick(0).is_of_type(sli3::doubletype));
    CHECK(h.i.OStack().pick(0).data_.double_val > 0.0);

    // tic / toc cycle: toc returns a non-negative double.
    h.prime("tic toc");
    h.run();
    CHECK(h.i.OStack().load() == 1);
    CHECK(h.i.OStack().pick(0).is_of_type(sli3::doubletype));
    CHECK(h.i.OStack().pick(0).data_.double_val >= 0.0);
}

// ---------- = / == output (sli-init.sli docstring contract) ------------
//
// `=`  is `cout exch <- endl ;`  -> Token::print  + newline
// `==` is `cout exch <-- endl ;` -> Token::pprint + newline
// Documented examples from sli-init.sli:
//   12 =          -> 12
//   {1211} =      -> <proceduretype>
//   {1211} ==     -> {1211}
//
// We can't easily redirect cout in the harness, so we exercise
// Token::print / Token::pprint directly on the same shapes.

void test_print_pprint_contract(Harness& h)
{
    SLIInterpreter& i = h.i;

    auto check_print = [](Token const& t, std::string const& want_p,
                          std::string const& want_pp) {
        std::ostringstream p, pp;
        t.print(p);  t.pprint(pp);
        CHECK(p.str()  == want_p);
        CHECK(pp.str() == want_pp);
    };

    check_print(i.new_token<sli3::integertype>(12L),    "12",  "12");
    check_print(i.new_token<sli3::doubletype>(3.5),     "3.5", "3.5");
    check_print(i.new_token<sli3::booltype>(true),      "true","true");

    {
        Token s(i.get_type(sli3::stringtype));
        s.data_.string_val = new SLIString("hello");
        check_print(s, "hello", "(hello)");
    }
    {
        // Build {1211} as a procedure.
        auto* arr = new TokenArray();
        arr->push_back(i.new_token<sli3::integertype>(1211L));
        Token p(i.get_type(sli3::proceduretype));
        p.data_.array_val = arr;
        check_print(p, "<proceduretype>", "{1211}");
    }
    {
        // Nested: [1 (s) {1 2 add}] pprint emits each element via pprint.
        auto* inner_proc = new TokenArray();
        inner_proc->push_back(i.new_token<sli3::integertype>(1L));
        inner_proc->push_back(i.new_token<sli3::integertype>(2L));
        Token pn(i.get_type(sli3::proceduretype));
        pn.data_.array_val = inner_proc;

        auto* outer = new TokenArray();
        outer->push_back(i.new_token<sli3::integertype>(1L));
        Token sn(i.get_type(sli3::stringtype));
        sn.data_.string_val = new SLIString("s");
        outer->push_back(sn);
        outer->push_back(pn);

        Token a(i.get_type(sli3::arraytype));
        a.data_.array_val = outer;
        check_print(a,
                    "[1 s <proceduretype>]",      // print uses element print
                    "[1 (s) {1 2}]");             // pprint recurses
    }
}

}  // namespace

int main()
{
    Harness h;
    test_stack(h);
    test_dictstack(h);
    test_containers(h);
    test_file_ops(h);
    test_time(h);
    test_print_pprint_contract(h);

    // Procedure refcount preservation under heavy dispatch.
    // Mirrors the user-supplied stress test:
    //   tic 100000 {1 1 1000 { 2 add pop} for } repeat toc ==
    // but at smaller magnitude so test_features stays under the 60 s
    // CTest timeout.
    {
        Harness rc;
        rc.prime("/p { 1 1 100 { 2 add pop } for } def");
        rc.run();
        // Capture the bound procedure's refcount before the workload.
        Token const& bound = rc.i.DStack().lookup(Name("p"));
        CHECK(bound.is_of_type(sli3::proceduretype));
        TokenArray* arr = bound.data_.array_val;
        unsigned long const refs_before = arr->references();

        rc.prime("100 { p } repeat");
        rc.run();
        // After the workload, /p must still resolve to the SAME
        // TokenArray (no copy-on-execute) and the refcount must be
        // back to whatever it was before the loop ran.
        Token const& bound_after = rc.i.DStack().lookup(Name("p"));
        CHECK(bound_after.data_.array_val == arr);
        CHECK(arr->references() == refs_before);
    }

    std::cout << "test_features: ok\n";
    return 0;
}
