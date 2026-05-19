// POSIX <regex.h> primitives. NEST 2.x sliregexp.cc ported to the
// sli3 ABI (16-byte Token, intrusive refcount, new-ABI ops). Three
// commands -- regcomp_, regexec_, regerror_ -- plus the regexdict
// flag dictionary. The friendlier surface (regcomp, regexec,
// regex_find, regex_replace, grep) is built on top of these in
// lib/sli/regexp.sli.

#include "sli_regex_module.h"

#include "sli_array.h"
#include "sli_dictionary.h"
#include "sli_function.h"
#include "sli_interpreter.h"
#include "sli_name.h"
#include "sli_regextype.h"
#include "sli_string.h"
#include "sli_token.h"
#include "sli_type.h"

#include <regex.h>
#include <string>
#include <vector>

namespace sli3
{
namespace
{

// `string integer regcomp_ -> regex true | regex integer false`.
// Never raises -- matches NEST 2.x's contract that lib/sli/regexp.sli
// depends on (the SLI-level /regcomp wrapper inspects the bool and
// either keeps the regex or raises /InvalidRegexError itself).
class RegcompFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(2);
        i->require_stack_type(1, sli3::stringtype);
        i->require_stack_type(0, sli3::integertype);

        std::string const& pat = i->pick(1).data_.string_val->str();
        int flags = static_cast<int>(i->pick(0).data_.long_val);

        Regex* r = new Regex;  // refs_=1
        int e = r->compile(pat, flags);
        i->pop(2);

        Token tok(i->get_type(sli3::regextype));
        tok.data_.regex_val = r;
        i->push(tok);  // stack copy bumps refs to 2

        if (e == 0)
        {
            i->push<bool>(true);
        }
        else
        {
            i->push<long>(static_cast<long>(e));
            i->push<bool>(false);
        }
        // `tok` destructor drops refs back to 1 (owner = stack).
    }
};

// `regex string size eflags regexec_ -> [array] integer`.
//   size > 0: pushes an array of size sub-arrays [so eo]; status code follows.
//   size == 0: only the status code is pushed.
// On no-match the array slots have rm_so == rm_eo == -1, matching POSIX.
class RegexecFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(4);
        i->require_stack_type(3, sli3::regextype);
        i->require_stack_type(2, sli3::stringtype);
        i->require_stack_type(1, sli3::integertype);
        i->require_stack_type(0, sli3::integertype);

        Regex* r = i->pick(3).data_.regex_val;
        if (r == 0 || !r->compiled())
        {
            // NEST 2.x asserts here; we'd rather surface a real error.
            // The SLI /regcomp wrapper makes this unreachable for
            // well-formed user code -- only a hand-crafted Token
            // (e.g. via a corrupt restorestate) can hit it.
            i->raiseerror(Name("regexec"), Name("InvalidRegexError"));
            return;
        }

        std::string const& subject = i->pick(2).data_.string_val->str();
        long size_l   = i->pick(1).data_.long_val;
        long eflags_l = i->pick(0).data_.long_val;
        if (size_l < 0) size_l = 0;
        std::size_t size = static_cast<std::size_t>(size_l);

        std::vector<regmatch_t> pm(size);
        int e = regexec(r->posix(), subject.c_str(),
                        size, size > 0 ? pm.data() : nullptr,
                        static_cast<int>(eflags_l));

        i->pop(4);

        if (size > 0)
        {
            TokenArray* out = new TokenArray();  // refs_=1
            out->reserve(size);
            for (std::size_t k = 0; k < size; ++k)
            {
                TokenArray* pair = new TokenArray();  // refs_=1
                pair->reserve(2);
                pair->push_back(i->new_token<sli3::integertype, long>(
                    static_cast<long>(pm[k].rm_so)));
                pair->push_back(i->new_token<sli3::integertype, long>(
                    static_cast<long>(pm[k].rm_eo)));
                out->push_back(i->new_token<sli3::arraytype, TokenArray*>(pair));
            }
            i->push(i->new_token<sli3::arraytype, TokenArray*>(out));
        }
        i->push<long>(static_cast<long>(e));
    }
};

