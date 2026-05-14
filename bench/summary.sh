#!/bin/bash
# bench/summary.sh -- one-screen summary of a bench run.
#
# Renders a markdown-ish table aligned with the "Bench standing"
# format in fix-plan.md / CLAUDE.md, plus a win-loss tally vs gs.
# Default is the most recent run; pass a run_id to summarise an
# older one. Use `bench/history.sh runs` to list run ids.
#
# Usage:
#   bench/summary.sh                # latest run
#   bench/summary.sh 1              # baseline run
#   bench/summary.sh --md           # markdown only, no header chrome
#   bench/summary.sh --diff         # add baseline-delta column
set -eu

ROOT=$(cd "$(dirname "$0")/.." && pwd)
DB=$ROOT/bench/results.db
[ -f "$DB" ] || {
    echo "results.db not found; run bench/init_db.sh first" >&2; exit 1; }

MD=0
DIFF=0
RID=""
while [ $# -gt 0 ]; do
    case $1 in
        --md)   MD=1; shift ;;
        --diff) DIFF=1; shift ;;
        -h|--help) sed -n '1,16p' "$0"; exit 0 ;;
        *) RID=$1; shift ;;
    esac
done

# Default: latest run.
if [ -z "$RID" ]; then
    RID=$(sqlite3 "$DB" "SELECT id FROM runs ORDER BY id DESC LIMIT 1;")
    [ -n "$RID" ] || { echo "(no runs in DB)" >&2; exit 1; }
fi

# Baseline id (for the Δ column).
BASE=$(sqlite3 "$DB" \
    "SELECT id FROM runs WHERE label LIKE 'published baseline%' LIMIT 1;")

# ----- header ---------------------------------------------------------
if [ "$MD" = 0 ]; then
    sqlite3 "$DB" "
        SELECT 'Run ' || id || '  ' || ts
               || '  commit=' || commit_sha
               || CASE WHEN dirty=1 THEN ' (dirty)' ELSE '' END
               || CASE WHEN host IS NOT NULL THEN '  host=' || host ELSE '' END
        FROM runs WHERE id=$RID;
        SELECT CASE WHEN label  IS NOT NULL THEN 'Label: ' || label  ELSE '' END
        FROM runs WHERE id=$RID;
        SELECT CASE WHEN note   IS NOT NULL THEN 'Note:  ' || note   ELSE '' END
        FROM runs WHERE id=$RID;
    " | grep -v '^$' || true
    echo
fi

# ----- table ----------------------------------------------------------
# Pivot results by bench so each row has sli3 / gs / nest side-by-side.
# Order benches by their natural sort key (B1, B2, B2b, B3, ...).
DIFF_COL=""
if [ "$DIFF" = 1 ]; then
    DIFF_COL=",
    CASE WHEN b1.best IS NULL OR r1.best IS NULL THEN '   --  '
         ELSE printf('%+5.1f %%', 100.0*(r1.best-b1.best)/b1.best)
    END AS dBase"
fi

SQL="
WITH r AS (SELECT bench, impl, best FROM results WHERE run_id=$RID),
     b AS (SELECT bench, impl, best FROM results WHERE run_id=$BASE)
SELECT
    r1.bench                                                    AS bench,
    printf('%6.2f', r1.best)                                    AS sli3,
    CASE WHEN g.best IS NULL THEN '   -- '
         ELSE printf('%6.2f', g.best) END                       AS gs,
    CASE WHEN n.best IS NULL THEN '   -- '
         ELSE printf('%6.2f', n.best) END                       AS nest$DIFF_COL,
    CASE WHEN g.best IS NULL OR r1.best IS NULL THEN '   --  '
         ELSE printf('%+5.1f %%', 100.0*(r1.best-g.best)/g.best)
    END                                                         AS vsGs
FROM r r1
LEFT JOIN r g  ON g.bench=r1.bench  AND g.impl='gs'
LEFT JOIN r n  ON n.bench=r1.bench  AND n.impl='nest'
LEFT JOIN b b1 ON b1.bench=r1.bench AND b1.impl='sli3'
WHERE r1.impl='sli3'
ORDER BY
    CAST(REPLACE(REPLACE(REPLACE(r1.bench,'B',''),'b',''),'c','') AS INTEGER),
    r1.bench;
"

# ----- render ---------------------------------------------------------
if [ "$MD" = 1 ]; then
    # Pure markdown.
    if [ "$DIFF" = 1 ]; then
        echo '| Bench | sli3 | gs | nest | Δ baseline | sli3 vs gs |'
        echo '|---|---:|---:|---:|---:|---:|'
    else
        echo '| Bench | sli3 | gs | nest | sli3 vs gs |'
        echo '|---|---:|---:|---:|---:|'
    fi
    # SQLite emits 'a|b|c'; wrap and space the separators.
    sqlite3 -separator '|' "$DB" "$SQL" \
        | awk -F'|' '{
            for (i=1;i<=NF;i++) {
                gsub(/^ +| +$/, "", $i);
                printf "%s%s", (i==1?"| ":" | "), $i;
            }
            print " |"
        }'
else
    # Aligned table for the terminal.
    sqlite3 -header -column "$DB" "$SQL"
fi

# ----- win-loss tally vs gs (sli3-only benches only) -----------------
if [ "$MD" = 0 ]; then
    echo
    sqlite3 "$DB" "
WITH r AS (SELECT bench, impl, best FROM results WHERE run_id=$RID),
     pairs AS (
       SELECT r1.bench, r1.best AS sli3, g.best AS gs
       FROM r r1 JOIN r g ON g.bench=r1.bench AND g.impl='gs'
       WHERE r1.impl='sli3'
     )
SELECT 'vs gs: ' || sum(CASE WHEN sli3 < gs THEN 1 ELSE 0 END)
                || ' wins, '
                || sum(CASE WHEN sli3 > gs THEN 1 ELSE 0 END)
                || ' losses, '
                || sum(CASE WHEN sli3 = gs THEN 1 ELSE 0 END)
                || ' ties  '
                || '(' || count(*) || ' benches with both)'
FROM pairs;"
fi
