# 11. Graphics

This chapter introduces the graphics module — an optional 2D drawing
surface that lets a SLI program produce live windows or save pictures
to PNG, PDF, and SVG files. The graphics operators are
PostScript-flavored, but **this chapter does not assume any prior
PostScript knowledge**. By the end you will be able to write SLI
programs that draw shapes, text, and curves in colors and save them as
files.

The module is optional. If `sli3` was built with `-DSLI3_GRAPHICS=OFF`
the operators below are absent. To check at runtime:

```
statusdict /have_graphics get =
true
```

A return of `false` means the build is graphics-off. Rebuild with
`-DSLI3_GRAPHICS=ON` (the default) to enable.

## The mental model

A *page* is a drawing surface — a rectangular canvas of pixels (or, for
PDF / SVG, a vector backing store). You open one, draw on it, then
close it. Every drawing operator works on the *current page*; you do
not pass the page through every call.

There are four kinds of pages:

| Kind | Operator | What it is |
|---|---|---|
| Window | `newpage` | A live SDL2 window you see on screen. |
| Offscreen | `newoffscreen` | An in-memory image, no window. |
| PDF | `newpdf` | A vector-backed page that streams to a `.pdf`. |
| SVG | `newsvg` | A single SVG file. |

Drawing operators are the same for all four. The only difference is
what `showpage` does at the end (present, no-op, or start a new page
in the file).

**Coordinates** run with the X axis pointing right and the **Y axis
pointing up** (like graph paper, unlike the screen). The origin
`(0, 0)` is at the bottom-left corner of the page.

**The path-then-paint model.** Most graphics operators do *not* draw
anything by themselves. They build up an invisible *path* — a list of
line and curve segments. Painting that path happens in a second step,
either as a stroke (outline) or a fill (filled interior).

```
newpath        % start with an empty path
0 0 moveto     % move the "pen" to (0, 0) without drawing
100 100 lineto % add a line segment to (100, 100)
stroke         % NOW actually draw the line
```

If you forget the `stroke` (or `fill`), nothing appears. The path is
built but never painted.

## Your first picture

Save the following as `first.sli`:

```sli
% Open a 200x150 offscreen surface, fill it with a red rectangle,
% write the result to /tmp/first.png.
200 150 newoffscreen pop
1 0 0 setrgbcolor
20 20 160 110 rect fill
(/tmp/first.png) currentpage writepng
currentpage closepage
quit
```

Run it:

```
./build/sli3 first.sli
```

Open `/tmp/first.png` in any image viewer (`open /tmp/first.png` on
macOS). You should see a red rectangle on a white background.

Line by line:

- `200 150 newoffscreen pop` opens a 200×150 in-memory surface. The
  `pop` discards the page object that `newoffscreen` left on the
  stack — we read it later through `currentpage` instead.
- `1 0 0 setrgbcolor` sets the current color to pure red. Each
  component is in the range 0–1.
- `20 20 160 110 rect fill` adds a rectangle at `(x=20, y=20)` of
  width 160 and height 110 to the path, then `fill` paints the
  rectangle's interior with the current color.
- `(/tmp/first.png) currentpage writepng` saves the current surface
  as a PNG. `currentpage` retrieves the active page; `writepng` takes
  a filename and a page.
- `currentpage closepage` releases the surface.

## Live window

To see your drawing in a live window instead of writing a file,
replace `newoffscreen` with `newpage` and add a wait:

```sli
% Open a 200x150 window for 4 seconds.
200 150 newpage pop
1 0 0 setrgbcolor
20 20 160 110 rect fill
showpage
4 wait
currentpage closepage
quit
```

- `newpage` takes width and height; the window opens immediately.
- `showpage` flushes the in-memory drawing buffer to the visible
  window. Without it the window stays white.
- `4 wait` blocks for up to 4 seconds while pumping window events
  (so the window can be dragged etc.). It returns early if you close
  the window.
- `closepage` tears the window down.

