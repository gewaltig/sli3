// Exhaustive Token round-trip test driven by sli_typeid.
//
// For every value of sli_typeid, build a representative Token, write
// to BinaryWriter, read with BinaryReader, and assert identity. Most
// typeids work today; the ones whose SLIType subclass lacks a
// serialize/deserialize override (Dictionary, FunctionType,
// ProcedureType, TrieType per code-review.md) are marked XFAIL
// pending Stage 6.
//
// Also covers cycle / aliasing / wire-format error paths that
// test_serialize.cpp does not exercise yet.

#include "sli_array.h"
#include "sli_dictionary.h"
#include "sli_interpreter.h"
#include "sli_name.h"
#include "sli_serialize.h"
#include "sli_string.h"
#include "sli_token.h"
#include "sli_type.h"

#include <cassert>
#include <cstdint>
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

// ---------- helpers ------------------------------------------------------

Token roundtrip(SLIInterpreter& i, Token const& in)
{
    std::stringstream buf;
    BinaryWriter w(buf);
    w.write_header();
    write_token(in, w);

    BinaryReader r(buf);
    r.read_header();
    return read_token(r, i);
}

// ---------- typeid coverage table ---------------------------------------
//
// Each entry: typeid + name + a builder that constructs a Token of
// that type for round-trip. Builders that return a default Token
// (type_=nullptr) signal "skip — pending stage X."

struct Case
{
    sli_typeid id;
    char const* name;
    bool xfail;          // expected to fail today (round-trip is not yet correct)
    char const* reason;  // why xfail
};

constexpr Case cases[] = {
    // Working today
    { sli3::integertype,    "integer",       false, nullptr },
    { sli3::doubletype,     "double",        false, nullptr },
    { sli3::booltype,       "bool",          false, nullptr },
    { sli3::stringtype,     "string",        false, nullptr },
    { sli3::arraytype,      "array",         false, nullptr },
    { sli3::nametype,       "name",          false, nullptr },
    { sli3::literaltype,    "literal",       false, nullptr },
    { sli3::symboltype,     "symbol",        false, nullptr },
    { sli3::marktype,       "mark",          false, nullptr },

    // Stream types — designed to load as closed streams (per
    // sli_iostreamtype.cpp). Round-trip assertion needs to check
    // type_ identity, not the stream pointer.
    { sli3::istreamtype,    "istream",       true,
      "stream tokens deserialize as closed-stream singletons (Stage 6 dedup)" },
    { sli3::ostreamtype,    "ostream",       true,
      "stream tokens deserialize as closed-stream singletons (Stage 6 dedup)" },

    // Stage 6 gaps — silent data loss today
    { sli3::dictionarytype, "dictionary",    true,
      "DictionaryType has no serialize override (Stage 6)" },
    { sli3::functiontype,   "function",      true,
      "FunctionType has no serialize override (Stage 6)" },
    { sli3::proceduretype,  "procedure",     true,
      "ProcedureType has no serialize override (Stage 6)" },
    { sli3::litproceduretype,"litprocedure", true,
      "LitprocedureType inherits ArrayType serialize but typeid context is lost (Stage 6)" },
    { sli3::trietype,       "trie",          true,
      "TrieType has no serialize override (Stage 6)" },

    // Operator markers — share LiteralType serialize so the typeid
    // collapses on load. Stage 6 work to preserve.
    { sli3::iiteratetype,   "iiterate",      true,
      "OperatorType<X> markers collapse to literaltype on load (Stage 6)" },
    { sli3::irepeattype,    "irepeat",       true, "(see iiterate)" },
    { sli3::ifortype,       "ifor",          true, "(see iiterate)" },
    { sli3::iforalltype,    "iforall",       true, "(see iiterate)" },
    { sli3::nooptype,       "noop",          true, "(see iiterate)" },
    { sli3::quittype,       "quit",          true, "(see iiterate)" },
};

Token build_token(SLIInterpreter& i, sli_typeid id)
{
    switch (id)
    {
    case sli3::integertype:
        return i.new_token<sli3::integertype>(42L);
    case sli3::doubletype:
        return i.new_token<sli3::doubletype>(3.14);
    case sli3::booltype:
        return i.new_token<sli3::booltype>(true);
    case sli3::stringtype:
    {
        Token t(i.get_type(sli3::stringtype));
        t.data_.string_val = new SLIString("hello");
        return t;
    }
    case sli3::arraytype:
    {
        auto* arr = new TokenArray();
        arr->push_back(i.new_token<sli3::integertype>(1L));
        arr->push_back(i.new_token<sli3::integertype>(2L));
        Token t(i.get_type(sli3::arraytype));
        t.data_.array_val = arr;
        return t;
    }
    case sli3::nametype:
        return i.new_token<sli3::nametype, Name>(Name("foo"));
    case sli3::literaltype:
        return i.new_token<sli3::literaltype, Name>(Name("/bar"));
    case sli3::symboltype:
        return i.new_token<sli3::symboltype, Name>(Name("baz"));
    case sli3::marktype:
        return Token(i.get_type(sli3::marktype));

    // The rest are placeholders for the xfail cases — we still need
    // to build a Token to attempt the round-trip. Most can be
    // constructed via the registered SLIType pointer.
    default:
    {
        SLIType* t = i.get_type(id);
        if (!t) return Token();  // unregistered — skip
        return Token(t);
    }
    }
}

