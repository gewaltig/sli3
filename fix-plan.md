# sli3 staged fix plan

A test-driven plan to retire the issues in `code-review.md`. Eight stages; each stage starts by writing failing tests, then makes them pass, then locks them in.

The order is deliberately bottom-up: type system → containers → error machinery → operators → serialization → dispatcher parity → hardening. Earlier stages establish invariants that later stages depend on, so a regression caught at stage N is almost always coming from stage N's own scope.

Throughout: **tests before fixes**. CLAUDE.md's `TDD-first when bootstrap blocks` rule applies — write a failing assertion first, then implement.

---

## Stage 0 — test infrastructure

**Goal:** make subsequent stages testable. Today's harness can only reach the dispatcher in `execute_dispatch_`, has no parity coverage, and leaks per snippet.

**Tests to add (these themselves act as the acceptance criteria):**

1. `tests/test_token.cpp`
   - Default-construct, copy-construct, copy-assign, move-construct, move-assign, swap, self-assign — for each of the 4 pointer-payload types (string, array, dict, stream) **at refcount 1** and **at refcount > 1**.
   - Verify allocated objects (`SLIString::allocated`, etc.) return to baseline.
2. `tests/test_round_trip.cpp` — for every value of `sli_typeid` (driven by a table), construct a Token, write to `BinaryWriter`, read with `BinaryReader`, assert `== ` and refcount discipline. Initially most cases will fail; that is the point.
3. `tests/test_dispatch_parity.cpp` — runs a corpus of small SLI snippets through `execute_(0)` and `execute_dispatch_(0)`, asserts the o-stack ends identical. Empty corpus initially; fill as bootstrap progresses.
4. `tests/test_two_interpreters.cpp` — constructs a `SLIInterpreter`, runs `1 1 add` (or whatever Slice 5b/5c reaches), destroys it, constructs another, runs the same thing. Today this crashes on the first cached `static SLIType*`. Stage 7 makes it pass.
5. Update `tests/test_harness.h`:
   - `eval()` (line 67) currently leaks `std::istringstream*` + `SLIistream*` per snippet. Wrap in RAII or run all snippets through a single reusable stream.
   - `clear_stacks()` should also clear the dictionary stack down to system+user, reset `error_dict_`, and zero `call_depth_`. Otherwise tests have order dependence.
6. Add a CTest `TIMEOUT` (e.g. 60 s) per test target — a hung dispatcher today hangs CI indefinitely.
7. Add a Debug build with `-fsanitize=address,undefined` enabled. Leak / UB findings from this run will surface during stages 1-6 even before that stage's tests are written.
8. Move `target_compile_options(sli3_core PRIVATE -Wall -Wextra ...)` to `INTERFACE` (or duplicate on each test target) so test files inherit warnings.

**Acceptance:** all four new test binaries compile and run; most assertions are `EXPECT_FAIL`-style stubs that will go green over the next stages. ASAN run finishes; leaks are catalogued.

---

## Stage 1 — memory & type-system foundation

Everything below depends on `Token` semantics and the `SLIType` virtual protocol being correct.

**Tests to add (most should already fail from Stage 0):**

- `Token` self-assign at refcount 1: `Token a(...); a = a;` survives without UAF.
- Null-payload safety: a Token tagged as `arraytype`/`dicttype`/`stringtype` with `data_.array_val == nullptr` (etc.) does not crash on copy, destruction, comparison, or print.
- `NameType::execute` reads the correct union member: build a Token with a non-trivial `name_val`, push to e-stack, run dispatcher, observe correct lookup.
- `ProcedureType::print` outputs `{...}`, not `{{...}}`.
- All `SLIType` subclasses use `override` (compile-only check via a deliberately wrong-signature override that should fail to compile).

**Fixes (in order):**

