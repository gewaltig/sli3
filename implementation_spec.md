# sli3 implementation spec

Working document for reviving sli3. Stage 1 (clean build) is done. This spec
covers Stages 2–5. **Edit the "Open questions" section first** — several
later decisions depend on those answers.

---

## Open questions

Answer these inline (replace `???` with your decision). Anything unanswered
becomes a default I'll guess at.

### Q1. Scope: drop-in NEST replacement or standalone interpreter?

The codebase is closer to "drop-in NEST sli replacement" — it has type stubs
for things like `iiteratetype`, `sli-init.sli` semantics baked in, and
`sli_processes.h` ships POSIX wrappers NEST uses. But it could also be cut
down to a standalone PostScript-ish interpreter.

**Pick one:**
- (b) Standalone SLI interpreter, NEST-compatible-ish but not bound to the
  NEST release. Goal: Language semantics should be compatible, but the neural simulations needn't be supported. Most of `sli-init.sli` should run.

**Answer:** 
(b)

### Q2. Target NEST version

If Q1 = (a) or (b), which NEST tree do we align against?

- The 2015 codebase forked from a NEST that's now ~10 years old.
- Current NEST mainline has evolved SLI's surface (new operators, removed
  ones, dictionary changes).

**Pick one:**
- (a) Current NEST `master`. Treat 2015 sli3 as a starting skeleton; track
  upstream from now on.
- (b) Pin to NEST 2.x (2015-era). Easier to compare; nobody runs it.
- (c) Other: ???

**Answer:** 
(b) again

### Q3. Which `sli-init.sli` is the source of truth?

Same question, different angle. The startup script defines half the
language. We need a copy in the tree (as `lib/sli/sli-init.sli` or similar)
or a path to it via env var.

**Pick one:**
- (a) Copy current NEST `lib/sli/sli-init.sli` into this repo, vendor it.
- (b) Load from `$NEST_INSTALL_DIR/share/nest/sli/` at runtime.
- (c) Write a minimal sli-init.sli from scratch tailored to sli3.

**Answer:** 
(a)

### Q4. Streams and processes

Three excluded files: `sli_io.cpp` (49 KB), `sli_fdstream.{h,cpp}` (custom
fd-backed iostream), `sli_processes.{h,cpp}` (fork/exec wrappers).

- I/O is needed for any non-trivial script (file read/write, pipes).
- `sli_fdstream` is a custom `std::streambuf` over POSIX fds — was written
  before C++ had usable equivalents. May be replaceable with `std::fstream`.
- `sli_processes` is fork/exec/waitpid wrappers; only useful if scripts are
  expected to spawn subprocesses.

**Pick one for each:**
- `sli_io.cpp`: include 
- `sli_fdstream`: replace with std::fstream 
- `sli_processes`: defer

### Q5. Performance baseline

"Faster and more memory-efficient" — than what?

- (a) vs current NEST `sli` binary on a fixed benchmark suite.
- (b) vs sli3's own 2015 numbers (if recoverable).
- (c) Just hit absolute targets (e.g. <10 ns per operator dispatch).

**Answer:**
(d) reasonable fast and efficient.

**Benchmark scripts:** do we have a corpus, or do we need to write one?
**Answer:**
We need to write one.

### Q6. Threading / concurrency

`sli_interpreter.cpp` has `#ifdef HAVE_PTHREADS` blocks around `Mutex barrier`
in `message()`. NEST runs threaded. Is multi-threaded SLI in scope, or is
sli3 single-threaded with NEST handling parallelism above it?

**Pick one:**
- (a) Single-threaded SLI; remove the HAVE_PTHREADS code paths.
- (b) Thread-safe message logging only (current state).
- (c) Full multi-threaded interpreter — much larger lift; affects Name table,
  Dictionary, allocator.

**Answer:** 
Single threaded.; remove PTHREADS

### Q7. C++ standard

I picked C++17 in CMakeLists for Stage 1. Is that fine, or do you want
C++20 (concepts, `std::span`, ranges)? C++23?

**Answer:** 
C++17

### Q8. Test framework

Stage 4 needs unit + parity tests. The repo has two ad-hoc files
(`test_token.cpp`, `test_dictionary.cpp`) that aren't built.

**Pick one:**
- (a) GoogleTest (heavy; familiar).
- (b) Catch2 single-header (lightweight).
- (c) Bare CTest with assert-based test executables — minimal deps.

**Answer:** 
(c)

### Q9. License

`sli_startup.cpp` carries "non-commercial use" header from NEST. NEST itself
relocated to GPLv2+. What license for sli3?

**Answer:**
GPL

### Q10. Authorship model

- (a) Solo project; optimise for your speed.
- (b) Open to contributors; need CONTRIBUTING.md, style guide, CI.

