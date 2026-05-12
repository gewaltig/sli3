// Axis II: inline body helpers for hot operators.
//
// Single source of truth: the operator class's execute() method
// delegates here, and the dispatcher's hot-op switch arms call
// the same helpers. Keep these bodies tiny -- they're inlined
// into the dispatcher's body-walk loop, where register pressure
// is high and any non-trivial work spills the operand-stack
// pointer.
//
// Each helper has the same contract as the SLIFunction's
// execute(): pre: ostack precondition met; post: result on
// ostack; on type/range error: i->raiseerror(...) and return.
// The helpers do NOT touch the e-stack (that's the dispatcher's
// job under the post-Axis-I ABI).

#ifndef SLI_OP_BODIES_H
#define SLI_OP_BODIES_H

#include "sli_interpreter.h"
#include "sli_tokenstack.h"

namespace sli3
{

// pop -- discard ostack top.
static inline void hot_op_pop(SLIInterpreter* i)
{
    i->require_stack_load(1);
    i->pop();
}

// dup -- push a copy of ostack top.
static inline void hot_op_dup(SLIInterpreter* i)
{
    i->require_stack_load(1);
    i->OStack().index(0);
}

// exch -- swap top two ostack slots.
static inline void hot_op_exch(SLIInterpreter* i)
{
    i->require_stack_load(2);
    i->OStack().swap();
}

// add_ii -- integer add with overflow check.
// pick(1) <- pick(1) + pick(0); pop.
static inline void hot_op_add_ii(SLIInterpreter* i)
{
    i->require_stack_load(2);
    long out;
    if (__builtin_add_overflow(i->pick(1).data_.long_val,
                                i->pick(0).data_.long_val,
                                &out))
    {
        i->raiseerror(i->RangeCheckError);
        return;
    }
    i->pick(1).data_.long_val = out;
    i->pop();
}

}  // namespace sli3

#endif  // SLI_OP_BODIES_H
