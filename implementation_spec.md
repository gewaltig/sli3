# sli3 implementation spec

Working document for reviving sli3. **Status as of 2026-05-12:**
Stages 1–5 done; bootstrap functional; full fix-plan Stages 0–5,
7, 9, 10 done; Stage 9 (trie compaction, 10 batches) complete at
tag `stage9-complete`; **Axis I bundle** (dispatcher pre-pop ABI)
complete at commit `151e5e5`. Work happens on the `revive`
branch. See the Decisions log at the bottom for chronological
entries.

Open questions Q1–Q13 are answered (see Decisions). Companion
documents:

- `fix-plan.md` — staged correctness fix plan (Stages 0–9).
- `doc/hot_ops.md` — operator catalog + current bench standing
  vs nest 2.20 and gs 10.07.
- `doc/compact_procedure_spec.md` — design for the next
  optimization (compact procedure storage, the only remaining
  path to within 15 % of gs on B2b).
- `bench/run.sh`, `bench/sli/*.sli`, `bench/ps/*.ps` —
  microbenchmark harness.

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

**Slice progress:** ✅ done · ▶ in progress · ☐ pending

| Slice | Title | Status | Commit |
|---|---|---|---|
| 1 | Drop `sli_allocator` | ✅ | `003b80b` |
| 1.5 | Serialization protocol scaffolding | ✅ | `003b80b` |
| 2 | TokenArray rewrite + serialization | ✅ | `0153017` |
| 3 | Drop lockPTR; modern string/stream wrappers + serialization | ✅ | `dc10c09` |
| 4 | Array stdlib rewrite (Map/Sort/Reverse/…) | ✅ | `d962dc6` |
| 5a | Bootstrap scaffolding (config.h, fresh sli_startup, sli-init.sli vendored, partial bootstrap) | ✅ | `5e40ced` |
| 5b | Fill in operators / fix issues until sli-init.sli loads cleanly | ▶ | (uncommitted) |
| 5c | Smoke test `1 1 add ==` passes | ☐ | — |
| 6 | Modern I/O layer | ☐ | — |
| 7 | argv-driven `sli_main.cpp`, REPL | ☐ | — |

### 3.1 Container audit

Survey each container in the build:

| File | Status | Modern shape |
|---|---|---|
| `sli_array.{h,cpp}` (TokenArray) | ✅ Slice 2 | `std::vector<Token>` storage + intrusive `uint32_t refs_`. Move-aware via Token's noexcept move ctor. ArrayType serialize/deserialize via object table. |
| `sli_string.h` (SLIString) | ✅ Slice 3 | Composes `std::string data_` + `uint32_t refs_`. Implicit conversion to `std::string&` so existing call sites compile unchanged. StringType serialize via object table. |
| `sli_iostream.{h,cpp}` (SLI{i,o}stream) | ✅ Slice 3 | Plain intrusive-refcount wrappers over raw stream pointers; borrow vs. own via flag. `lockptr.h` / `lockobj.h` deleted. IstreamType / OstreamType refcount overrides added. |
| `sli_allocator.{h,cpp}` | ✅ Slice 1 | Deleted. Default heap. |
| `sli_dictionary.{h,cpp}` | ☐ | Currently `std::map<Name, DictToken>`. Future: probably `std::unordered_map`; revisit `DictToken` (Token subclass) — may not need to be a subclass. |
| `sli_dictstack.{h,cpp}` | ☐ | Currently `std::vector<Dictionary*>`. Mostly fine; modernize iteration. |
| `sli_tokenstack.{h,cpp}` | ☐ | Currently inherits from TokenArray (private). Could become `std::vector<Token>` directly. |

**Constraint to preserve:** the protocol where `SLIType::add_reference(Token&)`
and `remove_reference(Token&)` manipulate the refcount on the pointer
payload (`data_.array_val`, etc.). The implementation under the hood is
intrusive `uint32_t refs_`; the type-side virtual interface stays.

### 3.2 Rewrite TokenArray ✅ Slice 2

Done. `std::vector<Token>` backing + intrusive `uint32_t refs_`. Token
gained noexcept move ctor / move-assign (essential for vector
reallocation efficiency). API preserved for dispatcher and existing
call sites. `ArrayType::serialize` / `deserialize` use the object table
for shared-pointer de-duplication. Bug fix: `ArrayType::references()`
was decrementing. Tests in `test_array.cpp`.

### 3.3 Rewrite the array stdlib (was `sli_array_module.cpp`) ✅ Slice 4

Done. Old 2288-line file replaced by a fresh `sli_array_module.{h,cpp}`
written against the new TokenArray. Operators registered:

