// Tests for the slice 4 array stdlib (sli_array_module.cpp).
//
// Two layers of coverage:
//
// 1. Direct .execute(&i) calls for operators that don't require the
//    interpreter dispatcher loop (Range, Reverse, Rotate, Flatten, Sort,
//    Transpose, Partition, arrayload, arraystore, GetMin/Max, Valid,
//    finite_q_d). For these we seed the operand stack, push a placeholder
//    on the execution stack (since the operator pops itself), invoke
//    execute(&i), and inspect the operand stack.
//
// 2. Dispatcher-driven tests for the Map family. Map sets up a
//    multi-frame state on the execution stack and relies on the
//    dispatch loop to keep re-invoking the iterator. We push a quit
//    token at the bottom of the estack and an entry-function token on
//    top, then call execute_dispatch_ and check the result.

#include "sli_array.h"
#include "sli_interpreter.h"
#include "sli_name.h"
#include "sli_string.h"
#include "sli_token.h"

#include <cmath>
#include <cstdlib>
#include <iostream>

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

// Push a placeholder onto the execution stack so a SLIFunction's
// trailing `EStack().pop()` finds something to pop.
void push_estack_placeholder(SLIInterpreter& i)
{
    i.EStack().push(i.new_token<sli3::marktype>());
}

// Build an arraytype Token that owns a fresh TokenArray with the given
// integer contents.
Token int_array(SLIInterpreter& i, std::initializer_list<long> xs)
{
    TokenArray* a = new TokenArray();
    a->reserve(xs.size());
    for (long x : xs)
        a->push_back(i.new_token<sli3::integertype>(x));
    return i.new_token<sli3::arraytype>(a);
}

Token double_array(SLIInterpreter& i, std::initializer_list<double> xs)
{
    TokenArray* a = new TokenArray();
    a->reserve(xs.size());
    for (double x : xs)
        a->push_back(i.new_token<sli3::doubletype>(x));
    return i.new_token<sli3::arraytype>(a);
}

// Drive the dispatcher to completion. EStack must end with a quittype
// at the bottom; the dispatcher exits when it sees quit on top.
void run_to_quit(SLIInterpreter& i)
{
    i.execute_dispatch_(0);
}

// Build a procedure Token whose body executes `ints...` (as literal
// integers, pushed to ostack) followed by a single nametype that
// resolves to a function at dispatch time. Example:
//   name_proc(i, {2}, "mul_ii") -> {2 mul_ii}
Token name_proc(SLIInterpreter& i, std::initializer_list<long> ints,
                std::string const& fn)
{
    TokenArray* body = new TokenArray();
    body->reserve(ints.size() + 1);
    for (long n : ints)
        body->push_back(i.new_token<sli3::integertype>(n));
    body->push_back(i.new_token<sli3::nametype, sli3::Name>(Name(fn.c_str())));
    return i.new_token<sli3::proceduretype, TokenArray*>(body);
}

Token lookup_fn(SLIInterpreter& i, char const* name)
{
    Token t;
    if (!i.lookup(Name(name), t))
    {
        std::cerr << "lookup_fn: " << name << " not in systemdict\n";
        std::exit(1);
    }
    return t;
}

void expect_int_array(SLIInterpreter& i,
                      std::initializer_list<long> expect,
                      int line)
{
    Token& t = i.top();
    if (!t.is_of_type(sli3::arraytype))
    {
        std::cerr << "FAIL @" << line << ": top is not arraytype\n";
        std::exit(1);
    }
    TokenArray* a = t.data_.array_val;
    if (a->size() != expect.size())
    {
        std::cerr << "FAIL @" << line
                  << ": array size " << a->size()
                  << ", expected " << expect.size() << "\n";
        std::exit(1);
    }
    size_t k = 0;
    for (long v : expect)
    {
        Token const& e = a->get(k);
        if (!e.is_of_type(sli3::integertype) || e.data_.long_val != v)
        {
            std::cerr << "FAIL @" << line
                      << ": element " << k
                      << " did not match " << v << "\n";
            std::exit(1);
        }
        ++k;
    }
    i.pop();
}

