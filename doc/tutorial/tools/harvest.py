#!/usr/bin/env python3
# Harvest Examples: blocks from SLI library, C++ builtins, tests, and
# bench scripts into doc/tutorial/examples_raw/ for later verification.
#
# Input sources:
#   lib/sli/*.sli          -- @BeginDocumentation blocks
#   src/builtins/*.cpp     -- BeginDocumentation blocks in C/C++ comments
#   tests/test_sli_eval.cpp, tests/test_array_module.cpp -- EVAL_* macros
#   bench/sli/B*.sli       -- whole-program benchmarks (copied verbatim)
#
# Output: doc/tutorial/examples_raw/<category>/<op>.sli  (+ _index.md)
#
# Each emitted .sli file carries a header comment block recording the
# source location and the original Examples: lines verbatim. No
# auto-construction of "runnable" forms -- the verifier (Phase 2) is
# responsible for turning `expr -> result` annotations into runnable
# snippets at check time.

from __future__ import annotations

import os
import re
import shutil
import sys
from collections import defaultdict
from pathlib import Path
from typing import Iterator, NamedTuple

REPO = Path(__file__).resolve().parents[3]
OUT_ROOT = REPO / "doc" / "tutorial" / "examples_raw"

# Map source file -> category bucket. Files not listed are skipped.
LIB_CATEGORY = {
    "arraylib.sli":      "arrays",
    "FormattedIO.sli":   "io",
    "debug.sli":         "debug",
    "helpinit.sli":      "help",
    "library.sli":       "meta",
    "mathematica.sli":   "math",
    "misc_helpers.sli":  "helpers",
    "oosupport.sli":     "oo",
    "ps-lib.sli":        "ps",
    "regexp.sli":        "regex",
    "sli-init.sli":      "core",
    "typeinit.sli":      "types",
}

CPP_CATEGORY = {
    "sli_math.cpp":           "math",
    "sli_stack.cpp":          "stack",
    "sli_control.cpp":        "control",
    "sli_container_ops.cpp":  "containers",
    "sli_array_module.cpp":   "arrays",
    "sli_typecheck.cpp":      "types",
    "sli_startup.cpp":        "io",
    "sli_io_ops.cpp":         "io",
    "sli_access_ops.cpp":     "access",
    "sli_state_ops.cpp":      "state",
    "sli_trace.cpp":          "debug",
}

# Section keywords used by the runtime help parser (lib/sli/helpinit.sli:38).
# Reused here so the parser stops at the same boundaries.
SECTION_KEYWORDS = (
    "Name:", "Synopsis:", "Description:", "Parameters:", "Options:",
    "Examples:", "Bugs:", "Diagnostics:", "Author:", "FirstVersion:",
    "Remarks:", "Availability:", "References:", "Source:", "Sends:",
    "Receives:", "Transmits:", "SeeAlso:", "Variants:",
)
SECTION_RE = re.compile(r"^\s*(?:" + "|".join(re.escape(k) for k in SECTION_KEYWORDS) + r")(?=\s|$)")

# Docstring block start markers. The closing `*/` ends the block.
SLI_DOC_OPEN_RE = re.compile(r"/\*\*\s*@BeginDocumentation\b")
CPP_DOC_OPEN_RE = re.compile(r"/\*\s*@?BeginDocumentation\b")
DOC_CLOSE_RE = re.compile(r"\*/")

# EVAL_* macros in tests. Capture the first string argument.
EVAL_MACRO_RE = re.compile(
    r'\bEVAL_(?:INT|DOUBLE|BOOL|STRING|DEPTH|CLEAR)\s*\(\s*[A-Za-z_][A-Za-z0-9_]*\s*,\s*'
    r'"((?:[^"\\]|\\.)*)"'
)


class DocBlock(NamedTuple):
    name: str           # raw operator name from Name: line, e.g. 'arraylib::Sum'
    summary: str        # text after " - " on the Name line, may be empty
    synopsis: str       # text from Synopsis: section, may be empty
    examples: list[str] # verbatim lines from Examples: section, leading "% " stripped
    src_file: str       # path relative to repo root
    src_line: int       # 1-based line where the doc block starts


def strip_leading_comment(line: str) -> str:
    """Strip SLI '%' and C-style '*' leading comment markers from a doc-block line."""
    s = line.rstrip("\n")
    # SLI uses '%' for comments outside doc blocks but doc blocks are
    # actual /** ... */ comments -- typically no per-line marker.
    # C-style docs sometimes prefix with ' * '. Strip that too.
    s = re.sub(r"^\s*\*+\s?", "", s)
    return s


