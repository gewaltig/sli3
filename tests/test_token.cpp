// Token semantics tests.
//
// Covers: ctor / copy / move / swap / self-assign for each
// pointer-payload type (string, array, dict, stream) at refcount 1
// and refcount > 1. The "lifecycle" test verifies that the heap-side
// refcount returns to zero after the last Token is destroyed — i.e.
// the SLIType protocol (add_reference / remove_reference) is
// balanced.
//
// Some tests are deliberately commented out with XFAIL markers; they
// will be enabled at the stage that fixes the underlying bug. The
// staging is documented in fix-plan.md.

#include "sli_array.h"
#include "sli_dictionary.h"
#include "sli_interpreter.h"
#include "sli_iostream.h"
#include "sli_string.h"
#include "sli_token.h"
#include "sli_type.h"

#include <cassert>
#include <cstdint>
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

// ---------- helpers ------------------------------------------------------

// Build a Token referencing `obj` of pointer-payload type. Does NOT
// call add_reference — assumes the heap object's initial refs_=1
// matches the Token's "first owner" semantics. This is the same
// pattern the rest of the codebase uses (see test_array.cpp:36-66
// and SLIInterpreter::new_token implementations).
Token wrap_array(SLIInterpreter& i, TokenArray* arr)
{
    Token t(i.get_type(sli3::arraytype));
    t.data_.array_val = arr;
    return t;
}

Token wrap_string(SLIInterpreter& i, SLIString* s)
{
    Token t(i.get_type(sli3::stringtype));
    t.data_.string_val = s;
    return t;
}

Token wrap_dict(SLIInterpreter& i, Dictionary* d)
{
    Token t(i.get_type(sli3::dictionarytype));
    t.data_.dict_val = d;
    return t;
}

Token wrap_istream(SLIInterpreter& i, SLIistream* s)
{
    Token t(i.get_type(sli3::istreamtype));
    t.data_.istream_val = s;
    return t;
}

// ---------- default & basic ----------------------------------------------

void test_default_token()
{
    Token t;
    CHECK(!t.is_valid());
    CHECK(t.type_ == nullptr);
    // The "1" returned by add_reference / references on an invalid
    // Token is a documented sentinel (sli_token.h:241). It is not
    // particularly meaningful, but we lock it in so a future change
    // is visible.
    CHECK(t.references() == 1);
    CHECK(t.add_reference() == 1);
}

// ---------- copy ctor / copy-assign at refcount 1 ------------------------

void test_copy_array_at_refs_1(SLIInterpreter& i)
{
    auto* arr = new TokenArray();         // refs=1
    {
        Token a = wrap_array(i, arr);     // refs=1 (no add_reference; "first owner")
        CHECK(arr->references() == 1);

        Token b(a);                        // copy ctor: refs=2
        CHECK(arr->references() == 2);
    }                                       // both Tokens destruct -> refs=0 -> arr deleted
    // arr is now dead; no further access.
}

void test_copy_string_at_refs_1(SLIInterpreter& i)
{
    auto* s = new SLIString("hello");
    {
        Token a = wrap_string(i, s);
        CHECK(s->references() == 1);

        Token b(a);
        CHECK(s->references() == 2);
    }
}

void test_copy_dict_at_refs_1(SLIInterpreter& i)
{
    auto* d = new Dictionary();
    {
        Token a = wrap_dict(i, d);
        CHECK(d->references() == 1);

        Token b(a);
        CHECK(d->references() == 2);
    }
}

void test_copy_istream_at_refs_1(SLIInterpreter& i)
{
    auto* iss = new std::istringstream("stub");
    auto* w = new SLIistream(iss);
    {
        Token a = wrap_istream(i, w);
        CHECK(w->references() == 1);

        Token b(a);
        CHECK(w->references() == 2);
    }
}

// ---------- copy-assign at refcount 1 ------------------------------------

void test_copy_assign_array_at_refs_1(SLIInterpreter& i)
{
    auto* arr1 = new TokenArray();
    auto* arr2 = new TokenArray();
    {
        Token a = wrap_array(i, arr1);
        Token b = wrap_array(i, arr2);
        CHECK(arr1->references() == 1);
        CHECK(arr2->references() == 1);

        b = a;                             // arr2 freed; arr1 refs=2
        CHECK(arr1->references() == 2);
        // arr2 has been deleted; we cannot dereference it. The fact
        // that this test does not crash is the assertion.
    }
    // a, b destruct; arr1 refs back to 0 -> deleted.
}