The rest of this chapter uses `newoffscreen` + `writepng` so the
examples are reproducible. To run them as live windows, swap the
opener and add `showpage 4 wait` before `closepage`.

## Paths in detail

A path is a sequence of segments. The basic builders are:

| Op | Stack | Effect |
|---|---|---|
| `newpath` | `--` | Drop the existing path. |
| `moveto` | `x y --` | Move the "pen" to `(x, y)` without drawing. |
| `lineto` | `x y --` | Add a line segment from the current pen position to `(x, y)`. |
| `rlineto` | `dx dy --` | Like `lineto` but `dx, dy` are relative to the current position. |
| `closepath` | `--` | Draw a line back to the most recent `moveto` point. |

Then `stroke` paints the outline and `fill` paints the interior:

```sli
% A stroked triangle. Save as triangle.sli.
200 200 newoffscreen pop
color /white get setrgbcolor 0 0 200 200 rect fill   % white background
color /black get setrgbcolor 3 setlinewidth
newpath
40  40 moveto
160 40 lineto
100 170 lineto
closepath
stroke
(/tmp/triangle.png) currentpage writepng
currentpage closepage
quit
```

The `color /white get` form looks up a color from the **color
dictionary** — a pre-built dictionary of named colors:

```
color /red    get =      [1 0 0]
color /blue   get =      [0 0 1]
color /orange get =      [1 0.65 0]
```

Each entry is a 3-element array `[r g b]` of doubles in `0..1`.
`setrgbcolor` accepts either three scalars (`1 0 0 setrgbcolor`)
or an array (`[1 0 0] setrgbcolor`).

Available names: `black`, `white`, `gray`/`grey`, `darkgray`,
`lightgray`, `red`, `green` (CSS half-bright), `lime` (pure green),
`blue`, `cyan`, `magenta`, `yellow`, `orange`, `purple`, `brown`,
`pink`, `navy`, `olive`, `teal`, `silver`.

To paint the triangle's interior instead of just the outline, swap
the last `stroke` for `fill`. The path doesn't change; only the
painting does.

## Built-in shapes

Three shapes are shortcuts for common paths:

| Op | Stack | What it adds |
|---|---|---|
| `rect` | `x y w h --` | An axis-aligned rectangle. |
| `circle` | `cx cy r --` | A full circle of radius `r`. |
| `arc` | `cx cy r a1 a2 --` | A circular arc from angle `a1` to `a2`, both in degrees. |

`rect`, `circle`, and `arc` add to the *current path* — you still need
`stroke` or `fill` afterwards.

```sli
% Three shapes side by side. Save as shapes.sli.
300 150 newoffscreen pop
color /white get setrgbcolor 0 0 300 150 rect fill

% Filled square on the left
color /red get setrgbcolor
20 25 80 100 rect fill

% Stroked circle in the middle
color /navy get setrgbcolor 3 setlinewidth
150 75 45 circle stroke

% Half arc on the right (90 to 270 degrees = left half of a circle)
color /purple get setrgbcolor 4 setlinewidth
250 75 45 90 270 arc stroke

(/tmp/shapes.png) currentpage writepng
currentpage closepage
quit
```

## Color and transparency

Four operators set the current color:

| Op | Stack | Notes |
|---|---|---|
| `setrgbcolor` | `r g b --` or `[r g b] --` or `[r g b a] --` | Polymorphic. |
| `setrgbacolor` | `r g b a --` or `[r g b a] --` | Explicit alpha. |
| `setgray` | `g --` | Grayscale. |
| `sethsbcolor` | `h s b --` | HSB color. |

The fourth component of `setrgbacolor` (or the 4-element array form
of `setrgbcolor`) is alpha — `0` is fully transparent, `1` is fully
opaque. Translucent shapes blend with what is underneath:

```sli
% Three translucent rectangles whose overlaps blend visibly.
240 160 newoffscreen pop
color /white get setrgbcolor 0 0 240 160 rect fill

[1 0 0 0.5]   setrgbcolor  20  20 120 80 rect fill
[0 0 1 0.5]   setrgbcolor  80  60 120 80 rect fill
[0 0.7 0 0.5] setrgbcolor  50  80 120 80 rect fill

(/tmp/alpha.png) currentpage writepng
currentpage closepage
quit
```