- Range, Reverse, Rotate, Flatten, Sort (int/double/string),
  Transpose, Partition_a_i_i
- arrayload, arraystore, GetMin, GetMax, valid_a, finite_q_d
- Map, MapIndexed_a, MapThread_a (entry functions) plus
  ::Map / ::MapIndexed / ::MapThread iterator functions

The Map family uses an iterator-on-EStack pattern: the entry function
sets up a frame `[mark, result, proc, pos, ::Iter]` (plus source for
MapThread), and the dispatcher keeps re-invoking the iterator
between user-procedure executions. After each call the iterator
collects the procedure's return value into the result slot, pushes
the next argument(s), and pushes the procedure for the dispatcher to
execute. When the source is exhausted the iterator pops its frame
and pushes the result onto the operand stack.

Module is a free-function `init_sliarray(SLIInterpreter*)` invoked
from `SLIInterpreter::init_internal_functions`. Operator function
objects are private (anonymous-namespace statics in the .cpp).

Tests: `test_array_module.cpp` (CTest), 14 cases — direct execute()
calls for the simple operators plus dispatcher-driven runs for
Map/MapIndexed/MapThread.

Deferred to a later pass: `area`/`area2`/`gabor_`/`gauss2d_`/`cv1d`/`cv2d`
— NEST-specific signal-processing helpers, not core SLI.
`array2intvector`/`intvector2array` need `intvectortype` /
`doublevectortype` implemented first (currently listed only as user-type
slots in the enum).

### 3.4 Modernize the legacy infrastructure ✅ Slices 1 + 3

Done. Specifically:

- ✅ `sli_allocator.{h,cpp}` deleted; default heap (Slice 1).
- ✅ `sli_lockptr.h`, `sli_lockobj.h` deleted; `SLIistream` / `SLIostream`
  rewritten as plain intrusive-refcount wrappers over raw stream
  pointers (Slice 3). Implementation diverged from the original Q4
  proposal of `std::shared_ptr<std::iostream>` because intrusive count
  matches the rest of the system better and avoids the shared_ptr
  control block; outcome is the same — clean lifetime, no lockPTR.
- ✅ `SLIString` no longer inherits `std::string` (composes); intrusive
  refcount; implicit conversion preserves call-site syntax (Slice 3).

### 3.5 Wire up startup, run sli-init.sli — Slice 5

After Slice 4 (array stdlib) is in:

- Generate `config.h` via `configure_file()`. Define only the few macros
  the (rewritten) startup actually reads.
- Rewrite `sli_startup.{h,cpp}` as a fresh `SLIModule` (do not port the
  NEST 2.x version — same 1996-era idioms).
- Vendor NEST 2.x `lib/sli/sli-init.sli` into the repo (clone NEST at a
  2.x tag like `v2.20.2`, copy the file).
- Register `SLIStartup` in `SLIInterpreter::init_internal_functions`.
- Register an `evalstring` function (currently pushed onto the estack
  but never registered) so `execute(const std::string&)` works.

### 3.6 First end-to-end smoke test — part of Slice 5

```sh
echo "1 1 add ==" | ./build/sli3
# expected: 2
```

### 3.7 Plug `sli_main.cpp` into argv — Slice 7

- `sli3` → REPL.
- `sli3 file.sli` → execute script.
- `sli3 -c "1 1 add =="` → execute string.
- `sli3 --debug` → debug mode.

Hand-rolled arg parsing; no third-party dep.

### 3.8 Modern I/O layer — Slice 6

Rewrite the SLI I/O operators (`open-r`, `open-w`, `read`, `write`,
`close`, `eof`, etc.) as a thin layer over `std::fstream` /
`std::stringstream` / `std::cin` / `std::cout`. Don't carry forward
`sli_fdstream` — modern stdlib covers what it was working around.

The infrastructure for I/O Tokens is already in place: `SLIistream` and
`SLIostream` (Slice 3) and `IstreamType` / `OstreamType` with refcount
+ serialize support. This slice adds the *operators*.

---

## Stage 4 — Tests

Depends on: Q8, Q3. **Partially in flight** — `test_serialize.cpp` and
`test_array.cpp` landed alongside Slices 1.5 / 2 / 3.

### 4.1 Wire `test_token.cpp` and `test_dictionary.cpp`

The legacy `test_token.cpp` / `test_dictionary.cpp` from the 2015 tree
are NEST 2.x-era and don't compile against the new core. Replace
piecemeal as we touch each container — `test_array.cpp` already covers
TokenArray + Token semantics. Slice 4 (array stdlib) and onward will
add `test_dict.cpp`, `test_tokenstack.cpp`, etc.

