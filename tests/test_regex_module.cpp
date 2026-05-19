// Tests for sli_regex_module.cpp -- the three POSIX <regex.h>
// primitives: regcomp_, regexec_, regerror_, plus the regexdict
// flag dictionary. These run against direct .execute(&i) on each
// op (the SLI-level wrappers in lib/sli/regexp.sli depend on
// trie + dispatch and are covered separately by full-bootstrap
// smoke checks).

#include "sli_array.h"
#include "sli_dictionary.h"
#include "sli_interpreter.h"
#include "sli_name.h"
#include "sli_regextype.h"
#include "sli_serialize.h"
#include "sli_string.h"
#include "sli_token.h"
#include "sli_tokenstack.h"

#include <cstdlib>
#include <iostream>
#include <regex.h>
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

// Mirrors push_estack_placeholder in test_array_module.cpp: the
// dispatcher pre-pops the fn slot under the new-ABI contract, but
// these tests invoke fn->execute(&i) directly, so we put a dummy on
// the estack for the operator's (now-removed) self-pop -- the three
// regex ops don't pop self anymore, so the placeholder just keeps
// the estack non-empty in case future code reintroduces a guard.
void push_estack_placeholder(SLIInterpreter& i)
{
    i.EStack().push(i.new_token<sli3::marktype>());
}

// Build a stringtype Token holding `s`.
Token str_token(SLIInterpreter& i, std::string s)
{
    return i.new_token<sli3::stringtype, std::string>(std::move(s));
}

// Build an integertype Token holding `n`.
Token int_token(SLIInterpreter& i, long n)
{
    return i.new_token<sli3::integertype, long>(n);
}

void test_regextype_registered(SLIInterpreter& i)
{
    SLIType* t = i.get_type(sli3::regextype);
    CHECK(t != 0);
    CHECK(t->get_typename() == Name("regextype"));
}

void test_regexdict(SLIInterpreter& i)
{
    Token rd;
    CHECK(i.lookup(Name("regexdict"), rd));
    CHECK(rd.is_of_type(sli3::dictionarytype));
    Dictionary* d = rd.data_.dict_val;
    CHECK(d != 0);
    CHECK(d->known(Name("REG_EXTENDED")));
    CHECK(d->known(Name("REG_ICASE")));
    CHECK(d->known(Name("REG_NOSUB")));
    CHECK(d->known(Name("REG_NEWLINE")));
    CHECK(d->known(Name("REG_NOTBOL")));
    CHECK(d->known(Name("REG_NOTEOL")));
    CHECK(d->known(Name("REG_BADPAT")));
    CHECK(d->known(Name("REG_EPAREN")));
}

void test_regcomp_good(SLIInterpreter& i)
{
    push_estack_placeholder(i);
    i.push(str_token(i, "(simple)"));
    i.push(int_token(i, REG_EXTENDED));
    Token& fn = i.lookup(Name("regcomp_"));
    fn.data_.func_val->execute(&i);
    // Expected ostack: bottom -> regex true
    CHECK(i.load() == 2);
    CHECK(i.top().is_of_type(sli3::booltype));
    CHECK(i.top().data_.bool_val == true);
    CHECK(i.pick(1).is_of_type(sli3::regextype));
    Regex* r = i.pick(1).data_.regex_val;
    CHECK(r != 0);
    CHECK(r->compiled());
    i.pop(2);
    i.EStack().pop();
}

// Capture the regex Token from a successful regcomp_ for use in
// downstream regexec_ / regerror_ tests.
Token compile_pattern(SLIInterpreter& i, std::string pat, int flags)
{
    push_estack_placeholder(i);
    i.push(str_token(i, std::move(pat)));
    i.push(int_token(i, flags));
    Token& fn = i.lookup(Name("regcomp_"));
    fn.data_.func_val->execute(&i);
    CHECK(i.load() >= 2);
    CHECK(i.top().is_of_type(sli3::booltype));
    bool ok = i.top().data_.bool_val;
    CHECK(ok);
    i.pop();  // bool
    Token rx = i.top();  // copy bumps refcount
    i.pop();             // takes original
    i.EStack().pop();
    return rx;
}

