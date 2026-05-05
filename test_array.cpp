// Tests for the rewritten sli3::TokenArray + ArrayType serialization.
//
// Covers: container API parity with the previous hand-rolled impl,
// intrusive refcount, value-copy assignment, and round-trip through the
// binary serializer including shared-pointer de-duplication for cyclic
// or aliased arrays.

#include "sli_array.h"
#include "sli_arraytype.h"
#include "sli_interpreter.h"
#include "sli_name.h"
#include "sli_serialize.h"
#include "sli_token.h"
#include "sli_type.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <sstream>

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

void test_basic_container(SLIInterpreter& interp)
{
    auto* a = new TokenArray();
    CHECK(a->size() == 0);
    CHECK(a->empty());
    CHECK(a->references() == 1);

    a->push_back(interp.new_token<sli3::integertype>(1L));
    a->push_back(interp.new_token<sli3::integertype>(2L));
    a->push_back(interp.new_token<sli3::integertype>(3L));

    CHECK(a->size() == 3);
    CHECK(!a->empty());
    CHECK(a->index_is_valid(0));
    CHECK(a->index_is_valid(2));
    CHECK(!a->index_is_valid(3));
    CHECK((*a)[0].data_.long_val == 1);
    CHECK((*a)[2].data_.long_val == 3);
    CHECK(a->get(1L).data_.long_val == 2);

    // Iterator semantics — Token* range works
    long sum = 0;
    for (Token* t = a->begin(); t != a->end(); ++t)
        sum += t->data_.long_val;
    CHECK(sum == 6);

    a->pop_back();
    CHECK(a->size() == 2);

    a->clear();
    CHECK(a->empty());

    CHECK(a->remove_reference() == 0);  // self-deletes
}

void test_refcount(SLIInterpreter& interp)
{
    auto* a = new TokenArray();
    CHECK(a->references() == 1);
    CHECK(a->add_reference() == 2);
    CHECK(a->add_reference() == 3);
    CHECK(a->references() == 3);
    CHECK(a->remove_reference() == 2);
    CHECK(a->remove_reference() == 1);
    CHECK(a->remove_reference() == 0);  // self-deletes; pointer dead

    // Token-level refcount through ArrayType
    TokenArray* shared = new TokenArray();
    shared->push_back(interp.new_token<sli3::integertype>(42L));

    Token t1(interp.get_type(sli3::arraytype));
    t1.data_.array_val = shared;
    CHECK(shared->references() == 1);  // Token ctor doesn't add_reference

    // Copy a Token: should bump refcount via ArrayType::add_reference
    Token t2 = t1;
    CHECK(shared->references() == 2);

    {
        Token t3 = t1;
        CHECK(shared->references() == 3);
    }
    CHECK(shared->references() == 2);

    t1.clear();
    CHECK(shared->references() == 1);
    t2.clear();
    // shared self-deleted; can't dereference further
    (void)interp;
}

void test_copy_and_assign(SLIInterpreter& interp)
{
    TokenArray a;
    a.push_back(interp.new_token<sli3::integertype>(7L));
    a.push_back(interp.new_token<sli3::integertype>(8L));

    TokenArray b(a);  // copy ctor: deep copy, fresh refs=1
    CHECK(b.references() == 1);
    CHECK(b.size() == 2);
    CHECK(b[0].data_.long_val == 7);

    // Assignment: replace content, keep identity (refs unchanged)
    TokenArray c;
    c.add_reference();  // simulate external reference
    c.add_reference();
    CHECK(c.references() == 3);
    c = a;
    CHECK(c.references() == 3);  // identity preserved
    CHECK(c.size() == 2);
    CHECK(c[1].data_.long_val == 8);

    // Self-assign no-op
    a = a;
    CHECK(a.size() == 2);
}

void test_erase_reduce_insert(SLIInterpreter& interp)
{
    TokenArray a;
    for (long i = 0; i < 5; ++i)
        a.push_back(interp.new_token<sli3::integertype>(i));
    CHECK(a.size() == 5);

    a.erase(1, 2);  // remove [1,3): leaves [0, 3, 4]
    CHECK(a.size() == 3);
    CHECK(a[0].data_.long_val == 0);
    CHECK(a[1].data_.long_val == 3);
    CHECK(a[2].data_.long_val == 4);

    // erase past end is clamped, not UB
    a.erase(2, 100);
    CHECK(a.size() == 2);

    // reduce: keep [first,last)
    TokenArray b;
    for (long i = 0; i < 5; ++i)
        b.push_back(interp.new_token<sli3::integertype>(i * 10));
    b.reduce(static_cast<size_t>(1), static_cast<size_t>(3));
    CHECK(b.size() == 3);
    CHECK(b[0].data_.long_val == 10);
    CHECK(b[2].data_.long_val == 30);

    // insert(i, n, t)
    TokenArray c;
    c.push_back(interp.new_token<sli3::integertype>(1L));
    c.push_back(interp.new_token<sli3::integertype>(4L));
    c.insert(1, 2, interp.new_token<sli3::integertype>(2L));
    CHECK(c.size() == 4);
    CHECK(c[0].data_.long_val == 1);
    CHECK(c[1].data_.long_val == 2);
    CHECK(c[2].data_.long_val == 2);
    CHECK(c[3].data_.long_val == 4);

    // append
    TokenArray d;
    d.push_back(interp.new_token<sli3::integertype>(99L));
    c.append(d);
    CHECK(c.size() == 5);
    CHECK(c[4].data_.long_val == 99);
}