### 4.2 Unit tests for the dispatcher

Each opcode in `execute_dispatch_` deserves a test that pushes a known
estack/ostack state, runs one cycle, and asserts the post-state. Catches
regressions if anyone "cleans up" the gotos. **Open after Slice 5** —
needs working dispatch first.

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

- 2026-05-07: **Scope cut: `iteratortype` / `forall_iter` out of scope.**
  The polymorphic iterator protocol (a cursor abstract base, an
  `IteratorType : SLIType`, an `iforalliter` loop function, and
  iterator constructors like `trieheads_iter` / `size_iter` /
  `Map_iter`) is beyond the core language. Deliberate divergence
  from NEST 2.20.2: the enum slot in `sli_type.h`, the dead
  `Forall_iterFunction` stub in `sli_control.{h,cpp}`, and the
  matching `[/iteratortype …] addtotrie` lines in
  `lib/sli/typeinit.sli` (forall, size, cva tries) and
  `lib/sli/mathematica.sli` (Map trie) are removed. The
  SLI-defined `/trieheads_iter` helper (misleadingly named — it
  operates on a regular array) stays.
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
- 2026-05-05: **Slice 1.5 done** (commit `003b80b`). Serialization
  protocol scaffolded: `Writer` / `Reader` abstract + `BinaryWriter` /
  `BinaryReader` LE binary impl; `serialize` / `deserialize` virtuals
  on `SLIType`; per-type implementations for scalars and Names.
  Wire format: magic `SLI3`, version 1, `[u16 type_id][payload]`.
  CMake restructured into `sli3_core` static lib + `sli3` exe + test
  exe pattern; `enable_testing()` + first `test_serialize` CTest
  target. CTest passes.
- 2026-05-05: **Slice 2 done** (commit `0153017`). TokenArray rewritten
  over `std::vector<Token>` + intrusive `uint32_t refs_`. Token gained
  noexcept move ctor / move-assign. ArrayType serialize/deserialize
  use the object table; aliasing preserved across save/load.
  `ArrayType::references()` bug fixed. New `test_array.cpp` covers
  container API, refcount, and shared-pointer round-trip.
  Token: 16 / Dictionary: 32 unchanged.
- 2026-05-05: **Slice 3 done** (commit `dc10c09`). `sli_lockptr.h` and
  `sli_lockobj.h` deleted. `SLIString` no longer inherits `std::string`
  (composes); `SLIistream` / `SLIostream` rewritten as plain
  intrusive-refcount wrappers over raw stream pointers. `IstreamType` /
  `OstreamType` refcount overrides added (silent leaks fixed).
  StringType serialize via object table; stream types emit warning +
  produce closed-stream Token on load. Same `references()` bug fixed
  in `StringType` as in `ArrayType`. Call-site updates in
  `sli_builtins.cpp` / `sli_control.cpp` for the new wrapper APIs.
  Token: 16 / Dictionary: 32 unchanged.
- 2026-05-05: Slice 4 (array stdlib rewrite) is the next block of
  work — Map / Sort / Reverse / Range / Partition / etc. on the new
  TokenArray, written against `std::sort` / `std::reverse` /
  `std::transform`. NEST-specific signal-processing helpers (area,
  gabor, gauss2d, cv1d, cv2d) and the int/double-vector conversions
  are deferred to a later pass.
- 2026-05-06: **Slice 5b iter 2** (uncommitted). Found and fixed the
  load-bearing bug: `Token::operator==(bool)` and `operator==(double)`
  both checked `get_typeid()==sli3::integertype` instead of booltype/
  doubletype — copy-paste error from operator==(int). This was making
  every `bool == true` comparison return false, which broke `if`'s
  condition check (it always took the else branch). Fixing this got
  the bootstrap past sli-init.sli's initial bind-itself step and into
  `(typeinit.sli) run`. Reverted my earlier mis-fix to put_a (NEST 2.x
  put_a leaves the array on top — pop 2, not pop 3 — so `bind`'s
  `2 copy ... put` pattern preserves the for-loop's [proc, i] state
  across iterations). Added: `xor`/`not` aliases (so typeinit.sli's
  `/xor_ /xor load def` works), `append_a`/`append_p`/`append_lp`/
  `append_s`. Bootstrap now stalls inside typeinit.sli on
  `gt_ss : load DictError` (string-comparison ops not yet
  registered). Roughly 30 more `_a`/`_s`/`_iter` ops missing.

