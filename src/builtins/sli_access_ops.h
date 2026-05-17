#ifndef SLI_ACCESS_OPS_H
#define SLI_ACCESS_OPS_H

namespace sli3
{
class SLIInterpreter;

// PostScript access-state operators. Each setter narrows obj's
// access (monotonically); the checks expose the state to SLI. All
// six dispatch on the top operand's tag so the same name works for
// arrays, procedures, dictionaries, and strings.
//
//   readonly     obj → obj      narrow to readonly
//   executeonly  obj → obj      narrow to executeonly
//   noaccess     obj → obj      narrow to noaccess
//   rcheck       obj → bool     true if read-accessible
//   wcheck       obj → bool     true if writable (unlimited)
//   xcheck       obj → bool     true if obj is a procedure
//                               (procedure / litprocedure tag)
void init_access_ops(SLIInterpreter*);

}  // namespace sli3

#endif
