# sli3 dispatcher microbenchmarks

Tiny scripts that exercise the interpreter's hot dispatch loop.
Each one is a tight `repeat` (or `for`) burning ~10⁸ iterations
over the same body, so wall time is dominated by per-iteration
dispatcher cost.

## Run

```sh
cmake --build build -j
bench/run.sh                 # uses ./build/sli3 by default
bench/run.sh build-asan/sli3 # or any other binary
```

The script tries to also run NEST 2.20.2's `sli` (path picked up
from `$NEST`, default `/Users/gewaltig/Code/nest-2.20.2/.local/bin/sli`)
and Ghostscript (`$GS`, default `gs`). Missing tools are skipped.

Five wall-time samples per workload, reported `real` from `time -p`.

## Workloads

| ID  | Body                                              | What it stresses                                |
|-----|---------------------------------------------------|-------------------------------------------------|
| B1  | `1 pop`                                           | bare push + pop loop -- pure dispatcher         |
| B2  | `1 1 add pop`                                     | add via name-bound trie / compact function      |
| B2b | `{1 1 add pop} bind repeat`                       | same body, `bind` resolves names ahead of time  |
| B3  | nested `for` over `2 add pop`                     | `for` continuation cost                         |
| B4  | `1 1 add_ii pop`                                  | direct typed-leaf call, bypasses any wrapper    |
| B5  | `<< /a 1 /b 2 >> begin a b add pop end`           | dict alloc + dictstack push/pop + name lookups  |
| B6  | `{ /test /Bench raiseerror } stopped { handleerror } if` | raiseerror -> stop -> stopped round-trip |
| B6b | B6 wrapped in 4 nested proc calls                 | same round-trip with estack depth 4 at error    |
| B6c | B6 with `mark 1 2 3 4 5` before, `counttomark 1 add_ii npop` after | same round-trip with 5+mark ostack at error |

Each of **B6 / B6b / B6c** has a `_norec` twin that sets
`errordict /recordstacks false put` first; subtracting the two
isolates the cost of raiseerror's three `toArray()` snapshots
(ostack / estack / dstack copied into `$errordict`). gs is
skipped for these — PostScript's `signalerror` dispatches through
`errordict /errname` and has no `recordstacks` equivalent.

## What the numbers mean

- **B1** isolates the dispatcher loop. ~16 ns/iter on sli3 today.
- **B2 − B1** = cost of one `add` (push + add). Was 24 ns before
  Stage 9, dropped to 18 ns after the trie was removed from `add`.
- **B2 vs B2b** measures name-lookup overhead. `bind` walks the
  procedure once and replaces nametype tokens with their resolved
  values, so subsequent invocations skip the dictstack lookup.
  ~5 ns saved per name on sli3.
- **B4** confirms B2 hits the same code path as the typed leaf
  after Stage 9: B2 ≈ B4 within noise.
- **B6 − B6_norec** = recordstacks overhead per error, ~0.04 µs
  shallow / ~0.33 µs with deep ostack. Tiny absolute number, but
  ~3x relative to bare raise+stop without `handleerror` printing.
  In real scripts that go through `handleerror` (default), the
  per-error cost is dominated by `message()`'s stderr write
  (~3 µs), so recordstacks accounts for only a few percent.

## When you change the dispatcher

Run `bench/run.sh` before committing. If a number on the table
in `fix-plan.md` shifts by more than a couple percent, update
the table — silent regressions are easier to ship than to find.
