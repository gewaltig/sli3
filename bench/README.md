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

| ID  | Body                          | What it stresses                              |
|-----|-------------------------------|-----------------------------------------------|
| B1  | `1 pop`                       | bare push + pop loop -- pure dispatcher       |
| B2  | `1 1 add pop`                 | add via name-bound trie / compact function    |
| B2b | `{1 1 add pop} bind repeat`   | same body, `bind` resolves names ahead of time |
| B3  | nested `for` over `2 add pop` | `for` continuation cost                       |
| B4  | `1 1 add_ii pop`              | direct typed-leaf call, bypasses any wrapper  |

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

## When you change the dispatcher

Run `bench/run.sh` before committing. If a number on the table
in `fix-plan.md` shifts by more than a couple percent, update
the table — silent regressions are easier to ship than to find.
