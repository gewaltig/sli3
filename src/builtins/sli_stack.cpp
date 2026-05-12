/*
 *  slistack.cc
 *
 *  This file is part of NEST
 *
 *  Copyright (C) 2004 by
 *  The NEST Initiative
 *
 *  See the file AUTHORS for details.
 *
 *  Permission is granted to compile and modify
 *  this file for non-commercial use.
 *  See the file LICENSE for details.
 *
 */

/* 
    slistack.cc
*/
#include "sli_stack.h"
#include "sli_interpreter.h"
#include "sli_op_bodies.h"
namespace sli3
{

//******************* Stack Functions
/*BeginDocumentation
Name: pop - Pop the top object off the stack

Description: Alternatives: You can use ; (undocumented), 
which is the same as pop.

Diagnostics: Raises StackUnderflow error if the stack is empty

SeeAlso: npop
 */

void PopFunction::execute(SLIInterpreter *i) const
{
    hot_op_pop(i);  // shared with dispatcher's inline arm
}

/*BeginDocumentation
Name: npop - Pop n object off the stack
Synopsis: obj_k ... obj_n+1 ojb_n ... obj_0 n pop -> obj_k ... obj_n
Diagnostics: Raises StackUnderflow error if the stack contains less
 than n elements.
SeeAlso: pop
 */

void NpopFunction::execute(SLIInterpreter *i) const
{
    i->require_stack_load(1);
    // Stage 5.8: range-check the count BEFORE casting to size_t.
    // A negative input would wrap to a huge unsigned and the
    // subsequent require_stack_load would raise the wrong error
    // (StackUnderflow with insane numbers).
    long const raw = i->top().data_.long_val;
    if (raw < 0)
    {
        i->raiseerror(i->RangeCheckError);
        return;
    }
    size_t n = static_cast<size_t>(raw);
    i->require_stack_load(n + 1);
    i->pop(n + 1);
}

/*BeginDocumentation
Name: dup - Duplicate the object which is on top of the stack
Synopsis: any dup -> any any
Diagnostics: Raises StackUnderflow error if the stack is empty.
Examples: 2 dup -> 2 2
(hello) dup -> (hello) (hello)
Author: docu edited by Sirko Straube
SeeAlso: over, index, copy
*/
void DupFunction::execute(SLIInterpreter *i) const
{
    hot_op_dup(i);  // shared with dispatcher's inline arm
}

/* BeginDocumentation
Name: over - Copy object at stack level 1
Synopsis: any obj over -> any obj any
Diagnostics: Raises StackUnderflow error if there are less than two objects on
  the stack.
Examples: 1 2 3 over -> 2
1 2 3 4 5 over -> 4
SeeAlso: dup, index, copy
*/
void OverFunction::execute(SLIInterpreter *i) const
{
    i->require_stack_load(2);
    i->OStack().index(1);
}

/*BeginDocumentation
Name: exch - Exchange the order of the first two stack objects.
Synopsis: obj1 obj2 exch -> obj2 obj1
Diagnostics: Raises StackUnderflow error if there are less than two objects on
  the stack.
SeeAlso: roll, rollu, rolld, rot
*/

void ExchFunction::execute(SLIInterpreter *i) const
{
    hot_op_exch(i);  // shared with dispatcher's inline arm
}

/* BeginDocumentation
Name: index - Copy object at stack level n
Synopsis: ... obj_n ... obj0 n index -> ... obj_n ... obj0 obj_n
Diagnostics: Raises StackUnderflow error if there are less than n+1 objects on
  the stack.
SeeAlso: dup, over, copy
*/
void IndexFunction::execute(SLIInterpreter *i) const
{
    i->require_stack_load(1);

    long const raw = i->top().data_.long_val;
    if (raw < 0)
    {
        i->raiseerror(i->RangeCheckError);
        return;
    }
    size_t const pos = static_cast<size_t>(raw) + 1;
    i->require_stack_load(pos);
    i->top() = i->pick(pos);
}

/* BeginDocumentation
Name: copy - Copy the top n stack objects
Synopsis: ... obj_n ... obj1 n copy -> ... obj_n ... obj1 obj_n ... obj1
Examples: 1 2 3 4 2 copy 
-> after this execution 1 2 3 4 3 4 lies on the stack (the last two elements
were copied). 
Diagnostics: Raises StackUnderflow error if there are less than n+1 objects on
  the stack.
Author: docu edited by Sirko Straube
SeeAlso: dup, over, index
*/
void CopyFunction::execute(SLIInterpreter *i) const
{
    i->require_stack_load(1);
    long const raw = i->top().data_.long_val;
    if (raw < 0)
    {
        i->raiseerror(i->RangeCheckError);
        return;
    }
    size_t const n = static_cast<size_t>(raw);
    i->require_stack_load(n + 1);    // n elements + the count itself
    i->pop();
    for (size_t p = 0; p < n; ++p)
        i->OStack().index(n - 1); // stack growing, argument constant
}

/*BeginDocumentation
Name: roll - Roll a portion n stack levels k times
Synopsis: objn ... obj1 n k roll
Description: 
roll performs a circular shift of the first n stack levels
by k positions. Before this is done, roll removes its arguments
from the stack. 

If k is positive, each shift consists of moving the contents of level
0 to level n-1, thereby moving elements at levels 1 through n-1 up one
stack level.

If k is negative, each shift consists of moving the contents of level
n-1 to level 0, thereby moving elements at levels 1 through n-1 down
one stack level.
 
Examples:
    (a) (b) (c) 3 1  roll -> (c) (a) (b)
    (a) (b) (c) 3 -1 roll -> (b) (c) (a)
    (a) (b) (c) 3 0  roll -> (a) (b) (c)
Diagnostics: Raises StackUnderflow error if there are less than n+2 objects
on the stack.
SeeAlso: exch, rollu, rolld, rot 
*/
void RollFunction::execute(SLIInterpreter *i) const
{
    i->require_stack_load(2);
    
    const long n=i->pick(1).data_.long_val;
    const long k=i->pick(0).data_.long_val;

    if(n < 0)
    {
	i->raiseerror(i->RangeCheckError);
	return;
    }
    i->require_stack_load(n+2);

    i->pop(2);
    
    i->OStack().roll(n,k);
}

/*BeginDocumentation
Name: rollu - Roll the three top stack elements upwards
Synopsis: obj1 obj2 obj3 rollu -> obj3 obj1 obj2
Description: rollu is equivalent to 3 1 roll
Diagnostics: Raises StackUnderflow error if there are less 
             than 3 objects on the stack.
SeeAlso: roll, rolld, rot
*/
void RolluFunction::execute(SLIInterpreter *i) const
{
    i->require_stack_load(3);
    
    i->OStack().roll(3,1);
}

/*BeginDocumentation
Name: rolld - Roll the three top stack elements downwards
Synopsis: obj1 obj2 obj3 rolld -> obj2 obj3 obj1
Description: rolld is equivalent to 3 -1 roll
Diagnostics: Raises StackUnderflow error if there are less 
             than 3 objects on the stack.
SeeAlso: roll, rollu, rot
*/
void RolldFunction::execute(SLIInterpreter *i) const
{
    i->require_stack_load(3);
    i->OStack().roll(3,-1);
}

/*BeginDocumentation
Name: rot - Rotate entire stack contents
Synopsis: obj_n ... obj1 obj0 rot -> obj0 obj_n ... obj1 
SeeAlso: roll, rollu, rolld
*/

void RotFunction::execute(SLIInterpreter *i) const
{
    
  i->OStack().roll(i->OStack().load(),1);
}

/*BeginDocumentation
Name: count - Count the number of objects on the stack.
Synopsis: obj_n-1 ... obj0 count -> obj_n-1 ... obj0 n 
*/
void CountFunction::execute(SLIInterpreter *i) const
{
    
    i->push(i->load());
}

/*BeginDocumentation
Name: clear - Clear the entire stack.
SeeAlso: pop, npop
*/
void ClearFunction::execute(SLIInterpreter *i) const
{
    i->OStack().clear();
}

/*BeginDocumentation
Name: execstack - Return the contents of the execution stack as array.
Synopsis: - execstack -> array
Description: execstack converts the current contents of the execution stack
into an array. The first array element corrensponds to the bottom and the
last array element to the top of the execution stack.
SeeAlso: restoreestack, operandstack
*/ 
void ExecstackFunction::execute(SLIInterpreter *i) const
{

    TokenArray *ta=i->EStack().toArray();
    i->push(i->new_token<sli3::arraytype>(ta));   
}

/*BeginDocumentation
Name: restoreestack - Restore the execution stack from an array.
Synopsis: array restoreexecstack -> -

Description: restoreexecstack is used to restore the execution stack
from an array. Most probably this array was obtained by saving a previous
state of the execution stack with execstack.

Be very careful with this command, as it can easily damage or terminate
the SLI interpreter.
Diagnostics: ArgumentType, StackUnderflow
SeeAlso: execstack
*/
void RestoreestackFunction::execute(SLIInterpreter *i) const
{
    i->require_stack_load(1);
    i->require_stack_type(0,sli3::arraytype);
    i->EStack()=*(i->top().data_.array_val);
    i->pop();
}

/*BeginDocumentation
Name: restoreostack - Restore the stack from an array.
Synopsis: [any0 ... any_n] restoreexecstack -> any0 ... any_n

Description: restoroexecstack is used to replace the contents of
the stack with the contents of the supplied array. The first element
of the array will become the bottom of the stack and the last element
of the array will become the top of the stack.

Diagnostics: ArgumentType, StackUnderflow
SeeAlso: operandstack, arrayload, arraystore
*/
void RestoreostackFunction::execute(SLIInterpreter *i) const
{
    i->require_stack_load(1);
    i->require_stack_type(0,sli3::arraytype);
    i->OStack()=*(i->top().data_.array_val);
}

/*BeginDocumentation
Name: operandstack - Return the contents of the stack as array.
Synopsis: anyn ... any0 operandstack -> [anyn ... any0]
SeeAlso: restoreostack, arrayload, arraystore
*/ 
void OperandstackFunction::execute(SLIInterpreter *i) const
{
    i->push(i->new_token<sli3::arraytype>(i->OStack().toArray()));
}

