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
| CRITICAL open | 1 | new (`Cvt_aFunction`); historical CRITICALs retired |
| HIGH open | ~10 | carryover + new container / restore-stack issues |
| MEDIUM open | ~25 | cleanup, Stage 6 closure, style |

The 2026-05-13 first-pass cleanup closed: `RestoreostackFunction`
UAF, `DictionaryStack::clear()` `basecache_` leak, asymmetric
inline-name hot-op switch, `TypeNode::toTokenArray` static
interpreter cache, `UndefFunction` catch-all, plus the comment
and dead-code items listed in "Just-fixed" below.

---

## CRITICAL — open

### `Cvt_aFunction` is broken
- `src/builtins/sli_typecheck.cpp:258-269` — Docstring promises
  `/name [array] cvt_a -> trie`. The body overwrites `top()`
  (the array) with a fresh trie token, leaving the literal name
  at `pick(1)`; the constructor `new TypeNode(name)` never walks
  the array, so the resulting trie is empty regardless. NEST
  2.20.2's `Cvt_aFunction` consumes both args, walks the array
  to populate the trie body, and pushes one result. Niche feature;
  if ever invoked from user code, silently corrupts state.

---

## HIGH — open

### Dispatcher / interpreter core
- `src/interpreter/sli_interpreter.cpp:619-626` — `execute(const std::string&)`
  (the C++-API string entry-point) routes through `execute_()`
  (plain mode), not the dispatch mode `sli_main` uses. C++-API
  consumers get different semantics from the REPL (no TCO,
  fallback iter cases).
- `src/interpreter/sli_interpreter.cpp:696-748` — `execute_debug_`
  swallows exceptions instead of calling `raiseerror` (line 723
  is commented out) and reads `status_dict_->lookup(Name("exitcode"))`
  unprotected at `:740` — guaranteed `UndefinedName` if exitcode
  is never set.
- `src/interpreter/sli_interpreter.cpp:284-292` — `startup()`
  only flips `is_initialized_` when the e-stack is non-empty at
  entry. A no-op startup (e.g. sli-init.sli not found) leaves
  `is_initialized() == false` forever.
- `src/interpreter/sli_interpreter.cpp:1705-1713` — `terminate()`
  does `delete this; std::exit(...)`. Static dtors that touch the
  interpreter run after free.
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

### Stack / restore-state UAFs
- `src/builtins/sli_state_ops.cpp:96-108` — `RestoreStateFunction`
  reads directly into the live OStack / EStack. If the second
  `read_token_stack` throws (truncated / corrupt snapshot), the
  operand stack is already overwritten and the e-stack is empty.
  Non-atomic. Buffer into local `TokenStack`s, swap on success.

### Dictionary stack / dictionary lifetime
- `src/builtins/sli_container_ops.cpp:625` + `src/containers/sli_dictionary.cpp:45`
  — `CleardictFunction` invokes `Dictionary::clear()` on a dict
  that may still be on the dictstack. The class even has
  `is_on_dictstack()` for exactly this case but it isn't consulted;
  cache / basecache pointers into the erased `TokenMap` nodes
  dangle. UAF the first time a cached name is re-looked-up.

### Operator surface (callable-by-name without type check)
- `src/builtins/sli_container_ops.cpp:1468-1469` — `WhereFunction`
  pushes `DStack().top()` (the topmost dict), not the dict that
  actually holds the name; the comment admits this.
  `DictionaryStack::where(Name, Token&)` is declared at
  `sli_dictstack.h:248` but **never defined** — dead declaration.
- `src/builtins/sli_math.cpp:1154-1165` + `:2004` — `Inv_dFunction`
  registered as bare `/inv` with no poly wrapper and no trie
  (grep `lib/sli/typeinit.sli` confirms). Reads `data_.double_val`
  unconditionally — `2 inv` reinterprets `long_val=2` as a
  double. Same exposure on every typed leaf registered at
  `:1980-1997` (`sin_d`, `cos_d`, …, `pow_dd`, `pow_di`): low
  risk because users call the trie-bound names, but the typed
  leaves are callable.

### Body-walk inline (Axis I slice 8) — soft footguns
- `src/interpreter/sli_interpreter.cpp:1498-1505, 1522-1529` —
  under `HOP_NONE && !uses_new_abi()`, the dispatcher pushes a
  `nulltype` sentinel and `goto resume_iter`. Today's surviving
  old-ABI ops all clean the sentinel as a side-effect of wholesale
  `EStack().pop(n)` or `EStack().swap`. A future old-ABI op that
  only pops its own self-slot would leak the sentinel; the outer
  switch's `default: top().execute()` then runs `SLIType::execute`
  on the nulltype, which `push(t); EStack().pop()` — depositing
  a nulltype Token on the operand stack. Add either
  `case sli3::nulltype: execution_stack_.pop(); break;` in the
  outer switch or an `assert(execution_stack_.top().tag() !=
  sli3::nulltype)` at the resume_iter default arm.

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
