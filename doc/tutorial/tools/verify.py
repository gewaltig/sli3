#!/usr/bin/env python3
# Verify harvested examples against ./build/sli3.
#
# Reads doc/tutorial/examples_raw/<category>/<op>.sli files produced by
# harvest.py, parses each "% expr -> result" annotation, runs the
# expression through `./build/sli3 -` with `expr ==` appended, and
# compares the captured stdout to the expected result.
#
# Writes a tab-separated status report to
# doc/tutorial/examples_raw/_status.tsv and prints a per-category
# summary to stdout.
#
# This is a first-pass tool. Lines whose expected result is a multi-token
# stack shape, a type-tag placeholder, an error marker, or a Synopsis-like
# stack-effect notation are SKIPped with a reason. Phase 4 (tutorial
# authoring) only uses examples that PASS here.

from __future__ import annotations

import re
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]
RAW = REPO / "doc" / "tutorial" / "examples_raw"
SLI3 = REPO / "build" / "sli3"
STATUS_PATH = RAW / "_status.tsv"

EXAMPLE_LINE_RE = re.compile(r"^%\s{2,}(?P<body>.*)$")
SEPARATOR_RE = re.compile(r"\s*->\s*")

# Right-hand-sides we cannot meaningfully compare to a single `==` output.
SKIP_RHS_PATTERNS = (
    (re.compile(r"<[A-Za-z]+type>$"),                "type-tag placeholder"),
    (re.compile(r"<ERROR>$"),                        "intentional error"),
    (re.compile(r"^any(\s+any)*$"),                  "stack-effect placeholder"),
    (re.compile(r"^(int|integer|double|number|num|bool|boolean|string|array|dict|name|literal|proc|procedure|trie|literalname)(\s+(int|integer|double|number|num|bool|boolean|string|array|dict|name|literal|proc|procedure|trie|literalname))*$"),
                                                      "stack-effect placeholder"),
)


@dataclass
class Check:
    file: Path
    expr: str
    expected: str
    runnable: str
    status: str    # PASS, FAIL, SKIP
    got: str       # captured stdout (stripped)
    reason: str    # skip reason or fail detail


def parse_examples(path: Path) -> list[tuple[str, str]]:
    """Pull (expr, expected) pairs from the Examples block of a harvested file.

    Looks for lines beginning with '%   ' (two or more spaces after the
    SLI comment marker) appearing after the 'Examples (verbatim ...)'
    header. Each such line is split on the first ' -> ' separator. Lines
    without that separator are ignored.
    """
    pairs: list[tuple[str, str]] = []
    text = path.read_text(encoding="utf-8", errors="replace")
    in_examples = False
    for line in text.splitlines():
        if line.startswith("% Examples"):
            in_examples = True
            continue
        if not in_examples:
            continue
        m = EXAMPLE_LINE_RE.match(line)
        if not m:
            continue
        body = m.group("body").strip()
        if not body:
            continue
        if SEPARATOR_RE.search(body) is None:
            continue
        expr, expected = SEPARATOR_RE.split(body, maxsplit=1)
        expr = expr.strip()
        expected = expected.strip()
        if expr and expected:
            pairs.append((expr, expected))
    return pairs


def classify_skip(expected: str) -> str | None:
    """Return a skip reason if expected RHS isn't directly comparable."""
    for pat, reason in SKIP_RHS_PATTERNS:
        if pat.match(expected):
            return reason
    # Multi-token RHS like '2 2' (from `2 dup -> 2 2`): the runnable form
    # `expr ==` only prints the top of stack. Skip and flag for manual
    # review.
    tokens = expected.split()
    if len(tokens) > 1 and not (expected.startswith("[") or expected.startswith("(") or expected.startswith("<<")):
        return "multi-value stack"
    return None


def run_sli(snippet: str, timeout: float = 5.0) -> tuple[int, str, str]:
    """Run snippet through ./build/sli3 -; return (exit, stdout, stderr)."""
    proc = subprocess.run(
        [str(SLI3), "-"],
        input=snippet + "\nquit\n",
        capture_output=True,
        text=True,
        timeout=timeout,
    )
    return proc.returncode, proc.stdout, proc.stderr


