// Stage 5: math operator UB / edge case fixes.
//
// 5.1 add_ii / sub_ii / mul_ii / div_ii / mod / abs_i / neg_i
//     raise RangeCheck on signed-int overflow instead of UB.
// 5.2 modf_d captures op1 by value before push (stack reallocation
//     would leave a reference dangling).
// 5.3 round_d uses std::round (half-away-from-zero), not floor(x+0.5).
// 5.4 UnitStep_d / UnitStep_i return correct values for negative inputs.
//
// Drives the typed leaf operators directly from the dispatcher.

#include "sli_dictionary.h"
#include "sli_exceptions.h"
#include "sli_interpreter.h"
#include "sli_iostream.h"
#include "sli_name.h"
#include "sli_token.h"
#include "sli_tokenstack.h"

#include <climits>
#include <cmath>
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

    Token x(i.get_type(sli3::xistreamtype));
    x.data_.istream_val = wrap;
    i.EStack().push(x);
    i.EStack().push(i.new_token<sli3::nametype, Name>(i.iparse_name));
}

std::string get_errorname(SLIInterpreter& i)
{
    if (!i.error_dict().known(Name("errorname"))) return "";
    Token const& t = i.error_dict().lookup(Name("errorname"));
    if (t.is_of_type(sli3::literaltype) ||
        t.is_of_type(sli3::nametype) ||
        t.is_of_type(sli3::symboltype))
        return Name(t.data_.name_val).toString();
    return "<not-name>";
}

void expect_error(SLIInterpreter& i, char const* src, char const* expected)
{
    prime_eval(i, src);
    try { i.execute_dispatch_(0); } catch (...) { /* swallowed */ }
    auto got = get_errorname(i);
    if (got != expected)
    {
        std::cerr << "FAIL `" << src << "` expected /errorname="
                  << expected << " got " << got << "\n";
        std::exit(1);
    }
}

void expect_int(SLIInterpreter& i, char const* src, long expected)
{
    prime_eval(i, src);
    i.execute_dispatch_(0);
    if (i.OStack().load() < 1)
    {
        std::cerr << "FAIL `" << src << "` ostack empty\n";
        std::exit(1);
    }
    Token const& t = i.OStack().pick(0);
    if (!t.is_of_type(sli3::integertype))
    {
        std::cerr << "FAIL `" << src << "` ostack[0] not int (typeid="
                  << t.tag() << ", load="
                  << i.OStack().load() << ")\n";
        std::exit(1);
    }
    if (t.data_.long_val != expected)
    {
        std::cerr << "FAIL `" << src << "` got " << t.data_.long_val
                  << ", expected " << expected << "\n";
        std::exit(1);
    }
}

void expect_double(SLIInterpreter& i, char const* src, double expected)
{
    prime_eval(i, src);
    i.execute_dispatch_(0);
    CHECK(i.OStack().load() >= 1);
    Token const& t = i.OStack().pick(0);
    CHECK(t.is_of_type(sli3::doubletype));
    double got = t.data_.double_val;
    if (std::fabs(got - expected) > 1e-9)
    {
        std::cerr << "FAIL `" << src << "` got " << got
                  << ", expected " << expected << "\n";
        std::exit(1);
    }
}

// ---------- Stage 5.1 -- integer overflow / div-by-zero ----------------

void test_add_overflow(SLIInterpreter& i)
{
    expect_int(i, "1 1 add_ii", 2);
    // LONG_MAX + 1 overflows.
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%ld 1 add_ii", LONG_MAX);
    expect_error(i, buf, "RangeCheck");
}

void test_sub_overflow(SLIInterpreter& i)
{
    expect_int(i, "5 3 sub_ii", 2);
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%ld 1 sub_ii", LONG_MIN);
    expect_error(i, buf, "RangeCheck");
}

void test_mul_overflow(SLIInterpreter& i)
{
    expect_int(i, "6 7 mul_ii", 42);
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%ld 2 mul_ii", LONG_MAX);
    expect_error(i, buf, "RangeCheck");
}

void test_div_min_neg1(SLIInterpreter& i)
{
    expect_int(i, "10 3 div_ii", 3);
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%ld -1 div_ii", LONG_MIN);
    expect_error(i, buf, "RangeCheck");
    // Division by zero still raises DivisionByZero (not RangeCheck).
    expect_error(i, "1 0 div_ii", "DivisionByZero");
}

void test_mod_min_neg1(SLIInterpreter& i)
{
    expect_int(i, "10 3 mod", 1);
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%ld -1 mod", LONG_MIN);
    expect_error(i, buf, "RangeCheck");
}

void test_abs_min(SLIInterpreter& i)
{
    expect_int(i, "-5 abs_i", 5);
    expect_int(i, "5 abs_i", 5);
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%ld abs_i", LONG_MIN);
    expect_error(i, buf, "RangeCheck");
}

void test_neg_min(SLIInterpreter& i)
{
    expect_int(i, "5 neg_i", -5);
    expect_int(i, "-5 neg_i", 5);
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%ld neg_i", LONG_MIN);
    expect_error(i, buf, "RangeCheck");
}

// ---------- Stage 5.3 -- Round_d ---------------------------------------

void test_round(SLIInterpreter& i)
{
    expect_double(i, "0.5 round_d", 1.0);
    expect_double(i, "-0.5 round_d", -1.0);   // half-away-from-zero
    expect_double(i, "2.5 round_d", 3.0);
    expect_double(i, "-2.5 round_d", -3.0);
    expect_double(i, "1.4 round_d", 1.0);
    expect_double(i, "-1.4 round_d", -1.0);
}

// ---------- Stage 5.4 -- UnitStep --------------------------------------

void test_unitstep_d(SLIInterpreter& i)
{
    expect_double(i, "1.5 UnitStep_d", 1.0);
    expect_double(i, "0.0 UnitStep_d", 1.0);
    expect_double(i, "-1.5 UnitStep_d", 0.0);   // <- previously left as-is
}

void test_unitstep_i(SLIInterpreter& i)
{
    expect_int(i, "5 UnitStep_i", 1);
    expect_int(i, "0 UnitStep_i", 1);
    expect_int(i, "-3 UnitStep_i", 0);
}

}  // namespace

int main()
{
    // Each test scopes its own SLIInterpreter so post-error state
    // (the /cmd literal raiseerror leaves on the ostack, the
    // half-rolled stop unwind, etc.) doesn't bleed across snippet
    // groups.
    { SLIInterpreter i; test_add_overflow(i); }
    { SLIInterpreter i; test_sub_overflow(i); }
    { SLIInterpreter i; test_mul_overflow(i); }
    { SLIInterpreter i; test_div_min_neg1(i); }
    { SLIInterpreter i; test_mod_min_neg1(i); }
    { SLIInterpreter i; test_abs_min(i); }
    { SLIInterpreter i; test_neg_min(i); }

    { SLIInterpreter i; test_round(i); }
    { SLIInterpreter i; test_unitstep_d(i); }
    { SLIInterpreter i; test_unitstep_i(i); }

    std::cout << "test_math_overflow: ok\n";
    return 0;
}
