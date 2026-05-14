#!/bin/bash
# bench/init_db.sh -- create bench/results.db (if missing) and
# seed it with the published "baseline" bench numbers from
# fix-plan.md. Idempotent: re-running does NOT duplicate rows.
#
# Schema:
#   runs(id, ts, commit_sha, dirty, label, host, note)
#   results(run_id, bench, impl, t1..t5, best)
#
# impl ∈ {sli3, nest, gs}. For pre-populated baselines we only
# have "best" (no per-run samples), so t1..t5 are NULL.
set -eu

ROOT=$(cd "$(dirname "$0")/.." && pwd)
DB=$ROOT/bench/results.db

sqlite3 "$DB" <<'SQL'
PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS runs (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    ts          TEXT    NOT NULL DEFAULT (datetime('now')),
    commit_sha  TEXT    NOT NULL,
    dirty       INTEGER NOT NULL DEFAULT 0,
    label       TEXT,
    host        TEXT,
    note        TEXT
);

CREATE TABLE IF NOT EXISTS results (
    run_id  INTEGER NOT NULL REFERENCES runs(id) ON DELETE CASCADE,
    bench   TEXT    NOT NULL,
    impl    TEXT    NOT NULL CHECK (impl IN ('sli3', 'nest', 'gs')),
    t1 REAL, t2 REAL, t3 REAL, t4 REAL, t5 REAL,
    best    REAL    NOT NULL,
    PRIMARY KEY (run_id, bench, impl)
);

CREATE INDEX IF NOT EXISTS idx_results_bench_impl ON results(bench, impl);
CREATE INDEX IF NOT EXISTS idx_runs_commit        ON runs(commit_sha);
CREATE INDEX IF NOT EXISTS idx_runs_ts            ON runs(ts);
SQL

# ----- baseline seed --------------------------------------------------
# Numbers from fix-plan.md's "Bench standing" table (the post-Axis-II
# step-2 published baseline), tagged with the closest doc-sync commit
# (80d5695, "docs: catch up to Axis I slice 8 + Axis II step 2").
#
# Idempotent: only insert if no row exists with this commit + label.

HAS_BASELINE=$(sqlite3 "$DB" \
  "SELECT count(*) FROM runs WHERE commit_sha='80d5695' AND label='published baseline (post-Axis-II step 2)';")

if [ "$HAS_BASELINE" -eq 0 ]; then
  # Two-phase: insert the run, capture its id, then bulk-insert
  # results with that id. We can't rely on last_insert_rowid()
  # across multiple INSERTs because results' implicit rowid bumps
  # it after each row.
  RUN_ID=$(sqlite3 "$DB" "
    INSERT INTO runs (ts, commit_sha, dirty, label, host, note)
    VALUES (
        '2026-05-13 00:00:00',
        '80d5695',
        0,
        'published baseline (post-Axis-II step 2)',
        NULL,
        'Seeded from fix-plan.md ''Bench standing'' table. Per-sample t1..t5 not retained; only best-of-five is recorded.'
    );
    SELECT last_insert_rowid();
  ")

  sqlite3 "$DB" <<SQL
BEGIN;
INSERT INTO results (run_id, bench, impl, t1, t2, t3, t4, t5, best) VALUES
    ($RUN_ID, 'B1',  'sli3', NULL, NULL, NULL, NULL, NULL, 0.85),
    ($RUN_ID, 'B1',  'gs',   NULL, NULL, NULL, NULL, NULL, 1.32),
    ($RUN_ID, 'B1',  'nest', NULL, NULL, NULL, NULL, NULL, 2.01),
    ($RUN_ID, 'B2',  'sli3', NULL, NULL, NULL, NULL, NULL, 1.81),
    ($RUN_ID, 'B2',  'gs',   NULL, NULL, NULL, NULL, NULL, 2.69),
    ($RUN_ID, 'B2',  'nest', NULL, NULL, NULL, NULL, NULL, 4.34),
    ($RUN_ID, 'B2b', 'sli3', NULL, NULL, NULL, NULL, NULL, 1.54),
    ($RUN_ID, 'B2b', 'gs',   NULL, NULL, NULL, NULL, NULL, 1.22),
    ($RUN_ID, 'B2b', 'nest', NULL, NULL, NULL, NULL, NULL, 3.39),
    ($RUN_ID, 'B3',  'sli3', NULL, NULL, NULL, NULL, NULL, 1.53),
    ($RUN_ID, 'B3',  'gs',   NULL, NULL, NULL, NULL, NULL, 2.58),
    ($RUN_ID, 'B3',  'nest', NULL, NULL, NULL, NULL, NULL, 4.30),
    ($RUN_ID, 'B5',  'sli3', NULL, NULL, NULL, NULL, NULL, 1.49),
    ($RUN_ID, 'B5',  'gs',   NULL, NULL, NULL, NULL, NULL, 2.49),
    ($RUN_ID, 'B5',  'nest', NULL, NULL, NULL, NULL, NULL, 4.32),
    ($RUN_ID, 'B7',  'sli3', NULL, NULL, NULL, NULL, NULL, 2.15),
    ($RUN_ID, 'B7',  'gs',   NULL, NULL, NULL, NULL, NULL, 1.99),
    ($RUN_ID, 'B8',  'sli3', NULL, NULL, NULL, NULL, NULL, 1.47),
    ($RUN_ID, 'B8',  'gs',   NULL, NULL, NULL, NULL, NULL, 1.01),
    ($RUN_ID, 'B9',  'sli3', NULL, NULL, NULL, NULL, NULL, 2.11),
    ($RUN_ID, 'B9',  'gs',   NULL, NULL, NULL, NULL, NULL, 1.71),
    ($RUN_ID, 'B9',  'nest', NULL, NULL, NULL, NULL, NULL, 4.27),
    ($RUN_ID, 'B10', 'sli3', NULL, NULL, NULL, NULL, NULL, 1.79),
    ($RUN_ID, 'B10', 'gs',   NULL, NULL, NULL, NULL, NULL, 1.90);
COMMIT;
SQL
  echo "Seeded baseline run for commit 80d5695 (run_id=$RUN_ID)."
else
  echo "Baseline already present; skipping seed."
fi

echo "DB at: $DB"
sqlite3 "$DB" "SELECT count(*) || ' runs, ' || (SELECT count(*) FROM results) || ' result rows.' FROM runs;"
