// Round-trip test for sli3 serialization protocol.
//
// Validates that scalar Tokens (int, double, bool, name, literal, symbol)
// survive a write -> read cycle through BinaryWriter/BinaryReader.
//
// Bare assertions; no test framework (per Q8). Run as part of the
// sli3 build target by hand: ./build/sli3 -- not yet wired into CTest.

#include "sli_serialize.h"
#include "sli_interpreter.h"
#include "sli_token.h"
#include "sli_type.h"
#include "sli_name.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <sstream>

using namespace sli3;

namespace
{

struct Fail
{
    Fail(const char* msg, int line)
    {
        std::cerr << "FAIL @ " << line << ": " << msg << "\n";
        std::exit(1);
    }
};

#define CHECK(cond)                                                    \
    do { if (!(cond)) Fail(#cond, __LINE__); } while (0)

void roundtrip_scalar(SLIInterpreter& interp, Token const& in)
{
    std::stringstream buf;
    BinaryWriter w(buf);
    w.write_header();
    write_token(in, w);

    BinaryReader r(buf);
    r.read_header();
    Token out = read_token(r, interp);

    CHECK(out == in);
    CHECK(out.type_ == in.type_);
}

}  // namespace

int main()
{
    SLIInterpreter interp;

    // Integer
    roundtrip_scalar(interp, interp.new_token<sli3::integertype>(42L));
    roundtrip_scalar(interp, interp.new_token<sli3::integertype>(-1L));
    roundtrip_scalar(interp, interp.new_token<sli3::integertype>(
                                 static_cast<long>(INT64_MIN)));

    // Double — incl. NaN-stable bit-exact via memcpy in BinaryWriter
    roundtrip_scalar(interp, interp.new_token<sli3::doubletype>(3.14159));
    roundtrip_scalar(interp, interp.new_token<sli3::doubletype>(0.0));
    roundtrip_scalar(interp, interp.new_token<sli3::doubletype>(-1e-300));

    // Bool
    roundtrip_scalar(interp, interp.new_token<sli3::booltype>(true));
    roundtrip_scalar(interp, interp.new_token<sli3::booltype>(false));

    // Name / Literal / Symbol — all share LiteralType serialize via base
    roundtrip_scalar(interp,
        interp.new_token<sli3::nametype, Name>(Name("foo")));
    roundtrip_scalar(interp,
        interp.new_token<sli3::literaltype, Name>(Name("/bar")));
    roundtrip_scalar(interp,
        interp.new_token<sli3::symboltype, Name>(Name("baz")));

    // Empty / null Token
    {
        std::stringstream buf;
        BinaryWriter w(buf);
        w.write_header();
        Token in;  // default-constructed: type_ == 0
        write_token(in, w);
        BinaryReader r(buf);
        r.read_header();
        Token out = read_token(r, interp);
        CHECK(!out.is_valid());
    }

    // Bad magic should throw
    {
        std::stringstream buf;
        buf.write("XXXX\x01\0\0\0", 8);
        BinaryReader r(buf);
        bool threw = false;
        try { r.read_header(); } catch (std::exception&) { threw = true; }
        CHECK(threw);
    }

    std::cerr << "test_serialize: all checks passed\n";
    return 0;
}