The `color` dictionary itself is read-only, so you can't `put` directly
into it. To add your own (e.g. translucent) entries, clone it into a
dictionary you control:

```sli
/mycolor color clonedict def
mycolor /red50 [1 0 0 0.5] put
mycolor /red50 get setrgbcolor
```

## Line settings

Strokes have width and end-cap style:

```sli
3 setlinewidth          % stroke width, default 2
0 setlinecap            % 0=butt, 1=round, 2=square
1 setlinejoin           % 0=miter, 1=round, 2=bevel
[6 3] 0 setdash         % 6-on, 3-off dash pattern; offset 0
[] 0 setdash            % solid (empty pattern resets)
```

A dashed horizontal line:

```sli
300 80 newoffscreen pop
color /white get setrgbcolor 0 0 300 80 rect fill
color /darkgray get setrgbcolor
2 setlinewidth
[8 4] 0 setdash
20 40 moveto 280 40 lineto stroke
(/tmp/dashed.png) currentpage writepng
currentpage closepage
quit
```

## Text

To render text you need a font. A *font* in this module is a
dictionary with four entries:

| Key | Value |
|---|---|
| `/Family` | font family name (string) — `(sans)`, `(serif)`, `(monospace)`, or any installed font name |
| `/Slant`  | `/normal`, `/italic`, or `/oblique` |
| `/Weight` | `/normal` or `/bold` |
| `/Size`   | the font size in user units (a number) |

Four operators set up and use fonts:

| Op | Stack | Effect |
|---|---|---|
| `findfont` | `name-or-string -- font` | Build a font dict for the named family with sensible defaults (Slant=normal, Weight=normal, Size=12). |
| `scalefont` | `font size -- font'` | Return a *new* dict with `/Size` set. |
| `setfont` | `font --` | Install the font for subsequent text ops. |
| `show` | `text --` | Render `text` starting at the current point. |

```sli
% Three fonts at the same size.
400 200 newoffscreen pop
color /white get setrgbcolor 0 0 400 200 rect fill
color /black get setrgbcolor

(sans)       findfont 18 scalefont setfont
20 150 moveto (Hello, sli3 — sans) show

(serif)      findfont 18 scalefont setfont
20 100 moveto (Hello, sli3 — serif) show

(monospace)  findfont 18 scalefont setfont
20  50 moveto (Hello, sli3 — monospace) show

(/tmp/text.png) currentpage writepng
currentpage closepage
quit
```

`show` draws at the current point and advances it; if you want the
next text on a different line, `moveto` again first. Calling `show`
(or `charpath`) without a prior `moveto` raises `NoCurrentPointError`
— same rule as PostScript.

To know how wide a piece of text will be, ask `stringwidth`:

```sli
(Hello) stringwidth =only ( ) =only =
30.6738... 0
```

The two return values are X advance and Y advance.

## Transformations

The drawing model has a *current transformation matrix* (CTM) that
maps your drawing coordinates to physical pixels. Three operators
modify it:

| Op | Stack | Effect |
|---|---|---|
| `translate` | `dx dy --` | Shift the origin by `(dx, dy)`. |
| `scale` | `sx sy --` | Multiply X and Y by these factors. |
| `rotate` | `deg --` | Rotate the coordinate frame counter-clockwise by `deg` degrees. |

These compose: a `translate` followed by a `rotate` rotates around
the new origin. They affect every subsequent drawing operator until
you reset.

To reset, you push the current state and pop it back:

| Op | Stack | Effect |
|---|---|---|
| `gsave`    | `--` | Push CTM and color/width/etc onto an internal stack. |
| `grestore` | `--` | Pop, restoring the saved state. |

A common pattern: surround a sub-drawing with `gsave ... grestore`
so that the transformations don't leak out.

