#!/bin/bash
# bench/history.sh -- query helpers for bench/results.db.
#
# Usage:
#   bench/history.sh                       # show 'latest' (default)
#   bench/history.sh latest                # latest run vs baseline
#   bench/history.sh runs                  # list all runs
#   bench/history.sh bench B7              # full history for one bench
#   bench/history.sh compare RUN1 RUN2     # delta between two run ids
#   bench/history.sh table RUN_ID          # full pretty table for one run
#   bench/history.sh raw '...sql...'       # run arbitrary SQL
set -eu

ROOT=$(cd "$(dirname "$0")/.." && pwd)
DB=$ROOT/bench/results.db
[ -f "$DB" ] || { echo "results.db not found; run bench/init_db.sh first" >&2; exit 1; }

cmd=${1:-latest}
shift || true

case "$cmd" in
    runs)
        sqlite3 -header -column "$DB" \
            "SELECT id, ts, commit_sha, dirty, label
             FROM runs ORDER BY id DESC LIMIT 30;"
        ;;

    latest)
        # Show latest sli3 run side-by-side with the baseline and the
        # gs/nest numbers from that latest run.
        LATEST=$(sqlite3 "$DB" "SELECT id FROM runs ORDER BY id DESC LIMIT 1;")
        BASE=$(sqlite3 "$DB" \
            "SELECT id FROM runs WHERE label LIKE 'published baseline%' LIMIT 1;")
        if [ -z "$LATEST" ]; then
            echo "(no runs yet)"; exit 0
        fi
        echo "latest run: $LATEST    baseline run: $BASE"
        sqlite3 -header -column "$DB" "
            WITH cur AS (
                SELECT bench, impl, best FROM results WHERE run_id=$LATEST
            ), base AS (
                SELECT bench, impl, best FROM results WHERE run_id=$BASE
            )
            SELECT
                COALESCE(cur.bench, base.bench)                AS bench,
                COALESCE(cur.impl,  base.impl)                 AS impl,
                CASE WHEN cur.best IS NULL THEN ''
                     ELSE printf('%6.2f', cur.best)  END       AS now_best,
                CASE WHEN base.best IS NULL THEN ''
                     ELSE printf('%6.2f', base.best) END       AS base_best,
                CASE
                    WHEN cur.best IS NULL OR base.best IS NULL THEN ''
                    ELSE printf('%+5.1f %%',
                                100.0 * (cur.best - base.best) / base.best)
                END                                            AS delta
            FROM cur LEFT JOIN base
                  ON cur.bench=base.bench AND cur.impl=base.impl
            UNION
            SELECT base.bench, base.impl,
                   '', printf('%6.2f', base.best), ''
            FROM base LEFT JOIN cur
                  ON cur.bench=base.bench AND cur.impl=base.impl
            WHERE cur.bench IS NULL
            ORDER BY bench, impl;
        "
        ;;

    bench)
        bench=${1:-}
        [ -n "$bench" ] || { echo "usage: bench/history.sh bench <ID>" >&2; exit 1; }
        sqlite3 -header -column "$DB" "
            SELECT r.run_id, runs.ts, runs.commit_sha, r.impl,
                   printf('%6.2f', r.best) AS best, runs.label
            FROM results r
            JOIN runs ON r.run_id=runs.id
            WHERE r.bench='$bench'
            ORDER BY runs.id, r.impl;
        "
        ;;

    compare)
        a=${1:-}; b=${2:-}
        [ -n "$a" ] && [ -n "$b" ] || \
            { echo "usage: bench/history.sh compare <RUN1> <RUN2>" >&2; exit 1; }
        sqlite3 -header -column "$DB" "
            WITH a AS (SELECT bench, impl, best FROM results WHERE run_id=$a),
                 b AS (SELECT bench, impl, best FROM results WHERE run_id=$b)
            SELECT a.bench, a.impl,
                   printf('%6.2f', a.best) AS run_${a},
                   printf('%6.2f', b.best) AS run_${b},
                   CASE WHEN a.best IS NULL OR b.best IS NULL THEN ''
                        ELSE printf('%+5.1f %%',
                                    100.0 * (b.best - a.best) / a.best) END
                       AS delta
            FROM a JOIN b USING (bench, impl)
            ORDER BY a.bench, a.impl;
        "
        ;;

    table)
        rid=${1:-}
        [ -n "$rid" ] || { echo "usage: bench/history.sh table <RUN_ID>" >&2; exit 1; }
        sqlite3 -header -column "$DB" "
            SELECT bench, impl,
                   printf('%6.2f', t1) AS t1,
                   printf('%6.2f', t2) AS t2,
                   printf('%6.2f', t3) AS t3,
                   printf('%6.2f', t4) AS t4,
                   printf('%6.2f', t5) AS t5,
                   printf('%6.2f', best) AS best
            FROM results WHERE run_id=$rid
            ORDER BY bench, impl;
        "
        ;;

    raw)
        sql=${1:-}
        [ -n "$sql" ] || { echo "usage: bench/history.sh raw '<SQL>'" >&2; exit 1; }
        sqlite3 -header -column "$DB" "$sql"
        ;;

    *)
        echo "Unknown command: $cmd" >&2
        sed -n '1,12p' "$0" >&2
        exit 1 ;;
esac
