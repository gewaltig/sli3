#!/bin/bash
# Runs the dispatcher microbenchmarks against sli3 (this build),
# NEST 2.20.2's `sli` (sibling checkout), and Ghostscript.
# Reports best-of-five wall time (real) per implementation, and
# stores all 5 samples + the best into bench/results.db (a SQLite
# file). Disable storage with --no-record or RECORD=0.
#
# Usage:
#   bench/run.sh                    # default: ./build/sli3, recorded
#   bench/run.sh build-asan/sli3    # alternate binary
#   bench/run.sh --no-record        # skip DB write
#   bench/run.sh --label "after X"  # tag this run with a label
#   bench/run.sh --note "..."       # free-form note column on the run
set -u

ROOT=$(cd "$(dirname "$0")/.." && pwd)
DB=$ROOT/bench/results.db

# ----- args -----------------------------------------------------------
SLI3=""
RECORD=${RECORD:-1}
LABEL=""
NOTE=""
while [ $# -gt 0 ]; do
    case $1 in
        --no-record) RECORD=0; shift ;;
        --label)     LABEL=$2; shift 2 ;;
        --note)      NOTE=$2;  shift 2 ;;
        -h|--help)
            sed -n '1,16p' "$0"; exit 0 ;;
        *)
            if [ -z "$SLI3" ]; then SLI3=$1; shift
            else echo "Unknown arg: $1" >&2; exit 1
            fi ;;
    esac
done
SLI3=${SLI3:-./build/sli3}
NEST=${NEST:-/Users/gewaltig/Code/nest-2.20.2/.local/bin/sli}
GS=${GS:-gs}

if [ ! -x "$SLI3" ]; then
    echo "sli3 binary not found at $SLI3 -- build first or pass a path" >&2
    exit 1
fi

SLI_DIR=$ROOT/bench/sli
PS_DIR=$ROOT/bench/ps

# ----- create run row in DB ------------------------------------------
RUN_ID=""
if [ "$RECORD" = 1 ]; then
    if [ ! -f "$DB" ]; then
        echo "results.db not found; initializing..." >&2
        "$ROOT/bench/init_db.sh" >/dev/null
    fi
    COMMIT_SHA=$(git -C "$ROOT" rev-parse --short HEAD 2>/dev/null || echo "unknown")
    DIRTY=0
    if [ -n "$(git -C "$ROOT" status --porcelain 2>/dev/null)" ]; then DIRTY=1; fi
    HOST=$(hostname -s 2>/dev/null || echo "unknown")
    # SQL-escape single quotes in LABEL / NOTE.
    L=$(printf "%s" "$LABEL" | sed "s/'/''/g")
    N=$(printf "%s" "$NOTE"  | sed "s/'/''/g")
    RUN_ID=$(sqlite3 "$DB" "
        INSERT INTO runs (commit_sha, dirty, label, host, note)
        VALUES ('$COMMIT_SHA', $DIRTY, NULLIF('$L',''), '$HOST', NULLIF('$N',''));
        SELECT last_insert_rowid();
    ")
    echo "Recording to $DB as run_id=$RUN_ID (commit=$COMMIT_SHA, dirty=$DIRTY)"
fi

# ----- helpers --------------------------------------------------------

# bench_id_from_label "B2b  {1 1 add pop} bind ..." -> "B2b"
bench_id_from_label() { echo "$1" | awk '{print $1}'; }

# Append one INSERT for (bench, impl, t1..t5, best). $1=bench, $2=impl,
# remaining args are the 5 sample times (whitespace-separated). Writes
# into the global SQL_BATCH file via FD 3 (opened in main below).
record_result() {
    [ "$RECORD" = 1 ] || return 0
    local bench=$1 impl=$2; shift 2
    # Parse the 5 sample values; if any is missing or non-numeric,
    # bail without recording (so partial / skipped impls don't get
    # bad rows).
    set -- $1   # the 5 samples were passed as a single space-string
    local t1=${1:-} t2=${2:-} t3=${3:-} t4=${4:-} t5=${5:-}
    # All five required; otherwise treat as no-record.
    case "$t1$t2$t3$t4$t5" in
        *[!0-9.]*|"") return 0 ;;
    esac
    local best=$(printf "%s\n%s\n%s\n%s\n%s\n" "$t1" "$t2" "$t3" "$t4" "$t5" \
                   | sort -g | head -1)
    printf "INSERT INTO results (run_id, bench, impl, t1, t2, t3, t4, t5, best) VALUES (%s, '%s', '%s', %s, %s, %s, %s, %s, %s);\n" \
        "$RUN_ID" "$bench" "$impl" "$t1" "$t2" "$t3" "$t4" "$t5" "$best" >&3
}

run_one() {
    local label=$1 sli=$2 ps=$3 nest_note=${4:-}
    local bench
    bench=$(bench_id_from_label "$label")
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
        record_result "$bench" "$impl" "$times"
    done
}

# ----- open SQL batch file (fd 3) if recording -----------------------
SQL_BATCH=""
if [ "$RECORD" = 1 ]; then
    SQL_BATCH=$(mktemp -t sli3_bench.XXXXXX.sql)
    trap 'rm -f "$SQL_BATCH"' EXIT
    exec 3>"$SQL_BATCH"
else
    exec 3>/dev/null
fi

# ----- run the workloads ---------------------------------------------

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

# ----- commit batched inserts ----------------------------------------
exec 3>&-
if [ "$RECORD" = 1 ] && [ -s "$SQL_BATCH" ]; then
    sqlite3 "$DB" <"$SQL_BATCH"
    echo
    echo "Recorded run $RUN_ID. Query with: bench/history.sh latest"
fi