```sli
% Rotated stars around a center point.
300 300 newoffscreen pop
color /white get setrgbcolor 0 0 300 300 rect fill

/star
{
  newpath
   0   30 moveto       9   9 lineto    29   9 lineto
  14  -5 lineto       21 -24 lineto    0 -12 lineto
 -21 -24 lineto      -14  -5 lineto  -29   9 lineto
  -9   9 lineto       closepath
  fill
} def

color /orange get setrgbcolor
0 1 5
{
  gsave
    150 150 translate            % move to center
    60 mul rotate                % rotate by 0, 60, 120, ...
    0 60 translate               % go outward along the new Y axis
    star
  grestore
} for

(/tmp/stars.png) currentpage writepng
currentpage closepage
quit
```

`gsave ... grestore` is essential here. Without it, each star would
inherit the rotation and translation of the previous one and the
result would spiral off the page.

## Saving and loading images

You have already seen `writepng`. Two other backends produce *vector*
output:

```sli
% Multi-page PDF
(/tmp/multi.pdf) 200 200 newpdf pop
color /red get setrgbcolor  50 50 100 100 rect fill
showpage                                            % start a new page
color /blue get setrgbcolor 75 75  50  50 rect fill
currentpage closepage
quit
```

`newpdf` takes the filename and dimensions. `showpage` on a PDF
backend means "finish this page, start the next." `closepage`
flushes and closes the file.

`newsvg` is the same shape but writes a single SVG file (Cairo's SVG
backend only renders the first page in most viewers).

To paint an existing PNG onto your current page, use `loadpng` and
`drawimage`:

```sli
% Composite an existing PNG twice at different sizes.
400 200 newoffscreen pop
color /lightgray get setrgbcolor 0 0 400 200 rect fill
(/tmp/first.png) loadpng           % returns a page object
dup dup                            % three copies on the stack
 20  40 100 80 drawimage           % first drawimage consumes one
150  30 200 140 drawimage          % second consumes another
closepage                          % third gets closed here
(/tmp/composite.png) currentpage writepng
currentpage closepage
quit
```

`loadpng` reads a file into an offscreen page. `drawimage` takes the
source page and a destination rectangle `(cx, cy, w, h)` in your
current page's coordinates. The image is scaled to fit.

## A worked example

Putting the pieces together: a labeled bar chart.

```sli
% A simple bar chart. Save as barchart.sli.
500 300 newoffscreen pop
color /white get setrgbcolor 0 0 500 300 rect fill

% Title
(sans) findfont 16 scalefont setfont
color /black get setrgbcolor
20 270 moveto (Quarterly sales) show

% Axis
2 setlinewidth color /darkgray get setrgbcolor
40 40 moveto 460 40 lineto stroke
40 40 moveto  40 240 lineto stroke

% Data: forall accumulator pattern. We push the column index 0
% BELOW the array; the body sees `idx item` on entry, consumes
% both, and pushes idx+1 as the new accumulator.
0
[ [(Q1) 80 /red] [(Q2) 130 /orange] [(Q3) 110 /green] [(Q4) 170 /blue] ]
{
  /:item exch def           % element on top -> :item
  /:idx  exch def           % accumulator below -> :idx
  /:label :item 0 get def
  /:value :item 1 get def
  /:cname :item 2 get def
  /:x 70 :idx 100 mul add def

  % Bar
  color :cname get setrgbcolor
  :x 40 60 :value rect fill

  % Label below
  (sans) findfont 11 scalefont setfont
  color /black get setrgbcolor
  :x 24 moveto :label show

  :idx 1 add
} forall pop

(/tmp/barchart.png) currentpage writepng
currentpage closepage
quit
```

This combines color lookup, paths, text, and a `forall` loop over a
nested array. The `gsave`/`grestore` from the previous example isn't
needed here because the transformations are all done by the explicit
`x` calculation, not by CTM changes.

## Gradients

A gradient is a pattern that smoothly transitions between colors.
Four operators build and install one:

