// Regression tests for the new-ABI dispatcher contract (Axis I bundle
// step 4). The bugs covered here were latent during step 3 because
// the post-pop fallback hid them; step 4's pre-pop made them visible.
//
// Coverage class A — operators reached via the iiterate fast path
// (a procedure body containing direct functiontype Tokens). Under the
// step-3 contract this path pushed a "sentinel" Token before
// fn->execute for old-ABI ops; if the op never self-popped, the
// sentinel leaked onto the e-stack and the next op saw the wrong
// stack shape. The pre-step-4 audit caught:
//
//   * `<--` / pwrite_fn — same body as `<-` / write_fn but only
//     `<-` had set_new_abi(). `<--` was old-ABI; its body did not
//     self-pop. Symptom: `42 ==` printed "42" then `endl` raised
//     TypeMismatch on a nulltype sentinel.
//   * arrayload_fn, getmax_fn, getmin_fn — bodies never self-popped
//     and the instances were not new-ABI. Direct .execute() call
//     paths in test_array_module.cpp masked it (they don't go
//     through iiterate). Symptom: a leftover ::nulltype sneaks
//     into the operand stack.
//
// Coverage class B — explicit-pop branches in operators marked
// new-ABI. Under step 3 the body pre-popped its own slot in
// special branches (e.g. "empty input" early return); under step 4
// the dispatcher already popped, so the body's manual pop took out
// the caller's frame. The pre-step-4 audit caught:
//
//   * Sort with empty array (sli_array_module.cpp).
//
// These tests use the test_harness eval() helper, which runs through
// the full dispatcher path. sli-init.sli is NOT loaded — we exercise
// the C++-registered functiontype directly.

#include "test_harness.h"

#include "sli_array.h"

#include <iostream>