**Answer:** 
(a) then later (b)
---

## Stage 2 — Wire up unbuilt files and clean warnings

Depends on: Q1, Q4.

### 2.1 Resolve "modified" files in git status

The `git status` shows ~80 files as modified, but `git diff` shows only mode
changes (100644 → 100755). One `git update-index --chmod=-x` sweep, or run
`git config core.fileMode false`. Decide which.

### 2.2 Pull in `sli_array_module.cpp`

Large standard array library; probably mandatory regardless of Q1 outcome.

- Add to `CMakeLists.txt` source list.
- Build will likely surface compile errors against modern libc++ (same class
  as the Stage 1 issues). Fix in place.
- Check the `init(SLIInterpreter*)` entry point at `sli_array_module.cpp:2256`
  is called from `SLIInterpreter::init_internal_functions`. Currently it is not.

### 2.3 Apply Q4 decision for streams / processes

Either include or move to `unported/` with a one-line note.

### 2.4 Fix the `Name` copy-assignment warning

`sli_name.h:61`: explicit copy ctor without explicit copy assignment. Add
`Name& operator=(const Name&) = default;` (or just remove the copy ctor —
the implicit one does the right thing for a single `handle_t` member).

### 2.5 Define or remove `Token::operator std::string() const`

Declared in `sli_token.h:75`, never defined. Either remove the declaration
(forces all callers to use the ref form) or define it as `return *data_.string_val;`.

### 2.6 Audit the other declared-but-not-defined operators

Likely candidates from `sli_token.h`:

- `operator std::vector<long>() const`
- `operator std::vector<double>() const`
- `operator std::istream&()`
- `operator std::ostream&()`

Grep for unused declarations and either define them or remove the
declarations. Linker is silent about these because they're never called.

### 2.7 Decide on the warning posture

Once warnings are at zero, turn on `-Werror` in CI (not local dev) so they
don't reaccumulate.

---

## Stage 3 — Modern-C++ rewrite of containers and stdlib

**Reframe (2026-05-05):** Stage 3 is no longer a "port the unbuilt files"
exercise. The Token/SLIType model already broke from NEST 2.x's
`Datum*` + `LockPTR` design — keeping 1996-era idioms in the containers
and stdlib that sit on top would just bolt the old design onto the new
one. Stage 3 is instead a **fresh modern-C++17 implementation** of the
container layer and the stdlib modules built on it.

### 3.1 Container audit

Survey each container in the build:

| File | Today | Modern shape |
|---|---|---|
| `sli_array.{h,cpp}` (TokenArray) | hand-rolled storage, custom `sli_allocator` pool, manual `refs_` field | `std::vector<Token>` storage, refcount via `std::shared_ptr` or kept on TokenArray for ArrayType protocol compatibility |
| `sli_dictionary.{h,cpp}` | `std::map<Name, DictToken>` | possibly `std::unordered_map`; revisit DictToken (Token subclass) — may not need to be a subclass |
| `sli_dictstack.{h,cpp}` | `std::vector<Dictionary*>` | mostly fine; modernize iteration |
| `sli_tokenstack.{h,cpp}` | hand-rolled stack | `std::vector<Token>` + thin stack ops |
| `sli_string.h` (SLIString) | thin wrapper | replace with `std::string` directly |
| `sli_iostream.h`, `sli_lockptr.h`, `sli_lockobj.h` | custom refcounted ptr | `std::shared_ptr<std::iostream>` |
| `sli_allocator.{h,cpp}` | custom slab pool | drop. Default `new`/`delete` is fast enough for this profile; reintroduce `std::pmr` only if profiling shows a hotspot |

**Constraint to preserve:** the protocol where `SLIType::add_reference(Token&)`
and `remove_reference(Token&)` manipulate the refcount on the pointer
payload (`data_.array_val`, etc.). The implementation under the hood can
move to `shared_ptr`, but the type-side virtual interface stays.

### 3.2 Rewrite TokenArray

Most-used container, biggest payoff. Vertical slice that proves the
approach before touching the others.

- Storage: `std::vector<Token>` member.
- API: keep the names the rest of the codebase already calls (`size()`,
  `get(long)`, `index_is_valid`, `push_back`, etc.) so the dispatcher in
  `execute_dispatch_` doesn't churn.
- Refcount: keep the `add_reference()`/`remove_reference()` member
  functions for the ArrayType protocol; back them with an embedded
  count or a `shared_ptr` indirection.
- Drop: `sli_allocator` pool, custom `operator new`/`delete`,
  `ARRAY_ALLOC_SIZE`, the manual `p` / `begin_of_free_storage` / `end_of_free_storage` triple.
- Tests: `test_token.cpp` already touches Token; add `test_array.cpp`
  asserting size/push_back/clear/resize/refcount semantics.

