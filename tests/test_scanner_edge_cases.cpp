// Stage 5.6 / 5.7: scanner edge cases.
//
// 5.6 Line counter: `\n\n\nfoo` should reach line 4 (one for each
//     leading newline plus the `foo` line). Previously every line
//     transition double-counted so the same input reached line 7.
// 5.7 Integer overflow: `9999999999999999999` (>LONG_MAX) raises a
//     clean SyntaxError instead of UB-wrapping via std::labs.

#include "sli_interpreter.h"
#include "sli_iostream.h"
#include "sli_parser.h"
#include "sli_scanner.h"
#include "sli_token.h"

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

void test_line_counter()
{
    // After three leading newlines, the next token sits on line 4.
    // Today the bug double-counts: every newline + first-char of
    // following line both bump line, so the same input reaches
    // line 7 instead of 4.
    SLIInterpreter i;
    std::istringstream in("\n\n\nfoo");
    Scanner s(&in);
    Token t;
    bool ok = s(i, t);
    CHECK(ok);
    CHECK(t.is_of_type(sli3::literaltype) || t.is_of_type(sli3::nametype));
    CHECK(s.get_line() == 4);
}

void test_integer_overflow()
{
    // 19-digit overflow value. strtoll reports ERANGE; the scanner
    // surfaces it as a SyntaxError-grade scan failure (return false).
    SLIInterpreter i;
    std::istringstream in("9999999999999999999");
    Scanner s(&in);
    Token t;
    bool ok = s(i, t);
    CHECK(!ok);   // scan failed cleanly; no UB.
}

void test_integer_at_long_max()
{
    SLIInterpreter i;
    std::istringstream in("9223372036854775807");   // LONG_MAX
    Scanner s(&in);
    Token t;
    bool ok = s(i, t);
    CHECK(ok);
    CHECK(t.is_of_type(sli3::integertype));
    CHECK(t.data_.long_val == 9223372036854775807L);
}

}  // namespace

int main()
{
    test_line_counter();
    test_integer_overflow();
    test_integer_at_long_max();
    std::cout << "test_scanner_edge_cases: ok\n";
    return 0;
}