1. `src/types/sli_token.h:222-226` — guard `Token::operator=(const Token&)` with `if (&t == this) return *this;` before `clear()`.
2. `src/types/sli_arraytype.h:18-30`, `sli_dicttype.h:23-43`, `sli_stringtype.cpp:7-22` — add `if (t.data_.array_val == nullptr) return 0;` (or equivalent) to all `add_reference` / `remove_reference` / `references` / `compare` / `print` paths. Pick **one** convention for "no payload" (e.g. always return `0`) and apply uniformly; document in `sli_type.h`.
3. `src/types/sli_nametype.cpp:12` — change `t.data_.long_val` to `t.data_.name_val`. (This is the same shape as the bug CLAUDE.md already records for `Token::operator==(bool)`.)
4. `src/types/sli_arraytype.cpp:51-69` — give `ProcedureType` its own `print` override (`{ ... }`); stop inheriting from `LitprocedureType`. While here, remove the `EStack().dump(std::cerr)` debug print at line 73.
5. `src/types/sli_arraytype.cpp:62-67` — `LitprocedureType::execute` mutates `t.type_` then pushes; replace with a copy-then-mutate pattern so the EStack-resident Token is not modified in place.
6. `src/types/sli_token.h:269-273` — make the `operator long&()`/`double&()`/`bool&()` overloads `private` or remove them; provide `as_long()` / `as_double()` / `as_bool()` that return by value.
7. `src/types/sli_token.cpp:25-29` — add null check before dereferencing `data_.string_val`.
8. Add `override` keyword to every overridden virtual on `IntegerType`, `DoubleType`, `BoolType`, `StringType`, `ArrayType`, `LitprocedureType`, `ProcedureType`, `LiteralType`, `NameType`, `SymbolType`, `MarkType`, `OperatorType<>`, `DictionaryType`, `FunctionType`, `IstreamType`, `XIstreamType`, `OstreamType`, `TrieType`. (~50 lines mechanical.)
9. `src/types/sli_token.cpp:47-101` — make `operator==(Token)` cross-type-equal for the type-economical aliases (nametype/literaltype/symboltype share Name; proceduretype/litproceduretype/arraytype share array_val). Add tests for both directions of asymmetry that previously existed.

**Acceptance:** `tests/test_token.cpp`, the relevant subset of `tests/test_round_trip.cpp` for value types, and the ASAN run all green. The `clear()` hierarchy across types is documented in a comment on `SLIType::clear`.

---

## Stage 2 — container correctness

`TokenStack`/`Dictionary`/`DictionaryStack`/`Name` carry too much UB-on-edge today. The dispatcher reads from `TokenStack` thousands of times per second; the bounds story has to be tight.

**Tests to add:**

- `tests/test_tokenstack.cpp`:
  - `size()` returns the number of pushed elements (currently returns `capacity()`). Check NEST whether this is correct.
  - `pop()`/`top()`/`pick(i)`/`swap()` on empty/short stacks raise (debug build) or are documented UB (release).
  - `roll(0, k)` does not divide by zero.
  - `index(0)` of a stack at capacity does not UAF.
- `tests/test_dictstack.cpp`:
  - Default-constructed `DictionaryStack` then `baselookup(...)` raises (does not segfault on uninitialised `base_`).
  - Push 5 dicts, copy the stack, destroy the original; `Dictionary::allocated` baseline preserved (refcounts paired).
  - Push, `def(/k 1)`, `lookup(/k)`, `undef(/k)`, `lookup(/k)` returns false **and** the cache no longer holds a Token pointer at that handle.
  - `pop`/`top` on empty stack raise.
- `tests/test_dictionary.cpp`:
  - `info()` output is sorted.
  - `add_dict`/`remove_dict` either work or are removed from the header (no dead declarations).
  - `lookup(Name, Token&)` propagates the access flag (currently returns by copy and the flag is lost).
- `tests/test_name.cpp`:
  - `Name(LONG_MAX).toString()` raises rather than indexing past the table.
  - Concurrent insert is not in scope (single-threaded), but exception safety on `table.push_back` is.

**Fixes:**