void run_map(SLIInterpreter& i,
             char const* fn_name,
             Token const& array_tok,
             Token const& proc_tok)
{
    i.EStack().clear();
    i.OStack().clear();
    i.EStack().push(i.new_token<sli3::quittype>());
    i.push(array_tok);
    i.push(proc_tok);
    i.EStack().push(lookup_fn(i, fn_name));
    run_to_quit(i);
}

void test_map(SLIInterpreter& i)
{
    // [1 2 3 4] {2 mul_ii} Map_a_p -> [2 4 6 8]
    run_map(i, "Map_a_p", int_array(i, {1, 2, 3, 4}),
            name_proc(i, {2}, "mul_ii"));
    expect_int_array(i, {2, 4, 6, 8}, __LINE__);

    // Empty source: result is the (empty) source untouched.
    run_map(i, "Map_a_p", int_array(i, {}),
            name_proc(i, {2}, "mul_ii"));
    expect_int_array(i, {}, __LINE__);

    // Empty proc: leaves source untouched.
    {
        TokenArray* empty_body = new TokenArray();
        Token empty_proc = i.new_token<sli3::proceduretype, TokenArray*>(empty_body);
        run_map(i, "Map_a_p", int_array(i, {7, 8, 9}), empty_proc);
        expect_int_array(i, {7, 8, 9}, __LINE__);
    }
}

void test_map_indexed(SLIInterpreter& i)
{
    // [10 10 10] {add_ii} MapIndexed_a_p -> [11 12 13]
    // 1-based indices per Mathematica convention.
    run_map(i, "MapIndexed_a_p", int_array(i, {10, 10, 10}),
            name_proc(i, {}, "add_ii"));
    expect_int_array(i, {11, 12, 13}, __LINE__);

    // [1 2 3 4 5] {add_ii} MapIndexed -> [2 4 6 8 10]
    // (Mathematica doc example from mathematica.sli:1067.)
    run_map(i, "MapIndexed_a_p", int_array(i, {1, 2, 3, 4, 5}),
            name_proc(i, {}, "add_ii"));
    expect_int_array(i, {2, 4, 6, 8, 10}, __LINE__);
}

void test_map_thread(SLIInterpreter& i)
{
    // [[1 2 3] [10 20 30]] {add_ii} MapThread_a_p -> [11 22 33]
    TokenArray* outer = new TokenArray();
    outer->push_back(int_array(i, {1, 2, 3}));
    outer->push_back(int_array(i, {10, 20, 30}));
    Token outer_tok = i.new_token<sli3::arraytype>(outer);
    run_map(i, "MapThread_a_p", outer_tok, name_proc(i, {}, "add_ii"));
    expect_int_array(i, {11, 22, 33}, __LINE__);
}

// -----------------------------------------------------------------
// Direct operator tests
// -----------------------------------------------------------------

