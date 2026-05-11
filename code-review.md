# sli3 code review

Reviewed: every C++ source file in `src/`, all four test binaries, `CMakeLists.txt`, and `config.h.in`. ~12 KLOC.

Grouped by severity. **Critical** = will crash, corrupt, or silently produce wrong output today. **High** = latent bug, broken under common scenarios. **Medium** = correctness in edge cases / dead code / style traps that mask real bugs.

> **Status: most CRITICAL/HIGH items resolved through tag `stage9-complete`
> (2026-05-11).** See the [Resolution status](#resolution-status) summary
> below for current state. The body of this document is preserved
> verbatim as the historical inventory — it shows what we found and
> the rationale, even when the issue is fixed. The summary tells you
> what is still open without rereading 250 lines of detail.

## Resolution status

Status is grouped by `fix-plan.md` stage. "Resolved (Stage N)" means
the corresponding stage closed the item; the body entry below carries
the original detail. Verified against the source tree at
`stage9-complete` (2026-05-11) — when the entry says "(verified)" I
re-read the current code; when it says "(per commit log / inline note)"
I trusted the commit annotation.

### Resolved (CRITICAL tier)

- Token self-assign UB (`sli_token.h:222-226`) — Stage 1
  (`sli_token.h:298`: `if (&t == this) return *this;` verified).
- `NameType::execute` reading wrong union member (`sli_nametype.cpp:12`)
  — Stage 1 (reads `name_val`, verified, with explanatory comment).
- `Token::operator==(bool)` / `operator==(double)` checking the wrong
  typeid (load-bearing bootstrap blocker) — Stage 1 / Slice 5b
  (verified at `sli_token.cpp:118`, `:123`; CLAUDE.md Decisions log
  2026-05-06).
- `IfFunction` accepted non-bool conditions — Stage 4.3 (verified
  at `sli_control.cpp:164` with `require_stack_type`).
- `IfelseFunction` read the wrong stack slot — Stage 4.4 (verified
  at `sli_control.cpp:201`, checks `pick(2)`).
- `DictError` ctor dropped its argument (UndefinedName collapsed to
  /DictError) — Stage 3 (verified at `sli_exceptions.h:230`).
- `Sleep_dFunction` read wrong union member — Stage 5.5 (verified at
  `sli_control.cpp:1585`).
- `UnitStep_dFunction` only wrote one branch — Stage 5.4 (verified at
  `sli_math.cpp:1810`).
- `UnitStep_iFunction` read `double_val`, wrote `long_val` — Stage 5.4
  (verified at `sli_math.cpp:1822`).
- `Modf_dFunction` reference into ostack invalidated by push — Stage 5.2
  (verified at `sli_math.cpp:1095`: captures by value).
- `AddtotrieFunction` reverse-pointer past `begin()` UB — Stage 4.7
  (verified at `sli_typecheck.cpp:95`: index-based loop).
- `AddtotrieFunction` `ostringstream` lifetime UB — Stage 4.7
  (verified at `sli_typecheck.cpp:100`: stores `msg` in a local).
- `TypeFunction:302` missing `.toIndex()` — Stage 4.8
  (verified at `sli_typecheck.cpp:307`: `.toIndex()` present).
- `DictionaryStack::toArray` missing `add_reference` (the recordstacks
  UAF) — commit `22aab36` (verified at `sli_dictstack.cpp:160`).
- `RaiseerrorFunction::execute` leaked ostack+estack frames — commit
  `2531c8f` (verified; matches NEST 2.x `slicontrol.cc:1187`).
- `Symbol_sFunction` empty body — Stage 4.10 (verified at
  `sli_control.cpp:1707`: implemented via `read_token`).
- Scanner integer-literal `std::labs(LONG_MIN)` UB — Stage 5.8
  (verified at `sli_scanner.cpp:570`: checked arithmetic).
- Integer-arithmetic UB on hot ops (`Add_ii`, `Sub_ii`, `Mul_ii`,
  `Div_ii`, `Mod_ii`, `Abs_i`, `Neg_i`) — Stage 5.1 (verified:
  `checked_arith` helper at `sli_math.cpp:42`, `__builtin_add_overflow`
  used at `sli_math.cpp:109` and elsewhere).
- TokenStack bounds checks — Stage 2.2 (verified: `assert(load() > 0)`
  at `sli_tokenstack.h:56` and surrounding lines).
- DictionaryStack `base_` member init + dual-ref copy semantics +
  paired remove_reference on destruction — Stage 2 (verified at
  `sli_dictstack.cpp:36-44`, `:115-124`).
- DICTSTACK_CACHE unconditionally active + unconditional invalidation —
  Stage 2.7 (verified at `sli_dictstack.cpp:20`, `:68-72`).
- Two-interpreter test passing — Stage 7 (static `SLIType*` caches
  inside `execute()` bodies removed, verified by
  `tests/test_two_interpreters.cpp` passing).

### Resolved (HIGH tier)

- `ArrayType::references()` decrement bug — Slice 2 (CLAUDE.md note).
- StringType / IstreamType / OstreamType refcount overrides (silent
  leaks) — Slice 3 (CLAUDE.md note).
- `ProcedureType::execute` `EStack().dump` debug print — Stage 7
  (removed; verified absent from `sli_arraytype.cpp:69-75`).
- `Token::operator long&()` / `double&()` / `bool&()` raw-reference
  hazard — Stage 1 (removed from `sli_token.h`).
- `FunctionType` serialize/deserialize — commit `3a7edec` (verified
  at `sli_functiontype.cpp:33`).
- `WhereFunction` returns the dict that actually held the key — Stage 4
  (audit + behavior change applied during Stage 9 batch 10).
- `Token_sFunction` ordering bug (pop estack before checks) — Stage 4.8
  (verified by inspection — checks now precede the pop).
- Round_d / Pow_dd numeric edge cases — Stage 5 (checked-arith path
  in `sli_math.cpp`).
- `--Wall -Wextra` PUBLIC on test targets — Stage 0/8 (verified in
  `CMakeLists.txt`).
- Test infrastructure (`test_round_trip.cpp`, `test_dispatch_parity.cpp`,
  `test_two_interpreters.cpp`, `test_dictstack.cpp`, `test_tokenstack.cpp`,
  `test_name.cpp`, `test_math_overflow.cpp`, `test_scanner_edge_cases.cpp`,
  `test_stack_ops_negatives.cpp`, `test_errors.cpp`,
  `test_errors_dispatch.cpp`, `test_state_ops.cpp`) — Stage 0 + Slice 10
  (verified by `ls tests/`).

### Open — structural / planned

These items have a planned resolution path; not yet started or
partial.

- **Axis I — dispatcher restructure** (`doc/dispatch_restructure_plan.md`).
  Subsumes:
  - "316 `i->EStack().pop()` sites" (per-op self-pop ABI).
  - Function-Token push on every operator dispatch.
  - `raiseerror` / `get_current_name` reading the e-stack top
    (Slice 1 introduces `current_op_`).
  - Tail-recursion limited to `iiteratetype` (Slice 8 adopts gs's
    `bot:`/`out:`/`up:` multi-level cascade; see also
    `doc/tail_recursion.md`).
  - Iter-case re-pick of `proc` / `pos` per token (Slice 3
    register-lift).
  Estimated bench delta from
  `doc/compact_procedure_spec.md`: B2b 1.91 s → ~1.55 s.
- **Axis II — hot-op inlining**
  (`doc/compact_procedure_spec.md`). Depends on Axis I.
- **Axis III — compact procedure storage** (`CompactProc`).
  Lowest-priority of the three axes; depends on I + II.

### Open — Stage 6 (serialization) gaps

CLAUDE.md states the `sli_typeid` enum is a permanent wire contract
and every SLIType with payload must override `serialize` / `deserialize`.
Slice 10 (savestate/restorestate, landed in `3a7edec`) closed
`FunctionType`. Still open:

- `DictionaryType::serialize` — `sli_dicttype.cpp` has no override.
  Round-trip drops the dict body. Required for full state ops
  (currently the dict stack is not snapshotted, per a known
  limitation in `sli_state_ops.cpp`).
- `TrieType::serialize` — `sli_trietype.h` has no override; the trie
  body is dropped. Should re-resolve via dictstack lookup, same
  pattern as `FunctionType::serialize`.
- `ProcedureType` / `LitprocedureType` need their own serialize
  overrides (or a documented inherit-from-ArrayType policy with a
  typeid-aware wire byte) — currently only `ArrayType::serialize`
  exists.
- `OperatorType<typeid>` markers (iiterate, irepeat, ifor, iforall,
  …) round-trip as plain `LiteralType` — typeid is lost.
- Stream-Token deserialize doesn't register in the Reader's object
  table; two aliased streams load as two independent closed-stream
  Tokens.
- `write_token`'s null sentinel is the same byte as a real
  `nulltype` Token — can't tell them apart on the wire.
- `TextWriter` / `TextReader` exist only as a comment in
  `sli_serialize.h:25`. Useful for debugging serialization, low
  priority for shipping.
- Round-trip-every-typeid test exists (`test_round_trip.cpp`); the
  gaps above show up there as `SKIP`s — flip to fail when each gap
  closes.

### Open — Stage 8 cleanup items

Not blocking work, but still pending:

- `::pop` still binds to `IlookupFunction` (`sli_interpreter.cpp:245`).
  There is no `IpopFunction` class. Any code path that pushes `::pop`
  onto the e-stack (`StopFunction` handling at `sli_control.cpp:1079`,
  `:1119`) re-runs lookup. Functional impact unclear in current
  bootstrap; harmless or load-bearing depending on the path. Decide
  whether to write an `IpopFunction` (pops one e-stack frame) or
  document the alias.
- `CurrentnameFunction` is stubbed to push `false` with a
  documenting comment (Stage 4.9). NEST 2.x walked an
  `::lookup` breadcrumb that sli3 doesn't push. To unstub, either
  reintroduce the breadcrumb (cost: one push per name dispatch) or
  re-derive from the iter-frame `pos` after Axis I lands.
- `Cvt_aFunction` constructs an empty `TypeNode` from the array's
  name only — does not walk the array to populate trie leaves
  (`sli_typecheck.cpp:262`). User-facing operator; not exercised by
  the bootstrap or the bench surface.
- `IloopFunction` empty body silently spins (dispatcher has no
  `iloop` case).
- `IforallindexedarrayFunction` is the only forall variant without
  a dispatcher case; the fallback in `sli_builtins.cpp:337` lacks
  TCO. Resolved indirectly when Axis I unifies the body-walk.
- Dead code: 8 `backtrace` overrides whose bodies are commented out
  (`sli_builtins.cpp:79, 153, 205, 265, 322, 378, 424, 432`); the
  `// if(command->gettypename()...)` block at
  `sli_interpreter.cpp:497-512`; the `#if 0` Taylor series in
  `sli_numerics.h:35-58`; `extern int signalflag;` with no signal
  handler.
- `Parser` has no destructor → leaks one Scanner per Parser
  instance (`sli_parser.cpp:38-60`).
- `DFA` transition table is per-instance (`sli_scanner.h:120`) —
  should be `static const`.
- `class CharCode : public std::vector<size_t>` — composition over
  inheritance.
- Argv handling in `sli_main.cpp` (still ignored).
- `is_initialized_` not set when e-stack is empty at startup
  (`sli_interpreter.cpp:269-281`) — every subsequent `execute(string)`
  re-runs startup.
- `intvectortype` / `doublevectortype` are placeholder enum slots
  with no implementation; out of scope today, but the placeholder
  is still load-bearing for the typeid wire-contract.

### Notes / decisions baked in

- "Currentname returns `false` rather than the caller name" is a
  documented divergence from NEST 2.x — Stage 4.9 comment in
  `sli_control.cpp:449` explains. Acceptable for standalone sli3.
- `recordstacks` defaults to `true` after `sli-init.sli` runs
  `init_errordict` (`lib/sli/sli-init.sli:217`). Documented in
  CLAUDE.md; cost is ~0.04-0.33 µs per error (see B6 bench
  family in `bench/sli/`).
- The single-source-of-truth for what is currently planned is
  `fix-plan.md` (correctness fixes) and `doc/compact_procedure_spec.md`
  + `doc/dispatch_restructure_plan.md` (next-phase performance).
  See `doc/next_steps.md` for the consolidated roadmap including
  gs-derived proposals beyond Axis I-III.

---

## CRITICAL — fix before anything else runs

### Wiring bugs in operator registration

- **`src/interpreter/sli_interpreter.cpp:236`** — `createcommand(ipop_name, &ilookupfunction)` binds `::pop` to `IlookupFunction`. There is no `IpopFunction` class anywhere. Anything that pushes `::pop` onto the e-stack (e.g. `sli_control.cpp:1079, 1119` in stop handling) re-runs lookup, recursing.
- **`src/builtins/sli_math.cpp:1541`** — `or` registered to the unsafe anonymous `orfunction` that lacks `require_stack_load(2)`. `xor`/`and` use the type-checked `*_bbfunction`. Calling `or` with <2 args reads garbage.
- **`src/interpreter/sli_interpreter.h:780` vs `:779`** — `template<> void push<TokenArray&>(TokenArray&)` declared, but the inline implementation has signature `TokenArray const&`. Declaration without definition.
- **`src/builtins/sli_builtins.cpp:28-49`** — `IlookupFunction::execute` is dead (defined, never registered). `IparsestdinFunction` pops 1 e-stack frame on EOF where its sibling pops 2 — silent inconsistency.

### Wrong union member / wrong type-id read

- **`src/builtins/sli_control.cpp:1402`** — `Sleep_dFunction` reads `pick(0).data_.long_val` after type-checking `doubletype`. Sleeps for the bit-pattern of the double interpreted as a long × 1e6. (Then casts to `int`, so sleeps wrap to negative for any non-tiny value.)
- **`src/types/sli_nametype.cpp:12`** — `NameType::execute` reads `t.data_.long_val` instead of `t.data_.name_val`. Works on 64-bit only because the slots alias bit-for-bit; UB by the language and inconsistent with every other site.
- **`src/builtins/sli_math.cpp:1225-1231`** — `UnitStep_iFunction` reads `data_.double_val`, writes `data_.long_val`, and never updates `type_`. Both source and destination union members are wrong.
- **`src/builtins/sli_math.cpp:1219-1222`** — `UnitStep_dFunction` only writes `1.0` for `>=0`, leaving negatives unchanged (should be `0.0`).
- **`src/builtins/sli_typecheck.cpp:302`** — `TypeFunction` does `top.data_.name_val = tmp.type_->get_typename()` without `.toIndex()`. Either it relies on an implicit `Name`→`size_t` that may not exist (build accident) or it stores a raw pointer-bit-pattern as a name handle.

### Type-check holes in control flow

- **`src/builtins/sli_control.cpp:164-174`** — `IfFunction` uses `i->pick(1) == true`, with no type guard. Any non-bool condition (array, dict, integer, …) silently takes the else branch. PostScript spec says `if` requires bool. Diverges from `IfelseFunction` (which now checks).
- **`src/builtins/sli_control.cpp:206-209`** — `IfelseFunction` itself silently degrades non-bool to false rather than raising `ArgumentTypeError`.
- **`src/builtins/sli_control.cpp:1162-1166`** — `CaseFunction` true branch leaves the proc on the **operand** stack instead of moving it to the e-stack. Contract was "execute proc on true match"; current code never executes it.

### Self-assignment / aliasing UB

- **`src/types/sli_token.h:222-226`** — `Token::operator=(const Token&)` calls `clear()` then `init(t)`. If `&t == this` (or `t` aliases a sub-element of `*this` whose only ref is via `*this`), `clear()` destroys the payload before `init` reads it. Self-assignment of a refcount-1 array/string/dict Token is use-after-free.
- **`src/containers/sli_array.cpp:99-107`** — `assign(*this)` and `append(*this)` are UB; `std::vector::insert/assign` reads source iterators that the operation invalidates.

### Container ownership / refcount

- **`src/containers/sli_dictstack.cpp:153-162`** — `DictionaryStack::toArray` constructed dict-Tokens via raw field assignment (`type_ = ...; data_.dict_val = ...`) without `add_reference`. The local's destructor decremented an unbumped refcount, so each snapshot drove the dict's true refcount down by 1. Surfaced as a heap-use-after-free on `systemdict` after 3 consecutive errors with `recordstacks=true` (ASAN confirmed); the silent dispatcher halt + segfault-on-quit after `foo foo foo`. **Resolved 2026-05-11** (commit `22aab36`): explicit `dicttoken.add_reference()` after raw assignment. General rule documented in CLAUDE.md "Stack-handling discipline".
- **`src/containers/sli_dictstack.h:64`, `src/containers/sli_dictstack.cpp:24-26`** — `DictionaryStack::base_` is a raw pointer with no member-init and no zeroing in the default ctor. `baselookup`/`basedef`/`set_basedict` all dereference it; calling any of them before the first `push` + `set_basedict` segfaults.
- **`src/containers/sli_dictstack.cpp:28-31, 204-214`** — Copy ctor and `operator=` copy the `list<Dictionary*>` of bare pointers without calling `add_reference`. Don't propagate `base_`, `cache_`, `basecache_`. Two stacks share dicts that one can free, dangling the other.
- **`src/containers/sli_dictstack.cpp:33-39, 101-107`** — Destructor and `clear()` call `dict->clear()` on every dict but **never call `remove_reference`** to pair with the `add_reference` that `push()` does. Every dict left on the stack at destruction leaks one ref.
- **`src/containers/sli_dictstack.h:130-157` + `:53-67` (cpp)** — `cache_token` is invoked unconditionally on every `lookup`, but `clear_token_from_cache` (in `undef`) is gated on `#ifdef DICTSTACK_CACHE`. The macro is defined nowhere in the build. Cache entries are never invalidated — every `undef` of a cached key leaves a dangling `Token*` in `cache_`. Next lookup of that key reads freed memory.
- **`src/containers/sli_dictstack.cpp:75-99, 110-113, 169-172`** — `pop()`, `top()`, `set_basedict()` all dereference `*d.begin()` / `--d.end()` with no bounds check. UB on empty stack.
- **`src/containers/sli_tokenstack.h:113`** — **`size()` returns `TokenArray::capacity()`**, not the number of pushed elements. Anyone reading `size()` to mean "depth" gets buffer capacity. Combined with `pop(size_t n)` having no bounds check, this is a classic overrun trap.
- **`src/containers/sli_tokenstack.h:53-94`** — `pop()`, `pop(n)`, `top()`, `pick(i)`, `swap()` have no bounds checks. UB on empty/short stacks.
- **`src/containers/sli_tokenstack.h:101-111`** — `roll(n, k)` does `k % n`. If `n == 0`, divide-by-zero.

### Math / arithmetic UB

- **`src/builtins/sli_math.cpp:71-78, 109-116, 146-153`** — `Add_iiFunction`/`Sub_iiFunction`/`Mul_iiFunction` no overflow check (`LONG_MIN + LONG_MIN`, etc.). Signed-int overflow is UB; compilers assume wrap can't happen and optimize accordingly.
- **`src/builtins/sli_math.cpp:183-195, 247-259`** — `Div_iiFunction`/`Mod_iiFunction` allow `LONG_MIN / -1` and `LONG_MIN % -1` — UB; SIGFPE on x86.
- **`src/builtins/sli_math.cpp:613-619, 661-667`** — `Abs_iFunction` calls `std::labs(LONG_MIN)`, `Neg_iFunction` does `-LONG_MIN` — both UB.
- **`src/builtins/sli_math.cpp:507-516`** — `Modf_dFunction` captures `op1` as a reference into ostack, then `i->push(0.0)` may reallocate the ostack, leaving `op1` dangling, then passes it to `std::modf`. Use-after-free.
- **`src/parser/sli_scanner.cpp:559`** — `intdgtst:` does `l = sg * (std::labs(l) * base + digval(c))`. `std::labs(LONG_MIN)` is UB; a 19-digit integer literal silently parses to garbage.
- **`src/builtins/sli_stack.cpp:172-190, 229-234`** — `RollFunction` with `n=0,k=0` and `RotFunction` on an empty stack feed `roll(0, k)` which divides by zero in `roll`.

### Scanner correctness

- **`src/parser/sli_scanner.cpp:531-547`** — Line-counter double-counts: `if (col++ == 0) ++line;` increments on first char of a line, but on a newline the code sets `col = 0`, so the *next* char's `col++ == 0` increments line a second time. Off-by-one error in **every** line/column report.
- **`src/parser/sli_scanner.cpp:529-540`** — `c = in->get()` (returning `int`) stored into `unsigned char c` truncates EOF (-1) to 0xFF. The DFA processes 0xFF as alpha for one cycle before the EOF branch fires.
- **`src/parser/sli_scanner.cpp:583-615`** — Parallel `d/p/sg/e` arithmetic accumulator for floats is **dead code** — the actual value comes from `std::atof(ds.c_str())`. Maintenance trap: someone "fixes" the parallel `d` and breaks edge cases.
- **`src/parser/sli_scanner.cpp:594-604, 627-632, 662-700`** — Multiple intentional `case` fall-throughs (decpdgtst→fracdgtst, minusst→plusst, sgalphast→literalst→stringst→alphast, aheadsgst→aheadalphst) with no `[[fallthrough]]` markers. Future maintainers will "fix" them.

### Serialization gaps

The `sli_typeid` enum is documented as a permanent wire contract, but multiple types ship with no `serialize`/`deserialize` override and silently use the no-op base default:

- **`src/types/sli_dicttype.h/.cpp`** — `DictionaryType` has pointer payload but no overrides. Round-tripping a Token whose payload is a Dictionary drops the entire dict body. Reload produces a Token with stale/null `dict_val` → next access null-derefs (or worse).
- **`src/types/sli_functiontype.h/.cpp`** — `FunctionType` has `func_val` payload, no override. CLAUDE.md claims "SLIFunction* re-resolves by name on load"; nothing implements this.
- **`src/types/sli_arraytype.cpp:51-67`** — `ProcedureType` has no override; inherits LitprocedureType's `serialize` for some methods but not for `serialize`/`deserialize`. A round-tripped procedure has undefined `array_val` → first `execute` crashes.
- **`src/trie/sli_trietype.h`** — `TrieType` no override. Same.
- **`src/serialize/sli_serialize.cpp:151-166`** — `write_token`'s null sentinel is `typeid 0`, but `nulltype` is also a real registered type at `types_[0]`. A real-nulltype Token round-trips as a default-constructed `Token`; you cannot tell them apart.

### Exception machinery

- **`src/util/sli_exceptions.h:225-227`** — `DictError(char const * const)` ignores its argument and always passes `"DictError"` to the parent `SLIException`. So when `UndefinedName` (line 242) calls the parent ctor with `"UndefinedName"`, the `what_` string ends up `"DictError"`. Every `errordict /errorname` from a dict failure is reported as the generic `DictError` — `UndefinedName`, `EntryTypeMismatch`, etc. all collapse.
- **`src/interpreter/sli_interpreter.cpp:304-308` vs `:312-335`** — `raiseerror(Name)` pops the e-stack first then delegates to `(Name,Name)`; `raiseerror(std::exception&)` does not pop. Two error entry-paths leave the e-stack in different states. Inconsistent error handling depending on which path threw. ~~Plus:~~ `RaiseerrorFunction::execute` (the SLI-callable version, `sli_control.cpp:1012`) does neither — leaks 2 ostack items per call. **Partially resolved 2026-05-11** (commit `2531c8f`): SLI-side `RaiseerrorFunction` now pops args + self per NEST 2.x `slicontrol.cc:1187`. The C++-side `(Name)` vs `(std::exception&)` inconsistency remains; both now pop estack, but the bigger cleanup (unify the three overloads) is pending.

### Bodies replaced with comments (registered, but functionally NULL)

- **`src/builtins/sli_control.cpp:439-467`** — `CurrentnameFunction::execute` body is entirely commented out except `i->EStack().pop()`. Registered at line 1853. Calling `currentname` returns nothing.
- **`src/builtins/sli_control.cpp:1489-1522`** — `Symbol_sFunction` same — comment block, only e-stack pop. **Pops one o-stack item it shouldn't** (it doesn't pop anything because the body is gone), so calling `symbol_s` corrupts the stack vs spec.
- **`src/builtins/sli_typecheck.cpp:249-261`** — `Cvt_aFunction` builds a `TypeNode` from the name + array but **never walks the array** to populate the trie. Always-empty trie matches no calls.