def iter_doc_blocks(path: Path, open_re: re.Pattern[str]) -> Iterator[DocBlock]:
    """Yield DocBlock objects for every documentation comment in path."""
    text = path.read_text(encoding="utf-8", errors="replace")
    lines = text.splitlines()
    rel = str(path.relative_to(REPO))

    i = 0
    n = len(lines)
    while i < n:
        if not open_re.search(lines[i]):
            i += 1
            continue
        start_line = i + 1  # 1-based
        # Collect until '*/'.
        body: list[str] = []
        j = i
        # Strip the opener from line[i] -- everything after the marker
        # might already be on the same line (rare).
        opener_tail = open_re.sub("", lines[i], count=1)
        if opener_tail.strip():
            body.append(strip_leading_comment(opener_tail))
        j += 1
        while j < n:
            if DOC_CLOSE_RE.search(lines[j]):
                pre = DOC_CLOSE_RE.split(lines[j], maxsplit=1)[0]
                if pre.strip():
                    body.append(strip_leading_comment(pre))
                break
            body.append(strip_leading_comment(lines[j]))
            j += 1

        block = _parse_block(body, rel, start_line)
        if block is not None:
            yield block
        i = j + 1


def _parse_block(body: list[str], src: str, src_line: int) -> DocBlock | None:
    name = ""
    summary = ""
    synopsis_lines: list[str] = []
    example_lines: list[str] = []

    section = None  # current active section
    for raw in body:
        ln = raw.rstrip()
        if SECTION_RE.match(ln):
            keyword = ln.lstrip().split(":", 1)[0] + ":"
            rest = ln.lstrip()[len(keyword):].strip()
            section = keyword
            if section == "Name:":
                # "Name: foo - one-line summary"
                if " - " in rest:
                    name, summary = rest.split(" - ", 1)
                    name = name.strip()
                    summary = summary.strip()
                else:
                    name = rest.strip()
            elif section == "Synopsis:" and rest:
                synopsis_lines.append(rest)
            elif section == "Examples:" and rest:
                example_lines.append(rest)
            # other sections: ignore content
            continue

        # Continuation of the active section
        if section == "Synopsis:":
            if ln.strip():
                synopsis_lines.append(ln.strip())
        elif section == "Examples:":
            # Preserve indentation differences but trim trailing whitespace
            if ln.strip() or example_lines:
                # Stop on blank line followed by lowercase identifier? No --
                # docstrings sometimes have intentional blank lines inside
                # an Examples block (regexp.sli). Keep blanks; the verifier
                # ignores them.
                example_lines.append(ln)
        # Inside other sections: ignore.

    if not name:
        return None
    return DocBlock(
        name=name,
        summary=summary,
        synopsis=" ".join(synopsis_lines),
        examples=example_lines,
        src_file=src,
        src_line=src_line,
    )


def sanitize_filename(name: str) -> str:
    """Map a SLI operator name to a safe filename stem."""
    translit = {
        "=":  "eq_print",
        "==": "eqeq_print",
        "<-": "write",
        "<--": "pwrite",
        "<<": "mark_open",
        ">>": "dict_close",
        "?":  "help",
    }
    if name in translit:
        return translit[name]
    # arraylib::Sum -> arraylib__Sum
    s = name.replace("::", "__")
    # Replace each remaining non-safe char with its ASCII code.
    out = []
    for ch in s:
        if ch.isalnum() or ch in "_-":
            out.append(ch)
        else:
            out.append(f"_x{ord(ch):02x}")
    return "".join(out)


def write_block(block: DocBlock, category: str) -> Path:
    out_dir = OUT_ROOT / category
    out_dir.mkdir(parents=True, exist_ok=True)
    stem = sanitize_filename(block.name)
    path = out_dir / f"{stem}.sli"
    # Disambiguate collisions (same op documented in multiple sources).
    suffix = 2
    while path.exists():
        path = out_dir / f"{stem}__{suffix}.sli"
        suffix += 1

    header = [
        f"% Harvested example for {block.name}",
        f"% src: {block.src_file}:{block.src_line}",
    ]
    if block.summary:
        header.append(f"% summary: {block.summary}")
    if block.synopsis:
        header.append(f"% synopsis: {block.synopsis}")
    header.append("%")
    header.append("% Examples (verbatim from docstring):")
    for ex in block.examples:
        header.append(f"%   {ex}")
    body = "\n".join(header) + "\n"
    path.write_text(body, encoding="utf-8")
    return path