void test_range(SLIInterpreter& i)
{
    // [5] Range -> [1 2 3 4 5]
    {
        push_estack_placeholder(i);
        i.push(int_array(i, {5}));
        Token map_t = i.lookup(Name("Range"));
        map_t.data_.func_val->execute(&i);
        TokenArray* r = i.top().data_.array_val;
        CHECK(r->size() == 5);
        for (long k = 0; k < 5; ++k)
            CHECK((*r)[k].data_.long_val == k + 1);
        i.pop();
    }
    // [3 7] Range -> [3 4 5 6 7]
    {
        push_estack_placeholder(i);
        i.push(int_array(i, {3, 7}));
        i.lookup(Name("Range")).data_.func_val->execute(&i);
        TokenArray* r = i.top().data_.array_val;
        CHECK(r->size() == 5);
        CHECK((*r)[0].data_.long_val == 3);
        CHECK((*r)[4].data_.long_val == 7);
        i.pop();
    }
    // [1 10 2] Range -> [1 3 5 7 9]
    {
        push_estack_placeholder(i);
        i.push(int_array(i, {1, 10, 2}));
        i.lookup(Name("Range")).data_.func_val->execute(&i);
        TokenArray* r = i.top().data_.array_val;
        CHECK(r->size() == 5);
        CHECK((*r)[0].data_.long_val == 1);
        CHECK((*r)[1].data_.long_val == 3);
        CHECK((*r)[4].data_.long_val == 9);
        i.pop();
    }
    // [0.0 1.0 0.5] Range -> [0.0 0.5 1.0]
    {
        push_estack_placeholder(i);
        i.push(double_array(i, {0.0, 1.0, 0.5}));
        i.lookup(Name("Range")).data_.func_val->execute(&i);
        TokenArray* r = i.top().data_.array_val;
        CHECK(r->size() == 3);
        CHECK(std::fabs((*r)[0].data_.double_val - 0.0) < 1e-12);
        CHECK(std::fabs((*r)[1].data_.double_val - 0.5) < 1e-12);
        CHECK(std::fabs((*r)[2].data_.double_val - 1.0) < 1e-12);
        i.pop();
    }
}

void test_array_op(SLIInterpreter& i)
{
    // n array_ -> [0 0 ... 0]  (n integer zeros)
    {
        push_estack_placeholder(i);
        i.push<long>(5);
        i.lookup(Name("array_")).data_.func_val->execute(&i);
        CHECK(i.top().is_of_type(sli3::arraytype));
        TokenArray* r = i.top().data_.array_val;
        CHECK(r->size() == 5);
        for (long k = 0; k < 5; ++k) {
            CHECK((*r)[k].is_of_type(sli3::integertype));
            CHECK((*r)[k].data_.long_val == 0);
        }
        i.pop();
    }
    // 0 array_ -> []
    {
        push_estack_placeholder(i);
        i.push<long>(0);
        i.lookup(Name("array_")).data_.func_val->execute(&i);
        CHECK(i.top().is_of_type(sli3::arraytype));
        CHECK(i.top().data_.array_val->size() == 0);
        i.pop();
    }
}

void test_reverse(SLIInterpreter& i)
{
    push_estack_placeholder(i);
    i.push(int_array(i, {1, 2, 3, 4}));
    TokenArray* original = i.top().data_.array_val;
    long original_refs = original->references();
    i.lookup(Name("Reverse")).data_.func_val->execute(&i);
    TokenArray* r = i.top().data_.array_val;
    CHECK(r != original);  // produced a fresh array
    CHECK(r->size() == 4);
    CHECK((*r)[0].data_.long_val == 4);
    CHECK((*r)[1].data_.long_val == 3);
    CHECK((*r)[2].data_.long_val == 2);
    CHECK((*r)[3].data_.long_val == 1);
    (void)original_refs;  // we don't need to validate this
    i.pop();
}

void test_rotate(SLIInterpreter& i)
{
    // [1 2 3 4 5] 2 Rotate -> [4 5 1 2 3] (right-rotate by 2)
    push_estack_placeholder(i);
    i.push(int_array(i, {1, 2, 3, 4, 5}));
    i.push<long>(2);
    i.lookup(Name("Rotate")).data_.func_val->execute(&i);
    TokenArray* r = i.top().data_.array_val;
    CHECK(r->size() == 5);
    CHECK((*r)[0].data_.long_val == 4);
    CHECK((*r)[1].data_.long_val == 5);
    CHECK((*r)[2].data_.long_val == 1);
    CHECK((*r)[3].data_.long_val == 2);
    CHECK((*r)[4].data_.long_val == 3);
    i.pop();
}