### Stop / loop / iterate divergence

- **`src/builtins/sli_control.cpp:303-353`** — `StopFunction` not-found path calls `i->EStack().clear()` (wipes the entire e-stack) and only logs to stderr. The dispatcher then exits with the o-stack in whatever shape user code left it.
- **`src/builtins/sli_control.cpp:1066-1104, 1106-1151`** — `SwitchFunction` reads `pick(0)` of an empty stack on `depth==0`. `SwitchdefaultFunction` throws raw `TypeMismatch` (not via `raiseerror`), so the e-stack frames it pushed earlier remain.
- **`src/builtins/sli_control.cpp:134-146`** — `ExitFunction` raises a false underflow when the mark is at the very bottom of the e-stack.
- **`src/builtins/sli_builtins.cpp:94-127, 129-151, 175-203, 229-263, 287-319`** — The fallback `Iiterate/Iloop/Irepeat/Ifor/Iforallarray::execute` bodies diverge from the dispatcher's inlined cases:
  - No tail-call optimization → recursive procedures blow the e-stack in `execute_` mode.
  - `IloopFunction` with empty body silently spins (dispatcher has no `iloop` case).
  - `dec_call_depth()` only runs in fallback path → call-depth diverges between modes.
- **`src/builtins/sli_builtins.cpp:337-422`** — `IforallindexedarrayFunction` is the **only** implementation (no dispatcher case); pick-layout vs comment is inconsistent. `IforallindexedstringFunction` calls `i->push(char)` which depends on a templated overload chain that may pick a wrong type.