| Op | Stack | Effect |
|---|---|---|
| `linearpattern` | `x0 y0 x1 y1 -- pattern` | Linear gradient along the line `(x0,y0)→(x1,y1)`. |
| `radialpattern` | `cx0 cy0 r0 cx1 cy1 r1 -- pattern` | Radial gradient between two circles. |
| `addcolorstop` | `pattern off r g b --` or `pattern off r g b a --` | Add a color stop to `pattern` at offset `off` ∈ `[0, 1]`. |
| `setpattern` | `pattern --` | Install the pattern as the current source. |

Build the gradient first, add stops to it, then install:

```sli
% Horizontal red-to-orange-to-yellow band, then a soft radial bubble.
400 250 newoffscreen pop
color /white get setrgbcolor 0 0 400 250 rect fill

0 0 400 0 linearpattern              % left edge to right edge
dup 0.0 1.0 0.0 0.0 addcolorstop     % red    at left
dup 0.5 1.0 0.6 0.0 addcolorstop     % orange in the middle
dup 1.0 1.0 1.0 0.0 addcolorstop     % yellow at right
setpattern
20 150 360 60 rect fill

200 70 0  200 70 70 radialpattern    % point at (200,70) -> circle r=70
dup 0.0 1.0 1.0 1.0 1.0 addcolorstop % opaque white center
dup 1.0 0.2 0.2 0.8 0.0 addcolorstop % transparent blue edge
setpattern
200 70 70 circle fill

(/tmp/gradient.png) currentpage writepng
currentpage closepage
quit
```

The `dup` before each `addcolorstop` is because `addcolorstop`
consumes its pattern operand; we duplicate so the *same* pattern
accumulates all the stops before being handed to `setpattern`. The
final `setpattern` consumes the last copy.

The 6-component form `pattern off r g b a addcolorstop` carries
alpha; the 5-component form is opaque. Mix them freely.

The pattern can be used for either `fill` or `stroke`. Patterns are
refcounted; you can hold one in a variable and reuse it across
multiple paints.

## Compositing operators

Every paint operation (`stroke`, `fill`, `drawimage`) combines the
new pixels with what's already on the surface. By default that
combination is "source over destination" — standard alpha blending.
You can replace that step with one of ~30 alternatives:

```sli
/multiply setoperator       % darken: source * destination per channel
color /red get setrgbcolor
50 50 100 100 rect fill     % painted with MULTIPLY blending
/over setoperator           % reset to default
```

The most commonly useful are:

| Name | What it does |
|---|---|
| `/over` | Default. Source α-blended over destination. |
| `/clear` | Source = transparent black (a "punch a hole" operator). |
| `/source` | Source replaces destination, ignoring alpha. |
| `/multiply` | Channel-wise multiply. Darkens; white is identity. |
| `/screen` | `1 - (1-s)(1-d)`. Lightens; black is identity. |
| `/add` | Saturating channel add. Additive lighting / glow. |
| `/dest_out` | Keeps destination where source is *transparent* — silhouette cutter. |
| `/difference` | `\|s - d\|` per channel. Inverts overlaps; great for diff visualization. |

Plus the photo-editor family: `/overlay`, `/hard_light`,
`/soft_light`, `/darken`, `/lighten`, `/color_dodge`, `/color_burn`,
`/exclusion`, and the HSL family `/hsl_hue`, `/hsl_saturation`,
`/hsl_color`, `/hsl_luminosity`.

`compositors` is a dictionary of `name → integer` containing every
supported operator name. List them:

```sli
compositors keys =
```

`currentoperator` returns the current operator as a literal name.
The operator state is part of the graphics state, so `gsave` /
`grestore` save and restore it automatically — a common pattern:

```sli
gsave
  /multiply setoperator
  ... draw with multiply blending ...
grestore                       % operator restored to whatever it was
```

`/clear` is the only operator that fills a real gap in the rest of
the module — it's the one way to *erase a region to transparency*
on an image surface:

```sli
gsave
  /clear setoperator
  100 100 50 circle fill       % punches a transparent hole
grestore
```

