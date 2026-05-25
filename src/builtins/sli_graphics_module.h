#ifndef SLI_GRAPHICS_MODULE_H
#define SLI_GRAPHICS_MODULE_H

namespace sli3
{
class SLIInterpreter;

// PostScript-flavored 2D graphics module. Backed by Cairo (for the
// imaging model) and SDL2 (for the live window). Implementations live
// in sli_graphics_module.cpp; this header exposes only the entry
// point. Operator function objects are private to the .cpp.
//
// Operators wired up here:
//   newpage, closepage, currentpage,
//   newpath, moveto, lineto, rlineto, closepath,
//   rect, arc, circle,
//   stroke, fill, erasepage,
//   setrgbcolor, setgray, setlinewidth,
//   gsave, grestore,
//   translate, scale, rotate,
//   showpage, flushpage, wait, windowclosed.
//
// There is intentionally NO blocking /interact: a terminal-launched
// sli3 on macOS isn't a Cocoa app, so the window's close button may
// not deliver an event back to us. A blocking loop in that state
// would hang the REPL forever; the bounded /wait <seconds> pump is
// the only safe surface.
//
// The currently-targeted graphics context lives in the `gfxstatus`
// dict (installed in systemdict at init time) under /currentpage, so
// PS-style code can be written without threading the surface through
// every operator: `400 400 newpage 1 0 0 setrgbcolor 50 50 100 100
// rect fill showpage 2 wait closepage`.
void init_sligraphics(SLIInterpreter*);

}  // namespace sli3

#endif