- 2026-05-06: **Slice 5b in progress** (uncommitted). Continuing from
  Slice 5a, now iterating to fill in the operator surface that
  sli-init.sli needs. Operators added in this slice:
  - String / dict ops (sli_container_ops.cpp): `join_s`, `search_s`,
    `cvi_s`, `cvd_s`, `known`, `where`, `undef`.
  - Stream I/O (new sli_io_ops.{h,cpp}): `ifstream`, `ofstream`,
    `cvx_f`, `close`, `eof`, `getline_is`. The minimal surface
    sli-init.sli's `searchifstream` / `searchfile` chain calls. Full
    modern I/O layer is still slice 6. `file` is intentionally NOT
    a C++ op — sli-init.sli's `/file` trie wraps `ifstream`/`ofstream`
    + searchifstream and reports `FileOpenError` itself.

  **Bug fixes uncovered while iterating** (each was blocking the
  bootstrap):
  - `IfelseFunction::execute` checked `pick(1)==true`, but for stack
    `bool tproc fproc` the bool is at `pick(2)` — so `pick(1)` (the
    tproc) was being compared to bool true and the false branch always
    ran. Replaced with a `pick(2).is_of_type(booltype) && bool_val`
    check and clean push of the chosen branch.
  - `PutArrayLikeFunction` (template put_a / put_p / put_lp) consumed
    only 2 of the 3 operands, leaving the array on the stack. Per
    PostScript convention `array index value put -> -` consumes all
    three, and `bind`'s `2 copy ... put` relies on this so the
    iteration's `[proc, idx]` survives across loop iterations.
    Without the fix, `bind` would lose the proc after the first put.
  - `TypeNode::lookup` had no actual handling for `anytype` wildcard —
    the loop walked the alt-list comparing types, and when an
    `anytype` node was reached the inner `if` only suppressed the
    throw but then dereferenced its (potentially null) `alt_`.
    Lookup now exits the inner loop when `pos->type_` is anytype,
    treating it as a wildcard match. Also gracefully handles partial
    trie nodes (next_ set, type_ unset) by throwing ArgumentType
    instead of segfaulting on `pos->type_->get_typeid()`.

  **Bootstrap progress**: configured SLI3_DATADIR points at the
  parent of lib/sli/ (so sli-init.sli's `prgdatadir + "/sli"` builds
  the right SLISearchPath). `(typeinit.sli) run` now reaches `bind`'s
  for-loop iterating typeinit.sli's `/length` trie body — fails on
  the first call to `get` with an unexpected stack shape (next round
  of debugging).

- 2026-05-06: **Slice 5a done** (`5e40ced`). Bootstrap
  scaffolding landed:
  - `config.h` generated via `configure_file` (`config.h.in` template).
    Defines version + `SLI3_DATADIR` (default search path for
    `sli-init.sli`); overridable via env `SLIDATADIR`.
  - Fresh `sli_startup.{h,cpp}` (free-function `init_slistartup`,
    statics in anon namespace). Sets up statusdict (argv, version,
    architecture, exitcodes), binds `cin` / `cout` / `cerr` to
    `std::cin/cout/cerr` in borrow mode, locates and opens
    `sli-init.sli`, pushes the resulting XIstream onto the execution
    stack so the dispatcher will parse and execute it on startup.
  - `evalstring` registered (consumed by
    `SLIInterpreter::execute(const std::string&)` — wraps the source
    in an istringstream + iparse).
  - Minimal stream output ops: `<-`, `<--`, `endl`, `flush`.
  - Dictionary-stack ops: `begin`, `end`, `dict`, `currentdict` plus
    public `SLIInterpreter::DStack()` accessor.
  - Container ops in new `sli_container_ops.{h,cpp}`: `length_*`,
    `get_*`, `put_*`, `put_a_a_t`. Needed by typeinit-style trie
    construction in sli-init.sli.
  - Type-conversion ops in `sli_typecheck.cpp`: `cvlit_n`, `cvn_l`,
    `cvlit_p`, `cvx_lp`. Convert between name<->literal and
    procedure<->literalprocedure (same payload, different SLIType*).
  - Boolean op `or` added to `sli_math.cpp`.
  - NEST 2.20.2 SLI sources vendored under `lib/sli/`: `sli-init.sli`,
    `typeinit.sli`, `misc_helpers.sli`, `library.sli`, `ps-lib.sli`,
    `FormattedIO.sli`, `debug.sli`, `helpinit.sli`, `mathematica.sli`,
    `oosupport.sli`, `regexp.sli`, `arraylib.sli` (~14k lines total).

  **Pre-existing bugs fixed along the way** (each would be required
  for any startup, not just sli-init.sli):
  - `XIstreamType::execute` pushed an extra copy of the stream onto
    the estack, creating an infinite re-parse loop.
  - `IparseFunction::execute` read the istream pointer from `top()`,
    but at dispatch time top is the iparse function itself; the
    stream lives at `pick(1)`. Now reads `pick(1)`.
  - `raiseerror(cmd, err)` recursed into itself with `BadErrorHandler`
    when `newerror` was stuck true, leading to stack overflow.
    Now reads the boolean *value* (not just presence) and on a
    nested error emits a diagnostic + pushes a single
    BadErrorHandler stop, no recursion.
  - `TrieType` had no `execute` override; only the inlined
    `execute_dispatch_` switch case dispatched tries. The plain
    `execute_` loop fell through to the base `SLIType::execute`
    (push to operand stack), so tries didn't work in startup mode.
    Override added that mirrors the dispatch logic.
  - The dispatcher's catch handler had `raiseerror(exc)` commented
    out and just popped the failing token — exceptions were silently
    swallowed. Now funnels them through `raiseerror` so
    `stopped`/`handleerror` contexts can react.
  - `litproceduretype` was registered with the type-name string
    `"litproceduretype"` but NEST 2.x `.sli` keys tries by
    `"literalproceduretype"`. Renamed.
  - `exit_interpreter` cleanup unconditionally read `status_dict_`
    state — guarded for early-exit paths.
  - `startup()` ran the slow `execute_` loop, which lacks the
    inlined dispatch cases for tries / for / forall / iiterate;
    switched to `execute_dispatch_(0)` so the bootstrap sees the
    same dispatch semantics as the main run.

  **Current bootstrap state**: `./build/sli3 < /dev/null` loads
  `sli-init.sli`, gets through ~309 lines (definition + recursive
  `bind` of bind itself), then fails on the first call to `join_s`
  (string concatenation) — the next batch of operators that need
  registering. Documented in the slice plan as 5b.