1. `src/containers/sli_tokenstack.h:113` — change `size()` to return `data_.size()`. Audit every caller; many places likely call `size()` meaning depth (so the change is forward-compatible). Rename the capacity-returning helper to `capacity()` if anyone needs it.
2. Add `assert(load() > 0)` (or stronger; per discussion) to `pop()`/`top()`/`pick`/`swap`.
3. `src/containers/sli_tokenstack.h:101-111` — `roll(n, k)` early-return when `n == 0`.
4. `src/containers/sli_dictstack.h:64` and `sli_dictstack.cpp:24-26` — initialise `base_ = nullptr` in the member-init list. `baselookup`/`basedef` raise `DictError("BasedictNotSet")` on null.
5. `sli_dictstack.cpp:28-31, 204-214` — copy ctor and `operator=` walk `d`, call `add_reference` on each dict, copy `base_`, copy `cache_`, copy `basecache_`.
6. `sli_dictstack.cpp:33-39, 101-107` — destructor and `clear()` walk `d` and `remove_reference()` each dict (paired with `push`'s `add_reference`).
7. Decide: define `DICTSTACK_CACHE` (compile-time on) and make invalidation paths active **always**, or remove the macro and the cache code entirely. Right now the cache is populated unconditionally and invalidated only behind the `#ifdef`, guaranteeing dangling `Token*` after every `undef`. Pick one. (Recommendation: define it on; the cache is load-bearing for repeated lookups in a tight inner loop.)
8. `sli_dictstack.cpp:41-56` — `undef(Name)` should erase from the **top** dict only, per PostScript spec, not walk the entire stack. Update the header doc to match.
9. `src/containers/sli_dictionary.cpp:51-81` — fix `info()` to actually print the sorted vector it builds.
10. `sli_dictionary.cpp:83-129` — either uncomment `add_dict`/`remove_dict` and make them work, or remove the declarations from the header.
11. `sli_dictionary.h:165-167` — make the bool-returning `lookup(Name, Token&)` overload propagate the access flag (or delete it; callers can use the reference-returning form and check via `Token::is_valid()`).
12. `src/containers/sli_name.h:56-57` — make `Name(long)` throw on out-of-range; or mark it `private` and friend only the parser.
13. `src/containers/sli_iostream.h` — add a real `closed()` flag separate from `stream_ == nullptr`; `valid()` returns `stream_ != nullptr && !closed_`.

**Acceptance:** all `tests/test_*stack*` and `test_dict*` pass. ASAN clean for stage-2 paths. Every `data_.<member>_val` access in the codebase is preceded by a type or null check (audit via `grep`).

---

## Stage 3 — error handling

`raiseerror` is the load-bearing assumption for half the operator surface. Today the contract differs depending on which entry point you take.

**Tests to add:**

- `tests/test_errors.cpp`:
  - Throwing `UndefinedName("/foo")` from C++ surfaces in `errordict /errorname` as the literal `/UndefinedName`, not `/DictError`.
  - Same for every `SLIException` subclass (`EntryTypeMismatch`, `RangeCheck`, `ArgumentTypeError`, `StackUnderflow`, …).
  - `raiseerror(Name)` and `raiseerror(std::exception&)` leave the e-stack in the same state for the same logical error.
  - `raiseerror` is re-entrant once: a throw during error handling sets `errordict /newerror` and unwinds, rather than spinning.
  - After a stop / handleerror cycle, the o-stack and e-stack are at the depths they were when the error was first raised (modulo the documented `/cmd` and `stop` pushes).

**Fixes:**

1. `src/util/sli_exceptions.h:225-227` — `DictError(char const * const errname)` should pass `errname` to the parent, not the literal `"DictError"`. Audit every subclass to make sure the right `errorname` reaches `SLIException::what_`.
2. `src/util/sli_exceptions.h:281` — change `StackUnderflow(int, int)` to `(size_t, size_t)`; fix the call site at `sli_interpreter.h:236`.
3. `src/interpreter/sli_interpreter.cpp:304-308 vs :312-335` — pick **one** convention (recommendation: `raiseerror` does **not** pop the e-stack; the caller pops before throwing). Apply uniformly. Audit every operator that calls `raiseerror`; either the caller pops or `raiseerror` does, but not both.
4. `sli_interpreter.cpp:359-396` — gate the diagnostic dump on a verbosity flag. Production noise on every error today.
5. `sli_interpreter.cpp:451-529` — move the `Token const&` from `error_dict_->lookup(...)` after any potentially-mutating call out of scope; copy the Token to a local first.
6. `sli_interpreter.cpp:497-512` — delete the large dead `// if(command->gettypename()...)` block.
7. `src/util/sli_exceptions.cpp:115` — fix the missing space in the message.

**Acceptance:** `tests/test_errors.cpp` green. Manual trace: every operator that calls `raiseerror` pops correctly per the new convention; cross-checked via `grep "raiseerror" src/builtins/`.

---

## Stage 4 — bootstrap-blocking operators

Goal: reach the Slice 5c smoke test (`1 1 add ==` returns `2`). Then keep going through `sli-init.sli` until it loads cleanly.

This is the stage that maps most directly onto CLAUDE.md's `TDD-first when bootstrap blocks` rule: before implementing each missing operator, write a failing SLI snippet through `tests/test_sli_eval.cpp`, then make it pass.

**Tests to add (one per fix; some already exist in `test_sli_eval.cpp`):**

- `1 1 add ==` → `2` and stack depth back to baseline.
- `[1 2 3] {…} if` raises `ArgumentTypeError` (currently returns false silently).
- `[1 2 3] {…} {…} ifelse` raises (currently returns the false branch).
- `1 { /one } { /two } { /default } case` pushes `/one` and executes it (currently leaves proc on o-stack).
- `/foo /bar def foo` returns `/bar`.
- `:: pop` does not loop (binding fixed to a real `IpopFunction`).
- `or` with one operand raises `StackUnderflow` (currently UB).
- `currentname` returns the executable name of the enclosing definition (currently returns nothing).
- `symbol_s` parses a string into a symbol (currently no-op).
- `cvt_a` builds a populated trie (currently empty).
- All operators currently noted "stalls on the next batch of unregistered ops" in CLAUDE.md (`gt_ss`, various `_ss`, `_iter`, `_a`, `_s`).

**Fixes:**

1. `src/interpreter/sli_interpreter.cpp:236` — write an `IpopFunction` (pops one e-stack frame; any o-stack effect is in the dispatcher) and bind `::pop` to it. Add `tests/test_sli_eval.cpp` snippet that exercises stop-handling.
2. `src/builtins/sli_math.cpp:1541` — register `or` to `or_bbfunction`, not the unsafe anonymous variant. Delete the unsafe variant.
3. `src/builtins/sli_control.cpp:164-174` — `IfFunction` should `require_stack_type(1, booltype)` before reading.
4. `sli_control.cpp:206-209` — `IfelseFunction` should raise on non-bool, not silently take false branch. Both `if` and `ifelse` should share a helper.
5. `sli_control.cpp:1162-1166` — `CaseFunction` true branch must push the proc onto the **e-stack**, not leave it on the o-stack.
6. `sli_control.cpp:439-467` — implement `CurrentnameFunction` properly (look up the name of the enclosing procedure on the e-stack); or remove its registration in `init_slicontrol` if not needed.
7. `sli_control.cpp:1489-1522` — implement `Symbol_sFunction` or remove its registration.
8. `sli_control.cpp:1421-1447` — `Token_sFunction` move `i->EStack().pop()` to **after** the type/arity checks and the in-place mutation. Decide on the in-place-mutation hazard: either copy-on-write (allocate a new SLIString if `references() > 1`) or document that the caller must `dup` first.
9. `src/builtins/sli_typecheck.cpp:249-261` — actually walk the array in `Cvt_aFunction` and populate the trie.
10. `sli_typecheck.cpp:107-109` — delete the unconditional `trie->info(std::cerr)`.
11. `sli_typecheck.cpp:71-105` — fix the `ostringstream` lifetime UB (store `message.str()` in a local before calling `c_str()`).
12. `sli_typecheck.cpp:88` — replace the reverse-pointer loop with an index-based loop (the `--t` past `begin()` is UB on raw pointers).
13. `sli_typecheck.cpp:302` — add `.toIndex()` for the `Name → size_t` conversion in `TypeFunction`.
14. Continue registering the operators that block sli-init progress, per `tests/test_sli_eval.cpp` snippets that surface them.

**Acceptance:** `1 1 add ==` (and the Slice 5c smoke test) passes. `sli-init.sli` loads without error. `typeinit.sli` loads. Every operator registered in `sli_builtins.cpp` and friends has at least one positive and one negative test in `test_sli_eval.cpp`.

---

## Stage 5 — math, scanner, and stack-op edge cases

Now that bootstrap works, harden the parts that produce silent-wrong results.

**Tests to add:**

- `tests/test_math.cpp`:
  - `LONG_MIN -1 div_ii` raises (currently UB, may SIGFPE).
  - `LONG_MIN -1 mod_ii` raises.
  - `LONG_MIN abs_i` raises.
  - `LONG_MIN neg_i` raises.
  - `LONG_MAX 1 add_ii` raises (overflow detection).
  - `0.5 modf` returns `0.5 0.0` and the o-stack is at the right depth (currently UAF).
  - `(-2.0) 0.5 pow_dd` raises rather than silently returning NaN.
  - `Round_d` rounds half-away-from-zero (`-0.5 round_d` → `-1.0`).
  - `UnitStep_d` returns `0.0` for negative inputs.
  - `UnitStep_i` returns the right type and reads the right union member.
  - `Sleep_d` sleeps for the documented time, not a garbage long.
  - `1 1.0 eq` — decide: numeric coercion or strict typeid match. Document and test.
- `tests/test_scanner.cpp`:
  - Line numbers are correct after `\n\n\nfoo` (currently double-counts).
  - `9999999999999999999` (>LONG_MAX) raises a clean error rather than UB.
  - Unterminated string `(foo` returns a recognisable error.
  - Backslash escapes: `\\`, `\(`, `\)`, `\n`, `\t`, plus `\r`, `\"`, `\xHH` (or document which subset is supported).
  - EOF mid-token returns a clean error.
- `tests/test_stack.cpp`:
  - `0 0 roll` raises (currently divides by zero).
  - `rot` on empty stack raises.
  - `-1 npop` raises a `RangeCheck`, not a `StackUnderflow` from a wrap-to-huge-unsigned.

**Fixes:**

1. Math UB — write a `checked_arith.h` helper using `__builtin_add_overflow` etc., apply to `Add_ii`/`Sub_ii`/`Mul_ii`/`Div_ii`/`Mod_ii`/`Abs_i`/`Neg_i`. Raise `RangeCheck` on overflow.
2. `src/builtins/sli_math.cpp:507-516` — `Modf_dFunction` reserve o-stack capacity before the push, or capture `op1` by value before pushing.
3. `sli_math.cpp:1283-1288` — `Round_dFunction` use `std::round`.
4. `sli_math.cpp:1219-1231` — fix `UnitStep_d` (set 0 for negatives) and `UnitStep_i` (read `long_val`, not `double_val`; update `type_`).
5. `sli_control.cpp:1402` — `Sleep_dFunction` read `data_.double_val`. While here, change the `static_cast<int>(...)` to `static_cast<long>(...)` to avoid 32-bit wrap on long sleeps.
6. `sli_control.cpp:1018-1032` and similar anonymous duplicates — delete the unsafe duplicate; everyone goes through `*_bbfunction`.
7. `src/parser/sli_scanner.cpp:531-547` — fix the line counter. The `++line` should fire only on newline transition, not on `col == 0` after reset. Add tests with multi-line input.
8. `sli_scanner.cpp:559` — replace `std::labs(l) * base + digval(c)` with checked arithmetic. Use `__builtin_smull_overflow` or accumulate as unsigned with explicit overflow detection.
9. `sli_scanner.cpp:583-615` — delete the parallel `d/p/sg/e` accumulator entirely. The result comes from `std::atof(ds)`; the parallel arithmetic is dead code and a maintenance trap.
10. `sli_scanner.cpp:594-700` — annotate every intentional fall-through with `[[fallthrough]]`.
11. `src/builtins/sli_stack.cpp:52-146` — every implicit `long → size_t` truncation: range-check first, raise `RangeCheck` on negative.
12. `sli_stack.cpp:172-190, 229-234` — `RollFunction` and `RotFunction` short-circuit `n == 0`.

**Acceptance:** `test_math.cpp`, `test_scanner.cpp`, `test_stack.cpp` green. ASAN+UBSan clean across the math operator surface (the integer overflow tests are the canary).

---

## Stage 6 — serialization

Now the type system is solid, fill in the serialize/deserialize gaps. This is mostly mechanical: every type with a payload must override.

**Tests to add (most already in `test_round_trip.cpp` from Stage 0):**

- For every entry in `sli_typeid` (`Token` per typeid → `BinaryWriter` → `BinaryReader` → assert `==`).
- Cycle test: an array `[a]` where `a[0]` is a Token whose `array_val == &a` (built via direct pointer injection — not constructible from SLI source). Writer's object table should break the cycle; reader should reconstruct it.
- Aliased shared-pointer test: two Tokens both holding the same `SLIString*` round-trip as two Tokens both holding the **same** loaded `SLIString*` (object table dedup).
- Stream Token round-trips as a closed-stream Token (per the documented design).
- `kSerializeVersion + 1` throws a clear `SerializeVersion` error, not a generic exception.
- Bad magic bytes throw `SerializeMagic`.
- Truncated input throws `SerializeShortRead` rather than asserting.

**Fixes:**

1. `src/types/sli_dicttype.{h,cpp}` — implement `serialize` (write key count, then each key as a string + the value Token recursively) and `deserialize` (mirror, with object-table dedup for the dict itself).
2. `src/types/sli_functiontype.{h,cpp}` — implement `serialize` (write the function's name) and `deserialize` (re-resolve via `SLIInterpreter::lookup_function(name)` — add this method if it doesn't exist).
3. `src/types/sli_arraytype.cpp` — give `ProcedureType` and `LitprocedureType` their own `serialize`/`deserialize` overrides if `ArrayType`'s isn't enough (today nothing distinguishes them on the wire because the dispatching typeid is set by `write_token`, but the loaded Token needs the right `type_` pointer; verify).
4. `src/trie/sli_trietype.h` — implement `serialize` (write the trie's name) and `deserialize` (re-resolve; tries are registered in the system_dict).
5. `src/serialize/sli_serialize.cpp:151-166` — disambiguate the null-Token sentinel from a real `nulltype` Token. Either add a separate sentinel byte, or reserve typeid 0 for "null Token only" and shift `nulltype` elsewhere (this is a wire-format break, so do it before any saved snapshots exist — which is now).
6. `src/types/sli_iostreamtype.cpp:53-73` — register stream Tokens in the Reader's object table on deserialize so aliased-stream Tokens load as a single closed-stream object, not N independent ones.
7. `src/types/sli_nametype.h:53-65` — `OperatorType<typeid>` markers need to round-trip with their typeid intact, not collapse to plain literals. Either each `OperatorType<X>` overrides `serialize` to write `X`, or `LiteralType::serialize` writes the typeid alongside the name.
8. `src/serialize/sli_serialize.h:20` — make the `kSerializeVersion` check forward-compatible: reject newer with a clear message; handle older with an explicit migration table (empty for now, but the hook needs to exist).
9. Add a `TextWriter`/`TextReader` for debugging — the header has advertised this since Slice 1.5; one debug round-trip test will pay for the implementation.

**Acceptance:** every typeid round-trips. `test_round_trip.cpp` is exhaustive (loops over the enum). ASAN clean for serialization paths.

---

## Stage 7 — dispatcher parity & static-cache lifetime

`execute_` (plain) and `execute_dispatch_` must agree. And a second `SLIInterpreter` constructed sequentially must work.

**Tests to add:**

- `test_dispatch_parity.cpp` (from Stage 0) now has a substantial corpus: every snippet from `test_sli_eval.cpp` runs through both modes; the o-stack must end identical.
- `test_two_interpreters.cpp` (from Stage 0) — deeper version: construct A, run a math op, destroy A; construct B, run **the same** math op. Today crashes on the cached `static SLIType*`.
- Tail-call test: a procedure that calls itself N=1000 deep through `iiterate` succeeds in `execute_` mode (currently blows the e-stack because the fallback doesn't do TCO).
- Empty-loop test: `{ } loop` raises after some bound, doesn't spin forever (today's `execute_` mode behaviour).
- `forallindexed` test: covers `IforallindexedarrayFunction` which is **only** reachable via fallback (no dispatcher case).

**Fixes:**

1. Audit every `static SLIType *foo = i->get_type(...)` inside an `execute()` body (~30 sites in `sli_math.cpp`, `sli_control.cpp`, `sli_typecheck.cpp`). Replace with one of:
   - `auto* foo = i->get_type(...);` per call — the lookup is an array index, basically free.
   - A per-interpreter cache populated at module-init time (pass a struct of pre-resolved pointers to each function via a member, not a static).
   - `static thread_local`-keyed map indexed by interpreter pointer — heavier but bullet-proof.
   The first option is the smallest diff and matches CLAUDE.md's "pointer to the interpreter's type table; remains valid for the interpreter's lifetime."
2. `src/builtins/sli_builtins.cpp:94-127, 129-151, 175-203, 229-263, 287-319` — port the dispatcher's tail-call optimization into the fallback `Iiterate/Iloop/Irepeat/Ifor/Iforallarray::execute`. Most of this is rewriting the `i->EStack().push(t)` line to mirror lines 786-790 of `sli_interpreter.cpp`. Or: have the fallback delegate to a shared helper that the dispatcher also uses, eliminating the duplication.
3. `sli_builtins.cpp:129-151` — guard against `IloopFunction` with empty body spinning. Detect `proc->size() == 0` and raise immediately or pop and return.
4. Decide whether `IforallindexedarrayFunction` deserves a dispatcher case (it's the only forall variant without one, per `sli_interpreter.cpp:893`). If yes, inline; if no, the fallback at `sli_builtins.cpp:337-422` is the implementation and needs the same TCO fix.
5. `sli_builtins.cpp:79-92, 153-167, 205-222, …` — the eight `backtrace` overrides whose bodies are entirely commented out: either implement (debugger feature) or remove and stop registering them.
6. `sli_interpreter.h:265` — `step_mode()` always-false: either implement step mode or remove the call sites in `sli_builtins.cpp:406-413, 451-456` (which today contain dangling-reference UB that's only unreachable because step_mode is dead).

**Acceptance:** `test_dispatch_parity.cpp` and `test_two_interpreters.cpp` pass. The dispatcher and fallback share a tail-call helper. `static SLIType*` count drops to zero (verify via `grep -r "static SLIType" src/`).

---

## Stage 8 — cleanup, hardening, build

Now make the codebase consistent and warning-clean.

**Tests to add:**

- Warnings: build under `-Werror -Wall -Wextra` (all targets including tests). CI fails on new warnings.
- ASAN+UBSan: full test suite green.
- A "no dead code" scan: `grep -rn "TODO\|FIXME\|XXX\|HACK\|KLUDGE" src/` returns empty (or every match is annotated with an issue link).
- Every operator declared in a `*.h` is registered in some `init_*` function, and vice versa (a static check, possibly via a script).

**Fixes:**

1. Move `target_compile_options(sli3_core ... -Wall -Wextra ...)` from `PRIVATE` to `INTERFACE` (so consumers, including tests, inherit) or duplicate on each test target.
2. Resolve the 4 pre-existing unused-variable warnings (CLAUDE.md notes `proc` in `sli_control.cpp`, `d` in `sli_scanner.cpp`).
3. Delete dead code:
   - The 8 `backtrace` overrides in `sli_builtins.cpp` (already done in Stage 7).
   - Commented blocks in `sli_control.cpp` (StopFunction debug, CloseinputFunction, DebugOnFunction, DebugOffFunction, DebugFunction, the StartFunction error, …).
   - The `// if(command->gettypename()...)` block at `sli_interpreter.cpp:497-512`.
   - The commented-out exit-code lines at `sli_interpreter.cpp:582-603`.
   - The `#if 0` Taylor series in `sli_numerics.h:35-58`.
   - `src/containers/sli_dictionary.cpp:83-129` — `add_dict`/`remove_dict` (already removed in Stage 2).
   - The `extern int signalflag;` if no signal handler is implemented in `sli_main.cpp`.
4. Implement signal handling in `sli_main.cpp`: register a `SIGINT` handler that sets `signalflag`, register a top-level try/catch around `engine.execute(2)`, propagate the exit code. Or document why none of these are implemented.
5. `src/parser/sli_parser.cpp:38-60` — give `Parser` a destructor that `delete`s its `Scanner`. Today every `Parser` instance leaks one Scanner.
6. `src/parser/sli_scanner.h:120` — make the DFA transition table a `static const` array; current per-instance array is ~4.8 KB.
7. `CMakeLists.txt` — add `add_test(... PROPERTIES TIMEOUT 60)` (already done in Stage 0). Decide on an `install()` rule; if installable, `SLI3_DATADIR` must point at the install location, not the source tree.
8. `src/parser/sli_charcode.h:33` — change `class CharCode : public std::vector<size_t>` to composition.
9. `src/types/sli_token.h:121-135` — audit the union for ABI hygiene; consider switching to `std::variant` if the runtime cost is acceptable (probably not, but at minimum document the active-member discipline).
10. `src/util/sli_exceptions.h:184-192` — rename the `SystemSignal` class (collides with the `Name SystemSignal` member of `SLIInterpreter`).
11. `src/interpreter/sli_module.h:53, 58` — either bring `unported/sli_module.cpp` back, or remove the declarations (latent link error if any concrete `SLIModule` subclass appears).
12. `src/interpreter/sli_main.cpp` — handle argv (run a script file when given a path, drop into REPL otherwise).
13. `src/interpreter/sli_interpreter.cpp:114` — move `parser_ = new Parser()` to before `init()` (or into a constructor member-init).
14. `src/interpreter/sli_interpreter.cpp:269-281` — set `is_initialized_ = true` even when the e-stack is empty at startup, otherwise every subsequent `execute(string)` re-runs startup.

**Acceptance:** `cmake -DCMAKE_CXX_FLAGS=-Werror -B build && cmake --build build && ctest --test-dir build` green. ASAN run clean. `grep TODO src/` empty.

---

## Performance results

Apple M-series, Release build, comparing against a Release build of
NEST 2.20.2's `sli` (sibling checkout at
`/Users/gewaltig/Code/nest-2.20.2/`). Each workload run 5 times,
best-of-five wall time reported (`/usr/bin/time -p`, real).

Workloads:

```sli
B1: tic 100000000 { 1 pop } repeat toc ==
B2: tic 100000000 { 1 1 add pop } repeat toc ==
B3: tic 100000 { 1 1 1000 { 2 add pop } for } repeat toc ==
B4: tic 100000000 { 1 1 add_ii pop } repeat toc ==   ; bypasses the trie
```

| Workload                          | sli3   | nest 2.20 | sli3 vs nest |
|-----------------------------------|--------|-----------|--------------|
| B1 — `1 pop`                      | 1.65 s | 2.00 s    | **−17.5 %**  |
| B2 — `1 1 add pop` (trie)         | 4.05 s | 4.29 s    | **−5.6 %**   |
| B3 — `1k for` × 100k              | 3.71 s | 4.28 s    | **−13.3 %**  |
| B4 — `1 1 add_ii pop` (no trie)   | 3.35 s | 3.71 s    | **−9.7 %**   |

Notes:

- Bypassing the trie (B2 → B4) saves ~0.70 s in sli3 and ~0.58 s in
  nest. Trie cost is roughly equivalent in both implementations, so
  it is **not** where sli3's lead comes from.
- sli3's lead is largest on the cheapest per-iteration workload (B1)
  and shrinks as per-iteration work grows (B2). This is consistent
  with the gain coming from dispatcher plumbing (16-byte Token,
  pointer-tagged typeid, inlined hot-op switch, `needs_refcount()`
  fast path on scalar refcount), not from individual operator
  implementations.
- Per-iteration cost on B4: ~33.5 ns sli3 vs ~37 ns nest — covers
  name lookup, dispatch, two pushes, two pops, and the integer add.

Reproducing: `for tag in bench1 bench2 bench3 bench_add_ii; do
/usr/bin/time -p ./build/sli3 < /tmp/$tag.sli > /dev/null; done`,
with the four scripts above written to `/tmp/$tag.sli`.

---

## Sequencing summary

```
0 — test infra ───────────► all subsequent stages need this
1 — Token / SLIType ──────► Stage 2, 6 depend on this
2 — containers ───────────► Stage 4, 7 depend on this
3 — error handling ───────► Stage 4 depends on this
4 — bootstrap operators ──► reaches Slice 5c smoke test
5 — math / scanner / stack
6 — serialization ────────► requires Stages 1-2
7 — dispatcher parity ────► requires Stages 1-4
8 — cleanup / hardening ──► last; requires everything
```

Stages 1-4 can be worked in strict sequence; 5 and 6 are independent of each other and of 7; 8 is final.

Each stage's "Acceptance" is a hard gate: don't move on until the listed tests are green and the listed audits are clean. The whole point of TDD-staging is that a regression at stage N is almost always coming from stage N's own scope, not from earlier work.

## What this plan does **not** cover

- Reintroducing `sli_processes` (deferred per CLAUDE.md Q4).
- The `iteratortype` / `forall_iter` protocol (out of scope per CLAUDE.md).
- Multi-threading. Single-threaded is the design choice; if that ever changes, every static counter (`TokenArray::allocations`, `Name::insert`, the dispatcher's cycle counter, …) becomes a race and needs revisiting.
- Performance work. Nothing here is a perf optimization; correctness first. Profiling-driven optimization is a separate effort once Stage 8 lands.
