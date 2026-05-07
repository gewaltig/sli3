// Stage 5.8: stack ops range-check long-to-size_t. Negative inputs
// previously wrapped to huge unsigned and tripped require_stack_load
// with a misleading StackUnderflow; now they raise RangeCheck up
// front.

#include "sli_dictionary.h"
#include "sli_exceptions.h"
#include "sli_interpreter.h"
#include "sli_iostream.h"
#include "sli_name.h"
#include "sli_token.h"
#include "sli_tokenstack.h"

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
    i.set_call_depth(0);

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
    try { i.execute_dispatch_(0); } catch (...) {}
    auto got = get_errorname(i);
    if (got != expected)
    {
        std::cerr << "FAIL `" << src << "` expected /errorname="
                  << expected << " got " << got << "\n";
        std::exit(1);
    }
}

void test_npop_negative(SLIInterpreter& i)    { expect_error(i, "1 2 3 -1 npop", "RangeCheck"); }
void test_index_negative(SLIInterpreter& i)   { expect_error(i, "1 2 3 -1 index", "RangeCheck"); }
void test_copy_negative(SLIInterpreter& i)    { expect_error(i, "1 2 3 -2 copy", "RangeCheck"); }
void test_roll_negative_n(SLIInterpreter& i)  { expect_error(i, "1 2 3 -1 1 roll", "RangeCheck"); }

}  // namespace

int main()
{
    SLIInterpreter i;
    test_npop_negative(i);
    test_index_negative(i);
    test_copy_negative(i);
    test_roll_negative_n(i);
    std::cout << "test_stack_ops_negatives: ok\n";
    return 0;
}