- 2026-05-05: **Slice 4 done** (`d962dc6`). New
  `sli_array_module.{h,cpp}` replaces the legacy 2288-line file.
  Registered: Range, Reverse, Rotate, Flatten, Sort (int/double/
  string), Transpose, Partition_a_i_i, arrayload, arraystore, GetMin,
  GetMax, valid_a, finite_q_d, Map, MapIndexed_a, MapThread_a, and
  internal ::Map / ::MapIndexed / ::MapThread iterator functions.
  Map family uses an iterator-on-EStack pattern: the entry function
  builds the iteration frame and exits; the dispatcher then re-invokes
  the iterator between user-procedure calls. Module is a free-function
  `init_sliarray(SLIInterpreter*)` called from
  `SLIInterpreter::init_internal_functions`. New `test_array_module`
  CTest binary covers the simple operators directly and drives the
  dispatcher for Map/MapIndexed/MapThread (3/3 tests pass).
  Token: 16 / Dictionary: 32 unchanged.

- 2026-05-07 → 05-08: **Bootstrap complete.** Slice 5b/5c done.
  `./build/sli3` boots to interactive prompt and runs `1 1 add ==`
  → `2`. Many small fixes along the way (raiseerror infinite
  recursion, IfelseFunction reading the wrong stack slot, dispatch
  mode wired into startup, anytype wildcard in TrieType::lookup,
  litproceduretype name mismatch, parser context, …). Recorded in
  the git log; see also `fix-plan.md`. A REPL with line editing and
  history (`~/.sli3_history`) lives behind the interactive prompt.

- 2026-05-08: **Fix-plan Stages 0–5 + 7 done.** Test infrastructure,
  Token/SLIType foundation, container correctness, error handling,
  bootstrap-blocking operators, math/scanner/stack edge cases,
  dispatcher-parity + static-cache lifetime. CTest suite at 18
  binaries; Release + ASAN both green.

- 2026-05-08: **Pointer tagging.** `Token::type_`'s top byte
  encodes the typeid (arm64 TBI; sanitizer builds fall back to
  reading `type_->get_typeid()` via `SLI3_NO_PTR_TAG`).
  Dispatcher saves one chained load per iteration. Bench gain:
  ~6–17 % on B1/B2/B3 vs nest 2.20. Branch-predictor hint
  (`__builtin_expect`) on the count-calls flag added; pointer-tag
  scheme documented in `sli_token.h`.

- 2026-05-08: **Stats mode.** `SLI3_STATS=1 ./build/sli3 <
  script.sli` prints a per-binding call count on exit, keyed by
  systemdict / userdict names. Implementation: one bool +
  `std::unordered_map<void const*, uint64_t>` on
  `SLIInterpreter`, incremented in the dispatcher's functiontype
  and trietype cases (guarded by the bool, hint-folded). The
  stats output guided every subsequent compaction priority.

