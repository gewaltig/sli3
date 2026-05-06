# CLAUDE.md

Context for working in this repo. Read this before making changes.

## What this is

`sli3` is a from-scratch C++ rewrite of NEST's SLI interpreter (a PostScript-style stack language used to script the NEST neural simulator). The goal is a **faster, more memory-efficient** standalone SLI interpreter with language semantics compatible with NEST 2.x — the neural-simulation surface is out of scope.

Original work: Marc-Oliver Gewaltig, 2014–2015. Paused June 2015. Revived May 2026.

## Project decisions (see `implementation_spec.md` for full record)

- **Scope:** standalone SLI; not a NEST drop-in. Most of `sli-init.sli` should run.
- **Reference version:** NEST 2.x (2015-era). `sli-init.sli` is vendored into this repo.
- **Concurrency:** single-threaded. `HAVE_PTHREADS` paths to be removed.
- **C++ standard:** C++17.
- **License:** GPL.
- **Tests:** bare CTest, assert-based. No external test framework.
- **`sli_processes`:** deferred (will move to `unported/`).

## Status

Work is on the `revive` branch.

- **Stage 1 (build cleanup):** done. Builds clean on AppleClang 21 / C++17, zero errors.
- **Stage 2 (strip non-core deps):** done. All `HAVE_PTHREADS` / `HAVE_MPI` / `HAVE_GSL` / `HAVE_MUSIC` / `HAVE_SSTREAM` / `HAVE_EXPM1` / `HAVE_M_E` / `HAVE_M_PI` / `HAVE_CMATH_MAKROS_IGNORED` paths gone. `sli_processes.{h,cpp}` moved to `unported/`. Build clean, only 4 pre-existing unused-variable warnings.
- **Stage 3 (modern-C++ rewrite of containers + stdlib):** in progress.
  - **Slice 1:** `sli_allocator.{h,cpp}` deleted; default heap. Sizes unchanged.
  - **Slice 1.5:** Serialization protocol scaffolded — `Writer` / `Reader` abstract + `BinaryWriter` / `BinaryReader` with object table for shared-pointer de-duplication. `serialize` / `deserialize` virtuals on `SLIType`. Per-type implementations for scalars (Integer, Double, Bool) and Name/Literal/Symbol/Mark.
  - **Slice 2:** `TokenArray` rewritten over `std::vector<Token>` + intrusive `uint32_t refs_`. Token gained proper noexcept move ctor / move-assign. `ArrayType::serialize` / `deserialize` use the object table for de-duplication; round-tripped aliasing preserved. Bug fix: `ArrayType::references()` was decrementing.
  - **Slice 3:** `SLIString` no longer inherits `std::string` (composes); `SLIistream` / `SLIostream` rewritten as plain intrusive-refcount wrappers. `sli_lockptr.h` and `sli_lockobj.h` deleted. `IstreamType` / `OstreamType` got missing refcount overrides (silent leaks fixed). String type serializes via the object table; stream types emit a stderr warning and produce closed-stream Tokens on load.
  - **Slice 4:** Fresh `sli_array_module.{h,cpp}` (replaces the legacy 2288-line file). Registers Range / Reverse / Rotate / Flatten / Sort / Transpose / Partition_a_i_i / arrayload / arraystore / GetMin / GetMax / valid_a / finite_q_d, plus Map / MapIndexed_a / MapThread_a (with internal `::Map` / `::MapIndexed` / `::MapThread` iterator functions). Module exposes only `init_sliarray(SLIInterpreter*)`; operator function objects are anonymous-namespace statics. The Map family uses an iterator-on-EStack pattern: the entry function builds a frame `[mark, result, proc, pos, ::Iter]` (plus source for MapThread) and exits; the dispatcher re-invokes the iterator between user-procedure executions. Each iteration collects the prior call's result into the output slot, pushes the next argument(s), and pushes the procedure for the dispatcher.
  - **Slice 5a (in progress):** bootstrap scaffolding. `config.h` is now generated via `configure_file` (template `config.h.in`); only `SLI3_VERSION_*` and `SLI3_DATADIR` are defined. NEST 2.20.2 `lib/sli/*.sli` (sli-init, typeinit, misc_helpers, library, ps-lib, FormattedIO, debug, helpinit, mathematica, oosupport, regexp, arraylib) are vendored under `lib/sli/`. Fresh `sli_startup.{h,cpp}` exposes `init_slistartup(SLIInterpreter*, int, char**)`: builds statusdict, binds `cin`/`cout`/`cerr` to `std::cin`/`std::cout`/`std::cerr` (borrow mode), opens `$SLIDATADIR/sli-init.sli` (or compile-time `SLI3_DATADIR`), and pushes an XIstream onto the execution stack so the dispatcher reads and executes it. Companion `sli_container_ops.{h,cpp}` adds `length_*`, `get_*`, `put_*`, `put_a_a_t`. Slice 5a also fixed several pre-existing bugs that prevented any file bootstrap: `XIstreamType::execute` infinite re-parse loop; `IparseFunction::execute` reading the wrong estack slot; `raiseerror` infinite recursion; `TrieType` missing `execute()` override (only inlined dispatcher case worked); commented-out `raiseerror(exc)` in the dispatcher's catch handler; the type-name mismatch `litproceduretype` vs `literalproceduretype`; `startup()` using the slow `execute_` loop instead of `execute_dispatch_`. Bootstrap now reaches ~line 309 of `sli-init.sli` (the recursive `bind` of bind itself succeeds) and stops at the first unregistered operator (`join_s`).
  - **Slice 5b (next):** finish the operator surface (string ops `join_s` / `search_s` / `cvi_s` / `cvd_s`, type info, dictstack, etc.) until `sli-init.sli` loads cleanly.
  - **Slice 5c:** the `1 1 add ==` smoke test passes.