`erasepage` by contrast clears to opaque white — useful for "blank
the page" but not for compositing.

## Font enumeration

Cairo's text API renders any font name your OS knows about, but it
won't tell you what those names are. `listfonts` enumerates them via
FontConfig:

```sli
listfonts length =          % count of installed font families
listfonts 0 get =           % the first one
listfonts {                 % all of them
  (  ) =only =
} forall
```

The list is deduplicated and lexicographically sorted. On a typical
macOS install you'll see a few hundred names. Pick any of them as
the argument to `findfont`:

```sli
(Avenir Next) findfont 20 scalefont setfont
50 100 moveto (Hello from Avenir Next) show
```

If the family isn't installed, Cairo silently substitutes a default
— no error, you just get whatever the system thinks is closest.

## Crisper rendering

A 600×800 PNG displayed on a 2× Retina screen at "actual size" is
soft because every PNG pixel covers a 2×2 block of physical pixels.
The same issue affects live windows on hi-DPI displays. Two knobs
together usually fix it:

**Hi-DPI offscreen surfaces.** `newhidpioffscreen w h scale` opens
an offscreen GC whose *logical* user-space coordinates are
`(w, h)` but whose backing pixel buffer is `(w*scale, h*scale)`.
Drawing code stays unchanged; the resulting PNG just has more
pixels to display.

```sli
% 600x800 logical page rendered into a 1200x1600 pixel buffer.
600 800 2 newhidpioffscreen pop
... usual drawing ops ...
(/tmp/page-2x.png) currentpage writepng
currentpage closepage
```

`pagesize` returns the *logical* dimensions; `pixel_width` /
`pixel_height` aren't exposed at the SLI level, but you can derive
them from `device_scale` (3×, 4× also work; 2× is the macOS Retina
standard).

**Font and shape antialiasing knobs.** Cairo's defaults are
conservative. Three operators turn on the crisp-text settings:

```sli
/best setantialias       % default | none | gray | subpixel | fast | good | best
/full sethinting         % default | none | slight | medium | full
/on   sethintmetrics     % default | on | off
```

- `setantialias` affects both shape and text rendering. `/best` is
  the highest-quality mode Cairo offers; `/subpixel` is the
  RGB-stripe-aware mode that produces extra-sharp text on most
  LCDs.
- `sethinting` snaps glyph stems to the pixel grid. Helps small
  sizes (8–14 pt); above ~18 pt the difference is subtle.
- `sethintmetrics /on` rounds glyph advance widths to whole pixels
  so successive characters land on the grid — crisper kerning at
  small sizes.

Each has a matching `current*` query. The state is part of the
graphics state, so `gsave` / `grestore` save and restore it.

A typical "make it crisp" preamble:

```sli
600 800 2 newhidpioffscreen pop
/best setantialias
/full sethinting
/on   sethintmetrics
... draw ...
```

The built-in `/fontdemopng` writes a 1× rendering; `/fontdemopng2x`
uses this preamble and writes the 1200×1600 hi-DPI version. Side
by side at the same display size, the difference is obvious.

## What else there is

The chapter has covered the operators you reach for most often. The
module ships more, briefly:

- **Curves**: `curveto`, `rcurveto` (cubic Beziers), `arct` (corner
  fillets), `arcn` (clockwise arcs).
- **Clipping**: `clip`, `eoclip` restrict drawing to the current
  path. `clippath` reads the clip extents back.
- **Path introspection**: `pathforall` walks the current path and
  calls procedures for each segment. `flattenpath` replaces curves
  with line approximations.
- **State queries**: `currentlinewidth`, `currentlinecap`,
  `currentlinejoin`, `currentrgbcolor`, `currentrgbacolor`,
  `currentdash`, `currentpoint`, `currentfont`.
- **Matrix operators**: `currentmatrix`, `setmatrix`, `concat`,
  `initmatrix`, `transform`, `itransform` — direct CTM manipulation.