- 2026-05-08 → 05-11: **Stage 9 trie compaction (10 batches),
  tag `stage9-complete`.** Removed the trie indirection from
  every common operator by writing a compact C++ function per
  name that switches on operand tags inline and delegates to
  the existing typed leaves. Trie machinery stays; typed leaves
  stay registered under their compound names. Batches:

  1. add / sub / mul / div
  2. max / min / gt / lt / leq / geq / eq / neq / and / or / xor / not
  3. sin / cos / asin / acos / exp / ln / log / sqr / sqrt / pow
  4. join / getinterval
  5. length / get / put
  6. forall
  7. def (4-arm trie: 2-arg → DefFunction; 3-arg → /:def_ SLI proc)
  8. cvi / cvd / cvlit / cvx / cvn
  9. forallindexed
  10. empty / size / reserve / insertelement / append / prepend /
      search / cva

  Templated combinators in `sli_op_bodies.h` (`binary_numeric_arith`,
  `compare`, `minmax`, `unary_to_double`) factor the four-arm /
  two-arm shapes into one body each, reused by the SLIFunction
  wrappers in `sli_math.cpp`. Container dispatchers
  (`/length`, `/get`, `/put`, `/empty`, `/size`, `/append`,
  `/prepend`, `/search`, `/cva`, `/getinterval`, `/join`) live
  in `sli_container_ops.cpp`. Control dispatchers (`/forall`,
  `/forallindexed`, `/def`) live in `sli_control.cpp`. cv*
  dispatchers in `sli_typecheck.cpp`.

  Stubbed-upstream names (`/capacity`, `/:resize`, `/shrink`,
  `/references`) stay trie-bound — their typed leaves are
  Unimplemented; compacting would relocate the error without
  fixing it.

- 2026-05-08: **Dispatcher iter-case inline.** The four iter
  cases (`iiteratetype`, `irepeattype`, `ifortype`,
  `iforalltype`) inline both `functiontype` and `nametype`
  dispatch so a procedure body's tokens are handled in one
  case-body iteration rather than via outer-switch round-trips.
  Bench impact:

      B2  unbound `{1 1 add pop} repeat`:   3.43 s → 2.40 s
      B2b bound `{1 1 add pop} bind repeat`: 2.39 s → 1.91 s

  Hot-op super-instructions tried via `HotOp` enum + dispatch
  switch in `sli_op_bodies.h::dispatch_hot()` — measured
  regression of 5–10 % across all benches; reverted. The
  virtual call to a single dominant target is well-predicted
  on M2 and the extra byte load + switch cost more than the
  call saves. Conclusion: the SLIFunction::execute virtual
  call is not the bottleneck on this architecture.

- 2026-05-08: **GhostScript reference checkout.** `/tmp/ghostpdl/`
  (GitHub mirror `ArtifexSoftware/ghostpdl`). Used to study
  gs's interpreter design: `ref` is a 16-byte struct (6-bit
  type tag in the ref itself + 8-byte value union + size
  field). Main dispatch is a label-goto big switch in
  `psi/interp.c` (~line 1000), with direct cases for hot
  operators (`tx_op_add`, `tx_op_pop`, etc.). `bind`
  (`psi/zmisc.c`) replaces name refs with operator refs in
  procedure bodies. Procedure bodies use `ref_packed` (2-byte
  packed elements) for tighter cache footprint.

- 2026-05-11: **Bench infrastructure** (`bench/`). `bench/run.sh`
  runs `./build/sli3`, NEST 2.20.2's `sli` (if present), and
  Ghostscript (if installed) against five workloads. Scripts
  under `bench/sli/` and `bench/ps/`; `bench/README.md`
  documents the suite. Workloads:

      B1   1 pop                              × 100M
      B2   1 1 add pop                        × 100M
      B2b  {1 1 add pop} bind repeat          × 100M
      B3   100k { 1 1 1000 { 2 add pop } for } repeat
      B4   1 1 add_ii pop                     × 100M
      B5   << /a 1 /b 2 >> begin a b add pop end × 100M

- 2026-05-11: **Bench standing at `stage9-complete`** (best of
  five, real wall):

      Bench  sli3       nest 2.20  gs 10.07   sli3 vs gs
      B1     1.33 s     2.01 s     1.32 s     tied
      B2     2.40 s     4.37 s     2.70 s     −11 %
      B2b    1.91 s     3.45 s     1.23 s     +55 %
      B3     2.10 s     4.34 s     2.58 s     −19 %
      B4     2.41 s     3.94 s     n/a        n/a
      B5    16.34 s    43.13 s    24.67 s     −34 %

  sli3 beats nest 2.20 on all six workloads. sli3 beats gs on
  B1/B2/B3/B5; B2b still trails gs because pre-bound procedure
  bodies have no name lookup to amortize and gs's threaded-code
  per-token dispatch is intrinsically tighter.

