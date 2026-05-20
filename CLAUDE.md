# CLAUDE.md

Context for working in this repo. Read this before making changes.

## What this is

`sli3` is a from-scratch C++ rewrite of NEST's SLI interpreter (a PostScript-style stack language used to script the NEST neural simulator). The goal is a **faster, more memory-efficient** standalone SLI interpreter with language semantics compatible with NEST 2.x, but without the neural-simulation part.

SLI was first written in 1996 — much of the existing scaffolding only
existed because pre-C++98 stdlib was unreliable. We don't need it now.

Original work: Marc-Oliver Gewaltig, 2014–2015. Paused June 2015. Revived May 2026.

## Project decisions (see `implementation_spec.md` for full record)

- **Scope:** standalone SLI; not a NEST drop-in. Most of `sli-init.sli` should run.
- **Reference version:** NEST 2.x (2015-era). `sli-init.sli` is vendored into this repo.
- **Concurrency:** single-threaded. `HAVE_PTHREADS` paths to be removed.
- **C++ standard:** C++17.
- **License:** GPL.
- **Tests:** bare CTest, assert-based. No external test framework.
- **`sli_processes`:** deferred (will move to `unported/`).
- **`iteratortype` / `forall_iter`:** to be removed. The iterator protocol
  (a polymorphic cursor over arbitrary container types — used in NEST
  2.x for `forall_iter`, `size_iter`, `Map_iter`, the
  `[/iteratortype] { {} Map }` branch of `cva`) is beyond the core
  language. The enum slot, the dead `Forall_iterFunction` stub, and
  the matching `addtotrie` lines in `lib/sli/typeinit.sli` /
  `lib/sli/mathematica.sli` are removed. The SLI-defined
  `/trieheads_iter` helper (despite the name) operates on a regular
  array, not iteratortype, and stays.

## Status

Work is on the `revive` branch.
Previously done work is described in ChangeLog.md

- **Bench standing** (best-of-five, latest `bench/run.sh`, vs gs 10.07 / nest 2.20). Reproduce with `bench/summary.sh --md`; full history lives in `bench/results.db`.

| Bench | sli3 | gs | nest | sli3 vs gs |
|---|---:|---:|---:|---:|
| B1   `1 pop`           | **0.96** | 1.29 | 1.94 | **−26 %** ⬇ |
| B2   `1 1 add pop`     | **1.72** | 2.66 | 4.31 | **−35 %** ⬇ |
| B2b  bound `{...}`     | 1.60 | **1.21** | 3.34 | +32 % ⬆ |
| B3   nested for        | **1.47** | 2.53 | 4.22 | **−42 %** ⬇ |
| B5   dict alloc+lookup | **1.11** | 2.44 | 4.26 | **−55 %** ⬇ |
| B7   bubble sort       | **1.81** | 1.98 |  —  | **−9 %** ⬇ |
| B8   insertion sort    | **0.96** | 1.00 |  —  | **−4 %** ⬇ |
| B9   recursive fib(28) | **1.72** | 1.76 | 4.34 | **−2 %** ⬇ |
| B10  matmul 50×50      | **1.68** | 1.94 |  —  | **−13 %** ⬇ |

  Score vs gs: **8 wins, 1 loss** (run 36, commit `0a927a5` — array-arm rename for /add /sub /mul /div). sli3 beats nest 2.0–3.0× across the board. Run 32 (`2b8f104`, regex/library/mathematica auto-load) wrapped /add /sub /mul /div in a defensive trie so mathematica.sli's array arms (`/add [/arraytype ...] /add_a_a load def`) wouldn't shadow the scalar C++ leaf. That trie cost ~10 ns per dispatch and regressed B2 1.65 → 2.69, B2b 1.54 → 2.55, B3 1.50 → 2.39. Run 36 reverses by renaming the array arms to /Add /Sub /Mul /Div in mathematica.sli (Mathematica convention — `+ - * /` stay scalar; capitalised forms are elementwise). /add and friends are direct functiontype leaves again, B2/B2b/B3 are back at run-31 levels, and B7/B8/B9/B10 actually improve below their run-31 best because they use add/mul inside their bodies and were paying the trie tax too. Bisected via `bench/_bisect.sh` (commit `5b8971a`). The previously best run was run 31 (commit `20b9a1b` — pool allocator); the underlying allocator wins (B5 −55 %, dict-scope code) are still in force. Two coupled allocator changes: (a) class-static `Pool` on `Dictionary` / `TokenArray` / `SLIString` via per-class `operator new` / `operator delete` (pools the heap-allocated headers, NEST-faithful); (b) `TokenMap` now uses `std::pmr::map` with a class-static `unsynchronized_pool_resource` (pools the std::map RB-tree nodes). The pmr route keeps the std::map template instantiation uniform across every TU that includes `sli_dictionary.h` — no per-TU template ripple, no regression on dispatcher-heavy benches. (An earlier attempt with `PoolAllocator<T>` as the std::map allocator template parameter — commit `c6c0170` — won bigger on dict-scope but regressed B1/B2/B2b/B3 by 6–15 % due to template propagation; the pmr-based variant captures most of the win without the cost.) Dictionary header grew 40 → 48 B for the pmr allocator pointer; the dictstack save/restore microbenchmark stays at 0.45 s.

  The prior cleanup (Phase 5, May 2026, commits `fa93310`..`59d0ee8`) flipped B7/B8 from losses to wins by converting all 11 iter-helper SLIFunction classes (loop, repeat, for, forall, forallindexed, parse, lookup, parsestdin, iterate) to TYPE markers handled inline by `body_walk`'s `body_exhausted` switch (`88770bb`, `168d015`); demoting the C++ Map family to pure SLI (`4c2bd10`); migrating all remaining old-ABI ops to new-ABI and retiring the `uses_new_abi()` switch (`242d2fb`); adding a native C++ `/bind` ~50× faster than the SLI loop it replaced (`a765314`); and removing the dead `call_depth_` / `step_mode` machinery (`59d0ee8`). B8 dropped from 1.39 to 0.95 s alone — the iter-helper conversion was a vindication of the whole cleanup arc.
