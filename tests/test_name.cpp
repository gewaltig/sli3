// Name interning + bounds-check tests.
//
// Stage 2 scope:
//   - Default Name == handle 0 (the seeded "0" entry).
//   - Same string -> same handle (interning).
//   - Name(LONG_MAX) is currently UB (bounds-unchecked); Stage 2.10
//     either makes the long-arg ctor private or adds a range check.

#include "sli_name.h"

#include <climits>
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

void test_default_name()
{
    Name n;
    CHECK(n.toIndex() == 0);
    CHECK(n.toString() == "0");   // seeded singleton
}

void test_intern_string()
{
    Name a("foo");
    Name b("foo");
    Name c("bar");

    CHECK(a == b);
    CHECK(!(a == c));
    CHECK(a.toIndex() == b.toIndex());
    CHECK(a.toIndex() != c.toIndex());

    Name d(std::string("foo"));
    CHECK(d == a);
}

void test_intern_persists()
{
    auto idx = Name("zzz_unique").toIndex();
    Name n("zzz_unique");
    CHECK(n.toIndex() == idx);
    CHECK(n.toString() == "zzz_unique");
}

// Stage 2.10: Name(long h) bounds-checks at construction.
// Slice 11 (2026-05-12): the check is gated on !NDEBUG to save
// ~10 % on B10 matmul (handleTableInstance_().size() resolution
// on every dispatcher name lookup). In Debug builds the throw
// still fires; in Release builds it doesn't. Test gated to match.
void test_name_long_out_of_range()
{
#ifndef NDEBUG
    bool threw = false;
    try { (void) Name(LONG_MAX); }
    catch (std::out_of_range const&) { threw = true; }
    CHECK(threw);

    threw = false;
    try { (void) Name(-1L); }
    catch (std::out_of_range const&) { threw = true; }
    CHECK(threw);
#endif

    // Valid handles still work in both Debug and Release.
    Name n("test_name_known");
    Name from_handle(static_cast<long>(n.toIndex()));
    CHECK(from_handle == n);
}

}  // namespace

int main()
{
    test_default_name();
    test_intern_string();
    test_intern_persists();
    test_name_long_out_of_range();

    std::cout << "test_name: ok\n";
    return 0;
}