- 2026-05-11: **Catalog**. `doc/hot_ops.md` lists every common
  operator in six tiers (dispatcher hot path, arithmetic /
  compare, container reads / writes, dictionary / dictstack,
  type / conversion / introspection, control-flow markers) with
  binding kind (`fn` / `op` / `trie`), compaction status, and
  notes. Includes the bench standing table. Used to prioritise
  compaction work and to guide future contributors to ops worth
  caring about.

- 2026-05-11: **Next-step spec** at
  `doc/compact_procedure_spec.md`. Handoff-ready design for
  compact procedure body storage (gs's `ref_packed` idea
  adapted to sli3): a `CompactProc` type with 4-byte slots
  plus per-procedure spill pools, produced at `bind` time.
  Targets B2b specifically; predicted gain takes B2b from
  1.91 s to ~1.3 s (within 15 % of gs). Spec preserves
  PostScript late-binding semantics — names without `bind`
  stay as names and are re-resolved at execution time; only
  the bound case gets the direct-dispatch win. Implementation
  is 6–8 self-contained slices; spec lists files,
  test strategy, risks, and bench expectations.

- 2026-05-11: **Two raiseerror-path bugs fixed** (commits
  `22aab36`, `2531c8f`), found while investigating the cost of
  recordstacks=true. Reproducer: `foo foo foo quit` interactively
  → silent dispatcher halt + segfault on quit.

  1. `DictionaryStack::toArray` (`sli_dictstack.cpp:153–162`)
     constructed dict-Tokens via raw field assignment
     (`type_ = ...; data_.dict_val = ...`) without calling
     `add_reference`. The local Token's destructor decremented
     a refcount that was never bumped → each recordstacks
     snapshot drove `systemdict`'s refcount one below true.
     After ~3 errors the dict was deleted while still in use →
     heap-use-after-free in `Dictionary::remove_reference`
     (ASAN confirmed). Fix: explicit `add_reference()` after
     raw assignment. **General rule**: any pointer-payload
     Token built by raw field assignment needs an explicit
     `add_reference` to balance the destructor's decrement.
     Prefer `new_token<sli3::xtype>(payload)` factories.
  2. `RaiseerrorFunction::execute` (`sli_control.cpp:1012`)
     didn't pop its `/cmd /err` args from the ostack or pop
     itself from the estack — NEST 2.x's
     `slicontrol.cc:1187-1192` does both. The leaked ostack
     items per call made `raiseerror`'s
     `operand_stack_.toArray()` O(N) (O(N²) total) in error
     loops, swamping any recordstacks measurement and
     potentially affecting any script that catches errors in
     tight loops. Fix: `i->pop(2); i->EStack().pop();` before
     delegating to the C++ `raiseerror(Name, Name)`.

  Documented in CLAUDE.md under "Stack-handling discipline"
  as the canonical contract for operator authoring.

- 2026-05-11: **Error-path bench family** (commit `eae8e32`).
  Six new SLI benches under `bench/sli/B6*.sli` measuring the
  per-error cost of raiseerror + stop + stopped + handleerror,
  with and without recordstacks. Paired entries in
  `bench/run.sh`. gs is skipped (PS has no `recordstacks`
  equivalent — `signalerror` dispatches through
  `errordict /errname`).

  Measured cost at 100k iters (post-fix, best of 5):

      Bench  rec=true  rec=false  delta/error
      B6     3.19 us   3.15 us    +0.04 us
      B6b    3.33 us   3.14 us    +0.19 us
      B6c    3.37 us   3.04 us    +0.33 us

  recordstacks adds 40-330 ns per error depending on depth.
  In real scripts that go through `handleerror`, the baseline
  ~3 µs is dominated by `message()`'s stderr write — recording
  is a few-percent overhead. Without `handleerror` printing the
  bare raise+stop path drops to ~0.18 µs and the same
  +0.33 µs becomes a 3x relative overhead. Conclusion: stack
  recovery is not the inefficiency it looked like before the
  bug fixes; the move-not-copy estack optimization discussed
  earlier would shave another ~0.05–0.10 µs and is not worth
  the work unless errors become a hot path.

  Cross-check vs gs's `signalerror`: stack effect matches
  (`/cmd /err raiseerror -> /cmd` ≡ `<obj> /errname signalerror`
  after handler runs). sli3's raise path (0.18–0.51 µs) is
  ~5x faster than gs's signalerror (2.7 µs) because sli3 does
  the populate-$errordict + push-stop work in C++ instead of
  dispatching through an `errordict /errname get exec`
  interpreted lookup. Trade-off: sli3 does NOT honor
  per-error-name handlers placed in `errordict` the way gs
  does. NEST 2.x never used that mechanism so this is fine for
  scope, but it's a documented PS-faithfulness gap.

