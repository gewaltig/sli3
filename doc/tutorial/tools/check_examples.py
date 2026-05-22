#!/usr/bin/env python3
# Run every example .sli file under doc/tutorial/examples/ through
# ./build/sli3 and compare the captured stdout to the
# "% Expected output:" block at the top of the file.
#
# This is the regression check for the chapter-paired runnable examples.
# A clean run means every code block the tutorial prose claims will work
# actually does.

from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]
SLI3 = REPO / "build" / "sli3"
EXAMPLES = REPO / "doc" / "tutorial" / "examples"

EXPECTED_HEADER_RE = re.compile(r"^%\s*Expected output:\s*$", re.IGNORECASE)
EXPECTED_LINE_RE = re.compile(r"^%\s{2,}(?P<line>.*)$")


def parse_expected(path: Path) -> list[str]:
    lines: list[str] = []
    in_block = False
    for raw in path.read_text(encoding="utf-8").splitlines():
        if EXPECTED_HEADER_RE.match(raw):
            in_block = True
            continue
        if not in_block:
            continue
        if not raw.startswith("%"):
            break  # blank/code line ends the expected block
        m = EXPECTED_LINE_RE.match(raw)
        if m:
            lines.append(m.group("line").rstrip())
    return lines


def run_file(path: Path, timeout: float = 10.0) -> tuple[int, str, str]:
    proc = subprocess.run(
        [str(SLI3), str(path)],
        capture_output=True,
        text=True,
        timeout=timeout,
    )
    return proc.returncode, proc.stdout, proc.stderr


# sli3 always prints the banner on stdout. Strip it for the comparison.
BANNER_RE = re.compile(
    r"^SLI \d+\.\d+\.\d+.*\n \(C\) 20\d\d Marc-Oliver Gewaltig\n",
    re.MULTILINE,
)


def strip_banner(s: str) -> str:
    return BANNER_RE.sub("", s, count=1)


def main() -> int:
    if not SLI3.exists():
        print(f"ERROR: {SLI3} missing — build first.", file=sys.stderr)
        return 2

    failures: list[str] = []
    skipped: list[str] = []
    passed = 0
    sli_files = sorted(EXAMPLES.rglob("*.sli"))
    for src in sli_files:
        rel = src.relative_to(REPO)
        expected = parse_expected(src)
        if not expected:
            skipped.append(f"{rel}: no expected-output block")
            continue
        try:
            code, out, err = run_file(src)
        except subprocess.TimeoutExpired:
            failures.append(f"{rel}: timeout")
            continue
        if code != 0:
            failures.append(f"{rel}: exit={code}\n  stderr={err.strip()[:200]}")
            continue
        got = strip_banner(out).rstrip("\n").splitlines()
        want = [w.rstrip() for w in expected]
        if got == want:
            passed += 1
        else:
            failures.append(
                f"{rel}: output mismatch\n"
                f"  expected: {want}\n"
                f"  got     : {got}"
            )

    print(f"checked {len(sli_files)} files: PASS {passed}  "
          f"FAIL {len(failures)}  SKIP {len(skipped)}")
    if skipped:
        print()
        print("Skipped:")
        for s in skipped:
            print(f"  {s}")
    if failures:
        print()
        print("Failures:")
        for f in failures:
            print(f"  {f}")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