- **Stage 4 (tests):** partial. `test_serialize`, `test_array`, and `test_array_module` wired into CTest; cover scalar/name/string/array round-trips, intrusive refcount, shared-pointer aliasing through serialization, basic container API, and the array-stdlib operators (direct execute() calls plus dispatcher-driven Map/MapIndexed/MapThread).
- **Runtime is not yet functional.** The interpreter binary starts up but has no `sli-init.sli` loader and no error-handler bootstrap, so user-level operators like `==`, `=`, `cvs` are not registered. Anything beyond stack manipulation and the now-registered array stdlib will fail or crash. That comes in Slice 5+ (startup wiring + sli-init vendoring).
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
- `test_serialize`, `test_array`, `test_array_module` — CTest binaries linking `sli3_core`

When adding a new test: drop `test_<thing>.cpp` next to the others, add a short `add_executable` + `target_link_libraries` + `add_test` block to `CMakeLists.txt`. No external test framework — bare assertions per Q8.

## Key architectural choices

- **Token is 16 bytes**: `SLIType*` + a POD `union value`. No vtable per datum — virtual dispatch lives on the `SLIType` object instead. Verified at runtime: `Token: 16`, `Dictionary: 32`.
- **Reference counting on the type, not the token.** `SLIType::add_reference(Token&)` / `remove_reference(Token&)` — pointer-payload types (arrays, strings, dicts) override these; scalar types are no-ops.
- **Dispatch is a single `switch` on `type_id`** in `SLIInterpreter::execute_dispatch_` (`sli_interpreter.cpp:634`). Hot opcodes — int/double/name/procedure/repeat/for/forall — are inlined into one big switch so the compiler can build a jump table. Some `case` arms use `goto` deliberately to keep the loop body tight; **don't "clean these up" without tests.**
- **Three execution modes** in `execute()`: plain (`execute_`), debug (`execute_debug_`), dispatch (`execute_dispatch_`). `sli_main.cpp` hard-codes mode 2 (dispatch).
- **Type-economical SLIType polymorphism.** Many entries in the `sli_typeid` enum share the same underlying payload and differ only in execution semantics. The 16-byte Token holds the payload once; the `SLIType*` discriminates behaviour. Examples:
  - `proceduretype` / `litproceduretype` / `arraytype` — all `TokenArray*` payload. Literal procedures don't execute when moved to the estack.
  - `nametype` / `literaltype` / `symboltype` — all `Name` handle.
  - `istreamtype` / `xistreamtype` / `ostreamtype` — all stream pointers.
  - `iiteratetype` / `irepeattype` / `ifortype` / `iforalltype` / `quittype` — operator markers, no payload.

  Don't introduce a concrete class per semantic variant; reuse storage, dispatch through `SLIType`.