void test_rotate(SLIInterpreter& interp)
{
    TokenArray a;
    for (long i = 0; i < 5; ++i)
        a.push_back(interp.new_token<sli3::integertype>(i));
    // rotate so element 2 becomes the first
    a.rotate(a.begin(), a.begin() + 2, a.end());
    CHECK(a[0].data_.long_val == 2);
    CHECK(a[1].data_.long_val == 3);
    CHECK(a[2].data_.long_val == 4);
    CHECK(a[3].data_.long_val == 0);
    CHECK(a[4].data_.long_val == 1);
}

void test_serialize_array(SLIInterpreter& interp)
{
    // Build an array with mixed scalar tokens.
    TokenArray* original = new TokenArray();
    original->push_back(interp.new_token<sli3::integertype>(123L));
    original->push_back(interp.new_token<sli3::doubletype>(2.5));
    original->push_back(interp.new_token<sli3::booltype>(true));
    original->push_back(interp.new_token<sli3::nametype, Name>(Name("hello")));

    Token in(interp.get_type(sli3::arraytype));
    in.data_.array_val = original;

    // Serialize
    std::stringstream buf;
    BinaryWriter w(buf);
    w.write_header();
    write_token(in, w);

    // Deserialize
    BinaryReader r(buf);
    r.read_header();
    Token out = read_token(r, interp);

    CHECK(out.is_valid());
    CHECK(out.type_->get_typeid() == sli3::arraytype);
    TokenArray* loaded = out.data_.array_val;
    CHECK(loaded != nullptr);
    CHECK(loaded != original);  // distinct heap object
    CHECK(loaded->size() == 4);
    CHECK((*loaded)[0].data_.long_val == 123);
    CHECK((*loaded)[1].data_.double_val == 2.5);
    CHECK((*loaded)[2].data_.bool_val == true);
    // Name re-interns: handle index may differ but the string round-trips.
    {
        Name n((*loaded)[3].data_.name_val);
        CHECK(n.toString() == "hello");
    }

    in.clear();   // releases original (refs goes 1->0, self-delete)
    out.clear();  // releases loaded (refs goes 1->0, self-delete)
}

void test_serialize_aliased(SLIInterpreter& interp)
{
    // Outer array contains two Tokens both pointing at the SAME inner.
    // The Writer's object table must dedup them; Reader rebuilds aliasing.
    TokenArray* inner = new TokenArray();
    inner->push_back(interp.new_token<sli3::integertype>(7L));
    CHECK(inner->references() == 1);

    Token inner_tok(interp.get_type(sli3::arraytype));
    inner_tok.data_.array_val = inner;
    inner->add_reference();  // for the second alias below
    CHECK(inner->references() == 2);

    TokenArray* outer = new TokenArray();
    outer->push_back(inner_tok);          // copy bumps to 3
    outer->push_back(inner_tok);          // copy bumps to 4
    CHECK(inner->references() == 4);

    Token outer_tok(interp.get_type(sli3::arraytype));
    outer_tok.data_.array_val = outer;

    std::stringstream buf;
    BinaryWriter w(buf);
    w.write_header();
    write_token(outer_tok, w);

    BinaryReader r(buf);
    r.read_header();
    Token loaded_tok = read_token(r, interp);

    CHECK(loaded_tok.is_valid());
    TokenArray* loaded_outer = loaded_tok.data_.array_val;
    CHECK(loaded_outer->size() == 2);
    TokenArray* loaded_inner_a = (*loaded_outer)[0].data_.array_val;
    TokenArray* loaded_inner_b = (*loaded_outer)[1].data_.array_val;
    CHECK(loaded_inner_a == loaded_inner_b);  // aliasing preserved
    CHECK(loaded_inner_a->size() == 1);
    CHECK((*loaded_inner_a)[0].data_.long_val == 7);
    CHECK(loaded_inner_a->references() == 2);  // outer holds 2 Token refs

    inner_tok.clear();
    outer_tok.clear();
    loaded_tok.clear();
}

}  // namespace

int main()
{
    SLIInterpreter interp;

    test_basic_container(interp);
    test_refcount(interp);
    test_copy_and_assign(interp);
    test_erase_reduce_insert(interp);
    test_rotate(interp);
    test_serialize_array(interp);
    test_serialize_aliased(interp);

    std::cerr << "test_array: all checks passed\n";
    return 0;
}