### Other

- **`src/builtins/sli_container_ops.cpp:1202-1230`** — `WhereFunction` returns `i->DStack().top()` regardless of which dict actually held the key. Comment acknowledges. Wrong vs spec.
- **`src/builtins/sli_container_ops.cpp:1234-1246`** — `UndefFunction::execute` swallows **all** exceptions in `try { i->undef(n); } catch(...) {}`. Hides every bug, not just missing-key.
- **`src/builtins/sli_control.cpp:1421-1447`** — `Token_sFunction` pops its own e-stack frame at line 1423 **before** running `require_stack_load(1)` on line 1424. If the precondition throws, the frame is already gone. Then mutates `sd->str() = s` in place — same in-place-under-shared-refcount hazard as the container ops, but in the control surface.

---

## HIGH — latent bugs you'll hit on the next slice

### Static-state / interpreter-lifetime hazards

- 25+ `static SLIType*` caches inside operator `execute()` bodies (`sli_math.cpp:49,61,234,235,585,624,625,727,753,777,790,804,817,841,854,867,880,1054,1058,1072,1087,1101,1125,1139,1154,1168,1182,1199`; `sli_control.cpp:234-235,624-625,1054`; `sli_typecheck.cpp:287`). Each binds the cached pointer to the **first** `SLIInterpreter` that called the op. Sequentially constructing a second interpreter (likely in tests) crashes on the stale pointer.
- **`src/builtins/sli_array_module.cpp` (multiple)** — same pattern noted in earlier review. Already fixed in `Forall_aFunction`; remainder still cached.