void test_flatten(SLIInterpreter& i)
{
    // [[1 2] 3 [4 5]] Flatten -> [1 2 3 4 5]
    TokenArray* outer = new TokenArray();
    outer->push_back(int_array(i, {1, 2}));
    outer->push_back(i.new_token<sli3::integertype>(3L));
    outer->push_back(int_array(i, {4, 5}));

    push_estack_placeholder(i);
    i.push(i.new_token<sli3::arraytype>(outer));
    i.lookup(Name("Flatten")).data_.func_val->execute(&i);
    TokenArray* r = i.top().data_.array_val;
    CHECK(r->size() == 5);
    for (long k = 0; k < 5; ++k)
        CHECK((*r)[k].data_.long_val == k + 1);
    i.pop();
}

void test_sort(SLIInterpreter& i)
{
    // ints
    {
        push_estack_placeholder(i);
        i.push(int_array(i, {5, 1, 4, 2, 3}));
        i.lookup(Name("Sort")).data_.func_val->execute(&i);
        TokenArray* r = i.top().data_.array_val;
        CHECK(r->size() == 5);
        for (long k = 0; k < 5; ++k)
            CHECK((*r)[k].data_.long_val == k + 1);
        i.pop();
    }
    // doubles
    {
        push_estack_placeholder(i);
        i.push(double_array(i, {3.5, 0.5, 2.5, 1.5}));
        i.lookup(Name("Sort")).data_.func_val->execute(&i);
        TokenArray* r = i.top().data_.array_val;
        CHECK(r->size() == 4);
        CHECK(std::fabs((*r)[0].data_.double_val - 0.5) < 1e-12);
        CHECK(std::fabs((*r)[3].data_.double_val - 3.5) < 1e-12);
        i.pop();
    }
    // strings
    {
        TokenArray* a = new TokenArray();
        a->push_back(i.new_token<sli3::stringtype, std::string>(std::string("banana")));
        a->push_back(i.new_token<sli3::stringtype, std::string>(std::string("apple")));
        a->push_back(i.new_token<sli3::stringtype, std::string>(std::string("cherry")));
        push_estack_placeholder(i);
        i.push(i.new_token<sli3::arraytype>(a));
        i.lookup(Name("Sort")).data_.func_val->execute(&i);
        TokenArray* r = i.top().data_.array_val;
        CHECK(r->size() == 3);
        CHECK((*r)[0].data_.string_val->str() == "apple");
        CHECK((*r)[1].data_.string_val->str() == "banana");
        CHECK((*r)[2].data_.string_val->str() == "cherry");
        i.pop();
    }
}

void test_transpose(SLIInterpreter& i)
{
    // [[3 4 5][6 7 8]] Transpose -> [[3 6][4 7][5 8]]
    TokenArray* outer = new TokenArray();
    outer->push_back(int_array(i, {3, 4, 5}));
    outer->push_back(int_array(i, {6, 7, 8}));

    push_estack_placeholder(i);
    i.push(i.new_token<sli3::arraytype>(outer));
    i.lookup(Name("Transpose")).data_.func_val->execute(&i);
    TokenArray* r = i.top().data_.array_val;
    CHECK(r->size() == 3);
    for (size_t j = 0; j < 3; ++j)
    {
        TokenArray* row = (*r)[j].data_.array_val;
        CHECK(row->size() == 2);
        CHECK((*row)[0].data_.long_val == static_cast<long>(3 + j));
        CHECK((*row)[1].data_.long_val == static_cast<long>(6 + j));
    }
    i.pop();
}

void test_partition(SLIInterpreter& i)
{
    // [1 2 3 4 5] 2 1 Partition -> [[1 2] [2 3] [3 4] [4 5]]
    push_estack_placeholder(i);
    i.push(int_array(i, {1, 2, 3, 4, 5}));
    i.push<long>(2);
    i.push<long>(1);
    i.lookup(Name("Partition_a_i_i")).data_.func_val->execute(&i);
    TokenArray* r = i.top().data_.array_val;
    CHECK(r->size() == 4);
    for (size_t j = 0; j < 4; ++j)
    {
        TokenArray* part = (*r)[j].data_.array_val;
        CHECK(part->size() == 2);
        CHECK((*part)[0].data_.long_val == static_cast<long>(j + 1));
        CHECK((*part)[1].data_.long_val == static_cast<long>(j + 2));
    }
    i.pop();
}

