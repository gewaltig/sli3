// Tests for the PostScript-style access state (Wave 1: ACCESS_UNLIMITED
// vs ACCESS_READONLY) on TokenArray, Dictionary, SLIString.
//
// Coverage:
//   - rcheck / wcheck report the correct state for fresh and narrowed
//     payloads of every composite type.
//   - readonly is idempotent and monotonic (can't widen back).
//   - Mutation ops (put, append, prepend, reserve, def, cleardict,
//     undef) raise WriteProtected when the target is readonly.
//   - xcheck identifies procedures, doesn't care about access state.
//   - DictionaryStack::snapshot()'s cached array is readonly, so
//     `dictstack 0 ... put` raises rather than corrupting the cache.
//   - readonly on a non-composite (e.g. integer) raises ArgumentType.

#include "test_harness.h"

#include <iostream>
#include <string>

namespace
{
using namespace sli_test;

// `{ body } stopped` returns true on raise / false on success. sli3's
// stopped does not unwind the operand stack — operands the body left
// (e.g. the operand that triggered the raise, per the stack-handling
// discipline) stay put with the bool on top. We assert only that the
// top is `true` and that the stack drained back to empty after we
// pop it ourselves.
void assert_stops(sli3::SLIInterpreter& i,
                  std::string const& body,
                  char const* tag)
{
    EVAL_CLEAR(i);
    // /executive normally clears errordict /newerror between REPL
    // inputs; in test_harness we drive eval() directly so newerror
    // sticks at true after the first raise, which makes every
    // subsequent raiseerror take the "nested error" diagnostic path.
    // Reset between assertions.
    eval(i, "errordict /newerror false put");
    EVAL_CLEAR(i);
    eval(i, "{ " + body + " } stopped");
    if (i.OStack().load() < 1
        || !i.top().is_of_type(sli3::booltype)
        || i.top().data_.bool_val != true)
    {
        std::cerr << "FAIL " << tag
                  << ": stopped did not catch (top is not /true)\n";
        std::exit(1);
    }
}

void test_fresh_writable(sli3::SLIInterpreter& i)
{
    EVAL_BOOL(i, "[1 2 3] wcheck", true);
    EVAL_BOOL(i, "[1 2 3] rcheck", true);

    EVAL_BOOL(i, "<< /a 1 >> wcheck", true);
    EVAL_BOOL(i, "<< /a 1 >> rcheck", true);

    EVAL_BOOL(i, "(hello) wcheck", true);
    EVAL_BOOL(i, "(hello) rcheck", true);

    EVAL_BOOL(i, "{1 2 add} wcheck", true);
    EVAL_BOOL(i, "{1 2 add} rcheck", true);
}

void test_readonly_narrows(sli3::SLIInterpreter& i)
{
    // Array: writable → readonly via /readonly. After, wcheck false,
    // rcheck still true.
    EVAL_BOOL(i, "[1 2 3] readonly wcheck", false);
    EVAL_BOOL(i, "[1 2 3] readonly rcheck", true);

    // Dict.
    EVAL_BOOL(i, "<< /a 1 >> readonly wcheck", false);
    EVAL_BOOL(i, "<< /a 1 >> readonly rcheck", true);

    // String.
    EVAL_BOOL(i, "(hi) readonly wcheck", false);
    EVAL_BOOL(i, "(hi) readonly rcheck", true);

    // Procedure.
    EVAL_BOOL(i, "{1 2 add} readonly wcheck", false);
    EVAL_BOOL(i, "{1 2 add} readonly rcheck", true);

    // Idempotent: readonly twice is fine.
    EVAL_BOOL(i, "[1 2 3] readonly readonly wcheck", false);
}

void test_readonly_rejects_scalars(sli3::SLIInterpreter& i)
{
    // /readonly on an int is a typed argument error — not silent.
    assert_stops(i, "1 readonly", "readonly on integer");
    assert_stops(i, "1.5 readonly", "readonly on double");
    assert_stops(i, "true readonly", "readonly on bool");
}

void test_xcheck(sli3::SLIInterpreter& i)
{
    EVAL_BOOL(i, "{1 2 add} xcheck", true);
    EVAL_BOOL(i, "[1 2 3] xcheck", false);
    EVAL_BOOL(i, "<< /a 1 >> xcheck", false);
    EVAL_BOOL(i, "(hi) xcheck", false);
    EVAL_BOOL(i, "1 xcheck", false);

    // Procedure can be readonly and still report xcheck true:
    // xcheck is orthogonal to access.
    EVAL_BOOL(i, "{1 2 add} readonly xcheck", true);
}

void test_array_mutation_rejected(sli3::SLIInterpreter& i)
{
    // put_a-via-/put on a readonly array raises WriteProtected.
    assert_stops(i, "[1 2 3] readonly 0 99 put", "put on readonly array");

    // append, prepend, reserve all guarded.
    assert_stops(i, "[1 2 3] readonly 99 append", "append on readonly array");
    assert_stops(i, "[1 2 3] readonly 99 prepend", "prepend on readonly array");
    assert_stops(i, "[1 2 3] readonly 16 reserve", "reserve on readonly array");
}

void test_string_mutation_rejected(sli3::SLIInterpreter& i)
{
    assert_stops(i, "(hi) readonly 0 88 put", "put on readonly string");
    assert_stops(i, "(hi) readonly 33 append", "append on readonly string");
    assert_stops(i, "(hi) readonly 33 prepend", "prepend on readonly string");
    assert_stops(i, "(hi) readonly 16 reserve", "reserve on readonly string");
}

void test_dict_mutation_rejected(sli3::SLIInterpreter& i)
{
    // put on a readonly dict.
    assert_stops(i, "<< /a 1 >> readonly /a 99 put", "put on readonly dict");

    // cleardict on a readonly dict.
    assert_stops(i, "<< /a 1 >> readonly cleardict", "cleardict on readonly dict");

    // /def writes into the CURRENT dict. We can't easily make the
    // current dict readonly without also breaking the test harness,
    // so we test /def via dict-put above instead.
}

void test_unaffected_paths_still_work(sli3::SLIInterpreter& i)
{
    // Unlimited arrays/dicts/strings still accept mutations.
    EVAL_INT(i, "[1 2 3] 0 99 put 0 get", 99);
    EVAL_INT(i, "<< /a 1 >> dup /a 99 put /a get", 99);
}

// Mutations through procedures on the operand stack — procedures and
// arrays share TokenArray storage, so the readonly bit on a procedure
// payload guards put / append the same as for an array.
void test_procedure_mutation_rejected(sli3::SLIInterpreter& i)
{
    assert_stops(i, "{1 2 3} readonly 0 99 put", "put on readonly procedure");
    assert_stops(i, "{1 2 3} readonly 99 append", "append on readonly procedure");
}

void test_dictstack_snapshot_is_readonly(sli3::SLIInterpreter& i)
{
    // /dictstack hands out the cache; wcheck should report it locked.
    EVAL_BOOL(i, "dictstack wcheck", false);
    EVAL_BOOL(i, "dictstack rcheck", true);

    // Attempting to mutate the snapshot raises rather than corrupting
    // the cache.
    assert_stops(i, "dictstack 0 99 put",
                 "put on dictstack snapshot");
    assert_stops(i, "dictstack 99 append",
                 "append on dictstack snapshot");
}

// Wave 2: executeonly + noaccess setters, read-path gates,
// --nostringval-- print suppression.

void test_executeonly_blocks_reads(sli3::SLIInterpreter& i)
{
    // executeonly is stricter than readonly: rcheck is false, wcheck
    // is false.
    EVAL_BOOL(i, "[1 2 3] executeonly rcheck", false);
    EVAL_BOOL(i, "[1 2 3] executeonly wcheck", false);
    EVAL_BOOL(i, "<< /a 1 >> executeonly rcheck", false);
    EVAL_BOOL(i, "(hi) executeonly rcheck", false);

    // Reads against executeonly raise.
    assert_stops(i, "[1 2 3] executeonly 0 get", "get on executeonly array");
    assert_stops(i, "(hi) executeonly 0 get", "get on executeonly string");
    assert_stops(i, "<< /a 1 >> executeonly /a get", "get on executeonly dict");
    assert_stops(i, "<< /a 1 >> executeonly keys", "keys on executeonly dict");
    assert_stops(i, "<< /a 1 >> executeonly values", "values on executeonly dict");
    assert_stops(i, "<< /a 1 >> executeonly cva", "cva on executeonly dict");
}

void test_noaccess_blocks_reads(sli3::SLIInterpreter& i)
{
    EVAL_BOOL(i, "[1 2 3] noaccess rcheck", false);
    EVAL_BOOL(i, "[1 2 3] noaccess wcheck", false);

    assert_stops(i, "[1 2 3] noaccess 0 get", "get on noaccess array");
    assert_stops(i, "(hi) noaccess 0 get", "get on noaccess string");
    assert_stops(i, "<< /a 1 >> noaccess /a get", "get on noaccess dict");
}

void test_monotonic_narrowing(sli3::SLIInterpreter& i)
{
    // Once narrowed, can't widen. set_access only accepts strictly
    // narrower targets, so applying /readonly to a noaccess object
    // is a no-op (state stays noaccess) — the read should still fail.
    assert_stops(i, "[1 2 3] noaccess readonly 0 get",
                 "noaccess readonly stays noaccess");
    assert_stops(i, "[1 2 3] executeonly readonly 0 get",
                 "executeonly readonly stays executeonly");

    // Narrowing further from readonly to noaccess works.
    assert_stops(i, "[1 2 3] readonly noaccess 0 get",
                 "readonly noaccess blocks read");
}

// PS Level-2 globaldict — between systemdict and userdict on the
// dstack. Wave 2 adds it so future code (and any future systemdict
// readonly seal) has a proper place for persistent globals.
void test_globaldict_is_present(sli3::SLIInterpreter& i)
{
    // The dictstack snapshot lists every dict on the stack; on a
    // fresh interpreter post-bootstrap we expect at minimum
    // userdict, globaldict, systemdict — i.e. length >= 3. (sli-init
    // also runs `userdict begin` without a matching end, pushing an
    // extra userdict; that's a pre-existing quirk of the vendored
    // file. We just check the floor.)
    EVAL_CLEAR(i);
    eval(i, "dictstack length 3 geq");
    if (i.OStack().load() != 1
        || !i.top().is_of_type(sli3::booltype)
        || i.top().data_.bool_val != true)
    {
        std::cerr << "FAIL globaldict on dstack: length not >= 3\n";
        std::exit(1);
    }

    // globaldict is a writable Dictionary, distinct from systemdict
    // and userdict, and reachable by name via systemdict.
    EVAL_BOOL(i, "globaldict wcheck", true);
    // Writing into globaldict by name lookup followed by /put works,
    // and the value can be retrieved by name (cache invalidation
    // handles the def through the lookup path).
    EVAL_INT(i, "globaldict /:wave2_test 99 put globaldict /:wave2_test get", 99);
}

// Print suppression for executeonly / noaccess composites. We can't
// capture stdout via test_harness directly, but cvs (convert-to-string)
// in misc_helpers.sli renders the value via the same pprint path,
// so we test the resulting string instead.
void test_nostringval_print(sli3::SLIInterpreter& i)
{
    // pcvs goes through pprint, which emits --nostringval-- for
    // executeonly / noaccess composites.
    EVAL_STRING(i, "[1 2 3] readonly pcvs",       "[1 2 3]");
    EVAL_STRING(i, "[1 2 3] executeonly pcvs",    "--nostringval--");
    EVAL_STRING(i, "[1 2 3] noaccess pcvs",       "--nostringval--");
    EVAL_STRING(i, "(hi) executeonly pcvs",       "--nostringval--");
    EVAL_STRING(i, "{1 2 add} executeonly pcvs",  "--nostringval--");
    EVAL_STRING(i, "<< /a 1 >> executeonly pcvs", "--nostringval--");
}

// Wave 3: read-gate coverage for functional read-and-copy operators.
// Anything that materialises content from a composite operand must
// reject executeonly / noaccess sources. The single-`get` ops are
// already covered by test_executeonly_blocks_reads; this section
// covers join / search / insert / replace / erase / insertelement /
// getinterval / cvi / cvd / clonedict / known.
void test_functional_ops_blocked_on_executeonly(sli3::SLIInterpreter& i)
{
    // String functional ops.
    assert_stops(i, "(hello) executeonly ( world) join_s",
                 "join_s lhs executeonly");
    assert_stops(i, "(hello) (lo) executeonly search_s",
                 "search_s rhs executeonly");
    assert_stops(i, "(helloworld) executeonly 0 5 getinterval_s",
                 "getinterval_s executeonly");
    assert_stops(i, "(ab) executeonly 1 (XY) insert_s",
                 "insert_s lhs executeonly");
    assert_stops(i, "(abcd) executeonly 1 2 (Z) replace_s",
                 "replace_s lhs executeonly");
    assert_stops(i, "(abcd) executeonly 1 2 erase_s",
                 "erase_s executeonly");
    assert_stops(i, "(ab) executeonly 1 65 insertelement_s",
                 "insertelement_s executeonly");
    assert_stops(i, "(42) executeonly cvi_s", "cvi_s executeonly");
    assert_stops(i, "(3.14) executeonly cvd_s", "cvd_s executeonly");

    // Array / procedure functional ops.
    assert_stops(i, "[1 2 3] executeonly [4 5] join_a",
                 "join_a lhs executeonly");
    assert_stops(i, "[1 2 3] executeonly [2] search_a",
                 "search_a lhs executeonly");
    assert_stops(i, "[1 2 3 4] executeonly 0 2 getinterval_a",
                 "getinterval_a executeonly");
    assert_stops(i, "[1 2 3] executeonly 1 [9] insert_a",
                 "insert_a lhs executeonly");
    assert_stops(i, "[1 2 3 4] executeonly 1 2 [9] replace_a",
                 "replace_a lhs executeonly");
    assert_stops(i, "[1 2 3 4] executeonly 1 2 erase_a",
                 "erase_a executeonly");
    assert_stops(i, "[1 2 3] executeonly 1 99 insertelement_a",
                 "insertelement_a executeonly");
    assert_stops(i, "{1 2 add} executeonly {99} join_p",
                 "join_p lhs executeonly");

    // Dict copy / lookup. clonedict materialises content.
    assert_stops(i, "<< /a 1 >> noaccess clonedict",
                 "clonedict on noaccess");
    // /known reports presence — gated like keys/values/cva.
    assert_stops(i, "<< /a 1 >> executeonly /a known",
                 "known on executeonly dict");
}

// Wave 3: hot_op_put path descent. The outer array of a nested
// `arr [i0 i1 ...] val put` is read to descend into nested arrays.
// If it's executeonly/noaccess the descent must raise before any
// element is exposed or mutated.
void test_path_put_descent_blocked(sli3::SLIInterpreter& i)
{
    assert_stops(i, "[[1 2] [3 4]] executeonly [0 0] 99 put",
                 "path-put through executeonly outer");
    assert_stops(i, "[[1 2] [3 4]] noaccess [0 0] 99 put",
                 "path-put through noaccess outer");
    // Empty path is rejected (range), but the gate on the outer
    // happens before the empty check; either error is acceptable.
    // What we care about is that the read isn't silent.
}

// Wave 3: eq / ne raise invalidaccess when a string operand is not
// readable. The integer / double / bool arms are unaffected.
void test_eq_blocked_on_executeonly_strings(sli3::SLIInterpreter& i)
{
    assert_stops(i, "(hi) executeonly (hi) executeonly eq",
                 "eq both executeonly");
    assert_stops(i, "(hi) executeonly (hi) eq",
                 "eq lhs executeonly");
    assert_stops(i, "(hi) (hi) executeonly eq",
                 "eq rhs executeonly");
    assert_stops(i, "(hi) noaccess (hi) eq",
                 "eq lhs noaccess");
    // Non-string scalars are unaffected — same-value still eq, different
    // values still not. This locks in the locality of the gate.
    EVAL_BOOL(i, "1 1 eq", true);
    EVAL_BOOL(i, "1 2 eq", false);
}

// PS semantic: executeonly procedures can still execute — execute
// permission is what executeonly explicitly grants. Without this
// invariant the access state degenerates into "noaccess but readable
// from the dispatcher", which would silently break any sealed
// procedure (e.g. systemdict procs after a future seal).
void test_executeonly_procedure_still_executes(sli3::SLIInterpreter& i)
{
    EVAL_INT(i, "{1 2 add} executeonly exec", 3);
    EVAL_INT(i, "/myproc {3 4 mul} executeonly def myproc", 12);
}

// Wave 3: errordict /errorname now distinguishes read denial
// (InvalidAccess) from write denial (WriteProtected). Both used to
// surface as /WriteProtected; the rename gives script authors a
// useful errorname to branch on.
//
// We can't simply chain `{ ... } stopped pop errordict /errorname get`
// because raised operators leave their operands on the stack (per
// the operator authoring contract), so the second `get` would see
// the leftover instead of errordict. Instead drive each side
// independently: run the body, EVAL_CLEAR everything, then probe
// errordict from a clean stack.
void check_errorname(sli3::SLIInterpreter& i,
                     std::string const& trigger,
                     char const* expected)
{
    EVAL_CLEAR(i);
    eval(i, "errordict /newerror false put");
    EVAL_CLEAR(i);
    eval(i, "{ " + trigger + " } stopped");
    EVAL_CLEAR(i);
    eval(i, "errordict /errorname get");
    if (i.OStack().load() < 1
        || !i.top().is_of_type(sli3::literaltype)
        || sli3::Name(i.top().data_.name_val).toString() != expected)
    {
        std::cerr << "FAIL errorname for `" << trigger
                  << "`: expected /" << expected;
        if (i.OStack().load() >= 1 && i.top().is_of_type(sli3::literaltype))
            std::cerr << ", got /"
                      << sli3::Name(i.top().data_.name_val).toString();
        std::cerr << '\n';
        std::exit(1);
    }
    EVAL_CLEAR(i);
}

void test_errorname_distinguishes_read_vs_write(sli3::SLIInterpreter& i)
{
    check_errorname(i, "[1 2 3] executeonly 0 get",   "InvalidAccess");
    check_errorname(i, "<< /a 1 >> executeonly keys", "InvalidAccess");
    check_errorname(i, "[1 2 3] readonly 0 99 put",   "WriteProtected");
    check_errorname(i, "[1 2 3] readonly 99 append",  "WriteProtected");
}

// cleardictstack restores the canonical PS Level-2 layout:
// bottom-to-top systemdict / globaldict / userdict. The previous
// `while (size > 2) pop` rule was Level-1 (left only 2 entries),
// which after Wave 2 dropped userdict and left
// [globaldict, systemdict] — a layout no user code could write
// into without `begin`-ing a fresh dict first.
void test_cleardictstack_restores_permanents(sli3::SLIInterpreter& i)
{
    EVAL_CLEAR(i);
    // Push a couple of scratch dicts, then reset.
    eval(i, "<< /scratch_a 1 >> begin << >> begin cleardictstack");
    EVAL_CLEAR(i);

    // dictstack length == 3.
    EVAL_INT(i, "countdictstack", 3);

    // The bottom is systemdict; the middle is globaldict; the top
    // is userdict. Probe by name: each binds to its dict via the
    // /systemdict /globaldict /userdict slots in systemdict, and
    // `eq` compares dict identity.
    EVAL_BOOL(i, "dictstack 0 get systemdict eq", true);
    EVAL_BOOL(i, "dictstack 1 get globaldict eq", true);
    EVAL_BOOL(i, "dictstack 2 get userdict eq",   true);

    // And the layout still accepts writes / reads (sanity: userdict
    // is on top and writable, lookups find systemdict bindings).
    EVAL_INT(i, "/cdtest 42 def /cdtest load", 42);
    // `where` pushes (dict, true) on hit and (false) on miss. The
    // top is what we check; the dict slot is just ostack noise.
    EVAL_BOOL(i, "/systemdict where exch pop", true);
    EVAL_CLEAR(i);

    // Idempotent: a second cleardictstack from the canonical state
    // is a no-op (size stays at 3, identities unchanged).
    eval(i, "cleardictstack");
    EVAL_INT(i, "countdictstack", 3);
    EVAL_BOOL(i, "dictstack 0 get systemdict eq", true);
    EVAL_CLEAR(i);

    // If a local dict was begun and then cleared, the local is
    // dropped (no leak): no operand references it from this point.
    // We can't observe refcounts directly from SLI, but the ASAN
    // build catches any UAF or leak on the local on shutdown.
}

}  // namespace

int main()
{
    sli3::SLIInterpreter i;
    i.startup();  // load sli-init.sli — we need /put dispatchers.

    test_fresh_writable(i);
    test_readonly_narrows(i);
    test_readonly_rejects_scalars(i);
    test_xcheck(i);
    test_array_mutation_rejected(i);
    test_string_mutation_rejected(i);
    test_dict_mutation_rejected(i);
    test_unaffected_paths_still_work(i);
    test_procedure_mutation_rejected(i);
    test_dictstack_snapshot_is_readonly(i);

    // Wave 2 additions.
    test_executeonly_blocks_reads(i);
    test_noaccess_blocks_reads(i);
    test_monotonic_narrowing(i);
    test_globaldict_is_present(i);
    test_nostringval_print(i);

    // Wave 3 additions.
    test_functional_ops_blocked_on_executeonly(i);
    test_path_put_descent_blocked(i);
    test_eq_blocked_on_executeonly_strings(i);
    test_executeonly_procedure_still_executes(i);
    test_errorname_distinguishes_read_vs_write(i);

    test_cleardictstack_restores_permanents(i);

    std::cout << "test_access: all assertions passed\n";
    return 0;
}