def harvest_docstrings() -> dict[tuple[str, str], list[Path]]:
    """Return mapping (category, operator) -> list of emitted file paths."""
    index: dict[tuple[str, str], list[Path]] = defaultdict(list)

    for fname, category in LIB_CATEGORY.items():
        p = REPO / "lib" / "sli" / fname
        if not p.exists():
            continue
        for block in iter_doc_blocks(p, SLI_DOC_OPEN_RE):
            if not block.examples:
                continue
            out = write_block(block, category)
            index[(category, block.name)].append(out)

    for fname, category in CPP_CATEGORY.items():
        p = REPO / "src" / "builtins" / fname
        if not p.exists():
            continue
        for block in iter_doc_blocks(p, CPP_DOC_OPEN_RE):
            if not block.examples:
                continue
            out = write_block(block, category)
            index[(category, block.name)].append(out)

    return index


def harvest_tests() -> list[Path]:
    """Pull EVAL_* macro snippets from test files into examples_raw/tests/."""
    out_dir = OUT_ROOT / "tests"
    out_dir.mkdir(parents=True, exist_ok=True)
    written: list[Path] = []

    targets = [
        REPO / "tests" / "test_sli_eval.cpp",
        REPO / "tests" / "test_array_module.cpp",
    ]
    for src in targets:
        if not src.exists():
            continue
        text = src.read_text(encoding="utf-8", errors="replace")
        snippets: list[tuple[int, str]] = []
        for m in EVAL_MACRO_RE.finditer(text):
            # Decode common escape sequences (\\n, \\", \\\\).
            raw = m.group(1)
            decoded = bytes(raw, "utf-8").decode("unicode_escape")
            line_no = text.count("\n", 0, m.start()) + 1
            snippets.append((line_no, decoded))
        if not snippets:
            continue
        rel = src.relative_to(REPO)
        out_path = out_dir / f"{src.stem}.sli"
        lines = [
            f"% Harvested snippets from {rel}",
            "% Each '%% src:' line names the C++ source line of the EVAL_* macro.",
            "% Each '>>' line is a self-contained SLI snippet expected to leave",
            "% something on the operand stack.",
            "",
        ]
        for ln, snippet in snippets:
            lines.append(f"% src: {rel}:{ln}")
            lines.append(f">> {snippet}")
        out_path.write_text("\n".join(lines) + "\n", encoding="utf-8")
        written.append(out_path)

    return written


def harvest_bench() -> list[Path]:
    """Copy bench/sli/B*.sli verbatim into examples_raw/bench/."""
    out_dir = OUT_ROOT / "bench"
    out_dir.mkdir(parents=True, exist_ok=True)
    written: list[Path] = []
    bench_dir = REPO / "bench" / "sli"
    if not bench_dir.exists():
        return written
    for f in sorted(bench_dir.glob("B*.sli")):
        dst = out_dir / f.name
        shutil.copy2(f, dst)
        written.append(dst)
    return written


def write_index(doc_index: dict[tuple[str, str], list[Path]],
                test_files: list[Path],
                bench_files: list[Path]) -> None:
    lines = [
        "# Harvested examples index",
        "",
        "Generated by `doc/tutorial/tools/harvest.py`.",
        "Each row links to the harvested file and notes the source.",
        "",
        "## Operator docstring examples",
        "",
        "| Category | Operator | Files |",
        "|----------|----------|-------|",
    ]
    for (category, op) in sorted(doc_index):
        files = doc_index[(category, op)]
        rels = ", ".join(f"[{p.relative_to(OUT_ROOT)}]({p.relative_to(OUT_ROOT)})" for p in files)
        lines.append(f"| {category} | `{op}` | {rels} |")

    if test_files:
        lines += ["", "## Test snippets (EVAL_* macros)", ""]
        for p in test_files:
            rel = p.relative_to(OUT_ROOT)
            lines.append(f"- [{rel}]({rel})")

    if bench_files:
        lines += ["", "## Benchmark programs", ""]
        for p in bench_files:
            rel = p.relative_to(OUT_ROOT)
            lines.append(f"- [{rel}]({rel})")

    (OUT_ROOT / "_index.md").write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    if OUT_ROOT.exists():
        shutil.rmtree(OUT_ROOT)
    OUT_ROOT.mkdir(parents=True)

    docs = harvest_docstrings()
    tests = harvest_tests()
    bench = harvest_bench()
    write_index(docs, tests, bench)

    # Summary
    doc_count = sum(len(v) for v in docs.values())
    print(f"harvested {doc_count} docstring example files "
          f"({len(docs)} unique operators), "
          f"{len(tests)} test files, "
          f"{len(bench)} bench programs")
    return 0


if __name__ == "__main__":
    sys.exit(main())