### 3.3 Rewrite the array stdlib (was `sli_array_module.cpp`)

The 2256-line NEST 2.x file is the source of *what operators we need*,
not *how to implement them*. New file (`sli_array_stdlib.{h,cpp}` or
keep the name) writes against the new TokenArray using `std::sort`,
`std::reverse`, `std::transform`, `std::accumulate`. Reproduce the SLI
semantics (Map, Sort, Reverse, Range, Partition, Flatten, Transpose,
GetMin/GetMax, ArrayLoad/ArrayStore) but with modern idioms.

Defer to a later pass: `area`/`area2`/`gabor_`/`gauss2d_`/`cv1d`/`cv2d`
— these are NEST-specific signal-processing helpers, not core SLI.
`array2intvector`/`intvector2array` need `intvectortype` /
`doublevectortype` implemented first (currently listed only as user-type
slots in the enum).

### 3.4 Modernize the legacy infrastructure

- Replace `lockPTR<T>` / `lockPTRDatum` usages with `std::shared_ptr<T>`.
- Replace `SLIString` with `std::string`. Update all `data_.string_val`
  consumers.
- Drop `sli_allocator.{h,cpp}` once nothing uses it.
- Rewrite `SLIistream`/`SLIostream` as `std::shared_ptr<std::istream>`
  / `std::shared_ptr<std::ostream>` aliases.

### 3.5 Wire up startup, run sli-init.sli

After the container layer is solid:

- Generate `config.h` via `configure_file()`. Define only the few macros
  the (rewritten) startup actually reads.
- Rewrite `sli_startup.{h,cpp}` as a fresh `SLIModule` (do not port the
  NEST 2.x version — same 1996-era idioms).
- Vendor NEST 2.x `lib/sli/sli-init.sli` into the repo (clone NEST at a
  2.x tag, copy the file).
- Register `SLIStartup` in `SLIInterpreter::init_internal_functions`.
- Register an `evalstring` function (currently pushed onto the estack
  but never registered) so `execute(const std::string&)` works.

### 3.6 First end-to-end smoke test

```sh
echo "1 1 add ==" | ./build/sli3
# expected: 2
```

### 3.7 Plug `sli_main.cpp` into argv

- `sli3` → REPL.
- `sli3 file.sli` → execute script.
- `sli3 -c "1 1 add =="` → execute string.
- `sli3 --debug` → debug mode.

Hand-rolled arg parsing; no third-party dep.

### 3.8 Modern I/O layer

Rewrite the SLI I/O operators (`open-r`, `open-w`, `read`, `write`,
`close`, `eof`, etc.) as a thin layer over `std::fstream` /
`std::stringstream` / `std::cin` / `std::cout`. Don't carry forward
`sli_fdstream` — modern stdlib covers what it was working around.

---

## Stage 4 — Tests

Depends on: Q8, Q3.

### 4.1 Wire `test_token.cpp` and `test_dictionary.cpp`

Already in the tree. Add `enable_testing()` + `add_test()` for each. Convert
to the framework chosen in Q8 if not bare-assert.

### 4.2 Unit tests for the dispatcher

Each opcode in `execute_dispatch_` deserves a test that pushes a known
estack/ostack state, runs one cycle, and asserts the post-state. Catches
regressions if anyone "cleans up" the gotos.

### 4.3 Parity test against NEST sli

For Q1 = (a) or (b): a `tests/sli/` directory of small `.sli` programs. Run
each under both NEST's `sli` and `./build/sli3`, diff stdout and final
operand-stack contents. Programs to start with:

- factorial, fibonacci (control flow)
- list reverse, sort, partition (array module)
- dictionary def/lookup/undef (dictionary stack)
- raiseerror / stopped (error path)
- procedure recursion to depth 100 (estack growth)

### 4.4 Microbenchmark harness

For Q5: a bench harness measuring ns/dispatch on tight loops. Compare
against current NEST sli on the same machine. Report cycles + allocations
(the existing `TokenArray::allocations` counter is a start).

---

## Stage 5 — Re-anchor to current NEST (only if Q1 = a)

Depends on: Q2.

### 5.1 Diff current NEST `sli/interpret.{h,cc}` against this repo

The 2015 codebase forked from NEST's interpreter at a known point. Today's
NEST has continued to evolve. Identify:

- New operators added upstream that sli3 doesn't have.
- Removed/renamed operators.
- Changes to error semantics (`raiseerror`, `stopped`, error dict layout).
- Dictionary stack ordering changes.

### 5.2 Sync the operator surface

Working list, not a full rewrite. For each missing operator: either port the
NEST implementation, or stub it with `NotImplemented` and track in a
checklist.

### 5.3 NEST kernel integration shim

If NEST is to load `libsli3.dylib` instead of its in-tree SLI, we need:

