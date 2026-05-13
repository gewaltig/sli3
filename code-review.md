# sli3 code review

Living inventory of correctness / latent-bug items grouped by
severity. The historical CRITICAL/HIGH inventory from Stage 0–9 and
Slice 5/Axis I/II is retired — that record lives in `fix-plan.md`
and the git history (tag `stage9-complete` and beyond).

Last full pass: **2026-05-13** (five parallel reviewers across
interpreter / types+serialize / containers+parser / math+control+typecheck+stack /
array+IO+startup+trie+tests). 20 / 20 tests passing, build clean
on AppleClang 21 / C++17, under both `SLI3_INLINE_BODY_WALK` OFF
(default) and ON.

A first-pass cleanup landed the same day; see the trailing
"Just-fixed (2026-05-13 pass)" section.

Severity:
**CRITICAL** = will crash, corrupt, or silently produce wrong output today.
**HIGH** = latent bug, broken under common scenarios.
**MEDIUM** = correctness in edge cases / dead code / style traps that mask real bugs.

---

## At a glance

| Tier | Count | Direction |
|---|---|---|
| CRITICAL open | 0 | the lone `Cvt_aFunction` CRITICAL closed in the 2026-05-13 follow-up pass |
| HIGH open | ~3 | dispatcher-magic-constants / sli_main / SLIModule link gap |
| MEDIUM open | ~25 | cleanup, Stage 6 closure, style |

The 2026-05-13 first-pass cleanup closed: `RestoreostackFunction`
UAF, `DictionaryStack::clear()` `basecache_` leak, asymmetric
inline-name hot-op switch, `TypeNode::toTokenArray` static
interpreter cache, `UndefFunction` catch-all, plus the comment
and dead-code items listed in "Just-fixed" below.

The 2026-05-13 follow-up pass closed the lone CRITICAL
(`Cvt_aFunction`) plus 9 HIGH items: `RestoreStateFunction`
atomic restore, `CleardictFunction` dictstack-cache invalidation,
`WhereFunction` returns the actual containing dict (via new
`DictionaryStack::where(Name)`), typed math leaves got
`require_stack_type` guards, `execute(string)` routes through
dispatch mode, `execute_debug_` calls `raiseerror(exc)` + guards
exitcode lookup, `startup()` flips the flag unconditionally,
`terminate()` no longer self-deletes, and both dispatcher outer
switches gained a `case sli3::nulltype: pop; break;` guard.

---

## HIGH — open

### Dispatcher / interpreter core
- `src/interpreter/sli_interpreter.cpp:233-239` — `init_dictionaries`
  inserts type-name entries for `t_id < sli3::symboltype`. The
  threshold is a load-bearing magic constant; reordering
  `sli_typeid` silently changes which types appear in `systemdict`.
- `src/interpreter/sli_main.cpp:8-13` — ignores `engine.execute(2)`
  return code (always returns 0); no signal handler (`extern int
  signalflag` is read at four dispatcher sites, written nowhere);
  no argc/argv pass-through; no top-level try/catch.
- `src/interpreter/sli_module.h:53,58` + `src/interpreter/sli_interpreter.h:1064-1071`
  — `SLIModule::commandstring` / `install` and `addmodule<T>` are
  declared but defined only in `unported/sli_module.cpp` (not
  built). Any concrete `SLIModule` subclass triggers a link error.

### Stage 6 (serialization) gaps
- `src/types/sli_dicttype.cpp` — no `serialize`/`deserialize`
  override. Round-trip drops the dict body.
- `src/trie/sli_trietype.h` — `TrieType` has no override; the trie
  body is dropped on round-trip. Should re-resolve via dictstack
  lookup (mirror `FunctionType::serialize`).
- `src/types/sli_arraytype.cpp` — `ProcedureType` /
  `LitprocedureType` share `ArrayType::serialize`. The wire
  records typeid once at `write_token`. Cross-typeid aliasing
  (procedure + litprocedure + array sharing the same
  `TokenArray*`) silently coerces the second loaded Token's
  typeid to whichever variant was decoded first — silent
  corruption, not "drops the typeid".