- 2026-05-12: **Axis I bundle complete** (commits `46ef896`
  through `151e5e5`). Dispatcher-restructure axis I shipped as a
  bundled change: `current_op_` field reroutes raiseerror /
  get_current_name off the e-stack-top read; per-op
  `EStack().pop()` ABI replaced by a dispatcher pre-pop
  contract; ~250 of ~316 operator sites converted to new-ABI
  via `set_new_abi()` opt-in.

  **Contract revision in step 4.** The original plan (step 3a
  dual-ABI skeleton) had the dispatcher post-check after
  `fn->execute` — if top is still the fn slot, pop. Step 4
  revised this to **pre-pop**: the dispatcher pops the fn
  slot BEFORE `fn->execute` for new-ABI ops. `raiseerror`
  conditionally skips its own pop based on
  `current_op_->uses_new_abi()`. `FunctionType::execute`
  (plain mode) mirrors the pre-pop so test_dispatch_parity
  remains valid. The pre-pop contract let ~30 more ops
  convert (conditionals, switch family, cv* dispatcher arms,
  savestate/restorestate) that the post-check couldn't
  handle cleanly.

  **Step-4 audit caught four latent bugs.** Operators whose
  bodies had no `EStack().pop()` AND weren't marked new-ABI
  were silently broken — the step-3 post-pop fallback was
  masking them. Pre-step-4 symptom was infinite loops or
  silent stack corruption; post-step-4 made the bugs visible
  immediately. Fixes: `pwrite_fn` (the `<--` operator — same
  body as `<-`/`write_fn` but `set_new_abi()` was missed in
  step 3f, so `42 ==` was broken end-to-end); `arrayload_fn`
  (silent ::nulltype on the operand stack); `getmax_fn` /
  `getmin_fn` (returned ::nulltype in REPL context). All four
  needed `set_new_abi()` calls in their files' init function.
  Regression test landed at `tests/test_dispatch_abi.cpp`
  — covers both the top-level dispatch path and the iiterate
  fast path (operator inside a `{ ... } exec` proc body), and
  asserts on e-stack depth post-call (since silent frame
  leaks were the failure mode).

  **Old-ABI holdouts (stay old by design).** `StopFunction`
  (multi-frame unwind by mark), `CloseinputFunction`
  (multi-frame unwind by xistreamtype), `Map_fn` /
  `MapIndexed_fn` / `MapThread_fn` (entry funcs that push iter
  frames), `IMap_fn` / `IMapIndexed_fn` / `IMapThread_fn` (the
  iter funcs themselves), `ExecFunction` (swap with ostack
  top), `ExitFunction` (multi-frame pop by mark),
  `EvalstringFunction` (sets up [xistream, iparse]),
  `CloseStreamFunction` (manages descriptor lifetime),
  `IparseFunction` / `IparsestdinFunction` (stay on the
  e-stack across stream reads).

  **Bench delta** (best-of-five vs `stage9-complete`): B1
  1.33→0.95 (−29 %), B2 2.40→1.98 (−18 %), B2b 1.91→1.79
  (−6 %), B3 2.10→1.52 (−28 %), B5 16.34→1.51 (−91 %, from
  Slice 11 pvalue cache landing in the same bundle), B7
  2.19→2.10, B9 2.19→2.08, B10 1.82→1.80. sli3 now beats gs
  on 5 of 9 benches by a wide margin (−27 % to −41 %); the
  remaining gaps on B2b / B7 / B8 / B9 are structural and the
  targets of Axis I slice 8 (inline body walk) plus Axes
  II / III.

  **Operator authoring contract under the new ABI** (added to
  CLAUDE.md's "Stack-handling discipline" section): an
  operator body with NO `i->EStack().pop()` MUST be marked
  new-ABI via `set_new_abi()` in its init function. Otherwise
  (a) the main dispatcher case sees the slot still on top
  after `fn->execute` and re-dispatches → infinite loop, or
  (b) the iiterate fast path's sentinel push for old-ABI ops
  leaks a nulltype onto the e-stack and the next op sees the
  wrong stack shape. Audit script: any class with
  `void execute` whose body has no `EStack().pop()` must
  appear in some init's `set_new_abi()` call list.