namespace
{
using namespace sli_test;

void test_arrayload_via_dispatch(sli3::SLIInterpreter& i)
{
    // [1 2 3] arrayload  ->  1 2 3 3   (elements then count)
    EVAL_CLEAR(i);
    eval(i, "[ 10 20 30 ] arrayload");

    if (i.OStack().load() != 4)
    {
        std::cerr << "FAIL arrayload depth: got " << i.OStack().load()
                  << ", expected 4\n";
        std::exit(1);
    }
    if (i.pick(0).data_.long_val != 3 ||
        i.pick(1).data_.long_val != 30 ||
        i.pick(2).data_.long_val != 20 ||
        i.pick(3).data_.long_val != 10)
    {
        std::cerr << "FAIL arrayload contents:";
        for (size_t k = 0; k < 4; ++k)
        {
            std::cerr << " [" << k << "]=";
            i.pick(k).pprint(std::cerr);
        }
        std::cerr << "\n";
        std::exit(1);
    }
    // Critical: no leftover sentinel on the e-stack. Under the
    // step-3 contract bug, this would be >1 if iiterate fired and
    // arrayload was old-ABI.
    // test_harness eval() leaves a /quit Token on the e-stack (the
    // dispatcher exits on quittype without popping). Anything more
    // is a leak from the operator under test.
    if (i.EStack().load() != 1)
    {
        std::cerr << "FAIL arrayload leaked estack frames: "
                  << i.EStack().load() << "\n";
        std::exit(1);
    }
}

void test_getmax_getmin_via_dispatch(sli3::SLIInterpreter& i)
{
    // [3 1 4 1 5 9 2 6] GetMax -> 9
    EVAL_INT(i, "[ 3 1 4 1 5 9 2 6 ] GetMax", 9);
    EVAL_INT(i, "[ 3 1 4 1 5 9 2 6 ] GetMin", 1);
}

void test_sort_empty_via_dispatch(sli3::SLIInterpreter& i)
{
    // Empty array passes through Sort unchanged. The empty-branch
    // pre-step-4 bug popped a frame it didn't own; symptom is the
    // ostack ends up empty (or the e-stack has a leak).
    EVAL_DEPTH(i, "[ ] Sort", 1);

    if (!i.top().is_of_type(sli3::arraytype))
    {
        std::cerr << "FAIL Sort empty: top not arraytype\n";
        std::exit(1);
    }
    sli3::TokenArray* a = i.top().data_.array_val;
    if (a->size() != 0)
    {
        std::cerr << "FAIL Sort empty: result has " << a->size()
                  << " elements\n";
        std::exit(1);
    }
    // test_harness eval() leaves a /quit Token on the e-stack (the
    // dispatcher exits on quittype without popping). Anything more
    // is a leak from the operator under test.
    if (i.EStack().load() != 1)
    {
        std::cerr << "FAIL Sort empty leaked estack frames: "
                  << i.EStack().load() << "\n";
        std::exit(1);
    }
}

void test_transpose_empty_via_dispatch(sli3::SLIInterpreter& i)
{
    // Same family: Transpose's empty branch had the same bug.
    EVAL_DEPTH(i, "[ ] Transpose", 1);
    if (!i.top().is_of_type(sli3::arraytype))
    {
        std::cerr << "FAIL Transpose empty: top not arraytype\n";
        std::exit(1);
    }
    if (i.top().data_.array_val->size() != 0)
    {
        std::cerr << "FAIL Transpose empty: non-empty result\n";
        std::exit(1);
    }
    // test_harness eval() leaves a /quit Token on the e-stack (the
    // dispatcher exits on quittype without popping). Anything more
    // is a leak from the operator under test.
    if (i.EStack().load() != 1)
    {
        std::cerr << "FAIL Transpose empty leaked estack frames: "
                  << i.EStack().load() << "\n";
        std::exit(1);
    }
}

// Execute an operator via a procedure body (which routes through
// the iiterate fast path in execute_dispatch_). Verifies the
// operator does not leak e-stack frames after the proc returns.
void test_op_in_proc(sli3::SLIInterpreter& i,
                     const std::string& setup,
                     const std::string& body,
                     long expected_top)
{
    EVAL_CLEAR(i);
    // Inline the operator in a procedure: `{ ... }` then `exec`.
    // Routes execution through proceduretype -> iiterate, exercising
    // the fast path (the codepath that pushes a sentinel for
    // old-ABI ops).
    eval(i, setup + " { " + body + " } exec");

    if (i.OStack().load() < 1)
    {
        std::cerr << "FAIL proc dispatch '" << body
                  << "': empty ostack\n";
        std::exit(1);
    }
    sli3::Token& t = i.top();
    if (!t.is_of_type(sli3::integertype))
    {
        std::cerr << "FAIL proc dispatch '" << body
                  << "': top is not integertype\n";
        std::exit(1);
    }
    if (t.data_.long_val != expected_top)
    {
        std::cerr << "FAIL proc dispatch '" << body
                  << "': got " << t.data_.long_val
                  << ", expected " << expected_top << "\n";
        std::exit(1);
    }
    // test_harness eval() leaves a /quit Token on the e-stack (the
    // dispatcher exits on quittype without popping). Anything more
    // is a leak from the operator under test.
    if (i.EStack().load() != 1)
    {
        std::cerr << "FAIL proc dispatch '" << body
                  << "' leaked estack frames: " << i.EStack().load()
                  << "\n";
        std::exit(1);
    }
}

}  // anonymous namespace

int main()
{
    sli3::SLIInterpreter i;

    // Direct dispatcher path (top-level eval).
    test_arrayload_via_dispatch(i);
    test_getmax_getmin_via_dispatch(i);
    test_sort_empty_via_dispatch(i);
    test_transpose_empty_via_dispatch(i);

    // Iiterate fast path: same operators called from inside a
    // proc-then-exec. This is the codepath that was actually
    // failing in the wild (e.g. `42 ==` -- `==` is a procedure
    // whose body contains `<--`).
    test_op_in_proc(i, "[ 5 2 8 1 4 ]", "GetMax", 8);
    test_op_in_proc(i, "[ 5 2 8 1 4 ]", "GetMin", 1);
    // arrayload-in-proc: [100 200 300] arrayload -> [100 200 300 3].
    // pop the count and last two elements; bottom element survives.
    test_op_in_proc(i, "[ 100 200 300 ]",
                    "arrayload pop pop pop", 100);

    std::cerr << "test_dispatch_abi: all tests passed\n";
    return 0;
}