### Refcount / null-payload inconsistency

- **`src/types/sli_arraytype.h:18-30`** — `ArrayType::add_reference`/`remove_reference` do **not** null-check `array_val`, while `references()` (line 40) does. A reachable null `array_val` (achievable via base `SLIType::deserialize` for a missing override, or via `Token::value()` zero-fill) null-derefs.
- **`src/types/sli_dicttype.h:23-43`** — Same: `DictionaryType::remove_reference` and `references()` deref `dict_val` unconditionally.
- **`src/types/sli_stringtype.cpp:7-22`** — `StringType::compare` and `print` deref `string_val` without null guard.
- Magic-number convention is inconsistent: `Token::add_reference()` returns `1` for invalid tokens (`sli_token.h:241`), while pointer-type `add_reference(Token&)` returns `0` for null payload (`sli_stringtype.h:24`). Off-by-one trap.

### Token / Type API

- **`src/types/sli_arraytype.cpp:69-75`** — `ProcedureType::execute` calls `EStack().dump(std::cerr)` unconditionally — debug code shipped in the dispatcher hot path.
- **`src/types/sli_arraytype.cpp:51-69`** — `ProcedureType` inherits `LitprocedureType::print` which prints `{{...}}` — bare procedures display with double braces. Wrong.
- **`src/types/sli_token.h:269-273`** — `operator long&()`, `operator double&()`, `operator bool&()` return references into the union. A caller holding the reference past a type change reads/writes the wrong slot.
- **`src/types/sli_token.h:86`** — `operator std::string&()` has no `const` overload. A `Token const&` cannot be converted to a string at all.
- **`src/types/sli_token.cpp:25-29`** — `operator std::string&()` calls `require_type(stringtype)` then dereferences `data_.string_val` without null-check; a "valid" string Token with null payload null-derefs.
- **`src/types/sli_token.cpp:47-101`** — `operator==(const Token&)` short-circuits on `t1.type_ != t2.type_`, so `nametype` Token vs `literaltype` Token holding the same `Name` returns false; `operator==(Name)` (line 86) treats them equal. Asymmetric semantics.
- All eight `SLIType` subclasses lack `override` keyword on overridden virtuals (`add_reference`, `remove_reference`, `clear`, `references`, `compare`, `print`, `pprint`, `execute`). A typo silently creates a missing override.
- **`src/types/sli_iostreamtype.cpp:53-73`** — Stream `serialize` writes nothing, `deserialize` allocates a fresh `SLIistream` without registering it in the Reader's object table. Two aliased stream Tokens deserialize as two independent closed streams.
- **`src/types/sli_iostreamtype.cpp:33-42`** — `XIstreamType::execute` doesn't override `serialize`; round-tripping loses the executable flag (loaded as plain `IstreamType`).

