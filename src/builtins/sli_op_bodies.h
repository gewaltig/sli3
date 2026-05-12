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

// add -- poly-arithmetic dispatcher (int+int, dd, di, id).
// Same body as AddFunction::execute; the standalone method
// delegates here.
static inline void hot_op_add(SLIInterpreter* i)
{
    i->require_stack_load(2);
    Token& a = i->pick(1);
    Token& b = i->pick(0);
    unsigned ta = a.tag();
    unsigned tb = b.tag();
    if (ta == sli3::integertype && tb == sli3::integertype) {
        long out;
        if (__builtin_add_overflow(a.data_.long_val, b.data_.long_val, &out)) {
            i->raiseerror(i->RangeCheckError);
            return;
        }
        a.data_.long_val = out;
        i->pop();
        return;
    }
    if (ta == sli3::doubletype && tb == sli3::doubletype) {
        a.data_.double_val += b.data_.double_val;
        i->pop();
        return;
    }
    if (ta == sli3::doubletype && tb == sli3::integertype) {
        a.data_.double_val += static_cast<double>(b.data_.long_val);
        i->pop();
        return;
    }
    if (ta == sli3::integertype && tb == sli3::doubletype) {
        a.data_.double_val =
            static_cast<double>(a.data_.long_val) + b.data_.double_val;
        a.type_ = b.type_;
        i->pop();
        return;
    }
    i->raiseerror(i->ArgumentTypeError);
}

// sub -- poly-arithmetic dispatcher, same shape as hot_op_add.
static inline void hot_op_sub(SLIInterpreter* i)
{
    i->require_stack_load(2);
    Token& a = i->pick(1);
    Token& b = i->pick(0);
    unsigned ta = a.tag();
    unsigned tb = b.tag();
    if (ta == sli3::integertype && tb == sli3::integertype) {
        long out;
        if (__builtin_sub_overflow(a.data_.long_val, b.data_.long_val, &out)) {
            i->raiseerror(i->RangeCheckError);
            return;
        }
        a.data_.long_val = out;
        i->pop();
        return;
    }
    if (ta == sli3::doubletype && tb == sli3::doubletype) {
        a.data_.double_val -= b.data_.double_val;
        i->pop();
        return;
    }
    if (ta == sli3::doubletype && tb == sli3::integertype) {
        a.data_.double_val -= static_cast<double>(b.data_.long_val);
        i->pop();
        return;
    }
    if (ta == sli3::integertype && tb == sli3::doubletype) {
        a.data_.double_val =
            static_cast<double>(a.data_.long_val) - b.data_.double_val;
        a.type_ = b.type_;
        i->pop();
        return;
    }
    i->raiseerror(i->ArgumentTypeError);
}

// if -- bool proc -> push proc on e-stack if bool is true.
// Under the new ABI, the dispatcher pre-popped /if's own slot
// (in the main case) or never pushed it (in the body-walk
// fast path). The body just consumes the two ostack args.
static inline void hot_op_if(SLIInterpreter* i)
{
    i->require_stack_load(2);
    i->require_stack_type(1, sli3::booltype);
    if (i->pick(1).data_.bool_val)
        i->EStack().push(i->top());
    i->pop(2);
}

// def -- /lit obj -> store under name in current dict.
static inline void hot_op_def(SLIInterpreter* i)
{
    i->require_stack_load(2);
    i->require_stack_type(1, sli3::literaltype);
    i->def(i->pick(1).data_.name_val, i->top());
    i->pop(2);
}

}  // namespace sli3

#endif  // SLI_OP_BODIES_H