- Full plan in `implementation_spec.md`.

## Build

```sh
cmake -S . -B build
cmake --build build -j
./build/sli3                        # boots, dumps systemdict, exits
ctest --test-dir build              # run unit tests
```

CMake ≥ 3.20, C++17, default Release. Per-target warnings; `-Wno-unused-parameter` because the original code ships with many.

The build produces these targets:

- `sli3_core` — static library with all interpreter sources
- `sli3` — main binary (links `sli3_core` + `sli_main.cpp`)
- ~20 `test_*` binaries linking `sli3_core` (see `tests/`)

When adding a new test: drop `test_<thing>.cpp` next to the others, add a short `add_executable` + `target_link_libraries` + `add_test` block to `CMakeLists.txt`. No external test framework — bare assertions per Q8.

## Key architectural choices

- **Token is 16 bytes**: `SLIType*` + a POD `union value`. No vtable per datum — virtual dispatch lives on the `SLIType` object instead. Verified at runtime: `Token: 16`, `Dictionary: 32`.
- **Typeid lives in the top byte of `Token::type_`.** `Token::tag()` is a pure shift; `SLIType` no longer carries a redundant `id_` field (kept only under `SLI3_NO_PTR_TAG` for sanitizer builds where pointers stay plain). `SLIInterpreter::types_[]` stores pre-tagged pointers from registration; `Token::pack_type(raw, id)` is the single tagging site, called from `init_types`; `Token::unpack_type` masks for `delete`.
- **Reference counting on the type, not the token.** `SLIType::add_reference(Token&)` / `remove_reference(Token&)` — pointer-payload types (arrays, strings, dicts) override these; scalar types are no-ops.
- **Dispatch is a single `switch` on `type_id`** in `SLIInterpreter::execute_dispatch_` (`sli_interpreter.cpp:696`). Hot opcodes — int/double/name/procedure/repeat/for/forall — are inlined into one big switch so the compiler can build a jump table. Some `case` arms use `goto` deliberately to keep the loop body tight; **don't "clean these up" without tests.**
- **Single execution mode**: `execute_dispatch_`. The pre-Phase-1 `execute_` (plain) and `execute_debug_` alternates have been retired; `sli_main.cpp` invokes `execute_dispatch_` directly. The legacy `int` parameter on `execute()` is kept for source compatibility but ignored.
- **Type-economical SLIType polymorphism.** Many entries in the `sli_typeid` enum share the same underlying payload and differ only in execution semantics. The 16-byte Token holds the payload once; the `SLIType*` discriminates behaviour. Examples:
  - `proceduretype` / `litproceduretype` / `arraytype` — all `TokenArray*` payload. Literal procedures don't execute when moved to the estack.
  - `nametype` / `literaltype` / `symboltype` — all `Name` handle.
  - `istreamtype` / `xistreamtype` / `ostreamtype` — all stream pointers.
  - `iiteratetype` / `irepeattype` / `ifortype` / `iforalltype` / `quittype` — operator markers, no payload.

  Don't introduce a concrete class per semantic variant; reuse storage, dispatch through `SLIType`.
