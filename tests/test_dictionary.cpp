// Dictionary invariants.
//
// Stage 2 scope:
//   2.7: info() builds a sorted vector then iterates the unsorted
//        map -- output today is unsorted. Test captures stream and
//        asserts ordering.
//   2.8: add_dict / remove_dict are declared in the header but the
//        bodies are commented out. Either implement or remove the
//        declarations; this test confirms the resolution stays
//        compile-time consistent.
//   2.9: Dictionary::lookup(Name, Token&) sets the access flag on
//        the stored DictToken before copying out (otherwise the
//        bool-overload silently defeats access tracking).

#include "sli_dictionary.h"
#include "sli_interpreter.h"
#include "sli_name.h"
#include "sli_token.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

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

// Stage 2.7: info() output is sorted by key (case-insensitive
// lexicographic, per DictItemLexicalOrder). Today the sorted vector
// is built and ignored; the loop iterates the std::map directly,
// which yields keys in handle-id order (interning order).
void test_info_sorted(SLIInterpreter& interp)
{
    Dictionary d;
    // Insert in non-alphabetic order. Names are interned in some
    // earlier order; map ordering is by Name handle index, not by
    // string. After Stage 2.7, info() should print alphabetically.
    d.insert(Name("zeta"),  interp.new_token<sli3::integertype>(1L));
    d.insert(Name("alpha"), interp.new_token<sli3::integertype>(2L));
    d.insert(Name("delta"), interp.new_token<sli3::integertype>(3L));
    d.insert(Name("Beta"),  interp.new_token<sli3::integertype>(4L));

    std::ostringstream oss;
    d.info(oss);
    std::string out = oss.str();

    auto pos_alpha = out.find("alpha");
    auto pos_beta  = out.find("Beta");
    auto pos_delta = out.find("delta");
    auto pos_zeta  = out.find("zeta");
    CHECK(pos_alpha != std::string::npos);
    CHECK(pos_beta  != std::string::npos);
    CHECK(pos_delta != std::string::npos);
    CHECK(pos_zeta  != std::string::npos);

    // Case-insensitive lexicographic: alpha < Beta < delta < zeta.
    CHECK(pos_alpha < pos_beta);
    CHECK(pos_beta  < pos_delta);
    CHECK(pos_delta < pos_zeta);
}

// Stage 2.9: bool lookup propagates the access flag onto the stored
// DictToken so all_accessed() and clear_access_flags() can track it.
void test_lookup_sets_access_flag(SLIInterpreter& interp)
{
    Dictionary d;
    d.insert(Name("k"), interp.new_token<sli3::integertype>(7L));

    // Reference-returning overload: caller can set the flag directly.
    DictToken& ref = d.lookup(Name("k"));
    ref.set_access_flag();
    CHECK(d.lookup(Name("k")).accessed());

    // Reset: clear flags, then exercise the bool overload.
    d.clear_access_flags();
    CHECK(!d.lookup(Name("k")).accessed());

    Token result;
    CHECK(d.lookup(Name("k"), result));
    // After Stage 2.9: the bool-returning overload also sets the
    // flag on the *stored* DictToken (not just the copied-out result).
    CHECK(d.lookup(Name("k")).accessed());
}

}  // namespace

int main()
{
    SLIInterpreter interp;

    test_info_sorted(interp);
    test_lookup_sets_access_flag(interp);

    std::cout << "test_dictionary: ok\n";
    return 0;
}
