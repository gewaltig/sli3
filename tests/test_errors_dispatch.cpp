// Stage-4 dispatch-driven error tests.
//
// Drive the type-check / wiring fixes (4.2 or-binding, 4.3 If
// type-check, 4.4 Ifelse type-check, 4.6 Token_s reorder, ...)
// end-to-end through the dispatcher via eval(). Verifies the
// happy paths AND that errors are raised rather than silently
// taking a wrong branch.

#include "sli_dictionary.h"
#include "sli_exceptions.h"
#include "sli_interpreter.h"
#include "sli_iostream.h"
#include "sli_name.h"
#include "sli_token.h"
#include "sli_tokenstack.h"

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

void prime_eval(SLIInterpreter& i, std::string const& src)
{
    auto* iss = new std::istringstream(src);
    auto* wrap = new SLIistream(iss);

    i.OStack().clear();
    i.EStack().clear();
    i.error_dict().clear();
    i.set_call_depth(0);

    Token x(i.get_type(sli3::xistreamtype));
    x.data_.istream_val = wrap;
    i.EStack().push(x);
    i.EStack().push(i.new_token<sli3::nametype, Name>(i.iparse_name));
}

// Did errordict /errorname end up set to `expected`? Returns the
// resolved string or "" if not set.
std::string get_errorname(SLIInterpreter& i)
{
    if (!i.error_dict().known(Name("errorname"))) return "";
    Token const& t = i.error_dict().lookup(Name("errorname"));
    if (t.is_of_type(sli3::literaltype) ||
        t.is_of_type(sli3::nametype) ||
        t.is_of_type(sli3::symboltype))
    {
        return Name(t.data_.name_val).toString();
    }
    return "<not-name>";
}

// Same shape for /commandname (the operator that triggered the error).
// Used to pin down nested-op attribution per Axis I Slice 2 audit:
// dispatchers (e.g. /get) calling typed leaves (e.g. /get_a) must
// report the user-facing dispatcher name, not the leaf.
std::string get_commandname(SLIInterpreter& i)
{
    if (!i.error_dict().known(Name("commandname"))) return "";
    Token const& t = i.error_dict().lookup(Name("commandname"));
    if (t.is_of_type(sli3::literaltype) ||
        t.is_of_type(sli3::nametype) ||
        t.is_of_type(sli3::symboltype))
    {
        return Name(t.data_.name_val).toString();
    }
    return "<not-name>";
}

void expect_commandname(SLIInterpreter& i, char const* src, char const* expected)
{
    prime_eval(i, src);
    try { i.execute_dispatch_(0); } catch (...) { /* swallowed */ }
    auto got = get_commandname(i);
    if (got != expected)
    {
        std::cerr << "FAIL `" << src << "` expected /commandname=" << expected
                  << " got " << got << "\n";
        std::exit(1);
    }
}

// Run a snippet that is expected to fail with errorname == expected.
// raiseerror's `stop` will unwind the e-stack; that's fine.
void expect_error(SLIInterpreter& i, char const* src, char const* expected)
{
    prime_eval(i, src);
    try { i.execute_dispatch_(0); } catch (...) { /* swallowed */ }
    auto got = get_errorname(i);
    if (got != expected)
    {
        std::cerr << "FAIL `" << src << "` expected /errorname=" << expected
                  << " got " << got << "\n";
        std::exit(1);
    }
}

// Run a snippet expected to succeed and leave a single integer on the ostack.
void expect_int(SLIInterpreter& i, char const* src, long expected)
{
    prime_eval(i, src);
    i.execute_dispatch_(0);
    CHECK(i.OStack().load() == 1);
    CHECK(i.OStack().pick(0).is_of_type(sli3::integertype));
    CHECK(i.OStack().pick(0).data_.long_val == expected);
}

// Run a snippet expected to leave nothing on the ostack.
void expect_empty(SLIInterpreter& i, char const* src)
{
    prime_eval(i, src);
    i.execute_dispatch_(0);
    CHECK(i.OStack().load() == 0);
}

// ---------- IfFunction (Stage 4.3) -------------------------------------