- **Procedure type-checking via `type_trie`** (`sli_type_trie.{h,cpp}`, `trietype`). SLI's main extension over PostScript. Part of the dispatcher; not optional. Each `TypeNode` stores an `unsigned int tag_` (sli3::nulltype = unset); `lookup` compares `Token::tag()` to `tag_` and treats `sli3::anytype` as the wildcard.
- **Composite access state** (`sli_access.h`, four values: `ACCESS_UNLIMITED → READONLY → EXECUTEONLY → NOACCESS`). One-byte `access_` field on `TokenArray` / `Dictionary` / `SLIString`. Monotonically narrowing via `set_access`. All gates go through inline helpers in `src/builtins/sli_access_check.h` — `require_writable` raises `WriteProtected`, `require_readable` raises `InvalidAccess` (separate error names so `errordict /errorname` distinguishes read vs write denials; both correspond to PS's single `invalidaccess`). Write gates cover every put / def / append / prepend / reserve / cleardict / undef / Set site, including the leaf of `arr [path] val put`. Read gates cover every get / keys / values / cva / known / clonedict / join / search / insert / replace / erase / getinterval / insertelement / cvi_s / cvd_s / eq-on-strings site, and path-put descent (Wave 3). Print and pprint emit `--nostringval--` on non-readable composites. Length is intentionally ungated (no content leak, and the dictstack-snapshot workflow needs it on a readonly array). `DictionaryStack::snapshot()` flips the cached array readonly so `dictstack` consumers can't corrupt the shared cache. SLI-callable setters/predicates: `/readonly /executeonly /noaccess /rcheck /wcheck /xcheck`.
- **Dictstack layout** (top → bottom after `init()` and after `cleardictstack`): `userdict` → `globaldict` → `systemdict`. `globaldict` is the PS Level-2 global namespace for persistent shared state. The 3 entries are protected: `end` raises `KernelError` (PS `dictstackunderflow`) when called with `size <= 3`. `cleardictstack` (via `SLIInterpreter::reset_dictstack`) drops everything and re-pushes the canonical layout; it is also the recovery primitive used by `/reset` and the error machinery. **`systemdict` is sealed `readonly` by the last command in `sli-init.sli`** (`systemdict readonly pop`). The previously-leaked NEST 2.x habit of mutating systemdict at runtime (`/tic /toc /clic /cloc /addpath /setpath /:warnings /scripterror`) was migrated to globaldict — the per-procedure `systemdict begin ... end` blocks in `lib/sli/sli-init.sli` now read `globaldict begin ... end`, and the initial defs of `:cliccycles / :clicclocs / :warnings / SLISearchPath` are wrapped in their own `globaldict begin ... end`. New system-level operators must be defined **above** the `systemdict readonly pop` line in `sli-init.sli`.
- **Memory-locality goal.** A procedure is one allocation holding N contiguous Tokens — not N tiny heap allocations behind pointers (the NEST 2.x failure mode). Don't undo this when modernizing containers.
- **Serialization is a first-class concern.** Every `SLIType` subclass implements `serialize(Token const&, Writer&)` / `deserialize(Reader&, Token&)`. The `sli_typeid` enum is a **permanent wire contract** (append-only). Pointer-payload types use the Writer/Reader object table for de-duplication and cycle handling. `Name` serializes as string + re-interns on load. `SLIFunction*` and trie tokens re-resolve by name on load. Streams are not serializable. Both binary (primary) and text (debugging) writers via the same `Writer`/`Reader` interface.

## File map

Sources are grouped by concern under `src/<area>/`. Every subdir is a
PUBLIC include path on `sli3_core`, so `#include "sli_token.h"` keeps
working regardless of where the includer lives.

| Subdir | Contents |
|---|---|
| `src/types/` | Core types (`sli_token.{h,cpp}`, `sli_type.{h,cpp}`) and per-typeid implementations (`sli_{integer,string,array,name,dict,function,iostream}type.{h,cpp}`). Double, bool, literal, mark and other discriminator-only types live inside `sli_type.cpp`. |
| `src/containers/` | Backing storage referenced from Tokens: `sli_array.{h,cpp}` (`TokenArray`), `sli_dictionary.{h,cpp}`, `sli_dictstack.{h,cpp}`, `sli_tokenstack.{h,cpp}`, `sli_string.h` (`SLIString`), `sli_iostream.{h,cpp}` (`SLIistream`/`SLIostream`), `sli_name.{h,cpp}` (Name interning). |
| `src/parser/` | `sli_scanner.{h,cpp}`, `sli_parser.{h,cpp}`, `sli_charcode.{h,cpp}`. |
| `src/interpreter/` | `sli_interpreter.{h,cpp}`, `sli_main.cpp`, `sli_module.h` (SLIModule base; declared but no concrete subclass yet), `sli_function.h` (SLIFunction interface), `sli_numerics.{h,cpp}`. |
| `src/builtins/` | All registered C++ operators. `sli_builtins.{h,cpp}`, `sli_control.{h,cpp}`, `sli_math.{h,cpp}`, `sli_stack.{h,cpp}`, `sli_typecheck.{h,cpp}` for the core surface; `sli_array_module.{h,cpp}` for the array stdlib (Range / Reverse / Rotate / Flatten / Sort / Transpose / Partition / arrayload / arraystore / GetMin / GetMax / valid_a / finite_q_d / Map / MapIndexed / MapThread); `sli_container_ops.{h,cpp}` for length / get / put / join_s / search_s / cvi_s / cvd_s / known / where / undef; `sli_io_ops.{h,cpp}` for ifstream / ofstream / cvx_f / close / eof / getline_is; `sli_startup.{h,cpp}` for statusdict / evalstring / cin / cout / cerr / `<-` / `<--` / endl / flush / begin / end / dict / currentdict and the sli-init.sli locator. |
| `src/trie/` | `sli_trie.h`, `sli_trietype.h`, `sli_type_trie.{h,cpp}` — the trie-based operator dispatch. |
| `src/serialize/` | `sli_serialize.{h,cpp}` — `Writer` / `Reader` abstract, `BinaryWriter` / `BinaryReader`, `write_token` / `read_token` entry points. |
| `src/util/` | `sli_exceptions.{h,cpp}`, `compose.hpp`. |
| `tests/` | Bare-assertion CTest binaries (~20). Highlights: `test_sli_eval` (SLI source through dispatcher via `test_harness.h`), `test_dispatch_parity` (plain vs dispatch mode equivalence), `test_dispatch_abi` (new-ABI pre-pop contract — see Axis I bundle), `test_errors_dispatch` (error attribution), `test_state_ops` (savestate/restorestate round-trip), `test_array_module`, `test_serialize`, `test_array`, `test_round_trip`. |
| `lib/sli/` | Vendored NEST 2.20.2 startup scripts: sli-init, typeinit, misc_helpers, library, ps-lib, FormattedIO, debug, helpinit, mathematica, oosupport, regexp, arraylib. |

### `unported/` — out-of-scope or NEST 2.x-only

Reference for what existed in the 2015 tree, not built. Two flavours:

1. **Out-of-scope but useful later:** `sli_processes.{h,cpp}` — POSIX
   `fork`/`exec`/`waitpid` wrappers, deferred per Q4.
2. **Dead-on-disk reference:** files using the legacy `Datum*` /
   `LockPTR` / `*Datum` API that don't match the modern core. Kept as
   documentation of what operators existed; the equivalents are being
   rewritten in fresh files in the project root.
   - `sli_io.{h,cpp}` — full I/O surface. Slice 6 rewrites as a thin
     layer over `std::fstream` / `std::stringstream`. The minimal
     subset already lives in `sli_io_ops.{h,cpp}`.
   - `sli_fdstream.{h,cpp}` — custom POSIX-fd `streambuf`. Replaced
     wholesale by `std::fstream` per Q4; will be deleted, not ported.
   - `sli_module.cpp` — `SLIModule::install` / `commandstring`
     definitions. Header stays in the build (declares the base class
     `SLIInterpreter::addmodule` accepts), but no concrete subclass
     exists, so the .cpp's symbols are not needed at link time. Also
     has a typo (`sli3::std::string`) that breaks compilation.
   - `sli_tokenutils.{h,cpp}` — `getValue<T>` / `setValue<T>` over the
     legacy `Datum*` abstraction. Modern code uses direct field
     access (`token.data_.<field>_val`) instead.
   - `sli_array_functions.h` — header for the legacy 2k-line array
     stdlib. Replaced by `sli_array_module.{h,cpp}` (Slice 4).
   - `sli_aggregatetoken.h` — NEST 2.x templated aggregate-token
     wrapper. The 16-byte `Token` + `SLIType` polymorphism covers the
     same job; nothing in the modern build references it.
   - `sli_config.h` — autoconf-style `@FOO@` placeholder header.
     Replaced by `config.h.in` + CMake `configure_file()` (Slice 5a).
   - `test_dictionary.cpp`, `test_token.cpp` — original 2015 tests
     against the legacy API. Replacements built incrementally
     (`test_array.cpp`, `test_serialize.cpp`, etc.).

   See `unported/README.md` for the full per-file rationale.

## Important: don't port NEST-era code line-by-line

When rewriting `sli_array_module.cpp`, `sli_io.cpp`, `sli_fdstream`,
`sli_startup.{h,cpp}`, etc. for use under sli3: the runtime model already
changed (16-byte value `Token` with type-side refcount, instead of NEST's
`Datum*` + `LockPTR`). These files are **reference for what operators
exist**, not implementation to translate.

- Containers (`TokenArray`, `Dictionary`, `TokenStack`): rewrite using
  `std::vector<Token>`, `std::unordered_map` where it helps. **TokenArray
  done in Slice 2.**
- String / stream wrappers: intrusive-refcount wrappers around
  `std::string` / raw stream pointers. **Done in Slice 3** — `sli_lockptr.h`
  and `sli_lockobj.h` are deleted. The `SLIType::add_reference /
  remove_reference` virtual protocol is the public refcount interface;
  implementations are intrusive `uint32_t refs_` on the heap object.
- Stdlib operators (Map/Sort/Reverse/Partition/etc.): use `std::sort`,
  `std::reverse`, `std::transform`, range-for. Don't translate the
  `Datum*` loops. **Done in Slice 4** — `sli_array_module.{h,cpp}`.
- I/O: build on `std::fstream`/`std::stringstream`. Don't carry forward
  `sli_fdstream`.
- The custom `sli_allocator` pool is **gone (Slice 1)**; default heap
  for everything. Reintroduce `std::pmr` only if profiling justifies.


## Stack-handling discipline (operator authoring contract)

Operators **must not modify either stack until they are guaranteed to succeed**. PostScript's `stopped`/`stop` recovery model depends on this: when `raiseerror` fires, the stacks should still reflect the call site so that `$errordict /ostack` and `/estack` snapshots (and the dispatcher's unwind) work. The convention:

