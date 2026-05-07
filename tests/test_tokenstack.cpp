// TokenStack invariants.
//
// Stage 2 scope:
//   - size() vs load() contract (NEST 2.x compat: size() returns
//     storage capacity, load() returns element count).
//   - roll(n, k) when n == 0 must not divide-by-zero.
//   - basic push/pop/swap/pick correctness.
//
// Bounds checks (assert(load()>0)) are debug-only; they fire under
// asan/debug builds. Tests that would trigger UB in release are
// gated behind --xfail.

#include "sli_interpreter.h"
#include "sli_token.h"
#include "sli_tokenstack.h"

#include <cstdlib>
#include <iostream>
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

void test_load_vs_size(SLIInterpreter& i)
{
    TokenStack s(8);             // capacity reservation = 8
    CHECK(s.load() == 0);        // no elements pushed
    // size() returns capacity per NEST 2.x convention; >= 8 because
    // std::vector reserves at least the requested amount.
    CHECK(s.size() >= 8);

    s.push(i.new_token<sli3::integertype>(1L));
    s.push(i.new_token<sli3::integertype>(2L));
    CHECK(s.load() == 2);
    CHECK(s.top().data_.long_val == 2);
    CHECK(s.pick(1).data_.long_val == 1);

    s.pop();
    CHECK(s.load() == 1);
    CHECK(s.top().data_.long_val == 1);

    s.clear();
    CHECK(s.load() == 0);
}

void test_swap(SLIInterpreter& i)
{
    TokenStack s(4);
    s.push(i.new_token<sli3::integertype>(1L));
    s.push(i.new_token<sli3::integertype>(2L));
    s.swap();
    CHECK(s.top().data_.long_val == 1);
    CHECK(s.pick(1).data_.long_val == 2);
}

// Stage 2.2: roll(0, k) historically did `k % 0`, divide-by-zero.
// On most CPUs that traps. After the fix it should be a no-op.
void test_roll_zero(SLIInterpreter& i)
{
    TokenStack s(4);
    // Push some tokens so the stack is non-empty; roll(0, ...) acts
    // on the top 0 elements -- should be a no-op regardless.
    s.push(i.new_token<sli3::integertype>(1L));
    s.push(i.new_token<sli3::integertype>(2L));

    s.roll(0, 0);     // <- divide-by-zero today
    s.roll(0, 5);     // <- divide-by-zero today
    s.roll(0, -3);    // <- divide-by-zero today

    CHECK(s.load() == 2);
    CHECK(s.top().data_.long_val == 2);
}

void test_roll_basic(SLIInterpreter& i)
{
    TokenStack s(8);
    // bottom -> top: 1 2 3 4
    for (long n = 1; n <= 4; ++n) s.push(i.new_token<sli3::integertype>(n));

    // roll(4, 1): rotate top 4 by 1 toward the top.
    // Implementation: rotate(end()-4, end()-(1%4)=end()-1, end()).
    // std::rotate moves the element at the middle to the front of the
    // range, so [1,2,3,4] becomes [4,1,2,3] (the previous top is now
    // at the bottom of the rolled range).
    s.roll(4, 1);
    CHECK(s.pick(0).data_.long_val == 3);
    CHECK(s.pick(1).data_.long_val == 2);
    CHECK(s.pick(2).data_.long_val == 1);
    CHECK(s.pick(3).data_.long_val == 4);
}

}  // namespace

int main()
{
    SLIInterpreter i;

    test_load_vs_size(i);
    test_swap(i);
    test_roll_basic(i);
    test_roll_zero(i);

    std::cout << "test_tokenstack: ok\n";
    return 0;
}