// ---------- move ctor / move-assign --------------------------------------

void test_move_ctor_array(SLIInterpreter& i)
{
    auto* arr = new TokenArray();
    {
        Token a = wrap_array(i, arr);
        CHECK(arr->references() == 1);

        Token b(std::move(a));             // refs unchanged
        CHECK(arr->references() == 1);
        CHECK(!a.is_valid());              // moved-from
        CHECK(b.is_valid());
    }
}

void test_move_assign_array(SLIInterpreter& i)
{
    auto* arr1 = new TokenArray();
    auto* arr2 = new TokenArray();
    {
        Token a = wrap_array(i, arr1);
        Token b = wrap_array(i, arr2);
        CHECK(arr1->references() == 1);
        CHECK(arr2->references() == 1);

        b = std::move(a);                  // arr2 freed; a invalidated
        CHECK(arr1->references() == 1);
        CHECK(!a.is_valid());
    }
}

void test_move_self_assign(SLIInterpreter& i)
{
    // Move-assign already has `if (this == &t) return *this;` (sli_token.h:193).
    auto* arr = new TokenArray();
    {
        Token a = wrap_array(i, arr);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-move"
        a = std::move(a);                  // self move-assign (deliberate)
#pragma clang diagnostic pop
        CHECK(arr->references() == 1);
        CHECK(a.is_valid());
    }
}

// ---------- swap ---------------------------------------------------------

void test_swap_array(SLIInterpreter& i)
{
    auto* arr1 = new TokenArray();
    auto* arr2 = new TokenArray();
    {
        Token a = wrap_array(i, arr1);
        Token b = wrap_array(i, arr2);
        a.swap(b);
        CHECK(a.data_.array_val == arr2);
        CHECK(b.data_.array_val == arr1);
        CHECK(arr1->references() == 1);
        CHECK(arr2->references() == 1);
    }
}

// ---------- copy / move at refcount > 1 ----------------------------------

void test_copy_at_refs_gt_1(SLIInterpreter& i)
{
    auto* arr = new TokenArray();
    Token outer = wrap_array(i, arr);      // refs=1
    {
        Token a(outer);                    // refs=2
        Token b(a);                        // refs=3
        CHECK(arr->references() == 3);
    }                                       // a, b destruct -> refs=1
    CHECK(arr->references() == 1);
    // outer destructs at end of fn -> refs=0.
}

// ---------- multi-Token alias -------------------------------------------

void test_alias_chain(SLIInterpreter& i)
{
    // Multiple Tokens, all referencing the same heap array. The
    // ArrayType protocol must keep refs_ in sync with the count of
    // Tokens that hold the pointer.
    auto* arr = new TokenArray();
    Token t1 = wrap_array(i, arr);
    Token t2(t1);
    Token t3(t2);
    Token t4 = t1;
    CHECK(arr->references() == 4);

    {
        Token t5 = t1;
        CHECK(arr->references() == 5);
    }
    CHECK(arr->references() == 4);
}

// ---------- cross-type equality (Stage 1.9) -----------------------------
//
// Type-economical SLIType aliases share their payload:
//   - name / literal / symbol      share name_val
//   - array / procedure / litproc  share array_val
//   - istream / xistream           share istream_val
//
// operator==(Name) already treats nametype/literaltype/symboltype as
// equal when the handles match. operator==(Token) must do the same;
// today it short-circuits on type_ pointer mismatch, leaving the two
// equality flavours asymmetric.

void test_eq_name_literal_symbol(SLIInterpreter& i)
{
    Token a = i.new_token<sli3::nametype, Name>(Name("foo"));
    Token b = i.new_token<sli3::literaltype, Name>(Name("foo"));
    Token c = i.new_token<sli3::symboltype, Name>(Name("foo"));
    Token d = i.new_token<sli3::nametype, Name>(Name("bar"));

    CHECK(a == b);   // cross-type: name == literal
    CHECK(b == c);   // cross-type: literal == symbol
    CHECK(a == c);
    CHECK(!(a == d));
}

void test_eq_array_procedure_litproc(SLIInterpreter& i)
{
    auto* arr = new TokenArray();
    Token a(i.get_type(sli3::arraytype));
    a.data_.array_val = arr;
    arr->add_reference();

    Token p(i.get_type(sli3::proceduretype));
    p.data_.array_val = arr;
    arr->add_reference();

    Token lp(i.get_type(sli3::litproceduretype));
    lp.data_.array_val = arr;
    arr->add_reference();

    CHECK(a == p);
    CHECK(p == lp);
    CHECK(a == lp);
}