- **Validate first.** `require_stack_load(N)`, `require_stack_type(...)`, range checks — all before mutating anything.
- **Peek operands, don't pop them.** Use `i->pick(n)` / `i->top()` to read; only call `i->pop(N)` once all checks pass and the work is done.
- **Pop self from estack on the success path only.** Most operators end with `i->EStack().pop()` after their work; on the error path they `raiseerror(...)` and return, leaving themselves on the estack — `raiseerror` will pop them.
- **For pointer-payload Tokens constructed by raw field assignment**, call `add_reference()` after setting `data_.<kind>_val`. Without it, the local's destructor decrement is unbalanced and you'll get a use-after-free once the value is shared. See `DictionaryStack::toArray` for the canonical fix (`sli_dictstack.cpp:158`). Prefer `new_token<sli3::xtype>(payload)` factories where they exist.
- **`raiseerror(Name, Name)`** (the C++ entry point, `sli_interpreter.cpp:410`) does *not* pop the calling operator from the estack. The single-arg `raiseerror(Name)` overload does. SLI-callable `RaiseerrorFunction` must therefore pop its args from ostack AND pop itself from estack before delegating (matches NEST 2.x `slicontrol.cc:1187`).

These rules are not just style — violating them produced two ~latent bugs that surfaced during the recordstacks investigation (see Decisions log in `implementation_spec.md`, 2026-05-11): a UAF on `systemdict` after three errors, and quadratic raiseerror cost under heavy error loops. Both are fixed; the contract above is the prophylactic.