- **Window ergonomics**: `setwindowtitle`, `resize`, `pagesize`, `setwindowpos`, `setfullscreen`.
- **Text alignment helpers** in `graphicslib.sli`: `showcentered` and `showright` use `stringwidth` + `rmoveto` to center / right-align at the current point.
- **`cleartransparent`**: companion to `erasepage` that clears the surface to fully transparent (alpha = 0) instead of opaque white. Useful when the output will be composited over something else.
- **Multiple pages**: `setpage` switches the "current page" to any
  open one, so you can juggle several windows or offscreen surfaces.

The full op list with one-line signatures is at the top of
`lib/sli/graphicslib.sli`.

## Built-in demos

`lib/sli/graphicslib.sli` defines several procedures you can run from
the REPL. By default each one writes its output PNG / PDF into your
home directory (`$HOME`).

The graphics module keeps its internal runtime state in a dedicated
`gfxstatus` dictionary (installed in `systemdict` at boot):

| Slot | Set by | Read by |
|---|---|---|
| `gfxstatus /currentpage`  | `newpage`, `newoffscreen`, `newpdf`, `newsvg`, `setpage` | every drawing op; cleared by `closepage` |
| `gfxstatus /currentfont`  | `setfont` | `currentfont` |
| `gfxstatus /gfxoutdir`    | initialized to `$HOME` at module load | demos, `gfxoutpath`, `gfxoutdir` |

To send all subsequent demo output somewhere else:

```sli
gfxstatus /gfxoutdir (/Users/me/Pictures/sli3) put
gradientdemo                  % now writes to /Users/me/Pictures/sli3/
```

Two convenience procedures live alongside `gfxstatus`:

- `gfxoutdir` returns the current output directory (i.e. `gfxstatus /gfxoutdir get`).
- `gfxoutpath` `( filename → fullpath )` joins it with a filename, single-slash separator.

Inspect everything in one go: `gfxstatus pstack`.


```
SLIDATADIR=lib ./build/sli3
SLI ] gfxdemo            % red square + blue circle in a 400x400 window
SLI ] textdemo           % text + curve + clipping
SLI ] offscreendemo      % writes sli3-gfxdemo.png (no window)
SLI ] pdfdemo            % writes sli3-gfxdemo.pdf
SLI ] imagedemo          % builds a source PNG, reloads it, composites
SLI ] testpage           % the full specimen: fonts, shapes, alpha
SLI ] testpagepng        % the specimen, written to sli3-testpage.png
SLI ] gradientdemo       % linear + radial gradients
SLI ] compositdemo       % the same shape painted under three operators
SLI ] fontdemo           % 11 typefaces, 5 sizes, slant + weight variants
SLI ] fontdemopng        % the font specimen, written to sli3-fontdemo.png
SLI ] fontdemopng2x      % the same specimen, 2x hi-DPI to sli3-fontdemo-2x.png
SLI ] clockdemo          % analog clock face at 10:10:30
SLI ] clockdemopng       % the clock, written to sli3-clockdemo.png
SLI ] fractaldemo        % Sierpinski triangle, depth 5
SLI ] fractaldemopng     % the fractal, written to sli3-fractaldemo.png
SLI ] plotdemo           % two-series line chart with markers + gradient fill
SLI ] plotdemopng        % the chart, written to sli3-plotdemo.png
SLI ] (path/to/foo.png) viewpng   % open a window showing foo.png at its native size
```

Each filename above is relative to `:gfxoutdir` (default `$HOME`).

`/testpage` is the best place to look for a quick visual summary of
every operator group.

## Examples

The runnable `.sli` files for this chapter live under
[`examples/ch11_graphics/`](examples/ch11_graphics/). Each one ends
with `quit` so you can pass it directly to the interpreter:

```
./build/sli3 doc/tutorial/examples/ch11_graphics/01_rect.sli
open /tmp/sli3-ch11-01-rect.png
```

The examples write into `/tmp/sli3-ch11-NN-name.png` (or `.pdf`), so
you can run them all and flip through the outputs.

---

Previous: [chapter 10 — three small programs](10_examples.md). Up:
[tutorial index](README.md).