- **Procedure type-checking via `type_trie`** (`sli_type_trie.{h,cpp}`, `trietype`). SLI's main extension over PostScript. Part of the dispatcher; not optional.
- **Memory-locality goal.** A procedure is one allocation holding N contiguous Tokens — not N tiny heap allocations behind pointers (the NEST 2.x failure mode). Don't undo this when modernizing containers.
- **Serialization is a first-class concern.** Every `SLIType` subclass implements `serialize(Token const&, Writer&)` / `deserialize(Reader&, Token&)`. The `sli_typeid` enum is a **permanent wire contract** (append-only). Pointer-payload types use the Writer/Reader object table for de-duplication and cycle handling. `Name` serializes as string + re-interns on load. `SLIFunction*` and trie tokens re-resolve by name on load. Streams are not serializable. Both binary (primary) and text (debugging) writers via the same `Writer`/`Reader` interface.

## File map

| Area | Files |
|---|---|
| Core types | `sli_token.{h,cpp}`, `sli_type.{h,cpp}`, `sli_aggregatetoken.h` |
| Type implementations | `sli_{integer,double,bool,name,literal,string,array,dict,function,iostream}type.{h,cpp}` |
| Containers | `sli_array.{h,cpp}`, `sli_dictionary.{h,cpp}`, `sli_dictstack.{h,cpp}`, `sli_tokenstack.{h,cpp}` |
| Strings & streams | `sli_string.h` (`SLIString`), `sli_iostream.{h,cpp}` (`SLIistream`/`SLIostream`) — intrusive-refcount wrappers |
| Names / interning | `sli_name.{h,cpp}` (Name is a uint handle into a global table) |
| Parser/scanner | `sli_scanner.{h,cpp}`, `sli_parser.{h,cpp}`, `sli_charcode.{h,cpp}` |
| Interpreter | `sli_interpreter.{h,cpp}`, `sli_main.cpp` |
| Builtins / control / math | `sli_builtins.{h,cpp}`, `sli_control.{h,cpp}`, `sli_math.{h,cpp}`, `sli_stack.{h,cpp}`, `sli_typecheck.{h,cpp}` |
| Tries (operator dispatch) | `sli_trie.h`, `sli_trietype.h`, `sli_type_trie.{h,cpp}` |
| Serialization | `sli_serialize.{h,cpp}` (`Writer`/`Reader` abstract; `BinaryWriter`/`Reader` impls; `write_token`/`read_token` entry points) |
| Array stdlib | `sli_array_module.{h,cpp}` (`init_sliarray`: Range / Reverse / Rotate / Flatten / Sort / Transpose / Partition_a_i_i / arrayload / arraystore / GetMin / GetMax / valid_a / finite_q_d / Map / MapIndexed_a / MapThread_a + internal `::Map` / `::MapIndexed` / `::MapThread` iterator functions) |
| Container ops | `sli_container_ops.{h,cpp}` (`init_container_ops`: length_* / get_* / put_* / put_a_a_t for arraytype, proceduretype, litproceduretype, stringtype, dictionarytype) |
| Startup | `sli_startup.{h,cpp}` (`init_slistartup`: statusdict, cin/cout/cerr, getenv / evalstring / `<-` / `<--` / endl / flush / begin / end / dict / currentdict; locates and opens sli-init.sli) |
| Vendored .sli | `lib/sli/*.sli` (NEST 2.20.2 — sli-init, typeinit, misc_helpers, library, ps-lib, FormattedIO, debug, helpinit, mathematica, oosupport, regexp, arraylib) |
| Tests | `test_serialize.cpp`, `test_array.cpp`, `test_array_module.cpp` (CTest, bare assertions) |
| Exceptions / utility | `sli_exceptions.{h,cpp}`, `compose.hpp` |

