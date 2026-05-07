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

    std::cout << "test_errors_dispatch: ok\n";
    return 0;
}