// `regex integer regerror_ -> string`. Decode a POSIX error code
// (returned by regcomp_) to a human-readable message via POSIX
// regerror. Never raises.
class RegerrorFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(2);
        i->require_stack_type(1, sli3::regextype);
        i->require_stack_type(0, sli3::integertype);

        Regex* r = i->pick(1).data_.regex_val;
        int code = static_cast<int>(i->pick(0).data_.long_val);

        // POSIX regerror takes a const regex_t* and tolerates a null
        // (or uncompiled) source for diagnostic codes that don't
        // depend on the compiled state -- but most implementations
        // dereference it, so pass r_ unconditionally. If compile
        // never succeeded the regex_t is undefined; we still pass it
        // through (matching NEST 2.x's assert-only guard).
        char buf[256];
        regerror(code, r ? r->posix() : nullptr, buf, sizeof buf);

        i->pop(2);
        i->push(i->new_token<sli3::stringtype, std::string>(std::string(buf)));
    }
};

RegcompFunction  regcomp_fn;
RegexecFunction  regexec_fn;
RegerrorFunction regerror_fn;

void def_int(SLIInterpreter* i, Dictionary* d, char const* name, int value)
{
    d->insert(Name(name), i->new_token<sli3::integertype, long>(
        static_cast<long>(value)));
}

}  // anonymous namespace

void init_regex_module(SLIInterpreter* i)
{
    Dictionary* d = new Dictionary;
    def_int(i, d, "REG_EXTENDED", REG_EXTENDED);
    def_int(i, d, "REG_ICASE",    REG_ICASE);
    def_int(i, d, "REG_NOSUB",    REG_NOSUB);
    def_int(i, d, "REG_NEWLINE",  REG_NEWLINE);
    def_int(i, d, "REG_NOTBOL",   REG_NOTBOL);
    def_int(i, d, "REG_NOTEOL",   REG_NOTEOL);
    def_int(i, d, "REG_ESPACE",   REG_ESPACE);
    def_int(i, d, "REG_BADPAT",   REG_BADPAT);
    def_int(i, d, "REG_ECOLLATE", REG_ECOLLATE);
    def_int(i, d, "REG_ECTYPE",   REG_ECTYPE);
    def_int(i, d, "REG_EESCAPE",  REG_EESCAPE);
    def_int(i, d, "REG_ESUBREG",  REG_ESUBREG);
    def_int(i, d, "REG_EBRACK",   REG_EBRACK);
    def_int(i, d, "REG_EPAREN",   REG_EPAREN);
    def_int(i, d, "REG_EBRACE",   REG_EBRACE);
    def_int(i, d, "REG_BADBR",    REG_BADBR);
    def_int(i, d, "REG_ERANGE",   REG_ERANGE);
    def_int(i, d, "REG_BADRPT",   REG_BADRPT);
    // regexdict lives in systemdict (same scope as the other dicts
    // exposed at boot). init_regex_module runs before sli-init.sli
    // seals systemdict.
    i->def(Name("regexdict"),
           i->new_token<sli3::dictionarytype, Dictionary*>(d));

    // Bind the literal name /regextype to the typeid integer, so
    // typetrie specs like `[/regextype /integertype] /regerror_ load
    // addtotrie` can resolve. init_dictionaries' name-binding loop
    // only covers typeids below symboltype; types past that point
    // (regextype is appended at the tail) need explicit binding.
    i->def(Name("regextype"),
           i->new_token<sli3::integertype, long>(
               static_cast<long>(sli3::regextype)));

    i->createcommand("regcomp_",  &regcomp_fn);
    i->createcommand("regexec_",  &regexec_fn);
    i->createcommand("regerror_", &regerror_fn);
}

}  // namespace sli3