def normalise(s: str) -> str:
    """Whitespace-normalise and trim a captured result for comparison."""
    return " ".join(s.split())


_FLOAT_RE = re.compile(r"^-?\d+\.\d+(e[+-]?\d+)?$", re.IGNORECASE)


def numerically_equal(got: str, want: str, rel_tol: float = 1e-4) -> bool:
    """True if got and want look like floats and agree within rel_tol.

    Handles docstring examples that show 6 sig-figs (e.g. 1.666667) while
    the runtime prints 15 (e.g. 1.66666666666667).
    """
    if not (_FLOAT_RE.match(got) and _FLOAT_RE.match(want)):
        return False
    try:
        g = float(got)
        w = float(want)
    except ValueError:
        return False
    if w == 0:
        return abs(g) < rel_tol
    return abs(g - w) / abs(w) < rel_tol


def verify_one(path: Path) -> list[Check]:
    checks: list[Check] = []
    pairs = parse_examples(path)
    for expr, expected in pairs:
        runnable = f"{expr} =="
        skip_reason = classify_skip(expected)
        if skip_reason:
            checks.append(Check(path, expr, expected, runnable,
                                "SKIP", "", skip_reason))
            continue
        try:
            code, out, err = run_sli(runnable)
        except subprocess.TimeoutExpired:
            checks.append(Check(path, expr, expected, runnable,
                                "FAIL", "", "timeout"))
            continue
        got = normalise(out)
        want = normalise(expected)
        if code != 0:
            checks.append(Check(path, expr, expected, runnable,
                                "FAIL", got, f"exit={code} err={err.strip()[:80]}"))
            continue
        if got == want or numerically_equal(got, want):
            checks.append(Check(path, expr, expected, runnable,
                                "PASS", got, ""))
        else:
            checks.append(Check(path, expr, expected, runnable,
                                "FAIL", got, "output mismatch"))
    return checks


def main() -> int:
    if not SLI3.exists():
        print(f"ERROR: {SLI3} not found. Run `cmake --build build -j` first.",
              file=sys.stderr)
        return 2

    sources: list[Path] = []
    for sub in sorted(RAW.iterdir()):
        if not sub.is_dir():
            continue
        if sub.name in ("bench", "tests", "_excluded"):
            continue
        for f in sorted(sub.glob("*.sli")):
            sources.append(f)

    all_checks: list[Check] = []
    for src in sources:
        all_checks.extend(verify_one(src))

    # Write TSV.
    with STATUS_PATH.open("w", encoding="utf-8") as fh:
        fh.write("status\tfile\texpr\texpected\tgot\treason\n")
        for c in all_checks:
            rel = c.file.relative_to(RAW)
            fh.write("\t".join([
                c.status,
                str(rel),
                c.expr.replace("\t", " "),
                c.expected.replace("\t", " "),
                c.got.replace("\t", " "),
                c.reason.replace("\t", " "),
            ]) + "\n")

    # Summary.
    counts = {"PASS": 0, "FAIL": 0, "SKIP": 0}
    by_cat: dict[str, dict[str, int]] = {}
    for c in all_checks:
        counts[c.status] += 1
        cat = c.file.relative_to(RAW).parts[0]
        by_cat.setdefault(cat, {"PASS": 0, "FAIL": 0, "SKIP": 0})
        by_cat[cat][c.status] += 1

    print(f"checked {len(all_checks)} example lines from {len(sources)} files")
    print(f"  PASS {counts['PASS']:4}  FAIL {counts['FAIL']:4}  SKIP {counts['SKIP']:4}")
    print("by category:")
    for cat in sorted(by_cat):
        b = by_cat[cat]
        print(f"  {cat:14}  PASS {b['PASS']:3}  FAIL {b['FAIL']:3}  SKIP {b['SKIP']:3}")
    print(f"detailed status: {STATUS_PATH.relative_to(REPO)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