### Interpreter

- **`src/interpreter/sli_interpreter.cpp:114`** — `parser_ = new Parser()` runs **after** `init()`; if anything in init touches the parser, NULL deref. Latent ordering bug.
- **`src/interpreter/sli_interpreter.cpp:269-281`** — `startup()` only sets `is_initialized_` when `execution_stack_.load() > 0` at entry. If the e-stack is empty at startup time, the flag is never set and every subsequent `execute(string)` re-runs startup forever.
- **`src/interpreter/sli_interpreter.cpp:222-228`** — `init_internal_functions` registers types `< symboltype` by name; types ≥ `symboltype` (string, array, procedure, …) are **not** put into `system_dict_`. Magic threshold; if `sli_typeid` is reordered, silent semantics change.
- **`src/interpreter/sli_interpreter.cpp:608-660`** — `execute_debug_` swallows exceptions instead of calling `raiseerror` (the line is commented out at 635); also reads `status_dict_->lookup(Name("exitcode"))` unprotected at 652 → guaranteed `UndefinedName` throw if exitcode never set.
- **`src/interpreter/sli_interpreter.cpp:561-605`** — `execute_` (plain) returns `exitcode = 0` always; the lookup is commented out. Plain mode loses the exit code completely.
- **`src/interpreter/sli_interpreter.cpp:295-302`** — `get_current_name` only handles `functiontype`. Trie-induced errors report `"SLIInterpreter::execute"` as caller. Misleading attribution.
- **`src/interpreter/sli_interpreter.cpp:977-985`** — `terminate()` does `delete this; std::exit(...)`. Static destructors that touch the interpreter run after.
- **`src/interpreter/sli_main.cpp`** — no signal handler (`signalflag` is checked everywhere but never set); ignores argv; ignores the return code from `engine.execute(2)`; no top-level try/catch.

