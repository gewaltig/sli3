#!/bin/bash
# Runs the dispatcher microbenchmarks against sli3 (this build),
# NEST 2.20.2's `sli` (sibling checkout), and Ghostscript.
# Reports best-of-five wall time (real) per implementation.
#
# Usage:  bench/run.sh [sli3-binary]
#         default sli3-binary is ./build/sli3
set -u

SLI3=${1:-./build/sli3}
NEST=${NEST:-/Users/gewaltig/Code/nest-2.20.2/.local/bin/sli}
GS=${GS:-gs}

if [ ! -x "$SLI3" ]; then
    echo "sli3 binary not found at $SLI3 -- build first or pass a path" >&2
    exit 1
fi

ROOT=$(cd "$(dirname "$0")/.." && pwd)
SLI_DIR=$ROOT/bench/sli
PS_DIR=$ROOT/bench/ps

run_one() {
    local label=$1 sli=$2 ps=$3 nest_note=${4:-}
    echo "=== $label ==="
    for impl in sli3 nest gs; do
        local cmd=""
        case $impl in
            sli3) cmd="$SLI3 < $sli" ;;
            nest) [ -x "$NEST" ] || { printf "%-5s: (not installed)\n" nest; continue; }
                  [ -n "$nest_note" ] && { printf "%-5s: %s\n" nest "$nest_note"; continue; }
                  cmd="$NEST < $sli" ;;
            gs)   command -v "$GS" >/dev/null || { printf "%-5s: (not installed)\n" gs; continue; }
                  [ -z "$ps" ] && { printf "%-5s: (no .ps script)\n" gs; continue; }
                  cmd="$GS -dNODISPLAY -dQUIET -dBATCH -q $ps" ;;
        esac
        local times=""
        for i in 1 2 3 4 5; do
            local t
            t=$( { /usr/bin/time -p sh -c "$cmd > /dev/null"; } 2>&1 | awk '/^real/ {print $2}')
            times="$times $t"
        done
        printf "%-5s:%s\n" "$impl" "$times"
    done
}

run_one "B1   1 pop                       x 100M" \
        "$SLI_DIR/B1_pop.sli"            "$PS_DIR/B1_pop.ps"

run_one "B2   1 1 add pop                 x 100M" \
        "$SLI_DIR/B2_add_pop.sli"        "$PS_DIR/B2_add_pop.ps"

run_one "B2b  {1 1 add pop} bind repeat   x 100M" \
        "$SLI_DIR/B2b_add_pop_bound.sli" "$PS_DIR/B2b_add_pop_bound.ps"

run_one "B3   100k {1 1 1k {2 add pop} for} repeat" \
        "$SLI_DIR/B3_for_loop.sli"       "$PS_DIR/B3_for_loop.ps"

run_one "B4   1 1 add_ii pop              x 100M  (typed-leaf direct)" \
        "$SLI_DIR/B4_add_ii.sli"         ""

run_one "B5   << ... >> begin ... end     x 10M   (dict alloc + dictstack)" \
        "$SLI_DIR/B5_dict_begin_end.sli" "$PS_DIR/B5_dict_begin_end.ps"

# B6/B6b/B6c -- stopped/raiseerror round-trip with vs. without stack
# recording. The pairs (B6 vs B6n, B6b vs B6bn, B6c vs B6cn) measure
# the per-error cost of $errordict's three toArray() snapshots
# (ostack + estack + dstack). gs is skipped: PS has no recordstacks /
# raiseerror equivalent.
run_one "B6   {/t /B raiseerror} stopped pop x 1M  (recordstacks=true)" \
        "$SLI_DIR/B6_stopped.sli"               ""
run_one "B6n  same, recordstacks=false       x 1M" \
        "$SLI_DIR/B6_stopped_norec.sli"         ""

run_one "B6b  deep estack at error           x 1M  (recordstacks=true)" \
        "$SLI_DIR/B6b_stopped_deep.sli"         ""
run_one "B6bn same, recordstacks=false       x 1M" \
        "$SLI_DIR/B6b_stopped_deep_norec.sli"   ""

run_one "B6c  ostack=5 at error              x 1M  (recordstacks=true)" \
        "$SLI_DIR/B6c_stopped_ostack.sli"       ""
run_one "B6cn same, recordstacks=false       x 1M" \
        "$SLI_DIR/B6c_stopped_ostack_norec.sli" ""

# B7-B10 -- organic workloads exercising the operator surface:
# get/put/comparison/for/if/recursion/arithmetic. The .sli files
# use sli3's `put`-returns-array semantics ("put pop" everywhere);
# the .ps files use PS-standard `put` (consumes, no return). nest
# 2.20 follows PS semantics so the .sli scripts as written will
# stack-underflow on nest -- skip with a note.

run_one "B7   bubble sort 100 x 1000" \
        "$SLI_DIR/B7_bubble_sort.sli" \
        "$PS_DIR/B7_bubble_sort.ps" \
        "(skip: sli3-specific put semantics)"

run_one "B8   insertion sort 100 x 1000" \
        "$SLI_DIR/B8_insertion_sort.sli" \
        "$PS_DIR/B8_insertion_sort.ps" \
        "(skip: sli3-specific put semantics)"

run_one "B9   recursive fib(28) x 20" \
        "$SLI_DIR/B9_fibonacci.sli" \
        "$PS_DIR/B9_fibonacci.ps"

run_one "B10  matmul 50x50 x 100" \
        "$SLI_DIR/B10_matmul.sli" \
        "$PS_DIR/B10_matmul.ps" \
        "(skip: sli3-specific put semantics)"