void test_if_truthy(SLIInterpreter& i)
{
    expect_int(i, "true { 42 } if", 42);
}

void test_if_falsy(SLIInterpreter& i)
{
    expect_empty(i, "false { 42 } if");
}

void test_if_non_bool_raises(SLIInterpreter& i)
{
    expect_error(i, "1 { 42 } if", "TypeMismatch");
    expect_error(i, "(foo) { 42 } if", "TypeMismatch");
}

// ---------- IfelseFunction (Stage 4.4) ---------------------------------

void test_ifelse_truthy(SLIInterpreter& i)
{
    expect_int(i, "true { 1 } { 2 } ifelse", 1);
}

void test_ifelse_falsy(SLIInterpreter& i)
{
    expect_int(i, "false { 1 } { 2 } ifelse", 2);
}

void test_ifelse_non_bool_raises(SLIInterpreter& i)
{
    expect_error(i, "1 { 1 } { 2 } ifelse", "TypeMismatch");
}

// ---------- or_bb (Stage 4.2) ------------------------------------------
//
// `or` is now bound to or_bbfunction which calls require_stack_load(2).
// Single-arg invocation must raise StackUnderflow rather than UB-read
// pick(1).

void test_or_arity(SLIInterpreter& i)
{
    expect_error(i, "true or", "StackUnderflow");
}

void test_or_happy_paths(SLIInterpreter& i)
{
    prime_eval(i, "true false or");
    i.execute_dispatch_(0);
    CHECK(i.OStack().load() == 1);
    CHECK(i.OStack().pick(0).is_of_type(sli3::booltype));
    CHECK(i.OStack().pick(0).data_.bool_val == true);
}

// ---------- Axis I Slice 2 — nested-op attribution -------------------
//
// Stage 9 compaction made common ops (`/get`, `/length`, `/forall`,
// `/def`, `/cvi`, ...) C++ dispatchers that synchronously call
// typed leaves (`/get_a`, `/length_s`, `/forall_a`, ...) on the same
// C stack frame. When the typed leaf raises, /commandname must
// name the user-facing dispatcher, not the implementation leaf.
//
// Today this works because get_current_name() reads the e-stack top
// (which holds the dispatcher's function Token, pushed by the main
// dispatcher loop before entering the dispatcher's execute body).
// After Axis I Slice 4 introduces current_op_, the same attribution
// must hold via the new field; the audit at doc/axis1_slice2_audit.md
// concluded that the default behavior (current_op_ unchanged across
// the inner .execute(i) call) gives the right answer at every site.
//
// These tests pin the contract so a regression at Slice 4 surfaces.

void test_attrib_get_range(SLIInterpreter& i)
{
    // [1 2 3] has length 3; index 99 is out of range. /get_a raises
    // RangeCheck. /commandname must be /get (the dispatcher), not
    // /get_a (the typed leaf).
    expect_commandname(i, "[1 2 3] 99 get", "get");
}

void test_attrib_put_range(SLIInterpreter& i)
{
    // Same shape for /put -> /put_a.
    expect_commandname(i, "[1 2 3] 99 42 put", "put");
}

void test_attrib_cvi_string(SLIInterpreter& i)
{
    // Inner-attribution path: /cvi for string input does NOT call
    // cvi_s directly; it baselookups /cvi_s and pushes it onto the
    // e-stack, so the dispatcher itself dispatches /cvi_s and that
    // becomes the current op. /commandname is /cvi_s, not /cvi.
    // Distinct from /get above which is direct-call.
    // See doc/axis1_slice2_audit.md for the two patterns.
    expect_commandname(i, "(notanumber) cvi", "cvi_s");
}

}  // namespace

int main()
{
    SLIInterpreter i;

    test_if_truthy(i);
    test_if_falsy(i);
    test_if_non_bool_raises(i);

    test_ifelse_truthy(i);
    test_ifelse_falsy(i);
    test_ifelse_non_bool_raises(i);

    test_or_arity(i);
    test_or_happy_paths(i);

    test_attrib_get_range(i);
    test_attrib_put_range(i);
    test_attrib_cvi_string(i);

    std::cout << "test_errors_dispatch: ok\n";
    return 0;
}