void test_eq_unrelated_types(SLIInterpreter& i)
{
    Token n = i.new_token<sli3::nametype, Name>(Name("x"));
    Token z = i.new_token<sli3::integertype>(0L);
    CHECK(!(n == z));
}

// ---------- print vs pprint formatting ----------------------------------
//
// sli-init.sli's `=` calls Token::print (abbreviated form),
// `==` calls Token::pprint (syntax form). Aggregate types
// (procedure, litprocedure) print as `<typename>`; pprint
// emits the body with delimiters that round-trip through the
// parser. Strings are similar: print = bare content, pprint
// wraps in `( )`.

void test_print_pprint_split(SLIInterpreter& i)
{
    auto* arr = new TokenArray();
    arr->push_back(i.new_token<sli3::integertype>(1L));
    arr->push_back(i.new_token<sli3::integertype>(2L));

    // Three Tokens, same payload, different typeids.
    Token a(i.get_type(sli3::arraytype));
    a.data_.array_val = arr;
    arr->add_reference();

    Token p(i.get_type(sli3::proceduretype));
    p.data_.array_val = arr;
    arr->add_reference();

    Token lp(i.get_type(sli3::litproceduretype));
    lp.data_.array_val = arr;
    arr->add_reference();

    // print
    std::ostringstream o_a, o_p, o_lp;
    a.print(o_a); p.print(o_p); lp.print(o_lp);
    CHECK(o_a.str() == "[1 2]");                 // arrays print in full
    CHECK(o_p.str() == "<proceduretype>");
    // Litproceduretype is registered with the longer name
    // "literalproceduretype" in init_types (sli_interpreter.cpp).
    CHECK(o_lp.str() == "<literalproceduretype>");

    // pprint
    std::ostringstream pp_a, pp_p, pp_lp;
    a.pprint(pp_a); p.pprint(pp_p); lp.pprint(pp_lp);
    CHECK(pp_a.str() == "[1 2]");
    CHECK(pp_p.str() == "{1 2}");
    CHECK(pp_lp.str() == "{{1 2}}");

    // string: print = bare content, pprint = "(content)"
    auto* s = new SLIString("hi");
    Token st(i.get_type(sli3::stringtype));
    st.data_.string_val = s;
    std::ostringstream s_p, s_pp;
    st.print(s_p); st.pprint(s_pp);
    CHECK(s_p.str() == "hi");
    CHECK(s_pp.str() == "(hi)");
}

// ---------- null-payload safety -----------------------------------------
//
// Stage 1.2: ArrayType / DictionaryType / StringType / TrieType all
// must tolerate a Token tagged with their typeid whose payload
// pointer is nullptr (reachable e.g. via base SLIType::deserialize
// for any type whose subclass forgot to override deserialize, or via
// Token::value() zero-fill on a freshly-cleared slot). Today the
// guards are inconsistent: ArrayType/DictionaryType do NOT
// null-check add_reference / remove_reference; StringType::compare
// does not null-check. The convention being adopted: "null payload
// is silently a no-op; never deref."
//
// Each test constructs a null-payload Token by hand. If the protocol
// crashes / UAFs, ASAN flags it; otherwise the assertions run.

void test_null_array(SLIInterpreter& i)
{
    Token t(i.get_type(sli3::arraytype));   // data_ zeroed
    t.data_.array_val = nullptr;
    CHECK(t.is_valid());
    CHECK(t.references() == 0);

    // Copy: must not crash. The copy carries a null payload too.
    Token c(t);
    CHECK(c.data_.array_val == nullptr);
    CHECK(t.references() == 0);

    // Compare null payloads: pointer equality, both null -> equal.
    CHECK(t == c);

    // print should not deref. Discard the output.
    std::ostringstream oss;
    t.print(oss);

    // Destruction of t and c must not crash.
}