void test_arrayload_arraystore(SLIInterpreter& i)
{
    // [10 20 30] arrayload -> 10 20 30 3
    push_estack_placeholder(i);
    i.push(int_array(i, {10, 20, 30}));
    i.lookup(Name("arrayload")).data_.func_val->execute(&i);
    CHECK(i.load() >= 4);
    CHECK(i.top().data_.long_val == 3);
    i.pop();
    CHECK(i.top().data_.long_val == 30);
    i.pop();
    CHECK(i.top().data_.long_val == 20);
    i.pop();
    CHECK(i.top().data_.long_val == 10);
    i.pop();

    // 10 20 30 3 arraystore -> [10 20 30]
    push_estack_placeholder(i);
    i.push<long>(10);
    i.push<long>(20);
    i.push<long>(30);
    i.push<long>(3);
    i.lookup(Name("arraystore")).data_.func_val->execute(&i);
    TokenArray* r = i.top().data_.array_val;
    CHECK(r->size() == 3);
    CHECK((*r)[0].data_.long_val == 10);
    CHECK((*r)[1].data_.long_val == 20);
    CHECK((*r)[2].data_.long_val == 30);
    i.pop();
}

void test_min_max(SLIInterpreter& i)
{
    push_estack_placeholder(i);
    i.push(int_array(i, {3, 1, 4, 1, 5, 9, 2, 6}));
    i.lookup(Name("GetMax")).data_.func_val->execute(&i);
    CHECK(i.top().data_.long_val == 9);
    i.pop();

    push_estack_placeholder(i);
    i.push(int_array(i, {3, 1, 4, 1, 5, 9, 2, 6}));
    i.lookup(Name("GetMin")).data_.func_val->execute(&i);
    CHECK(i.top().data_.long_val == 1);
    i.pop();

    push_estack_placeholder(i);
    i.push(double_array(i, {3.5, -1.0, 2.0}));
    i.lookup(Name("GetMax")).data_.func_val->execute(&i);
    CHECK(std::fabs(i.top().data_.double_val - 3.5) < 1e-12);
    i.pop();
}

void test_valid_finite(SLIInterpreter& i)
{
    push_estack_placeholder(i);
    i.push(int_array(i, {1, 2, 3}));
    i.lookup(Name("valid_a")).data_.func_val->execute(&i);
    CHECK(i.top().data_.bool_val == true);
    i.pop();

    push_estack_placeholder(i);
    i.push<double>(1.0 / 0.0);  // inf
    i.lookup(Name("finite_q_d")).data_.func_val->execute(&i);
    CHECK(i.top().data_.bool_val == false);
    i.pop();

    push_estack_placeholder(i);
    i.push<double>(3.14);
    i.lookup(Name("finite_q_d")).data_.func_val->execute(&i);
    CHECK(i.top().data_.bool_val == true);
    i.pop();
}

}  // namespace

int main()
{
    SLIInterpreter i;

    test_range(i);
    test_array_op(i);
    test_reverse(i);
    test_rotate(i);
    test_flatten(i);
    test_sort(i);
    test_transpose(i);
    test_partition(i);
    test_arrayload_arraystore(i);
    test_min_max(i);
    test_valid_finite(i);

    // Map family (C++ port) — dispatcher-driven tests on the
    // typed leaves Map_a_p / MapIndexed_a_p / MapThread_a_p.
    test_map(i);
    test_map_indexed(i);
    test_map_thread(i);

    std::cerr << "test_array_module: all checks passed\n";
    return 0;
}