 PopFunction              popfunction;
 NpopFunction             npopfunction;
 ExchFunction             exchfunction;
 DupFunction              dupfunction;
 IndexFunction            indexfunction;
 CopyFunction             copyfunction;
 RollFunction             rollfunction;
 CountFunction            countfunction;
 ClearFunction            clearfunction;

 RotFunction              rotfunction;
 RolluFunction            rollufunction;
 RolldFunction            rolldfunction;
 OverFunction             overfunction;

 ExecstackFunction        execstackfunction;
 RestoreestackFunction    restoreestackfunction;
 RestoreostackFunction    restoreostackfunction;
 OperandstackFunction     operandstackfunction;

void  init_slistack(SLIInterpreter *i)
{
        // Stack routines
    i->createcommand("pop",&popfunction);
    i->createcommand("npop",&npopfunction);
    i->createcommand(";",&popfunction);
    i->createcommand("dup",&dupfunction);
    i->createcommand("exch",&exchfunction);
    i->createcommand("index",&indexfunction);
    i->createcommand("copy",&copyfunction);
    i->createcommand("roll",&rollfunction);
    i->createcommand("count",&countfunction);
    i->createcommand("clear",&clearfunction);

    i->createcommand("rollu",&rollufunction);
    i->createcommand("rolld",&rolldfunction);
    i->createcommand("rot",  &rotfunction);
    i->createcommand("over", &overfunction);

    i->createcommand("execstack",&execstackfunction);
    i->createcommand("restoreestack",&restoreestackfunction);
    i->createcommand("restoreostack",&restoreostackfunction);
    i->createcommand("operandstack",&operandstackfunction);

    // Axis I bundle step 3: ops in sli_stack.cpp converted to new ABI.
    // RestoreestackFunction stays old-ABI -- it replaces the entire
    // e-stack via i->EStack() = *array_val, so the dispatcher post-
    // check (compares fn pointer identity) must not run.
    clearfunction.set_new_abi();
    copyfunction.set_new_abi();
    countfunction.set_new_abi();
    dupfunction.set_new_abi();
    exchfunction.set_new_abi();
    execstackfunction.set_new_abi();
    indexfunction.set_new_abi();
    npopfunction.set_new_abi();
    operandstackfunction.set_new_abi();
    overfunction.set_new_abi();
    popfunction.set_new_abi();
    restoreostackfunction.set_new_abi();
    rolldfunction.set_new_abi();
    rollfunction.set_new_abi();
    rollufunction.set_new_abi();
    rotfunction.set_new_abi();

    // Axis II step 1: hot-op tags for the dispatcher's inline path.
    // Bodies of these ops are inlined in the body-walk hot path
    // via sli_op_bodies.h (single source of truth -- the bodies
    // here remain authoritative; the dispatcher's inline arm
    // calls the same static helper).
    popfunction.set_hot_op(HOP_POP);
    dupfunction.set_hot_op(HOP_DUP);
    exchfunction.set_hot_op(HOP_EXCH);

    // Axis I bundle step 3: ops in sli_stack.cpp converted to the
    // new ABI -- the dispatcher pops the fn slot after execute()
    // (when the slot is still on top; raiseerror's pop+push-/stop
    // is handled separately). RestoreestackFunction stays old-ABI
    // because it replaces the entire e-stack and the post-execute
    // top is not its fn slot.
    popfunction.set_new_abi();
    npopfunction.set_new_abi();
    dupfunction.set_new_abi();
    exchfunction.set_new_abi();
    indexfunction.set_new_abi();
    copyfunction.set_new_abi();
    rollfunction.set_new_abi();
    countfunction.set_new_abi();
    clearfunction.set_new_abi();
    rollufunction.set_new_abi();
    rolldfunction.set_new_abi();
    rotfunction.set_new_abi();
    overfunction.set_new_abi();
    execstackfunction.set_new_abi();
    restoreostackfunction.set_new_abi();
    operandstackfunction.set_new_abi();
}
}