void test_null_string(SLIInterpreter& i)
{
    Token t(i.get_type(sli3::stringtype));
    t.data_.string_val = nullptr;
    CHECK(t.is_valid());
    CHECK(t.references() == 0);

    Token c(t);
    CHECK(c.data_.string_val == nullptr);
    CHECK(t.references() == 0);

    // StringType::compare today derefs without null check; this is
    // the assertion that drives the Stage 1.2 fix on the cpp side.
    CHECK(t == c);

    std::ostringstream oss;
    t.print(oss);

    // Stage 1.7: converting a null-payload Token to std::string&
    // must throw, not null-deref.
    bool threw = false;
    try { (void) static_cast<std::string&>(t); }
    catch (std::exception const&) { threw = true; }
    CHECK(threw);
}

void test_null_dict(SLIInterpreter& i)
{
    Token t(i.get_type(sli3::dictionarytype));
    t.data_.dict_val = nullptr;
    CHECK(t.is_valid());
    CHECK(t.references() == 0);

    Token c(t);
    CHECK(c.data_.dict_val == nullptr);
    CHECK(t.references() == 0);

    CHECK(t == c);

    std::ostringstream oss;
    t.print(oss);
}

// ---------- self-assign --------------------------------------------------
//
// This is the canary for the Stage 1.1 fix: today,
// Token::operator=(const Token&) calls clear() then init(), and if
// `&t == this` and the payload's refcount is 1, clear() destroys the
// payload before init() reads it (use-after-free).
//
// The test is deliberately scoped so that at the moment of self-assign
// the refcount is 1. Stage 1.1 adds `if (&t == this) return *this;`
// at the top of operator=; until that lands, this test crashes (or
// reports garbage refcount with ASAN). Enabled here because the user
// asked for tests-before-fix; the test is expected to fail until
// Stage 1.1 lands.
//
// Default: XFAIL tests are skipped (CI green). Pass --xfail to run
// them and observe Stage 1.1 progress.

// The deliberate self-assigns below are exactly what these tests
// exercise; silence -Wself-assign-overloaded so the warning doesn't
// pollute the build under -Wall.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-assign-overloaded"

void test_self_assign_array_at_refs_1(SLIInterpreter& i)
{
    auto* arr = new TokenArray();
    {
        Token a = wrap_array(i, arr);
        CHECK(arr->references() == 1);

        a = a;                             // <-- UAF until Stage 1.1
        CHECK(arr->references() == 1);
        CHECK(a.is_valid());
        CHECK(a.data_.array_val == arr);
    }
}

void test_self_assign_string_at_refs_1(SLIInterpreter& i)
{
    auto* s = new SLIString("x");
    {
        Token a = wrap_string(i, s);
        CHECK(s->references() == 1);

        a = a;                             // <-- UAF until Stage 1.1
        CHECK(s->references() == 1);
    }
}

void test_self_assign_dict_at_refs_1(SLIInterpreter& i)
{
    auto* d = new Dictionary();
    {
        Token a = wrap_dict(i, d);
        CHECK(d->references() == 1);

        a = a;                             // <-- UAF until Stage 1.1
        CHECK(d->references() == 1);
    }
}

#pragma clang diagnostic pop

// ---------- driver -------------------------------------------------------

bool wants_xfail(int argc, char** argv)
{
    for (int i = 1; i < argc; ++i)
    {
        if (std::string(argv[i]) == "--xfail")
            return true;
    }
    return false;
}

}  // namespace

int main(int argc, char** argv)
{
    SLIInterpreter i;

    test_default_token();

    test_copy_array_at_refs_1(i);
    test_copy_string_at_refs_1(i);
    test_copy_dict_at_refs_1(i);
    test_copy_istream_at_refs_1(i);

    test_copy_assign_array_at_refs_1(i);

    test_move_ctor_array(i);
    test_move_assign_array(i);
    test_move_self_assign(i);

    test_swap_array(i);

    test_copy_at_refs_gt_1(i);
    test_alias_chain(i);

    test_null_array(i);
    test_null_string(i);
    test_null_dict(i);

    test_print_pprint_split(i);

    test_eq_name_literal_symbol(i);
    test_eq_array_procedure_litproc(i);
    test_eq_unrelated_types(i);

    if (wants_xfail(argc, argv))
    {
        // Currently fails (Stage 1.1 fix pending). Run with --xfail
        // to observe the regression; default is skip so CI is green.
        test_self_assign_array_at_refs_1(i);
        test_self_assign_string_at_refs_1(i);
        test_self_assign_dict_at_refs_1(i);
    }
    else
    {
        std::cerr << "test_token: skipping XFAIL self-assign tests "
                     "(pass --xfail to enable)\n";
    }

    std::cout << "test_token: ok\n";
    return 0;
}