void test_regcomp_bad(SLIInterpreter& i)
{
    push_estack_placeholder(i);
    i.push(str_token(i, "(unclosed"));
    i.push(int_token(i, REG_EXTENDED));
    Token& fn = i.lookup(Name("regcomp_"));
    fn.data_.func_val->execute(&i);
    // Expected ostack: bottom -> regex int_code false
    CHECK(i.load() == 3);
    CHECK(i.top().is_of_type(sli3::booltype));
    CHECK(i.top().data_.bool_val == false);
    CHECK(i.pick(1).is_of_type(sli3::integertype));
    long code = i.pick(1).data_.long_val;
    CHECK(code != 0);
    CHECK(i.pick(2).is_of_type(sli3::regextype));
    Regex* r = i.pick(2).data_.regex_val;
    CHECK(r != 0);
    CHECK(!r->compiled());

    // Now decode via regerror_. Stack on entry: regex int_code.
    // (drop the false flag first)
    i.pop();  // drop bool
    Token& er = i.lookup(Name("regerror_"));
    er.data_.func_val->execute(&i);
    CHECK(i.load() == 1);
    CHECK(i.top().is_of_type(sli3::stringtype));
    std::string const& msg = i.top().data_.string_val->str();
    CHECK(!msg.empty());
    i.pop();
    i.EStack().pop();
}

void test_regexec_no_capture(SLIInterpreter& i)
{
    Token rx = compile_pattern(i, "simple", REG_EXTENDED);

    push_estack_placeholder(i);
    i.push(rx);  // regex
    i.push(str_token(i, "   simple   "));
    i.push(int_token(i, 0));  // size
    i.push(int_token(i, 0));  // eflags
    Token& fn = i.lookup(Name("regexec_"));
    fn.data_.func_val->execute(&i);
    // size=0 -> only the status code is pushed.
    CHECK(i.load() == 1);
    CHECK(i.top().is_of_type(sli3::integertype));
    CHECK(i.top().data_.long_val == 0);  // match
    i.pop();
    i.EStack().pop();

    // No match path.
    push_estack_placeholder(i);
    i.push(rx);
    i.push(str_token(i, "no_match_here"));
    i.push(int_token(i, 0));
    i.push(int_token(i, 0));
    fn.data_.func_val->execute(&i);
    CHECK(i.load() == 1);
    CHECK(i.top().is_of_type(sli3::integertype));
    CHECK(i.top().data_.long_val == REG_NOMATCH);
    i.pop();
    i.EStack().pop();
}

void test_regexec_with_captures(SLIInterpreter& i)
{
    // Example from lib/sli/regexp.sli line 145:
    //   (simple) regcomp; "   simple   " 1 0 regexec -> [[3 9]] 0
    Token rx = compile_pattern(i, "simple", REG_EXTENDED);

    push_estack_placeholder(i);
    i.push(rx);
    i.push(str_token(i, "   simple   "));
    i.push(int_token(i, 1));
    i.push(int_token(i, 0));
    Token& fn = i.lookup(Name("regexec_"));
    fn.data_.func_val->execute(&i);
    // ostack: [[so eo]] status
    CHECK(i.load() == 2);
    CHECK(i.top().is_of_type(sli3::integertype));
    CHECK(i.top().data_.long_val == 0);
    CHECK(i.pick(1).is_of_type(sli3::arraytype));
    TokenArray* outer = i.pick(1).data_.array_val;
    CHECK(outer != 0);
    CHECK(outer->size() == 1);
    Token const& pair_t = (*outer)[0];
    CHECK(pair_t.is_of_type(sli3::arraytype));
    TokenArray* pair = pair_t.data_.array_val;
    CHECK(pair->size() == 2);
    CHECK((*pair)[0].data_.long_val == 3);
    CHECK((*pair)[1].data_.long_val == 9);
    i.pop(2);
    i.EStack().pop();
}