### Container details

- **`src/containers/sli_dictionary.cpp:51-81`** — `info()` builds a sorted vector via `std::copy(..., std::inserter(data, data.begin()))` and never iterates it. Prints unsorted from `begin()`/`end()` instead.
- **`src/containers/sli_dictionary.cpp:83-129`** — `add_dict` and `remove_dict` declared in header, **bodies commented out**. Link error if anything calls them.
- **`src/containers/sli_dictionary.cpp:131-147`** — `clear_access_flags` recurses into `it->second.data_.dict_val` with no null check.
- **`src/containers/sli_dictionary.h:165-167`** — `lookup(Name, Token&)` returns by copy; the access-flag mutation tracked via `mutable bool access_flag_` is invisible to callers using this overload, defeating the whole reason both overloads exist.
- **`src/containers/sli_dictstack.cpp:41-56`** — `undef(Name)` walks the entire dict stack erasing matches. PostScript spec says top dict only. Header comment says "all dicts," contradicting spec.
- **`src/containers/sli_iostream.h:32-69`** — `valid()` returns `stream_ != nullptr`; once a stream is closed via `get()->close()`, `valid()` still returns true.
- **`src/containers/sli_name.cpp:62-80`** — `Name::insert` is not exception-safe across `map.insert` + `table.push_back`; if push_back throws, the map keeps an orphan handle that points past the table.
- **`src/containers/sli_name.h:56-57, 61-64`** — `Name(long h)` constructor takes a raw integer with no bounds check — `Name(99999).toString()` is UB. `operator int()` implicit conversion makes a Name silently appear as int in arithmetic.
- **`src/containers/sli_string.h:49-65`** — `str()` and the implicit `operator std::string&()` expose mutable shared payload. Any caller holding the reference can mutate the string visible to all other refcount holders.

### Builtins

- **`src/builtins/sli_container_ops.cpp` (many)** — In-place mutation under shared refcount: `PutArrayLikeFunction:189-213`, `PutStringFunction:215-236`, `AppendArrayLikeFunction:1259-1271`, `AppendStringFunction:1273-1286`, `PrependArrayLikeFunction:1293-1306`, `PrependStringFunction:1308-1321`. `dup put` aliases.
- **`src/builtins/sli_container_ops.cpp:441-453`** — `CvdStringFunction` uses `std::to_string(double)` which is locale-dependent, 6-digit precision. Loss-of-info round-trip.
- **`src/builtins/sli_container_ops.cpp:163-183`** — `GetDictFunction` raises on missing key but doesn't pop the operand stack first; on raiseerror unwind, the dict + key remain.
- **`src/builtins/sli_io_ops.cpp:106-122, 124-145, 147-176, 284-302`** — `CvxFFunction`, `CloseStreamFunction` (close is implicit-on-destroy, deviates from PS), `EofStreamFunction` (eofbit timing subtlety), `StrFunction` returns empty silently for non-ostringstream backings.
- **`src/builtins/sli_startup.cpp:286-302`** — `EndFunction` refuses to pop when `DStack().size() <= 2`. With sli-init's `errordict` bringing the bottom count to 3, `end` would happily pop `errordict`.
- **`src/builtins/sli_startup.cpp:355-367`** — `locate_sli_init` only checks `<datadir>/sli/sli-init.sli`; if user sets `SLIDATADIR` already including the `sli/` subdir, lookup fails.
- **`src/builtins/sli_typecheck.cpp:107-109`** — `AddtotrieFunction` calls `trie->info(std::cerr)` on every successful insert. Spam.
- **`src/builtins/sli_typecheck.cpp:88`** — `for (Token *t = ad->end()-1; t >= ad->begin(); --t)` decrements past begin on the last iteration → UB on raw `std::vector::data()` pointer.
- **`src/builtins/sli_math.cpp:725-736`** — `Eq` requires identical typeid; `1 1.0 eq` returns false. May or may not match NEST 2.x semantics; verify.
- **`src/builtins/sli_math.cpp:1283-1288`** — `Round_dFunction` does `floor(val + 0.5)` rather than `std::round` — wrong for half-away-from-zero on negatives, fails for very large doubles where 0.5 < ULP.
- **`src/builtins/sli_math.cpp:462-492`** — `Pow_ddFunction`/`Pow_diFunction` only guard `0^negative`; `(-2.0)^0.5` silently returns NaN.
- **`src/builtins/sli_stack.cpp:286-323`** — `RestoreestackFunction`/`RestoreostackFunction`/`OperandstackFunction` do self-aliasing assignments through `i->top().data_.array_val`. Relies on `TokenArray::operator=` snapshotting the source before destroying the destination.