## Known issues / gotchas

- `Token::operator std::string&()` is the only string-conversion operator (the const-by-value form was removed in Stage 2). Callers needing a `std::string&` from a Token use `static_cast<std::string&>(token)` or `token.operator std::string&()`.
- Streams are not serializable. `IstreamType` / `OstreamType::serialize` emit a stderr warning and write no payload; deserialize produces a Token whose underlying `SLIistream`/`SLIostream` has `valid() == false`. Per design — OS resources don't survive a snapshot.
- The `sli_typeid` enum is a **permanent wire contract** (append-only). Renumbering an existing entry breaks every saved snapshot ever made. Adding a new type: append at the end and bump `kSerializeVersion`.
- `recordstacks` defaults to `true` after `sli-init.sli` runs `init_errordict` (`lib/sli/sli-init.sli:217`), not `false` as the C++ default at registration time (`sli_control.cpp:2072`) suggests. Every `raiseerror` therefore snapshots ostack/estack/dstack into `$errordict` unless explicitly disabled. Cost is ~0.04–0.33 µs per error depending on depth — see `bench/sli/B6*.sli`.
- **Operator ABI: uniform pre-pop.** The dispatcher pre-pops the fn slot before `fn->execute`; the operator body must not pop its own slot. Phase 5 retired the old-ABI / new-ABI split — every op is new-ABI, the per-instance `set_new_abi()` opt-in is gone, and `raiseerror` no longer branches on `uses_new_abi()`. The dispatcher's "pre-pop only on success" contract still applies: on `raiseerror`, the body returns with operands intact so the error machinery can snapshot the stacks. See Axis I bundle (commit `151e5e5`) for the original conversion and `tests/test_dispatch_abi.cpp` for the regression test.