void test_regexec_multi_group(SLIInterpreter& i)
{
    // Example 4 in lib/sli/regexp.sli line 153-159:
    //   /rx (:(passed|result|error):([0-9]+):(.*):) regcomp def
    //   /st (:result:4:this is the result:) def
    //   rx st 2 0 regexec -> [[0 29] [1 7]] 0
    Token rx = compile_pattern(i,
        ":(passed|result|error):([0-9]+):(.*):", REG_EXTENDED);

    push_estack_placeholder(i);
    i.push(rx);
    i.push(str_token(i, ":result:4:this is the result:"));
    i.push(int_token(i, 2));
    i.push(int_token(i, 0));
    Token& fn = i.lookup(Name("regexec_"));
    fn.data_.func_val->execute(&i);
    CHECK(i.load() == 2);
    CHECK(i.top().data_.long_val == 0);
    TokenArray* outer = i.pick(1).data_.array_val;
    CHECK(outer->size() == 2);
    TokenArray* g0 = (*outer)[0].data_.array_val;
    TokenArray* g1 = (*outer)[1].data_.array_val;
    CHECK((*g0)[0].data_.long_val == 0);
    CHECK((*g0)[1].data_.long_val == 29);
    CHECK((*g1)[0].data_.long_val == 1);
    CHECK((*g1)[1].data_.long_val == 7);
    i.pop(2);
    i.EStack().pop();
}

void test_regexec_nomatch_array(SLIInterpreter& i)
{
    Token rx = compile_pattern(i, "xyz", REG_EXTENDED);

    push_estack_placeholder(i);
    i.push(rx);
    i.push(str_token(i, "abc"));
    i.push(int_token(i, 1));
    i.push(int_token(i, 0));
    Token& fn = i.lookup(Name("regexec_"));
    fn.data_.func_val->execute(&i);
    CHECK(i.load() == 2);
    CHECK(i.top().data_.long_val == REG_NOMATCH);
    TokenArray* outer = i.pick(1).data_.array_val;
    CHECK(outer->size() == 1);
    TokenArray* pair = (*outer)[0].data_.array_val;
    // POSIX leaves unmatched slot values unspecified on REG_NOMATCH;
    // we only assert the array shape (two integertype slots).
    CHECK(pair->size() == 2);
    CHECK((*pair)[0].is_of_type(sli3::integertype));
    CHECK((*pair)[1].is_of_type(sli3::integertype));
    i.pop(2);
    i.EStack().pop();
}

void test_regex_icase(SLIInterpreter& i)
{
    Token rx = compile_pattern(i, "hello", REG_EXTENDED | REG_ICASE);

    push_estack_placeholder(i);
    i.push(rx);
    i.push(str_token(i, "HeLlO world"));
    i.push(int_token(i, 0));
    i.push(int_token(i, 0));
    Token& fn = i.lookup(Name("regexec_"));
    fn.data_.func_val->execute(&i);
    CHECK(i.load() == 1);
    CHECK(i.top().data_.long_val == 0);
    i.pop();
    i.EStack().pop();
}

void test_serialize_roundtrip(SLIInterpreter& i)
{
    // Serialize a regextype Token; deserialize into a fresh Token;
    // confirm the round-tripped regex still matches the same subject.
    Token rx = compile_pattern(i, "f.o+", REG_EXTENDED);

    std::stringstream buf;
    BinaryWriter w(buf);
    w.write_header();
    write_token(rx, w);

    BinaryReader r(buf);
    r.read_header();
    Token rx2 = read_token(r, i);
    CHECK(rx2.is_of_type(sli3::regextype));
    Regex* g = rx2.data_.regex_val;
    CHECK(g != 0);
    CHECK(g->compiled());
    CHECK(g->pattern() == "f.o+");

    push_estack_placeholder(i);
    i.push(rx2);
    i.push(str_token(i, "say fooo please"));
    i.push(int_token(i, 1));
    i.push(int_token(i, 0));
    Token& fn = i.lookup(Name("regexec_"));
    fn.data_.func_val->execute(&i);
    CHECK(i.load() == 2);
    CHECK(i.top().data_.long_val == 0);
    TokenArray* outer = i.pick(1).data_.array_val;
    TokenArray* pair = (*outer)[0].data_.array_val;
    CHECK((*pair)[0].data_.long_val == 4);  // "fooo" starts at index 4
    CHECK((*pair)[1].data_.long_val == 8);
    i.pop(2);
    i.EStack().pop();
}

}  // anonymous namespace

int main()
{
    SLIInterpreter i;
    test_regextype_registered(i);
    test_regexdict(i);
    test_regcomp_good(i);
    test_regcomp_bad(i);
    test_regexec_no_capture(i);
    test_regexec_with_captures(i);
    test_regexec_multi_group(i);
    test_regexec_nomatch_array(i);
    test_regex_icase(i);
    test_serialize_roundtrip(i);
    std::cout << "test_regex_module: OK\n";
    return 0;
}