### Tests

- **`CMakeLists.txt:126-129`** — `test_sli_eval` is wired into CTest. Per CLAUDE.md and `tests/test_harness.h:22-26`, it does **not** call `startup()`, so `sli-init.sli` does not load — yet the test file exercises operators that only `sli-init.sli` registers (`==`, `def`, `if`, `<-`, `keys`, `values`, …). CLAUDE.md says "Slice 5c (next): the `1 1 add ==` smoke test passes" — meaning none of these tests should pass yet. Either the tests are currently broken or CLAUDE.md is stale.
- **`tests/test_harness.h:67-79`** — `eval` allocates `new std::istringstream` + `new SLIistream` on every snippet with no cleanup path; hundreds of leaks per run. ASAN would flag every one.
- **`tests/test_harness.h:46-50`** — `clear_stacks` clears OStack/EStack but not dictstack, so dict bindings persist between tests. Order-dependent.
- **`tests/test_array_module.cpp:328`** — Literal `1.0 / 0.0`. Constant-folded by clang; UB pre-C++20.
- No round-trip serialization test for Dictionary, TrieType, FunctionType, ProcedureType — exactly the types whose `serialize` overrides are missing.
- No cycle test, no `read_header` truncation test, no version-mismatch test.
- No CTest `TIMEOUT` — a hung dispatcher hangs CI indefinitely.

### Build

- **`CMakeLists.txt:96-100`** — `target_compile_options(sli3_core PRIVATE -Wall -Wextra -Wno-unused-parameter)` is **PRIVATE**, so test files don't inherit `-Wall -Wextra`. Test bugs (unused vars, sign comparisons) won't surface.
- **`CMakeLists.txt`** — no `install()` rule, but `SLI3_DATADIR` is set to the source tree (`${CMAKE_CURRENT_SOURCE_DIR}/lib`). An installed binary couldn't find `sli-init.sli`.
- **`config.h.in:10-13`** — `SLI3_VERSION_MAJOR/MINOR` substituted as bare ints, `SLI3_VERSION_PATCH` as quoted string. Inconsistent.

---

## MEDIUM — cleanup, dead code, correctness in edge cases

### Dead / commented-out code

- **`src/builtins/sli_builtins.cpp:79-92, 153-167, 205-222, 265-280, 322-330, 368-376, 378-386, 424-432`** — eight `backtrace` overrides whose entire body is a comment.
- **`src/builtins/sli_control.cpp`** — large commented blocks: `IiterateFunction::backtrace` (79-92), `IloopFunction::backtrace` (153-167), `IrepeatFunction::backtrace` (205-222), `IforFunction::backtrace` (265-280), forall variants (322-330, …), `StopFunction` debug code (319-342), `CloseinputFunction` (370-396), `DebugOnFunction`/`DebugOffFunction`/`DebugFunction` no-ops (1632-1688), `StartFunction` startup error message (1716-1722).
- **`src/interpreter/sli_interpreter.cpp:497-512`** — large `// if(command->gettypename()...)` block referencing legacy `TrieDatum`.
- **`src/interpreter/sli_interpreter.cpp:582-603`** — commented-out `M_FATAL` and exit-code lines.
- **`src/interpreter/sli_numerics.h:35-58`** — `#if 0` Taylor-series fallback for `expm1`.
- **`src/interpreter/sli_interpreter.cpp:298-301`** — three commented TrieDatum lines.
- **`src/interpreter/sli_interpreter.h:265`** — `step_mode()` always returns false; debug step-mode is dead. Code paths in `sli_builtins.cpp:406-413, 451-456` that depend on step_mode are unreachable but **broken** (dangling refs after estack push) if ever enabled.
- **`src/interpreter/sli_interpreter.h:26`** — `extern int signalflag` checked everywhere, written nowhere.

### Style / robustness traps

