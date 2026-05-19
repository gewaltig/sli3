#ifndef SLI_REGEX_MODULE_H
#define SLI_REGEX_MODULE_H

namespace sli3
{
class SLIInterpreter;

// POSIX <regex.h> primitives + the regexdict flag dictionary. Mirrors
// NEST 2.x's sliregexp.{h,cc}: registers regcomp_ / regexec_ /
// regerror_; the friendlier surface (regcomp, regexec, regex_find,
// regex_replace, grep) lives in lib/sli/regexp.sli.
void init_regex_module(SLIInterpreter*);

}  // namespace sli3

#endif
