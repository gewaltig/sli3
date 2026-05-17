#ifndef SLI_ACCESS_OPS_H
#define SLI_ACCESS_OPS_H

namespace sli3
{
class SLIInterpreter;

// PostScript access-state operators (Wave 1: readonly + the three
// checks; Wave 2 will add executeonly / noaccess). Each operator
// dispatches on the top operand's tag so the same name works for
// arrays, procedures, dictionaries, and strings.
//
//   readonly  obj → obj          narrow obj's access to readonly
//   rcheck    obj → bool         true if readable
//   wcheck    obj → bool         true if writable (unlimited)
//   xcheck    obj → bool         true if obj is a procedure
//                                (procedure / litprocedure tag)
void init_access_ops(SLIInterpreter*);

}  // namespace sli3

#endif
