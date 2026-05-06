#ifndef SLI_CONTAINER_OPS_H
#define SLI_CONTAINER_OPS_H

namespace sli3
{
class SLIInterpreter;

// Registers length_*, get_*, put_* operators for arrays, procedures,
// litprocedures, strings, and dictionaries — the C++ surface that
// typeinit.sli builds tries on top of. Called from
// SLIInterpreter::init_internal_functions.
void init_container_ops(SLIInterpreter*);

}  // namespace sli3

#endif
