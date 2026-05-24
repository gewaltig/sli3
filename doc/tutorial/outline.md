# Tutorial outline

This is the working outline that drives chapters 00–11. It also records
which operators each chapter introduces and which harvested examples need
hand-authored replacements (the **gaps**).

## Chapters

| # | File | Topic | Core operators introduced |
|---|------|-------|---------------------------|
| 00 | `00_intro.md` | What SLI is, how to launch `sli3`, the REPL prompt, `quit` | `quit` |
| 01 | `01_stack.md` | RPN explained; pushing and popping; stack inspection | `dup`, `pop`, `exch`, `over`, `count`, `clear`, `index`, `roll`, `stack`, `=` |
| 02 | `02_arithmetic.md` | Numbers and arithmetic; integer vs double | `add`, `sub`, `mul`, `div`, `mod`, `neg`, `abs`, `sqrt`, `pow` |
| 03 | `03_names.md` | Names, the dictstack, procedures | `def`, `load`, `lookup`, `cvx`, `cvlit`, `/foo` literal |
| 04 | `04_conditionals.md` | Booleans and branching | `eq`, `neq`, `gt`, `lt`, `geq`, `leq`, `and`, `or`, `not`, `if`, `ifelse` |
| 05 | `05_loops.md` | Iteration | `repeat`, `for`, `loop`, `exit`, `forall`, `forallindexed` |
| 06 | `06_arrays.md` | Arrays | `[ ]` literal, `length`, `get`, `put`, `append`, `prepend`, `join`, `First`, `Rest`, `array`, `Range` |
| 07 | `07_dicts.md` | Dictionaries and the dictstack | `<< >>` literal, `dict`, `begin`, `end`, `known`, `keys` |
| 08 | `08_types.md` | Types and conversion | `type`, `cvi`, `cvd`, `cvs` |
| 09 | `09_printing.md` | Printing output and inspecting state | `=`, `==`, `=only`, `stack`, `pstack`, `endl`, `<-`, `<--` |
| 10 | `10_examples.md` | Three end-to-end programs | (reuses everything from 01–09) |
| 11 | `11_graphics.md` | The graphics module (optional). Live windows, offscreen PNG, PDF, SVG. Paths, shapes, colors, transparency, text, transforms. | Surface lifecycle: `newpage`, `newoffscreen`, `newpdf`, `newsvg`, `closepage`, `currentpage`, `setpage`, `writepng`, `loadpng`, `drawimage`, `pagesize`, `setwindowtitle`, `resize`. Paths: `newpath`, `moveto`, `lineto`, `rlineto`, `closepath`, `rect`, `circle`, `arc`, `arcn`, `arct`, `curveto`, `rcurveto`. Painting: `stroke`, `fill`, `erasepage`, `clip`, `eoclip`, `clippath`, `currentpoint`, `pathbbox`, `pathforall`, `flattenpath`. Color: `setrgbcolor`, `setrgbacolor`, `setgray`, `sethsbcolor`, `currentrgbcolor`, `currentrgbacolor`, `color`. Line state: `setlinewidth`, `setlinecap`, `setlinejoin`, `setmiterlimit`, `setdash`, `currentlinewidth`, `currentlinecap`, `currentlinejoin`, `currentdash`. State + transforms: `gsave`, `grestore`, `translate`, `scale`, `rotate`, `currentmatrix`, `setmatrix`, `concat`, `initmatrix`, `transform`, `itransform`. Text: `findfont`, `scalefont`, `setfont`, `currentfont`, `show`, `stringwidth`, `textextents`, `charpath`. Display: `showpage`, `flushpage`, `wait`, `windowclosed`. |

## Gaps (operators in the core set without a directly-usable harvested example)

The Phase-2 verifier found ~118 PASSes out of 393 example lines, but the
PASSes are heavily skewed toward `math/`. Most stack, control, name,
array, and dictionary operators either lack a docstring `Examples:` block
in the sources (`pop`, `exch`, `count`, `clear`, `def`, `for`, `repeat`,
`length`, `put`, `append`, `prepend`, `join`, `dict`, `begin`, `end`,
`known`, `keys`, `tic`, `toc`) or have examples that are output-style and
not directly comparable (`if`, `ifelse`).

These are filled in Phase 4 by hand-authored runnable snippets under
`doc/tutorial/examples/` — one short file per chapter section. Each
file ends with `quit`, runs cleanly through `./build/sli3 -`, and prints
the result the chapter prose claims.

## Conventions

- Stack-effect notation introduced once in chapter 01: `add (num num -- num)`.
- Inline code in chapters uses fenced blocks; the prose immediately
  before the block describes the stack state going in and out.
- `core_commands.md` is the reference table linked from every chapter
  for readers who want to look up an operator quickly.
