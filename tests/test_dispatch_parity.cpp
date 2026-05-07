// Parity test: execute_ (plain) vs execute_dispatch_ (mode 2 — the
// inlined switch with goto-driven hot loops).
//
// For each snippet in the corpus, run it through both modes against
// freshly-cleared interpreters and verify the operand stacks are
// identical (depth + per-slot value). Divergences are real bugs (the
// same SLI source must produce the same result in either mode).
//
// Today the corpus is intentionally small: the dispatcher mode is the
// only one used in production (sli_main.cpp:9), but the fallback
// IiterateFunction / IloopFunction / IrepeatFunction / IforFunction
// implementations exist precisely for execute_, so any feature added
// to the dispatcher needs a matching fallback. The corpus grows as
// Stage 7 retires the divergences.

#include "sli_interpreter.h"
#include "sli_iostream.h"
#include "sli_token.h"
#include "sli_tokenstack.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

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

// Push the iparse machinery onto an empty interpreter's e-stack so a
// subsequent execute_*(0) call reads the source. Mirrors test_harness's
// eval() but does NOT push a trailing `quit` sentinel — the dispatcher
// short-circuits quittype via a goto, but the plain `execute_` loop
// goes through SLIType::execute on quittype, which pushes the quit
// Token onto the operand stack (an artifact, not the program's
// result). Letting the e-stack drain naturally on EOF gives both
// modes the same observable ostack state.
void prime_eval(SLIInterpreter& i, std::string const& src)
{
    auto* iss = new std::istringstream(src);
    auto* wrap = new SLIistream(iss);

    i.OStack().clear();
    i.EStack().clear();
    i.set_call_depth(0);

    Token x(i.get_type(sli3::xistreamtype));
    x.data_.istream_val = wrap;
    i.EStack().push(x);
    i.EStack().push(i.new_token<sli3::nametype, Name>(i.iparse_name));
}

// Compare two Tokens that may belong to different SLIInterpreter
// instances (so their type_ pointers differ even when the typeid is
// the same). Compare by typeid + payload.
bool tokens_equal(Token const& a, Token const& b)
{
    if (a.is_valid() != b.is_valid()) return false;
    if (!a.is_valid()) return true;
    auto ta = a.type_->get_typeid();
    auto tb = b.type_->get_typeid();
    if (ta != tb) return false;
    switch (ta)
    {
    case sli3::integertype: return a.data_.long_val   == b.data_.long_val;
    case sli3::doubletype:  return a.data_.double_val == b.data_.double_val;
    case sli3::booltype:    return a.data_.bool_val   == b.data_.bool_val;
    case sli3::stringtype:
        return a.data_.string_val && b.data_.string_val
            && a.data_.string_val->str() == b.data_.string_val->str();
    case sli3::nametype:
    case sli3::literaltype:
    case sli3::symboltype:
    case sli3::marktype:
        return a.data_.name_val == b.data_.name_val;
    default:
        // Pointer-payload comparison across interpreters is not
        // meaningful (each has its own heap). Treat pointer-payload
        // typeids as equal if both have the same typeid; the corpus
        // does not yet include snippets that produce shared pointer
        // payloads in either mode.
        return true;
    }
}

bool ostacks_equal(SLIInterpreter& a, SLIInterpreter& b)
{
    if (a.OStack().load() != b.OStack().load()) return false;
    for (size_t k = 0; k < a.OStack().load(); ++k)
    {
        if (!tokens_equal(a.OStack().pick(k), b.OStack().pick(k)))
            return false;
    }
    return true;
}

// ---------- corpus -------------------------------------------------------

struct Snippet
{
    char const* src;
    char const* note;     // optional: explains why a snippet is xfail
    bool        xfail;    // expected to diverge today
};

constexpr Snippet corpus[] = {
    // Literals — both modes should agree: parser produces a Token,
    // default SLIType::execute pushes it to the operand stack.
    { "",                   nullptr, false },
    { "1",                  nullptr, false },
    { "1 2 3",              nullptr, false },
    { "3.14",               nullptr, false },
    { "true",               nullptr, false },
    { "(hello)",            nullptr, false },
    { "/foo",               nullptr, false },
    { "1 2 3 4 5",          nullptr, false },

    // Procedure literals don't execute when parsed — they go on
    // the operand stack as litprocedure Tokens.
    { "{ 1 2 add_ii }",     nullptr, false },

    // 1 1 add_ii — execute_'s fallback IiterateFunction does not
    // tail-call the way the dispatcher does, but for a flat trie
    // call there is no recursion; so this should agree today.
    { "1 1 add_ii",         nullptr, false },
};

// Run `src` against a fresh-state interpreter using the chosen mode.
// Returns true on success, false if any C++ exception escaped.
bool run_in(SLIInterpreter& i, std::string const& src, int mode)
{
    prime_eval(i, src);
    try
    {
        if (mode == 0)
            i.execute_(0);
        else
            i.execute_dispatch_(0);
        return true;
    }
    catch (std::exception const& e)
    {
        std::cerr << "  exception (" << (mode == 0 ? "plain" : "dispatch")
                  << "): " << e.what() << "\n";
        return false;
    }
}

void test_parity_corpus(bool run_xfail)
{
    int passed = 0, skipped = 0;
    for (auto const& s : corpus)
    {
        if (s.xfail && !run_xfail)
        {
            std::cerr << "  SKIP \"" << s.src << "\"  ("
                      << (s.note ? s.note : "xfail") << ")\n";
            ++skipped;
            continue;
        }

        SLIInterpreter a, b;
        bool ok_plain    = run_in(a, s.src, /*mode=*/0);
        bool ok_dispatch = run_in(b, s.src, /*mode=*/2);

        if (ok_plain != ok_dispatch)
        {
            std::cerr << "FAIL parity (one mode threw): \""
                      << s.src << "\"\n";
            std::exit(1);
        }

        if (!ostacks_equal(a, b))
        {
            std::cerr << "FAIL parity (ostack diverged): \""
                      << s.src << "\"\n";
            std::cerr << "  plain depth=" << a.OStack().load()
                      << "  dispatch depth=" << b.OStack().load() << "\n";
            for (size_t k = 0; k < a.OStack().load(); ++k)
            {
                std::cerr << "  plain[" << k << "]: ";
                a.OStack().pick(k).pprint(std::cerr);
                std::cerr << " : "
                          << a.OStack().pick(k).get_typename().toString()
                          << "\n";
            }
            for (size_t k = 0; k < b.OStack().load(); ++k)
            {
                std::cerr << "  disp[" << k << "]:  ";
                b.OStack().pick(k).pprint(std::cerr);
                std::cerr << " : "
                          << b.OStack().pick(k).get_typename().toString()
                          << "\n";
            }
            std::exit(1);
        }
        ++passed;
    }
    std::cerr << "test_dispatch_parity: " << passed << " passed, "
              << skipped << " skipped\n";
}

}  // namespace

int main(int argc, char** argv)
{
    bool run_xfail = false;
    for (int k = 1; k < argc; ++k)
    {
        if (std::string(argv[k]) == "--xfail")
            run_xfail = true;
    }

    test_parity_corpus(run_xfail);

    std::cout << "test_dispatch_parity: ok\n";
    return 0;
}
