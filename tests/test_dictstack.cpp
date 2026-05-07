// DictionaryStack invariants.
//
// Stage 2 scope:
//   2.3: default-constructed DictionaryStack must not segfault on
//        baselookup(); base_ now defaults to nullptr.
//   2.4: destroyed/cleared stacks must remove_reference each held
//        dictionary, paired with the add_reference push() does.
//   2.5: undef must invalidate the cached Token* (currently leaves
//        a dangling pointer that the next lookup reads as freed memory).
//   2.6: undef removes the binding from the TOP dictionary only,
//        per PostScript spec (current code walks the entire stack).

#include "sli_dictionary.h"
#include "sli_dictstack.h"
#include "sli_exceptions.h"
#include "sli_interpreter.h"
#include "sli_token.h"

#include <cstdlib>
#include <iostream>
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

// Build a Token wrapping a fresh Dictionary (refs=1, owned by this token).
Token wrap_dict(SLIInterpreter& i, Dictionary* d)
{
    Token t(i.get_type(sli3::dictionarytype));
    t.data_.dict_val = d;
    return t;
}

// ---------- Stage 2.3: base_ uninit guard -------------------------------

void test_baselookup_unset_xfail(SLIInterpreter& interp)
{
    DictionaryStack ds;
    // No set_basedict() call -> base_ is uninitialised. Today:
    // segfault. After Stage 2.3: throw a clean error.
    bool threw = false;
    try { (void) ds.baselookup(Name("x")); }
    catch (std::exception const&) { threw = true; }
    CHECK(threw);
}

// ---------- Stage 2.4: refcount on destruction --------------------------
//
// Refcount accounting note. wrap_dict() builds a Token without
// calling add_reference -- it is the "anointed" first owner of the
// dict's initial refs=1. Passing through push()'s by-value parameter
// destroys that Token at end-of-push (under C++17 mandatory copy
// elision the parameter IS the wrap_dict return value), which
// returns the anointed ref. Inside push(), an explicit
// add_reference() bumps the count for the stack's own bookkeeping.
// Net: after push(), the dict has refs equal to whatever the caller
// established before pushing. To observe the stack's contribution
// independently, the caller adds an extra ref up-front.

void test_destroy_balances_refs(SLIInterpreter& interp)
{
    auto* dA = new Dictionary();   // refs=1 (anointed, will be claimed/returned)
    auto* dB = new Dictionary();
    dA->add_reference();           // refs=2 (extra observer ref)
    dB->add_reference();

    {
        DictionaryStack ds;
        ds.push(wrap_dict(interp, dA));
        ds.push(wrap_dict(interp, dB));
        // anointed-1 returned, stack added +1 -> net unchanged at 2.
        // Visually: 1 (observer) + 1 (stack).
        CHECK(dA->references() == 2);
        CHECK(dB->references() == 2);
    }
    // ds destroyed. Stage 2.4: each dict's refcount drops by 1
    // (the stack's ref). The observer ref keeps each dict alive.
    CHECK(dA->references() == 1);
    CHECK(dB->references() == 1);

    dA->remove_reference();   // self-deletes
    dB->remove_reference();
}

void test_clear_balances_refs(SLIInterpreter& interp)
{
    auto* d = new Dictionary();
    d->add_reference();              // refs=2 (observer)

    DictionaryStack ds;
    ds.push(wrap_dict(interp, d));
    CHECK(d->references() == 2);

    ds.clear();
    CHECK(d->references() == 1);

    d->remove_reference();
}

// ---------- Stage 2.5: undef invalidates the cache ----------------------

void test_undef_invalidates_cache(SLIInterpreter& interp)
{
    auto* d = new Dictionary();
    DictionaryStack ds;
    ds.push(wrap_dict(interp, d));

    // Define -> lookup (populates cache).
    ds.def(Name("k"), interp.new_token<sli3::integertype>(42L));
    Token result;
    CHECK(ds.lookup(Name("k"), result));
    CHECK(result.data_.long_val == 42);

    // Undef. Today: the cache still holds a Token* into the now-erased
    // map node; the next lookup reads freed memory. ASAN catches it.
    ds.undef(Name("k"));
    Token after;
    bool found = ds.lookup(Name("k"), after);
    CHECK(!found);

    // Cleanup: pop releases the dict.
    ds.pop();
    // d's only ref was on the stack; now self-deleted.
}

// ---------- Stage 2.6: undef erases only from top dict ------------------

void test_undef_top_only(SLIInterpreter& interp)
{
    auto* lower = new Dictionary();
    auto* upper = new Dictionary();
    DictionaryStack ds;
    ds.push(wrap_dict(interp, lower));
    ds.push(wrap_dict(interp, upper));   // upper is top

    // Bind /k in BOTH dicts to distinguishable values.
    lower->insert(Name("k"), interp.new_token<sli3::integertype>(1L));
    upper->insert(Name("k"), interp.new_token<sli3::integertype>(2L));

    Token before;
    CHECK(ds.lookup(Name("k"), before));
    CHECK(before.data_.long_val == 2);   // top wins

    // After Stage 2.6: undef removes from upper only.
    ds.undef(Name("k"));

    Token after;
    CHECK(ds.lookup(Name("k"), after));
    CHECK(after.data_.long_val == 1);    // lower's binding survives

    ds.pop();
    ds.pop();
}

}  // namespace

int main()
{
    SLIInterpreter interp;

    test_baselookup_unset_xfail(interp);
    test_destroy_balances_refs(interp);
    test_clear_balances_refs(interp);
    test_undef_invalidates_cache(interp);
    test_undef_top_only(interp);

    std::cout << "test_dictstack: ok\n";
    return 0;
}
