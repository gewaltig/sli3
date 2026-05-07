// Functional test harness for evaluating SLI source against an
// interpreter and asserting the resulting operand stack.
//
// Usage:
//
//     #include "test_harness.h"
//     using namespace sli_test;
//
//     int main() {
//         sli3::SLIInterpreter i;
//         EVAL_INT(i, "1 1 add_ii", 2);
//         EVAL_BOOL(i, "2 1 gt_ii", true);
//         return 0;
//     }
//
// Each macro evaluates the snippet, then checks the top of the operand
// stack and the stack depth (1 unless overridden). On failure it prints
// the source, the expected vs. observed value, and the source line of
// the assertion, then exits non-zero (CTest convention).
//
// The harness intentionally does NOT call SLIInterpreter::startup(),
// so it does not load lib/sli/sli-init.sli. Tests target individual
// typed-leaf operators registered by the C++ side. After the bootstrap
// is fixed (Slice 5c) a parallel harness can run snippets through the
// fully bootstrapped interpreter to cover trie wrappers and the .sli
// standard library.

#ifndef SLI_TEST_HARNESS_H
#define SLI_TEST_HARNESS_H

#include "sli_interpreter.h"
#include "sli_iostream.h"
#include "sli_token.h"
#include "sli_tokenstack.h"
#include "sli_exceptions.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

namespace sli_test
{

inline void clear_stacks(sli3::SLIInterpreter& i)
{
    i.OStack().clear();
    i.EStack().clear();
}

// Parse and execute a SLI source string against an interpreter. Does
// NOT invoke startup() — sli-init.sli is NOT loaded. Anything not
// registered as a C++ builtin will trigger an UndefinedName error.
//
// Mechanics: wrap the source in an istringstream owned by an SLIistream,
// build an xistreamtype Token, push [quit, xistream, /::parse] on the
// execution stack, and run execute_dispatch_(0). The dispatcher reads
// tokens off the stream, executes them, and exits when the quit
// surfaces (i.e. when the stream is exhausted and iparse drains).
//
// Throws sli3::IllegalOperation or similar if the snippet errors out
// at the C++ level. SLI-level errors (raiseerror) leave state on the
// estack/error_dict_ for callers to inspect — assert helpers below
// treat that as a failure.
inline void eval(sli3::SLIInterpreter& i, const std::string& src)
{
    auto* iss = new std::istringstream(src);
    auto* wrap = new sli3::SLIistream(iss);  // takes ownership of iss

    // quit at the bottom — dispatcher exits cleanly when iparse ends.
    i.EStack().push(i.new_token<sli3::quittype>());
    sli3::Token x(i.get_type(sli3::xistreamtype));
    x.data_.istream_val = wrap;
    i.EStack().push(x);
    i.EStack().push(i.new_token<sli3::nametype, sli3::Name>(i.iparse_name));

    i.execute_dispatch_(0);
}

// ---------- failure reporting ---------------------------------------

[[noreturn]] inline void fail(const char* file,
                              int line,
                              const std::string& src,
                              const std::string& detail)
{
    std::cerr << "FAIL " << file << ":" << line << "\n"
              << "  src:    " << src << "\n"
              << "  detail: " << detail << "\n";
    std::exit(1);
}

// ---------- ostack inspection helpers --------------------------------

inline void check_depth(sli3::SLIInterpreter& i,
                        size_t expected,
                        const char* file,
                        int line,
                        const std::string& src)
{
    size_t d = i.OStack().load();
    if (d != expected)
    {
        std::ostringstream m;
        m << "ostack depth " << d << ", expected " << expected;
        fail(file, line, src, m.str());
    }
}

inline void check_int(sli3::SLIInterpreter& i,
                      long expected,
                      const char* file,
                      int line,
                      const std::string& src)
{
    sli3::Token& t = i.top();
    if (!t.is_of_type(sli3::integertype))
    {
        fail(file, line, src, "ostack top is not integertype");
    }
    long got = t.data_.long_val;
    if (got != expected)
    {
        std::ostringstream m;
        m << "got " << got << ", expected " << expected;
        fail(file, line, src, m.str());
    }
}

inline void check_double(sli3::SLIInterpreter& i,
                         double expected,
                         double tol,
                         const char* file,
                         int line,
                         const std::string& src)
{
    sli3::Token& t = i.top();
    if (!t.is_of_type(sli3::doubletype))
    {
        fail(file, line, src, "ostack top is not doubletype");
    }
    double got = t.data_.double_val;
    if (std::fabs(got - expected) > tol)
    {
        std::ostringstream m;
        m << "got " << got << ", expected " << expected
          << " (tol " << tol << ")";
        fail(file, line, src, m.str());
    }
}

inline void check_bool(sli3::SLIInterpreter& i,
                       bool expected,
                       const char* file,
                       int line,
                       const std::string& src)
{
    sli3::Token& t = i.top();
    if (!t.is_of_type(sli3::booltype))
    {
        fail(file, line, src, "ostack top is not booltype");
    }
    bool got = t.data_.bool_val;
    if (got != expected)
    {
        std::ostringstream m;
        m << "got " << (got ? "true" : "false")
          << ", expected " << (expected ? "true" : "false");
        fail(file, line, src, m.str());
    }
}

inline void check_string(sli3::SLIInterpreter& i,
                         const std::string& expected,
                         const char* file,
                         int line,
                         const std::string& src)
{
    sli3::Token& t = i.top();
    if (!t.is_of_type(sli3::stringtype))
    {
        fail(file, line, src, "ostack top is not stringtype");
    }
    const std::string& got = static_cast<std::string&>(t);
    if (got != expected)
    {
        std::ostringstream m;
        m << "got \"" << got << "\", expected \"" << expected << "\"";
        fail(file, line, src, m.str());
    }
}

}  // namespace sli_test