### On disk but **not** in the build (rewrite, don't port)

These files are NEST 2.x verbatim. They use the old `Datum*`/`LockPTR` API,
not sli3's `Token`/`SLIType`, and would not compile against the modernized
core. Per the rewrite directive: treat each as a **reference for which
operators exist**, then write fresh implementations against the new
container layer.

- `sli_io.cpp` — stream I/O. Slice 6 will rewrite as a thin layer over
  `std::fstream` / `std::stringstream`.
- `sli_fdstream.{h,cpp}` — custom POSIX-fd `streambuf`. Replaced wholesale
  by `std::fstream` per Q4; will be deleted, not ported.
- `sli_startup.{h,cpp}` — locates and loads `sli-init.sli`. Slice 5
  rewrites as a fresh `SLIModule` and vendors NEST 2.x `sli-init.sli`.
- `sli_module.cpp`, `sli_tokenutils.cpp` — referenced from headers; check
  before excluding.

### `unported/`

`sli_processes.{h,cpp}` — POSIX `fork`/`exec`/`waitpid` wrappers. Out of
scope for the core-language phase per Q4.

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

SLI was first written in 1996 — much of the existing scaffolding only
existed because pre-C++98 stdlib was unreliable. We don't need it now.

## Known issues / gotchas

- `Token::operator std::string&()` is the only string-conversion operator (the const-by-value form was removed in Stage 2). Callers needing a `std::string&` from a Token use `static_cast<std::string&>(token)` or `token.operator std::string&()`.
- `config.h` is empty. Only `sli_startup.cpp` actually consumes the `SLI_*` macros it would define; build works without them. Generate via `configure_file()` in Slice 5 when startup is wired in.
- `evalstring` is pushed onto the execution stack in `SLIInterpreter::execute(const std::string&)` (line 494) but never registered as a function — the path is dead until Slice 5 wires startup.
- Streams are not serializable. `IstreamType` / `OstreamType::serialize` emit a stderr warning and write no payload; deserialize produces a Token whose underlying `SLIistream`/`SLIostream` has `valid() == false`. Per design — OS resources don't survive a snapshot.
- The `sli_typeid` enum is a **permanent wire contract** (append-only). Renumbering an existing entry breaks every saved snapshot ever made. Adding a new type: append at the end and bump `kSerializeVersion`.

## When working in this repo

- Don't touch the `goto`-driven loops in `execute_dispatch_` without a parity test in place. They are deliberate.
- Don't enable `-Werror` yet — there are 4 pre-existing unused-variable warnings (`proc` in `sli_control.cpp`, `d` in `sli_scanner.cpp`) that haven't been audited.
- Don't add features to the C++ side that should live in `sli-init.sli`. The split-of-responsibility matters for performance.
- The `.o` files, `sli3` binary, `build/`, `CMakeFiles/`, and `*~` backup files are gitignored. Don't commit them.
- New `SLIType` subclasses **must** override `serialize` / `deserialize`. The base class default is no-op — fine for marker types, silently wrong for any type with payload.
- Pointer-payload types must use `Writer::intern_object` / `Reader::lookup_object` for shared identity. See `ArrayType::serialize` and `StringType::serialize` for the pattern.
- Run `ctest --test-dir build` before claiming a slice is done.