- `libsli3` target in CMakeLists (alongside the executable).
- C++ ABI matching what NEST's loader expects — double-check namespace
  (`sli3::` vs NEST's `nest::`/global `SLI*`).
- Symbol export decisions.

This is substantial work and only justified if Q1 = (a) is firm.

---

## Out of scope for now

- GUI / IDE integration (NEST has a Python frontend that mostly bypasses sli).
- Network / distributed SLI.
- JIT / bytecode compilation. The existing dispatcher is fast enough that
  adding a JIT only makes sense after the parity tests pass.

---

## Decisions log

When you answer a question above, copy the decision here with the date so
it's discoverable later without scrolling:

- 2026-05-05: Stage 1 complete. Build is clean, runtime not yet functional.
- 2026-05-05: Q1 = (b). Standalone SLI interpreter, NEST-compatible language
  semantics, no neural-simulation surface. Stage 5 is therefore out of scope.
- 2026-05-05: Q2 = (b). Target NEST 2.x (2015-era) as the language-surface
  reference.
- 2026-05-05: Q3 = (a). Vendor `sli-init.sli` into the repo. Combined with
  Q2, this means vendoring the NEST 2.x-era sli-init.sli, not the current
  one.
- 2026-05-05: Q4. `sli_io.cpp` → include; `sli_fdstream` → replace with
  `std::fstream`; `sli_processes` → defer (move to `unported/`).
- 2026-05-05: Q5. No hard performance target — "reasonable". Write a
  benchmark corpus from scratch.
- 2026-05-05: Q6. Single-threaded interpreter. Remove all `HAVE_PTHREADS`
  code paths.
- 2026-05-05: Q7. C++17.
- 2026-05-05: Q8. Bare CTest with assert-based test executables. No
  external test framework dependency.
- 2026-05-05: Q9. GPL (matches current NEST). Replace the
  "non-commercial use" headers wherever they appear.
- 2026-05-05: Q10. Solo project for now; revisit when contributors arrive.
  No CONTRIBUTING.md / CI yet.
- 2026-05-05: Strip *all* NEST optional-dep flags (MPI, MUSIC, GSL,
  PTHREADS, NDEBUG-conditionals, HAVE_SSTREAM, HAVE_EXPM1, HAVE_M_E/PI,
  HAVE_CMATH_MAKROS_IGNORED). Core SLI only. User directive.
- 2026-05-05: Stage 2 partial complete. File-mode noise quieted, pthreads
  removed, Name copy-assignment warning fixed, undefined Token operators
  removed, sli_processes moved to `unported/`, all NEST optional-dep
  conditionals stripped. Clean build with 4 pre-existing
  unused-variable warnings.
- 2026-05-05: Stage 2 deferred items. `sli_array_module.cpp`, `sli_io.cpp`,
  `sli_fdstream`, `sli_startup.{h,cpp}` are NEST 2.x verbatim — they use
  the old `Datum`/`LockPTR` API, not sli3's `Token`/`SLIType`.
  License-header replacement (NEST non-commercial → GPL) likewise
  deferred to Stage 3, applied per-file as each is rewritten.
- 2026-05-05: **Stage 3 reframe.** Don't port these files line-by-line.
  SLI was first written in 1996; the unported sources carry pre-modern-C++
  idioms (custom slab allocator, `LockPTR`, `Datum*` hierarchy). sli3's
  runtime model already changed (16-byte value `Token`, refcount on the
  type). Containers and stdlib should be **rewritten** with C++17
  idioms (`std::vector<Token>`, `std::shared_ptr`, `std::string`,
  range-for, `std::sort`/`std::reverse`/`std::transform`), not translated
  from the old code. Old files become *reference-only* for "what
  operators exist".
- 2026-05-05: Slice 1 done. `sli_allocator.{h,cpp}` deleted; `TokenArray`
  and `Dictionary` no longer use the custom pool. Build clean. Sizes
  unchanged: `Token: 16`, `Dictionary: 32`.
- 2026-05-05: **Q11–13 (serialization)** answered.
  - Q11 = (c) both: workspace save (dicts + user procs at idle) and full
    snapshot (also estack/ostack mid-execution).
  - Q12 = (c) both formats via single Writer/Reader interface;
    BinaryWriter primary, TextWriter for debugging/tests. No external
    library dep.
  - Q13 = (b) magic + uint32 version + tag-based body; major-version
    mismatch refused, unknown body tags skipped (forward compat).
  Invariants: `sli_typeid` enum is wire contract (append-only); pointer
  payloads use object table for de-dup and cycles; `Name` serialized as
  string per occurrence and re-interned; `SLIFunction*`/`TypeNode*`
  re-resolved by name on load; streams are not serializable. Detail:
  see `project_serialization.md` memory.
