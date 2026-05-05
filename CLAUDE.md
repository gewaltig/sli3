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

- **Stage 1 (build cleanup) complete.** Builds clean on AppleClang 21 / C++17, zero errors.
- **Runtime is not yet functional.** The interpreter binary starts up but has no `sli-init.sli` loader and no error-handler bootstrap, so user-level operators like `==`, `=`, `cvs` are not registered. Anything beyond stack manipulation will fail or crash.
- Stages 2–5 are described in `implementation_spec.md`.

## Build

```sh
cmake -S . -B build
cmake --build build -j
./build/sli3
```

CMake ≥ 3.20, C++17, default Release. Per-target warnings; `-Wno-unused-parameter` because the original code ships with many.

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
| Names / interning | `sli_name.{h,cpp}` (Name is a uint handle into a global table) |
| Parser/scanner | `sli_scanner.{h,cpp}`, `sli_parser.{h,cpp}`, `sli_charcode.{h,cpp}` |
| Interpreter | `sli_interpreter.{h,cpp}`, `sli_main.cpp` |
| Builtins / control / math | `sli_builtins.{h,cpp}`, `sli_control.{h,cpp}`, `sli_math.{h,cpp}`, `sli_stack.{h,cpp}`, `sli_typecheck.{h,cpp}` |
| Tries (operator dispatch) | `sli_trie.h`, `sli_trietype.h`, `sli_type_trie.{h,cpp}` |
| Exceptions / utility | `sli_exceptions.{h,cpp}`, `sli_allocator.{h,cpp}`, `sli_lockptr.h`, `sli_lockobj.h`, `compose.hpp` |

### On disk but **not** in the build (need porting before they can be wired up)

These files are NEST 2.x verbatim. They use the old `Datum*`/`LockPTR` API,
not sli3's `Token`/`SLIType`. Each compiles with **hundreds of errors** and
needs real porting work, not just a CMake entry. Tackle one at a time in
Stage 3.

- `sli_array_module.cpp` — Map/Sort/Reverse/Range/Partition (~63 KB). Many
  ops also need `intvectortype` / `doublevectortype` to be implemented in
  the type system first.
- `sli_io.cpp` — stream I/O.
- `sli_fdstream.{h,cpp}` — custom POSIX-fd `streambuf`. Per Q4, slated for
  replacement with `std::fstream`.
- `sli_startup.{h,cpp}` — locates and loads `sli-init.sli`. Highest
  priority for Stage 3 — without it the interpreter is unusable.
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
  `std::vector<Token>`, `std::shared_ptr`, `std::string`, `std::unordered_map` where it helps.
- Stdlib operators (Map/Sort/Reverse/Partition/etc.): use `std::sort`,
  `std::reverse`, `std::transform`, range-for. Don't translate the
  `Datum*` loops.
- I/O: build on `std::fstream`/`std::stringstream`. Don't carry forward
  `sli_fdstream`.
- Drop the custom `sli_allocator` pool unless profiling shows it matters.

SLI was first written in 1996 — much of the existing scaffolding only
existed because pre-C++98 stdlib was unreliable. We don't need it now.

## Known issues / gotchas

- `Name` has a user-provided copy constructor but uses the implicit copy-assignment operator. Modern clang warns (`-Wdeprecated-copy-with-user-provided-copy`). Trivial to fix; deferred to Stage 2.
- `Token::operator std::string() const` is **declared in the header but not defined**. Use `Token::operator std::string&()` (non-const ref form) — this is what `static_cast<std::string&>(token)` selects. Two call sites in `sli_interpreter.cpp` already use this.
- `config.h` is empty. Only `sli_startup.cpp` actually consumes the `SLI_*` macros it would define; build works without them. Generate via `configure_file()` in Stage 3 when startup is wired in.
- `evalstring` is pushed onto the execution stack in `SLIInterpreter::execute(const std::string&)` (line 494) but never registered as a function — the path is dead until startup is wired up.
- Many files have `mode 100755` instead of `100644`. Cosmetic only. `git config core.fileMode false` to silence.

## When working in this repo

- Don't touch the `goto`-driven loops in `execute_dispatch_` without a parity test in place. They are deliberate.
- Don't enable `-Werror` yet — there are pre-existing warnings (`-Wdeprecated-copy`, infinite-recursion in unfixed code paths, etc.) that need separate cleanup.
- Don't add features to the C++ side that should live in `sli-init.sli`. The split-of-responsibility matters for performance.
- The `.o` files, `sli3` binary, `build/`, `CMakeFiles/`, and `*~` backup files are gitignored. Don't commit them.