// ---------- assertion macros ----------------------------------------
//
// Each macro evaluates the source, expects depth==1 unless the _D
// variant is used, and checks the ostack top against the expected
// value. The interpreter stacks are cleared before evaluating, so
// tests are independent.

#define EVAL_CLEAR(I) ::sli_test::clear_stacks(I)

#define EVAL_INT(I, SRC, N)                                              \
    do {                                                                 \
        ::sli_test::clear_stacks(I);                                     \
        ::sli_test::eval((I), (SRC));                                    \
        ::sli_test::check_depth((I), 1, __FILE__, __LINE__, (SRC));      \
        ::sli_test::check_int((I), (N), __FILE__, __LINE__, (SRC));      \
    } while (0)

#define EVAL_DOUBLE(I, SRC, X, TOL)                                      \
    do {                                                                 \
        ::sli_test::clear_stacks(I);                                     \
        ::sli_test::eval((I), (SRC));                                    \
        ::sli_test::check_depth((I), 1, __FILE__, __LINE__, (SRC));      \
        ::sli_test::check_double((I), (X), (TOL),                        \
                                 __FILE__, __LINE__, (SRC));             \
    } while (0)

#define EVAL_BOOL(I, SRC, B)                                             \
    do {                                                                 \
        ::sli_test::clear_stacks(I);                                     \
        ::sli_test::eval((I), (SRC));                                    \
        ::sli_test::check_depth((I), 1, __FILE__, __LINE__, (SRC));      \
        ::sli_test::check_bool((I), (B), __FILE__, __LINE__, (SRC));     \
    } while (0)

#define EVAL_STRING(I, SRC, S)                                           \
    do {                                                                 \
        ::sli_test::clear_stacks(I);                                     \
        ::sli_test::eval((I), (SRC));                                    \
        ::sli_test::check_depth((I), 1, __FILE__, __LINE__, (SRC));      \
        ::sli_test::check_string((I), (S), __FILE__, __LINE__, (SRC));   \
    } while (0)

// Evaluate, then check ostack depth (no value check). Useful when a
// snippet should leave the stack in a particular shape.
#define EVAL_DEPTH(I, SRC, N)                                            \
    do {                                                                 \
        ::sli_test::clear_stacks(I);                                     \
        ::sli_test::eval((I), (SRC));                                    \
        ::sli_test::check_depth((I), (N), __FILE__, __LINE__, (SRC));    \
    } while (0)

#endif  // SLI_TEST_HARNESS_H