## Reference NEST 2.20.2 checkout

A shallow checkout of upstream NEST 2.20.2 lives at
`/Users/gewaltig/Code/nest-2.20.2/` (sibling to this repo, NOT
inside the project tree). Use it to cross-check operator
semantics, dispatch ordering, print/pprint conventions, and
typeinit wiring against the canonical reference.

Most relevant files:

- `nest-2.20.2/sli/` — interpreter sources (interpret.cc,
  slicontrol.cc, slimath.cc, slibuiltins.cc, sliarray.cc,
  dictstack.cc, arraydatum.cc, stringdatum.cc, …).
- `nest-2.20.2/lib/sli/` — the original `*.sli` startup scripts
  (sli-init.sli, typeinit.sli, mathematica.sli, …) that the
  vendored copies under `lib/sli/` here track.

When the agent reviews caught "the C++ does X but typeinit wires
the trie expecting Y", checking the upstream version is usually
how to disambiguate. Several past divergences are documented in
git history (e.g. NEST 2.20.2's `::pop` also points to
`ilookupfunction`; `case` also leaves the obj on the operand
stack rather than executing it; `cvi_s` / `cvd_s` are
string-input ops, not string-output).

The checkout has no git history (depth 1). If a deeper inspection
is needed, replace it with a full clone:
`git clone --branch v2.20.2 https://github.com/nest/nest-simulator.git nest-2.20.2`.

## When working in this repo

- Don't touch the `goto`-driven loops in `execute_dispatch_` without a parity test in place. They are deliberate.
- Both Release and ASAN builds compile clean on `-Wall -Wextra` (the four pre-existing unused-variable warnings — `proc` in `sli_control.cpp`, `d` in `sli_scanner.cpp` — were retired during the Stage 5 cleanup). `-Werror` is feasible whenever you want to flip it on.
- Don't add features to the C++ side that should live in `sli-init.sli`. The split-of-responsibility matters for performance.
- The `.o` files, `sli3` binary, `build/`, `CMakeFiles/`, and `*~` backup files are gitignored. Don't commit them.
- New `SLIType` subclasses **must** override `serialize` / `deserialize`. The base class default is no-op — fine for marker types, silently wrong for any type with payload.
- Pointer-payload types must use `Writer::intern_object` / `Reader::lookup_object` for shared identity. See `ArrayType::serialize` and `StringType::serialize` for the pattern.
- Run `ctest --test-dir build` before claiming a slice is done.