- `src/types/sli_iostreamtype.cpp:53-66` — `serialize` writes no
  payload; `deserialize` reads no payload. Aliased stream Tokens
  load as two distinct closed-stream `SLIistream` heap objects
  (no Reader object-table registration).
- `src/serialize/sli_serialize.cpp:152-162` — `write_token`'s
  null sentinel uses the same 2-byte pattern as a hypothetical
  real `nulltype` Token. Can't disambiguate on the wire.
- `src/types/sli_type.cpp:13-17` — `SLIType::deserialize` base
  default leaves `data_` indeterminate rather than nulling it.
  The "null-payload" convention documented at `sli_type.h:106-115`
  is not enforced by the default.
- **Correction to historical review.** `OperatorType<typeid>`
  markers (iiterate/irepeat/ifor/iforall/quit/noop) *do*
  round-trip with typeid preserved (`write_token` writes the
  SLIType's `id_` field, which is the right enum slot). What's
  ambiguous is the marker's `name_val` payload — markers are
  typically constructed with `name_val=0`. The earlier "typeid is
  lost" framing in this file was incorrect; retire it.

---

## MEDIUM — open

### Dispatcher hot-path
- `src/interpreter/sli_interpreter.cpp:750-758` vs
  `:1336-1500` — when `SLI3_INLINE_BODY_WALK` is OFF, the inline
  dispatcher (~330 lines) is dead. When ON, the OFF-path body is
  dead. Half the dispatcher is unused in every build configuration
  until Axis I slice 8 step 4 lands (flip default + delete OFF
  path).
- `src/interpreter/sli_interpreter.cpp:870-885, 1396-1412` — the
  `SLI3_TRACE` env-var read + per-fn-call printout is duplicated
  verbatim across both dispatcher loops. Two copies to maintain.
- `src/builtins/sli_op_bodies.h` — `static inline` helpers in a
  header included from 4 TUs. Each TU gets its own copy. Use
  `inline` (no `static`) to allow ODR merging, or move bodies
  into a single .cpp.
- `src/interpreter/sli_interpreter.cpp:367-383` — `get_current_name`
  reads `current_op_` first, then falls back to the e-stack top.
  Fixes attribution for direct fn dispatches; the trie path
  (which doesn't write `current_op_`) still falls back to
  `interpreter_name`. Partial.

### Container / parser
- `src/containers/sli_dictstack.h:282-283` — `def` does
  `insert(n, t)` then `(**d.begin())[n] = t;` — second line
  re-runs `std::map::operator[]` and DictToken conversion-assignment.
  Hot path. Inefficient; behaviour correct.
- `src/parser/sli_scanner.h:120` — DFA `trans[lastscanstate][lastcode]`
  is a per-instance ~620-byte array. Make `static const`.
- `src/parser/sli_charcode.h:33` — `class CharCode : public
  std::vector<size_t>`. Compose, don't inherit.
- `src/containers/sli_name.h:165-171` — Name's `handleTableInstance_`
  / `handleMapInstance_` are Meyers singletons. Single-threaded
  per project; flag with a header comment if anyone revisits
  threading.

### Types / serialization
- `src/types/sli_arraytype.h:34`, `src/types/sli_iostreamtype.h:39, 79`
  — misleading `// This clears all fields to 0` comments on
  `clear` overrides that only zero one union member. Either
  drop the comments or memset.
- `src/types/sli_token.h:228-233` — `Token::move(Token&)` is
  callable as `t.move(t)`; move-ctor has no self-move guard
  (move-assign at `:260` does).
- `src/types/sli_nametype.cpp:19-24` — `LiteralType::compare`
  is inherited by `MarkType` / `OperatorType<…>` and compares
  `name_val`. Marker tokens with uninitialized `name_val`
  accidentally compare equal via `0 == 0`. Works for marker
  interchangeability; contract is accidental rather than
  intentional.

### Builtins / cleanup
- `src/builtins/sli_io_ops.cpp:121-142` — `CloseStreamFunction`
  is old-ABI; flip to new-ABI for uniformity with the rest of
  the file.
- `src/builtins/sli_container_ops.cpp:747-811` — `getinterval_a/s`
  rejects `idx == size && count == 0` (empty trailing interval).
  NEST 2.20.2 accepts. Minor compat gap.
- `src/builtins/sli_array_module.cpp:710-731` — `MapFunction` /
  `MapIndexedFunction` early-return on empty proc / src by
  leaving the source array on top. NEST 2.x returns a copy.
  Verify parity.
- `src/builtins/sli_container_ops.cpp:1426` — `CvaFunction` trie
  arm pushes `cva_t` on the e-stack but leaves the `cva` slot
  below; comment at `:1419-1423` admits the wrapper's `exch pop`
  cleanup is unhandled.
- `src/builtins/sli_startup.cpp:390-402` — `locate_sli_init` only
  probes `<datadir>/sli/sli-init.sli`. If `SLIDATADIR` already
  ends in `sli/`, lookup fails. Add a `<datadir>/sli-init.sli`
  fallback.

### Dead / commented-out code
- `src/builtins/sli_builtins.cpp:79, 153, 205, 265, 322, 368,
  378, 424` — eight `backtrace` overrides whose bodies are
  commented out. Header `sli_builtins.h:58-115` still declares
  them.
- `src/interpreter/sli_interpreter.h:265` — `step_mode()` always
  returns false; call sites in `sli_builtins.cpp:406-413,
  451-456` still contain (unreachable) dangling-reference UB.
- `src/interpreter/sli_interpreter.h:26` — `extern int signalflag`
  read at four dispatcher sites, written nowhere.

### Tests / coverage
- `tests/test_round_trip.cpp:90-109` — Dictionary, Procedure,
  Trie, operator markers (iiterate/irepeat/ifor/iforall/noop/
  quit), `litproceduretype` still xfail. Flip each as the
  Stage 6 gap closes.
- `tests/test_serialize.cpp` — missing: cycle test (array
  containing itself), version-mismatch, multi-token streams.
- `tests/test_two_interpreters.cpp:120` —
  `test_xfail_static_cache_after_destroy` no longer crashes
  (static `SLIType*` caches removed). Stale xfail framing.
  With the trie static cache now removed too, add a positive
  test that constructs a trie, calls `cva_t`, destroys the
  interpreter, builds a second one, and repeats.
- `tests/test_two_interpreters.cpp:46-58` (`prime_eval`)
  duplicates `test_harness.h::eval` instead of reusing it.

### Misc
- `src/util/sli_exceptions.h:182` + `src/interpreter/sli_interpreter.h:548`
  — `class SystemSignal` collides with `Name SystemSignal`
  member. Inside any `SLIInterpreter` method, `throw
  SystemSignal(SIGINT)` resolves to the `Name` member ctor. Latent
  — `signalflag` is dead so no throw site exists today.
- `src/types/sli_type.h:49-50` — `intvectortype` /
  `doublevectortype` enum slots placed **after** `num_sli_types`.
  `types_.resize(num_sli_types)` means any access via these IDs
  is out-of-bounds. Placeholders; not exercised today, but the
  presence is load-bearing for the typeid wire-contract.
---

## Notes / decisions baked in

- `::pop` binds to `IlookupFunction` (`sli_interpreter.cpp:247`).
  Matches NEST 2.20.2 (`interpret.cc`). No `IpopFunction` needed.
- `CurrentnameFunction::execute` pushes `false` by design (Stage
  4.9 documented divergence from NEST 2.x).
- `TokenStack::size()` returns capacity — documented in
  `sli_tokenstack.h:127-143`; matches NEST 2.x. Cosmetic rename
  to `capacity()` only.
- `recordstacks` defaults to `true` after sli-init.sli runs
  `init_errordict`; cost is ~0.04–0.33 µs per error per the B6
  bench family.
- The pre-Axis-I CRITICAL/HIGH inventory is closed; the historical
  detail lives in git (`stage9-complete` tag and earlier commits).

---

## Confirmed resolved on this pass

Items the previous review listed as open that the 2026-05-13 pass
verified fixed (lines verified at the noted file:line):

- `override` keyword applied throughout the `SLIType` hierarchy
  (`ArrayType`, `LitprocedureType`, `ProcedureType`,
  `DictionaryType`, `FunctionType`, `IntegerType`, `DoubleType`,
  `BoolType`, `IstreamType`, `XIstreamType`, `OstreamType`,
  `LiteralType`, `NameType`, `SymbolType`, `MarkType`,
  `OperatorType`, `StringType`).
- `operator long&/double&/bool&` removed from `Token`
  (`sli_token.h:79-85` only declares by-value conversions plus
  the two `operator <ref>&()` overloads for string and TokenArray).
- `FunctionType::serialize/deserialize` implemented and
  re-resolves by name (`sli_functiontype.cpp:33-63`).
- `Dictionary::info()` prints sorted (`sli_dictionary.cpp:70-77`).
- `add_dict` / `remove_dict` removed (declarations at
  `sli_dictionary.h:186-190` document removal).
- `Name(long)` bounds check gated on `!NDEBUG` (`sli_name.h:71-77`).
- `SLIistream::valid()` after close — there is no in-place
  `close()`; refcount handles teardown.
- `Token_sFunction` validates before popping
  (`sli_control.cpp:1570-1620`).
- `CvdStringFunction` uses `std::stod` (C-locale by default)
  (`sli_container_ops.cpp:447-470`).
- `CMakeLists.txt:106-111` — `sli3_warnings` INTERFACE library
  propagates `-Wall -Wextra` to test targets.
- `CMakeLists.txt:154` — `sli3_add_test()` applies `TIMEOUT 60`
  to every test.
- `tests/test_harness.h:81-94` — `eval()` no longer leaks
  `istringstream` / `SLIistream`.
- `raiseerror(Name)` vs `raiseerror(std::exception&)`
  inconsistency: both branch on `current_op_->uses_new_abi()`
  and pop the e-stack only for old-ABI ops
  (`sli_interpreter.cpp:392-394, :409-412`). Symmetric.
- `current_op_` field threaded through `get_current_name` and
  both `raiseerror` overloads.
- `TokenStack` bounds checks (`assert(load() > 0)` at
  `sli_tokenstack.h:56, 62, 69, 75, 81, 87, 95, 101`).
- Math UB on hot ops: `__builtin_*_overflow` helpers at
  `sli_math.cpp:45-70` + checked arms in Add/Sub/Mul/Div/Mod/Abs/Neg.
- Scanner `labs(LONG_MIN)` UB fixed; checked integer-literal
  arithmetic.
- `Modf_dFunction` captures by value (`sli_math.cpp:961`).
- `Round_dFunction` uses `std::round` (`sli_math.cpp:1709`).
- `UnitStep_dFunction` writes `0.0` for negatives
  (`sli_math.cpp:1638-1639`).
- `UnitStep_iFunction` reads/writes `long_val`
  (`sli_math.cpp:1650-1651`).
- `Sleep_dFunction` reads `double_val` (`sli_control.cpp:1546`).
- `IfFunction` / `IfelseFunction` type-check the bool
  (`sli_op_bodies.h:148`, `sli_control.cpp:193`).
- `AddtotrieFunction` index-based reverse loop, stringstream
  stored in local (`sli_typecheck.cpp:94-107`).
- `TypeFunction` calls `.toIndex()` (`sli_typecheck.cpp:314`).
- `Symbol_sFunction` has a real body (`sli_control.cpp:1659-1704`).
- `RollFunction` short-circuits `n==0` inside `TokenStack::roll`
  (`sli_tokenstack.h:110-125`); `RotFunction` safe via the same
  code path.
- `/or` registered to `orpolyfunction` (type-checked).
- `DictionaryStack::toArray` `add_reference` fix.
- `RaiseerrorFunction::execute` pops args + relies on dispatcher
  pre-pop (new-ABI marked, `sli_control.cpp:998-1001, :2162`).
- All 20 test source files have a matching `sli3_add_test`
  (`CMakeLists.txt:157-176`).

---

## Coverage / not reviewed in depth

- `lib/sli/*.sli` — vendored NEST 2.20.2 startup scripts.
- `src/util/linenoise/*.c` — vendored, built with `-w`.
- Cross-reference between operator surface and `sli-init.sli`
  bootstrap calls beyond what tests exercise.

---

## Just-fixed (2026-05-13 cleanup pass)

Closed in the same session as the deep review. All 20 ctest
targets green under both `SLI3_INLINE_BODY_WALK` OFF (default) and
ON.

HIGH-tier:
- `RestoreostackFunction` UAF (`sli_stack.cpp:311-323`) — buffer
  source `TokenArray` into a local before assigning over `OStack`.
- `DictionaryStack::clear_cache()` (`sli_dictstack.h:125-138`) —
  now also zeros `basecache_`, paired with the existing
  destructor that resets `base_ = nullptr`.
- Asymmetric inline-name hot-op switch
  (`sli_interpreter.cpp:1517-1530`) — added `HOP_ADD`, `HOP_SUB`,
  `HOP_IF`, `HOP_DEF` arms so name-resolved bodies don't fall
  through to virtual dispatch.
- `TypeNode::toTokenArray` static interpreter cache
  (`sli_type_trie.cpp:70`) — drop the static; read
  `type_->get_interpreter()` per call.
- `UndefFunction::execute` (`sli_container_ops.cpp:1491`) —
  narrowed `catch (...)` to `catch (UndefinedName&)`.

MEDIUM-tier:
- `TokenArray::insert/append/assign` self-aliasing UB guards
  (`sli_array.cpp:94-107`).
- `DictionaryStack::push(Token)` → `push(Token const&)`
  (`sli_dictstack.{h,cpp}`).
- `DictionaryStack::set_basedict()` — added
  `assert(!d.empty())`.
- `Parser::~Parser()` — added; deletes the owned `Scanner`.
- `SLIInterpreter` ctor (`sli_interpreter.cpp:118-119`) — moved
  `parser_ = new Parser()` to before `init()`.
- `SLIType::deserialize` / `SLIType::clear`
  (`sli_type.cpp:13-29`) — memset the union to zero so the base
  default matches the null-payload contract.
- `static const Name exitcode_name("exitcode")` in both
  dispatchers (`sli_interpreter.cpp:749, 1325`).
- `tests/test_round_trip.cpp:92` — `functiontype` flipped from
  xfail to expect-pass (the override has been implemented since
  commit `3a7edec`).
- Stale `or_bbfunction` comment removed (`sli_math.cpp:1458`).
- Duplicate `set_new_abi()` block removed (`sli_stack.cpp:408-423`).
- Stale `#include "sli_functiontype.h"` removed from
  `sli_iostreamtype.cpp`.
- `sli_op_bodies.h` docstring updated — control-flow helpers
  (`hot_op_if`) do push/consume e-stack frames as part of their
  semantics; the prior "helpers do NOT touch the e-stack" wording
  was misleading.
- `[[fallthrough]]` annotations on the two intentional
  fall-throughs in `sli_scanner.cpp` (`:687`, `:714`).
- `MapThreadFunction` empty-thread comment corrected
  (`sli_array_module.cpp:792-801`).
- Three dead commented-out blocks removed from
  `sli_interpreter.cpp` (legacy `TrieDatum` lines in
  `get_current_name`; the `// message(M_FATAL, ...)` placeholder
  in `execute_`; the commented-out exit-code lookup block).
- `sli_numerics.{h,cpp}` — deleted the unused `ld_round`,
  `dround`, `dtruncate`, and `numerics::expm1` (including the
  `#if 0` Taylor-series fallback). Kept `numerics::e`,
  `numerics::pi`.

---

## Just-fixed (2026-05-13 follow-up pass)

Closed the lone CRITICAL plus the remaining HIGH dispatcher /
runtime items from the prior pass. All 20 ctest targets green
on AppleClang 21 / C++17 (both `SLI3_INLINE_BODY_WALK` modes).

CRITICAL:
- `Cvt_aFunction` (`sli_typecheck.cpp:258-280`) — pops the
  literal name AND the array, walks the array body via new
  `TypeNode::from_token_array` (mirroring NEST 2.20.2's
  `newnode`), produces a fully-formed trie. Operator-authoring
  contract preserved: the array is walked into a fresh
  `TypeNode*` before either stack slot is mutated, so a
  malformed input throws cleanly.

HIGH:
- `RestoreStateFunction` (`sli_state_ops.cpp:92-114`) — reads
  both snapshots into local `TokenStack`s first; only assigns
  to live `OStack`/`EStack` after both succeed.
- `CleardictFunction` (`sli_container_ops.cpp:617-643`) —
  consults `is_on_dictstack()`; invalidates the dictstack's
  cache AND basecache entries for the dict's keys before
  calling `dict->clear()`.
- `WhereFunction` (`sli_container_ops.cpp:1456-1478`) — walks
  the dictstack top-down via new `DictionaryStack::where(Name)`
  (returns `Dictionary*` or nullptr); returns the dict that
  actually holds the binding, not always `DStack().top()`.
- Typed math leaves
  (`sli_math.cpp:734,755,777,798,816,832,854,873,889,911,926,954,986,1012,1033,1063,1094,1115,1141,1154`)
  — added `require_stack_type` to every typed leaf (`sin_d`,
  `asin_d`, `cos_d`, `acos_d`, `exp_d`, `log_d`, `ln_d`,
  `sqr_d`, `sqrt_d`, `pow_dd`, `pow_di`, `modf_d`, `frexp_d`,
  `ldexp_di`, `dexp_i`, `abs_i`, `abs_d`, `neg_i`, `neg_d`,
  `inv_d`). Bare-name invocation on the wrong type now raises
  `TypeMismatch` instead of misinterpreting the union slot.
- `execute(const std::string&)` (`sli_interpreter.cpp:622-636`)
  — routes through `execute_dispatch_()` instead of `execute_()`
  so C++-API consumers see the same TCO / hot-op-inlining /
  body-walk semantics as the REPL.
- `execute_debug_` (`sli_interpreter.cpp:717-758`) — calls
  `raiseerror(exc)` (previously commented out) so debug-mode
  exceptions reach `stopped` / `handleerror`; exitcode lookup
  guarded the same way as the main dispatcher.
- `startup()` (`sli_interpreter.cpp:281-300`) — always sets
  `is_initialized_ = true`; a no-op startup (e.g. sli-init.sli
  not found) is still "initialized" for `is_initialized()`.
- `terminate()` (`sli_interpreter.cpp:1741-1755`) — no longer
  does `delete this` before `std::exit`. Static destructors
  that touch the interpreter no longer chase a freed pointer.
- Nulltype sentinel guard
  (`sli_interpreter.cpp` outer switches in both `execute_dispatch_`
  and `execute_dispatch_inline_`) — `case sli3::nulltype:
  execution_stack_.pop(); break;` cleanly absorbs any stray
  sentinel leaked by a future old-ABI op that only pops its
  own self-slot.

New regression test:
- `tests/test_sli_eval.cpp` — trie round-trip
  (`/foo trie [/integertype] {1 add_ii} addtotrie cva_t cvt_a
   def 5 foo` → 6); polymorphic two-arm trie round-trips through
  cva_t/cvt_a and still dispatches both arms.

New API:
- `DictionaryStack::where(Name) → Dictionary*` (header inline).
  Replaces the dead `bool where(Name, Token&)` declaration that
  existed since the 2014 import.
- `TypeNode::from_token_array(SLIInterpreter*, Name, TokenArray)`
  (static factory) plus a private `build_node` recursive helper.
  Inverse of `toTokenArray`; throws `ArgumentType` on malformed
  input.
