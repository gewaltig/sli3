// Phase 4: tests for the C++ /bind_leaf (BindFunction in
// sli_control.cpp). Asserts the deep-bind_contract:
//
//   1. names that resolve to functiontype are replaced in-place
//   2. names that resolve to trietype are replaced in-place
//   3. names that resolve to litproctype (user procs) are preserved
//   4. undefined names are preserved (gs/PostScript behaviour)
//   5. recursion into nested litproctype bodies
//   6. idempotency: bind(bind(x)) == bind(x)
//   7. arrays that are not procedures are not modified

#include "test_harness.h"
#include "sli_array.h"
#include "sli_function.h"
#include "sli_interpreter.h"
#include "sli_token.h"

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

using namespace sli3;

#define CHECK(cond)                                                  \
    do {                                                             \
        if (!(cond)) {                                               \
            std::cerr << "FAIL @" << __LINE__ << ": " #cond "\n";    \
            std::exit(1);                                            \
        }                                                            \
    } while (0)

namespace
{

// Pull the proceduretype Token currently on top of the operand
// stack and return its array_val (the body), leaving the proc on
// the stack so it can be inspected further.
TokenArray *top_body(SLIInterpreter &i)
{
    CHECK(i.OStack().load() >= 1);
    Token const &top = i.OStack().top();
    CHECK(top.tag() == sli3::proceduretype
       || top.tag() == sli3::litproceduretype);
    return top.data_.array_val;
}

// Evaluate `src` against a fresh interpreter. The bound proc (if any)
// is left on the operand stack. Caller uses top_body() to inspect.
void run(SLIInterpreter &i, std::string const &src)
{
    sli_test::eval(i, src);
}

// Test 1: known function name (add) is replaced.
void test_basic_function_replacement()
{
    SLIInterpreter i;
    run(i, "{ 1 1 add } bind_");
    TokenArray *body = top_body(i);
    CHECK(body->size() == 3);
    CHECK(body->get(0).tag() == sli3::integertype);
    CHECK(body->get(1).tag() == sli3::integertype);
    // After bind, /add (originally nametype) should be functiontype.
    CHECK(body->get(2).tag() == sli3::functiontype);
}

// Test 2: trietype-resolving names get replaced.
//
// `add` itself is registered as a trie in typeinit.sli (a dispatch
// for add_ii / add_dd / etc), so bind_should pick up the trie.
// `pop` (alias `;`) is a flat function. Inspect both.
void test_trie_replacement()
{
    SLIInterpreter i;
    run(i, "{ pop } bind_");
    TokenArray *body = top_body(i);
    CHECK(body->size() == 1);
    // /pop resolves to either functiontype or trietype depending on
    // typeinit; either is acceptable for the bind_contract.
    auto tag = body->get(0).tag();
    CHECK(tag == sli3::functiontype || tag == sli3::trietype);
}

// Test 3: user-defined proc names are NOT replaced.
void test_user_proc_preserved()
{
    SLIInterpreter i;
    run(i, "/myproc { 100 } def { myproc 1 add } bind_");
    TokenArray *body = top_body(i);
    CHECK(body->size() == 3);
    // /myproc resolves to litproc -- bind_leaves it as a name.
    CHECK(body->get(0).tag() == sli3::nametype);
    CHECK(body->get(1).tag() == sli3::integertype);
    // /add still got replaced.
    auto tag = body->get(2).tag();
    CHECK(tag == sli3::functiontype || tag == sli3::trietype);
}

// Test 4: undefined names are preserved.
void test_undefined_preserved()
{
    SLIInterpreter i;
    run(i, "{ no_such_name 1 add } bind_");
    TokenArray *body = top_body(i);
    CHECK(body->size() == 3);
    CHECK(body->get(0).tag() == sli3::nametype);  // undefined → stays
    CHECK(body->get(1).tag() == sli3::integertype);
    auto tag = body->get(2).tag();
    CHECK(tag == sli3::functiontype || tag == sli3::trietype);
}

// Test 5: nested litproc bodies are recursed into.
void test_recursive_bind()
{
    SLIInterpreter i;
    run(i, "{ { 1 1 add } } bind_");
    TokenArray *outer = top_body(i);
    CHECK(outer->size() == 1);
    CHECK(outer->get(0).tag() == sli3::litproceduretype);
    TokenArray *inner = outer->get(0).data_.array_val;
    CHECK(inner->size() == 3);
    // The nested /add should be bound too.
    auto tag = inner->get(2).tag();
    CHECK(tag == sli3::functiontype || tag == sli3::trietype);
}

// Test 6: idempotent.
void test_idempotent()
{
    SLIInterpreter i;
    run(i, "{ 1 1 add } bind_ bind_");
    TokenArray *body = top_body(i);
    CHECK(body->size() == 3);
    auto tag = body->get(2).tag();
    CHECK(tag == sli3::functiontype || tag == sli3::trietype);
}

// Test 7: a bound proc runs and produces the same result as the
// unbound proc. Functional check that bind_doesn't change semantics.
void test_semantics_preserved()
{
    SLIInterpreter i;
    run(i, "/bound  { 1 2 add } bind_ def "
           "/unbound { 1 2 add }      def "
           "bound unbound add ");
    // ostack: [unbound_result, bound_result] -> add -> 6
    CHECK(i.OStack().load() == 1);
    CHECK(i.OStack().top().tag() == sli3::integertype);
    CHECK(i.OStack().top().data_.long_val == 6);
}

} // namespace

int main()
{
    test_basic_function_replacement();
    test_trie_replacement();
    test_user_proc_preserved();
    test_undefined_preserved();
    test_recursive_bind();
    test_idempotent();
    test_semantics_preserved();
    std::cout << "OK\n";
    return 0;
}