- **`src/types/sli_type.h:21-51`** — `intvectortype` and `doublevectortype` placed *after* `num_sli_types` with implicit successor numbering. If anyone sorts the enum, wire IDs renumber.
- **`src/parser/sli_charcode.h:33`** — `class CharCode : public std::vector<size_t>` — public inheritance from STL container with no virtual destructor.
- **`src/parser/sli_scanner.h:120`** — DFA transition table is a per-instance ~4.8 KB array, initialised in the ctor. Should be static.
- **`src/parser/sli_parser.cpp:38-60`** — `Parser::init` does `s = new Scanner(...)` but the class has no destructor → leaks one Scanner per Parser instance.
- **`src/parser/sli_scanner.cpp:241-246`** — backslash escape table only handles `\n \t \\ \( \)`. Missing `\r \" \0 \xHH \NNN`.
- **`src/parser/sli_scanner.cpp:121`** — `"` is in the alpha char group; `"foo"` parses as a name with embedded quotes, not a string.
- **`src/util/sli_exceptions.h:281`** — `StackUnderflow(int n, int g)` — `int` for stack sizes; truncated from `size_t` at `sli_interpreter.h:236`.
- **`src/builtins/sli_stack.cpp:52-146`** — `NpopFunction`, `IndexFunction`, `CopyFunction` all do implicit `long` → `size_t` truncation on negative inputs (then raise misleading "stack underflow").
- **`src/types/sli_type.cpp:13-46`** — `SLIType::clear` zeros without `remove_reference`; `pprint` default reads `data_.func_val` as `void*` for any type (wrong active union member but harmless on this platform).
- **`src/types/sli_type.cpp:32-36`** — `SLIType::execute` pushes to ostack then pops estack; if push throws, estack pop never runs.
- **`src/types/sli_iostreamtype.h:25, 65, 105`** — `executable_` set by mutation in derived ctor instead of passed through base ctor.
- **`src/containers/sli_array.{h,cpp}`** — many bounds checks missing on `insert`/`erase`/`assign`/`rotate`. Single-threaded so `static size_t allocations` (line 41) is fine.
- **`src/interpreter/sli_module.h:53, 58`** — `commandstring` and `install` declared in header, implementations live in `unported/sli_module.cpp` (not built). Latent link error if any concrete `SLIModule` subclass appears.
- **`src/interpreter/sli_function.h:32-34`** — default `SLIFunction` ctor leaves `name_` as the empty Name (handle 0 = `"0"`).
- **`src/interpreter/sli_numerics.cpp:26-29, 31-34, 36-42`** — `ld_round`/`dround` overflow on `x` near `LONG_MAX`; `dtruncate` no NaN guard.
- Many ops "raiseerror without estack pop" — relies on `raiseerror` to pop the frame. If anyone changes that contract, every such site becomes an infinite re-dispatch.

### Trie

- **`src/trie/sli_type_trie.cpp:62-83`** — `toTokenArray` caches the interpreter pointer as `static` on first call.
- **`src/trie/sli_type_trie.cpp:85-110`** — recursive `info` has no depth bound.
- **`src/trie/sli_type_trie.h:99, 140-146`** — `refs_` is unsigned; underflow on extra `remove_reference`. Recursive destructor can stack-overflow on deep tries.
- **`src/trie/sli_trietype.h:14-46`** — `clear` calls `remove_reference` (which already zeros) and zeros the same fields again.
- **`src/trie/sli_trie.h:1-71`** — header guard `SLI_TYPECHECK_H` mismatches the file name.

### Serialization

- **`src/serialize/sli_serialize.h:20`** — `kSerializeVersion` reader rejects both newer and older versions; reasonable, but no migration story.
- **`src/serialize/sli_serialize.cpp:65-86`** — object table has no per-object typeid record; theoretical address-reuse alias risk. No way to detect "clean end of file" vs "truncated mid-payload."
- **`src/types/sli_nametype.h:53-65`** — `OperatorType<typeid>` markers (iiterate, irepeat, ifor, iforall, …) inherit `LiteralType`'s serialize — the wire form encodes only the name string, not the typeid. Round-trip silently degrades operator markers to plain literals.

### Naming / readability

- **`src/parser/sli_parser.cpp:104-110`** — Error label `endarrayexpected` printed as "Closed bracket missing" but the failing token is a brace. Diagnostic mismatch.
- **`src/builtins/sli_typecheck.cpp:71-105`** — error message ostringstream lifetime: `message.str().c_str()` builds a temporary whose pointer dies before the variadic `i->message` call uses it. UB.
- **`src/util/sli_exceptions.cpp:115`** — missing space: `"argumentsthan"` in "Command needs more argumentsthan 3".
- **`src/util/sli_exceptions.h:184-192`** — class `SystemSignal` collides with `Name SystemSignal` in `SLIInterpreter` (sli_interpreter.h:522).

---

## Cross-cutting recommendations

1. **Add `override` everywhere** on the `SLIType` subclasses. One-line change; surfaces every silent-override typo in this codebase.
2. **Round-trip-every-typeid serialization test.** A single test that constructs a Token of every `sli_typeid`, serializes, deserializes, asserts identity. Will immediately surface the dict / function / procedure / trie gaps.
3. **Remove `static SLIType*` caches inside `execute()`.** The lookup is a single array index; the cache buys nothing and breaks on a second interpreter.
4. **Define `DICTSTACK_CACHE` (or remove it).** The current state — populate but never invalidate — is a guaranteed UAF on `undef`.
5. **Decide whether `raiseerror` is responsible for popping the e-stack** and apply uniformly. Today some ops pop before `raiseerror`, some after, some not at all.
6. **Add a parity test for `execute_` vs `execute_dispatch_`.** The fallback `IiterateFunction` etc. have diverged; tail-call optimization is in the dispatcher only.
7. **Bounds-check the `TokenStack` API** at least under `assert` — the current state is "everything is UB on empty/short stack, including `top()` which is the most-called function in the dispatcher."
8. **Fix `Token::operator=` self-assign**: `if (this == &t) return *this;` at the top.
9. **`Sleep_dFunction` and `UnitStep_iFunction` and `TypeFunction:302` and `NameType::execute:12`** are all immediate one-line fixes for the wrong-union-member family.
10. **`-Wall -Wextra` should be PUBLIC** (or at least applied to test targets), and consider `-Wself-assign`, `-Wfloat-equal`, `-Wshadow`. With current settings, tests compile under default flags.

---

## Coverage / what was not deeply reviewed

- `lib/sli/*.sli` — vendored NEST 2.20.2 startup scripts, treated as input; not reviewed for compatibility with the new C++ surface.
- `src/util/linenoise/*.c` — vendored, built with `-w`; out of scope.
- The 4 pre-existing unused-variable warnings called out in CLAUDE.md (`proc` in `sli_control.cpp`, `d` in `sli_scanner.cpp`) — confirmed present, low priority.
- Cross-reference between `sli-init.sli` and the C++ operator surface for missing operators — earlier slice notes (5b, 5c) suggest this is the active work.
