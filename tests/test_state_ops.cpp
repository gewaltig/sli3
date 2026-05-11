// Tests for the savestate / restorestate operators and the
// supporting serialize helpers (FunctionType::serialize,
// write_token_stack, read_token_stack).
//
// The tests use the C++ entry points (BinaryWriter / Reader +
// the stack helpers) directly. The SLI surface drives the
// operator registration check, but cross-eval round-trips are
// out of scope here because the eval test harness drives parsing
// via a live xistream on the e-stack — and streams are documented
// as non-serializable (IstreamType::serialize warns and writes
// no payload; the load produces a closed-stream placeholder).
// Running the loaded e-stack would crash on the closed stream.
//
// In practice savestate / restorestate are taken at quiescent
// points (between commands, in a debugger checkpoint) where the
// e-stack does not contain transient parser frames. The tests
// here reflect that: we construct quiescent states manually.

#include "test_harness.h"
#include "sli_array.h"
#include "sli_function.h"
#include "sli_interpreter.h"
#include "sli_name.h"
#include "sli_serialize.h"
#include "sli_token.h"
#include "sli_tokenstack.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace
{

const char* TMPFILE = "/tmp/sli3_test_state_ops.bin";

void cleanup_tmpfile() { std::remove(TMPFILE); }

[[noreturn]] void fail_at(int line, const std::string& msg)
{
    std::cerr << "FAIL @ " << line << ": " << msg << "\n";
    cleanup_tmpfile();
    std::exit(1);
}

#define REQUIRE(cond)                                                    \
    do { if (!(cond)) fail_at(__LINE__, #cond); } while (0)


// ---------------------------------------------------------------------
// Test 1 — write_token_stack / read_token_stack round-trip on a
// fresh operand stack populated by C++.
// ---------------------------------------------------------------------
void test_ostack_helpers_roundtrip()
{
    cleanup_tmpfile();
    sli3::SLIInterpreter src;
    sli3::SLIInterpreter dst;

    src.OStack().push(src.new_token<sli3::integertype>(42L));
    src.OStack().push(src.new_token<sli3::doubletype>(3.14));
    src.OStack().push(src.new_token<sli3::literaltype, sli3::Name>(
                          sli3::Name("foo")));
    REQUIRE(src.OStack().load() == 3);

    {
        std::ofstream out(TMPFILE, std::ios::binary);
        sli3::BinaryWriter w(out);
        w.write_header();
        write_token_stack(src.OStack(), w);
    }

    REQUIRE(dst.OStack().load() == 0);
    {
        std::ifstream in(TMPFILE, std::ios::binary);
        sli3::BinaryReader r(in);
        r.read_header();
        read_token_stack(dst.OStack(), r, dst);
    }

    REQUIRE(dst.OStack().load() == 3);
    REQUIRE(dst.OStack().pick(2).is_of_type(sli3::integertype));
    REQUIRE(dst.OStack().pick(2).data_.long_val == 42);
    REQUIRE(dst.OStack().pick(1).is_of_type(sli3::doubletype));
    REQUIRE(dst.OStack().pick(1).data_.double_val == 3.14);
    REQUIRE(dst.OStack().pick(0).is_of_type(sli3::literaltype));
    REQUIRE(sli3::Name(dst.OStack().pick(0).data_.name_val).toString()
            == "foo");
    cleanup_tmpfile();
}


// ---------------------------------------------------------------------
// Test 2 — FunctionType serialize round-trip.
//
// Verifies the new FunctionType::serialize / deserialize: a
// functiontype Token serializes by name; the loading interpreter
// resolves the same operator instance via dictionary lookup. Same
// operator object in both interpreters means built-in registration
// is consistent across SLIInterpreter constructions.
// ---------------------------------------------------------------------
void test_function_type_roundtrip()
{
    sli3::SLIInterpreter interp;
    sli3::Token pop_token;
    REQUIRE(interp.lookup(sli3::Name("pop"), pop_token));
    REQUIRE(pop_token.is_of_type(sli3::functiontype));
    sli3::SLIFunction* orig_fn = pop_token.data_.func_val;
    REQUIRE(orig_fn != nullptr);

    std::stringstream buf;
    sli3::BinaryWriter w(buf);
    w.write_header();
    write_token(pop_token, w);

    sli3::BinaryReader r(buf);
    r.read_header();
    sli3::Token restored = read_token(r, interp);
    REQUIRE(restored.is_of_type(sli3::functiontype));
    // Same SLIFunction* on both sides — the serialize-by-name
    // round-trip resolves back to the canonical operator instance.
    REQUIRE(restored.data_.func_val == orig_fn);
    // The function's stored name may be any of its aliases
    // (createcommand calls set_name(); the last alias wins).
    // PopFunction is bound under both "pop" and ";"; either is
    // acceptable here. The semantic identity is the SLIFunction*
    // pointer checked above.
}


// ---------------------------------------------------------------------
// Test 3 — FunctionType across two interpreters: same SLIFunction*
// because operator instances are statically allocated.
// ---------------------------------------------------------------------
void test_function_type_cross_interpreter()
{
    sli3::SLIInterpreter src;
    sli3::SLIInterpreter dst;

    sli3::Token src_pop;
    REQUIRE(src.lookup(sli3::Name("pop"), src_pop));
    sli3::Token dst_pop;
    REQUIRE(dst.lookup(sli3::Name("pop"), dst_pop));
    REQUIRE(src_pop.data_.func_val == dst_pop.data_.func_val);

    std::stringstream buf;
    sli3::BinaryWriter w(buf);
    w.write_header();
    write_token(src_pop, w);

    sli3::BinaryReader r(buf);
    r.read_header();
    sli3::Token restored = read_token(r, dst);
    REQUIRE(restored.is_of_type(sli3::functiontype));
    REQUIRE(restored.data_.func_val == dst_pop.data_.func_val);
}


// ---------------------------------------------------------------------
// Test 4 — bound-procedure body round-trips.
//
// Construct a TokenArray (procedure body) containing a literal
// followed by a functiontype Token. Serialize via write_token.
// Read back; verify the array contents survived, including the
// resolved function pointer.
// ---------------------------------------------------------------------
void test_proc_body_with_function_token()
{
    sli3::SLIInterpreter interp;

    sli3::Token add_token;
    REQUIRE(interp.lookup(sli3::Name("add_ii"), add_token));
    REQUIRE(add_token.is_of_type(sli3::functiontype));

    // Build a procedure body: { 1 2 add_ii }
    sli3::Token proc_token = interp.new_token<sli3::proceduretype,
                                              sli3::TokenArray*>(
        new sli3::TokenArray());
    sli3::TokenArray* body = proc_token.data_.array_val;
    body->push_back(interp.new_token<sli3::integertype>(1L));
    body->push_back(interp.new_token<sli3::integertype>(2L));
    body->push_back(add_token);

    std::stringstream buf;
    sli3::BinaryWriter w(buf);
    w.write_header();
    write_token(proc_token, w);

    sli3::BinaryReader r(buf);
    r.read_header();
    sli3::Token restored = read_token(r, interp);
    REQUIRE(restored.is_of_type(sli3::proceduretype));
    sli3::TokenArray* rbody = restored.data_.array_val;
    REQUIRE(rbody != nullptr);
    REQUIRE(rbody->size() == 3);
    REQUIRE((*rbody)[0].is_of_type(sli3::integertype));
    REQUIRE((*rbody)[0].data_.long_val == 1);
    REQUIRE((*rbody)[1].is_of_type(sli3::integertype));
    REQUIRE((*rbody)[1].data_.long_val == 2);
    REQUIRE((*rbody)[2].is_of_type(sli3::functiontype));
    REQUIRE((*rbody)[2].data_.func_val == add_token.data_.func_val);
}


// ---------------------------------------------------------------------
// Test 5 — operators are registered and callable.
//
// Just look them up via the dictionary stack. End-to-end SLI
// invocation requires either a manually-constructed quiescent
// state (the eval harness pushes a stream frame that conflicts
// with restore) or a future test using a clean checkpoint API.
// ---------------------------------------------------------------------
void test_operators_registered()
{
    sli3::SLIInterpreter interp;
    sli3::Token save_tok;
    REQUIRE(interp.lookup(sli3::Name("savestate"), save_tok));
    REQUIRE(save_tok.is_of_type(sli3::functiontype));
    REQUIRE(save_tok.data_.func_val != nullptr);
    REQUIRE(save_tok.data_.func_val->get_name() == sli3::Name("savestate"));

    sli3::Token restore_tok;
    REQUIRE(interp.lookup(sli3::Name("restorestate"), restore_tok));
    REQUIRE(restore_tok.is_of_type(sli3::functiontype));
    REQUIRE(restore_tok.data_.func_val != nullptr);
    REQUIRE(restore_tok.data_.func_val->get_name() == sli3::Name("restorestate"));
}


// ---------------------------------------------------------------------
// Test 6 — savestate via SLI eval writes a snapshot file.
//
// Only checks the WRITE half. The snapshot's e-stack contains the
// eval harness's parser frame; restoring it would replay a closed
// stream and crash (see file-header comment). We verify the file
// is created and the operator chain executes cleanly.
// ---------------------------------------------------------------------
void test_sli_savestate_writes_file()
{
    cleanup_tmpfile();
    sli3::SLIInterpreter i;
    sli_test::eval(i, std::string("1 2 3 (") + TMPFILE + ") savestate");
    // After savestate returns, ostack still has 1 2 3 (savestate
    // doesn't touch operand-stack contents below itself).
    REQUIRE(i.OStack().load() == 3);

    std::ifstream in(TMPFILE, std::ios::binary);
    REQUIRE(in.good());

    // Header is 8 bytes (magic + version); the smallest valid
    // snapshot should be larger than that.
    in.seekg(0, std::ios::end);
    std::streampos size = in.tellg();
    REQUIRE(size > 8);
    cleanup_tmpfile();
}

}  // namespace

int main()
{
    test_ostack_helpers_roundtrip();
    test_function_type_roundtrip();
    test_function_type_cross_interpreter();
    test_proc_body_with_function_token();
    test_operators_registered();
    test_sli_savestate_writes_file();
    std::cout << "test_state_ops: OK\n";
    return 0;
}
