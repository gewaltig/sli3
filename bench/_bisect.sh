#!/bin/bash
# Single-shot bisect of the B2 regression. Drives `git bisect run`
# in the same process so the harness sees one completion event.
#
# Threshold: B2 best-of-3 (real seconds from /usr/bin/time -p).
#   < 2.0   → fast (good)
#   >= 2.0  → slow (bad)
#   any run that doesn't finish in 5 s → slow
#   build fails                       → skip
set +u
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT" || exit 2

LOG=/tmp/sli3_bisect.log
RESULT=/tmp/sli3_bisect_result.txt
: > "$LOG"
: > "$RESULT"

# Per-commit probe used by `git bisect run`. sli3 reads stdin only,
# so the bench script is fed via `<`. /usr/bin/time -p does NOT have
# a built-in timeout, so we wrap the inner sh in `gtimeout 5` (or
# fall back to /usr/bin/timeout on Linux). If gtimeout is not
# installed, perl handles it portably.
cat > /tmp/_sli3_probe.sh <<'PROBE_EOF'
#!/bin/bash
set +u
cd "$(git rev-parse --show-toplevel)" || exit 125

LOG=/tmp/sli3_bisect.log
sha=$(git rev-parse --short HEAD)

# Pick a portable timeout wrapper that returns 124 on hit. macOS:
# install via `brew install coreutils` (gtimeout). Fallback uses
# perl with alarm.
if command -v gtimeout >/dev/null; then    timeout5='gtimeout 5'
elif command -v timeout  >/dev/null; then  timeout5='timeout 5'
else timeout5='perl -e "alarm 5; exec @ARGV" '; fi

# Build. Skip on failure.
if ! cmake --build build -j --target sli3 >> /tmp/_sli3_build.log 2>&1; then
    echo "$sha  BUILD_FAIL  -> skip" >> "$LOG"
    exit 125
fi
[ -x ./build/sli3 ] || { echo "$sha  NO_BINARY  -> skip" >> "$LOG"; exit 125; }

best=""
for k in 1 2 3; do
    # Time the bench loop itself via the script's `tic ... toc ==`,
    # not /usr/bin/time -p, so newly-added .sli files loaded at
    # startup (regexp.sli, mathematica.sli, library.sli) don't get
    # billed against the regression. We still cap wall-clock at 5s
    # so a runaway dispatch can't stall the bisect.
    out=$( $timeout5 ./build/sli3 < bench/sli/B2_add_pop.sli 2>/dev/null )
    rc=$?
    if [ "$rc" = 124 ] || [ "$rc" = 137 ]; then
        echo "$sha  TIMEOUT(5s)  -> slow" >> "$LOG"
        exit 1
    fi
    t=$(echo "$out" | awk '/^[0-9]+(\.[0-9]+)?$/ {print; exit}')
    if [ -z "$t" ]; then
        echo "$sha  BENCH_NO_TIME(rc=$rc)  -> skip" >> "$LOG"
        exit 125
    fi
    if [ -z "$best" ] || [ "$(awk "BEGIN{print ($t<$best)?1:0}")" = 1 ]; then
        best=$t
    fi
done

if [ "$(awk "BEGIN{print ($best<2.0)?1:0}")" = 1 ]; then
    echo "$sha  best=$best  -> fast" >> "$LOG"
    exit 0
else
    echo "$sha  best=$best  -> slow" >> "$LOG"
    exit 1
fi
PROBE_EOF
chmod +x /tmp/_sli3_probe.sh

git bisect reset >> "$LOG" 2>&1
echo "=== bisect start  fast=bd9743f  slow=2b8f104 ===" >> "$LOG"
git bisect start --term-new=slow --term-old=fast 2b8f104 bd9743f >> "$LOG" 2>&1

git bisect run /tmp/_sli3_probe.sh >> "$LOG" 2>&1

echo "=== verdict ===" >> "$LOG"
git bisect log >> "$LOG" 2>&1
{
    echo "=== bisect probes (in order) ==="
    grep -E "best=|TIMEOUT|BUILD_FAIL|NO_BINARY|BENCH_NO_TIME" "$LOG"
    echo
    echo "=== first slow commit ==="
    grep -E "is the first slow commit|There are only" "$LOG" || tail -20 "$LOG"
} > "$RESULT"

git bisect reset >> "$LOG" 2>&1
git checkout regex >> "$LOG" 2>&1
echo "=== DONE ===" >> "$LOG"
cat "$RESULT"