// ---------- the actual tests --------------------------------------------

void test_typeid_coverage(SLIInterpreter& i, bool run_xfail)
{
    int passed = 0, skipped = 0;
    for (auto const& c : cases)
    {
        if (c.xfail && !run_xfail)
        {
            std::cerr << "  SKIP " << c.name
                      << "  (" << (c.reason ? c.reason : "") << ")\n";
            ++skipped;
            continue;
        }
        Token in = build_token(i, c.id);
        if (in.type_ == nullptr)
        {
            std::cerr << "  SKIP " << c.name << "  (no builder / unregistered)\n";
            ++skipped;
            continue;
        }
        try
        {
            Token out = roundtrip(i, in);
            CHECK(out.type_ != nullptr);
            CHECK(out.type_->get_typeid() == c.id);
            // For pointer-payload types the equality check is
            // pointer-identity-based (per *Type::compare), which
            // would fail for a fresh-allocated round-trip. We
            // check the typeid here; per-payload structural equality
            // is left to the type-specific tests.
            ++passed;
        }
        catch (std::exception const& e)
        {
            if (c.xfail)
            {
                std::cerr << "  XFAIL " << c.name << ": " << e.what() << "\n";
                ++skipped;
            }
            else
            {
                std::cerr << "FAIL roundtrip " << c.name << ": " << e.what() << "\n";
                std::exit(1);
            }
        }
    }
    std::cerr << "test_round_trip: " << passed << " passed, "
              << skipped << " skipped\n";
}

// ---------- aliasing ----------------------------------------------------
//
// Two Tokens both holding the same SLIString must round-trip as two
// Tokens both holding the SAME deserialized SLIString. test_serialize.cpp
// already covers this for strings inside an array; replicate for
// directly-aliased Tokens at the top level.

void test_alias_string(SLIInterpreter& i)
{
    auto* s = new SLIString("shared");
    Token a(i.get_type(sli3::stringtype));
    a.data_.string_val = s;
    s->add_reference();              // a now holds 1 ref; s = 2
    Token b(i.get_type(sli3::stringtype));
    b.data_.string_val = s;          // b also holds; s = 2

    // Build an outer array containing a and b so a single round-trip
    // sees both.
    auto* arr = new TokenArray();
    arr->push_back(a);  // ref +1 = 3
    arr->push_back(b);  // ref +1 = 4
    Token outer(i.get_type(sli3::arraytype));
    outer.data_.array_val = arr;

    Token loaded = roundtrip(i, outer);
    CHECK(loaded.is_valid());
    CHECK(loaded.type_->get_typeid() == sli3::arraytype);

    auto* la = loaded.data_.array_val;
    CHECK(la->size() == 2);
    SLIString* ls0 = (*la)[0].data_.string_val;
    SLIString* ls1 = (*la)[1].data_.string_val;
    CHECK(ls0 != nullptr);
    CHECK(ls0 == ls1);                // shared identity preserved
    CHECK(ls0->str() == "shared");
}

// ---------- wire-format errors ------------------------------------------

void test_bad_magic()
{
    std::stringstream buf;
    buf.write("XXXX\x01\0\0\0", 8);
    SLIInterpreter i;
    BinaryReader r(buf);
    bool threw = false;
    try { r.read_header(); }
    catch (std::exception const&) { threw = true; }
    CHECK(threw);
}

void test_bad_version()
{
    std::stringstream buf;
    BinaryWriter w(buf);
    w.write_u32(kSerializeMagic);
    w.write_u32(kSerializeVersion + 1);
    SLIInterpreter i;
    BinaryReader r(buf);
    bool threw = false;
    try { r.read_header(); }
    catch (std::exception const&) { threw = true; }
    CHECK(threw);
}

void test_truncated_stream()
{
    // Header only, no token body. write_token would throw on the
    // first read attempt. Use SLIInterpreter to drive read_token.
    std::stringstream buf;
    BinaryWriter w(buf);
    w.write_header();
    SLIInterpreter i;
    BinaryReader r(buf);
    r.read_header();
    bool threw = false;
    try { (void) read_token(r, i); }
    catch (std::exception const&) { threw = true; }
    CHECK(threw);
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

    SLIInterpreter i;

    test_typeid_coverage(i, run_xfail);
    test_alias_string(i);
    test_bad_magic();
    test_bad_version();
    test_truncated_stream();

    std::cout << "test_round_trip: ok\n";
    return 0;
}
