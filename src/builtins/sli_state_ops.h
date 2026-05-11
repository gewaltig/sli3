#ifndef SLI_STATE_OPS_H
#define SLI_STATE_OPS_H

namespace sli3
{
class SLIInterpreter;

/**
 * Register the savestate / restorestate operators.
 *
 *   (filename) savestate     -> -
 *   (filename) restorestate  -> -
 *
 * savestate writes the current operand stack and execution stack
 * to a binary file, after popping itself so the snapshot reflects
 * "savestate has returned". restorestate reads the snapshot back
 * and replaces the live stacks with its contents.
 *
 * Scope and limitations:
 * - The dict stack is NOT snapshotted in this initial cut. The
 *   loading interpreter must have an equivalent dict-stack
 *   environment for name lookups to resolve correctly. Permanent
 *   dicts (systemdict / errordict / userdict / statusdict) are
 *   reachable by name via the existing interpreter; user-defined
 *   begin/end frames are not preserved.
 * - Function references are restored by *name* (see
 *   FunctionType::serialize). The loading interpreter must have
 *   the same operators registered. This holds within a single
 *   binary; cross-build snapshots may produce null function
 *   tokens for missing operators.
 * - Object identity across save/restore is not preserved. A
 *   TokenArray (procedure body) appearing on both the operand
 *   stack and the execution stack is restored as a single shared
 *   instance within the snapshot, but it is a NEW instance
 *   distinct from any pre-save object.
 *
 * Called from SLIInterpreter::init_internal_functions.
 */
void init_state_ops(SLIInterpreter*);

}  // namespace sli3

#endif
