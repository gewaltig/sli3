#!/bin/bash
# Quick B2b-only timer for bisecting the post-Axis-II-step-3
# regression. 10 samples, prints best, mean, stddev (rough).
#
# Usage: bench/b2b_only.sh [./build/sli3] [n_samples]
set -eu
ROOT=$(cd "$(dirname "$0")/.." && pwd)
SLI3=${1:-$ROOT/build/sli3}
N=${2:-10}
[ -x "$SLI3" ] || { echo "missing $SLI3" >&2; exit 1; }

times=()
for i in $(seq 1 "$N"); do
    t=$( { /usr/bin/time -p sh -c "$SLI3 < $ROOT/bench/sli/B2b_add_pop_bound.sli > /dev/null"; } 2>&1 \
           | awk '/^real/ {print $2}')
    times+=("$t")
done
printf "samples: %s\n" "${times[*]}"
printf "best:    %s\n" "$(printf '%s\n' "${times[@]}" | sort -g | head -1)"
printf "mean:    %s\n" "$(printf '%s\n' "${times[@]}" \
                            | awk '{s+=$1; n++} END {printf "%.3f", s/n}')"
