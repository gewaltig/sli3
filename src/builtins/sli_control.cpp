/*
 *  slicontrol.cc
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
    SLI's control structures
*/

//#include "config.h"


//#include "sliconfig.h"
#include "sli_control.h"
#include "sli_op_bodies.h"
#include "sli_scanner.h"
#include "sli_parser.h"
#include "sli_iostreamtype.h"
#include "sli_iostream.h"
#include "sli_stringtype.h"
#include "sli_dictstack.h"
#include "sli_functiontype.h"
//#include "sli_processes.h"
#include "sli_exceptions.h"

#include <time.h>
#include <sys/time.h>  // required to fix header dependencies in OS X, HEP
#include <sys/times.h>
#include <sys/resource.h>
#include <unistd.h>
#include <sstream>

namespace sli3
{

/*BeginDocumentation
Name: backtrace_on - enable stack backtrace on error.
Synopsis: backtrace_on -> -
Description:

This functions enables a human readable backtrace of the execution
stack. This is useful to locate where precisely an error occured. Note
that this function also disables the interpreter's tail recursion
optimization and will therefore impose a small performance
penalty. The command backtrace_off disables the stack backtrace and
re-enables tail recursion optimization.

Example:

SeeAlso:
backtrace_off
*/
void Backtrace_onFunction::execute(SLIInterpreter *i) const
{
  i->backtrace_on();
}

/*BeginDocumentation
Name: backtrace_off - Disable the stack backtrace on error.
Synopsis: backtrace_off -> -

Description:
This functions disables the backtrace of the execution
stack and re-enables tail recursion optimization.

SeeAlso: backtrace_on

*/
void Backtrace_offFunction::execute(SLIInterpreter *i) const
{
  i->backtrace_off();
}

void OStackdumpFunction::execute(SLIInterpreter *i) const
{
    // Axis I bundle step 4: dispatcher pre-popped /ostackdump.
    i->OStack().dump(std::cout);
}

void EStackdumpFunction::execute(SLIInterpreter *i) const
{
    // Axis I bundle step 4: dispatcher pre-popped /estackdump.
    i->EStack().dump(std::cout);
}

/*BeginDocumentation
Name: loop - repeatedly execute a procedure
Synopsis: proc loop -
Description:
 loop takes a procedure object an executes it repeatedly.
 Since there is no direct termination condition, the loop
 has to be terminated by calling exit.
 If the procedure has to be evaluated for a certain number of
 times, consider the use of repeat or for.
 If some container should be iterated, consider forall or Map
SeeAlso: exit, repeat, for, forall, forallindexed, Map
*/

void LoopFunction::execute(SLIInterpreter *i) const
{
    i->require_stack_load(1);
    i->require_stack_type(0, sli3::proceduretype);

    // Phase 5: push the iloop TYPE marker instead of the legacy
    // IloopFunction token. body_walk's `case sli3::ilooptype` walks
    // the proc body inline (same shape as iiterate); the
    // body_exhausted handler resets pos=0 instead of popping, so
    // the loop runs forever until exit / stop unwinds it.
    // Frame: [mark, proc, pos=0, ilooptype].
    i->EStack().push(i->baselookup(i->mark_name));
    i->EStack().push(i->top());
    i->EStack().push(0);
    i->EStack().push(Token(i->get_type(sli3::ilooptype)));
    i->pop();
}

/*BeginDocumentation
Name: exit - exit a loop construct
Description: exit can be used to leave loop structures
             like loop, repeat, for, forall, Map, etc.
             in a clean way.
Remarks: This command does not exit the SLI interpreter! Use quit instead.
*/
void ExitFunction::execute(SLIInterpreter *i) const
{
    // Axis I bundle Phase 2a: new-ABI. Dispatcher pre-popped the
    // /exit slot, so the scan starts at pick(0) (the call site) and
    // walks upward looking for the nearest marktype sentinel. All
    // loop ops (loop / repeat / for / forall / ...) push a marktype
    // as their exit anchor. pop(n + 1) drops everything from top
    // down to and including the mark.
    size_t n = 0;
    size_t const l = i->EStack().load();
    while (n < l && !i->EStack().pick(n).is_of_type(sli3::marktype)) ++n;
    if (n >= l) {
        i->raiseerror("EStackUnderflow");
        return;
    }
    i->EStack().pop(n + 1);
}

/*BeginDocumentation
Name: if - conditionaly execute a procedure
Synopsis:
  boolean {procedure} if -> -
  boolean   anytoken  if -> -
Description: if executes the supplied token if the boolean
             is true. The supplied token usually is a procedure.

	     Alternatives: Function if_ (undocumented)
	     -> behaviour and synopsis are the same, except that no
	     warnings or error messages are thrown.

Examples:  1 0 gt { (1 > 0) = } if -> (1 > 0)

SeeAlso: ifelse
*/
void IfFunction::execute(SLIInterpreter *i) const
{
    hot_op_if(i);  // shared with dispatcher's inline arm
}

/*BeginDocumentation
Name: ifelse - conditionaly execute a procedure
Synopsis:
  boolean  {proc1}  {proc2}   ifelse -> -
  boolean anytoken1 anytoken1 ifelse -> -
Description:
  ifelse executes anytoken1 if the boolean is true, and anytoken2
  otherwise.
  The supplied tokens usually are procedures.

  Alternatives: Function ifelse_ (undocumented)
  -> behaviour and synopsis are the same, except that no
  warnings or error messages are thrown.

Examples:
     0 1 gt { (1 > 0) = } { (1 <= 0) =} ifelse -> (0 <= 1)

SeeAlso: if
*/
void IfelseFunction::execute(SLIInterpreter *i) const
{
    // OStack: bool tproc fproc
    //          2    1      0
    // EStack (new ABI, step 4): dispatcher pre-popped /ifelse's
    // slot. Move the chosen branch onto the estack via swap rather
    // than copy: push an empty Token, swap with the ostack slot, then
    // pop(3). The swapped-out slot's dtor is a no-op (type_=0), so we
    // save one ArrayType::add_reference / remove_reference pair per
    // ifelse call (a hot path in recursive procedures like fib).
    i->require_stack_load(3);
    i->require_stack_type(2, sli3::booltype);

    Token& chosen = i->pick(2).data_.bool_val ? i->pick(1) : i->pick(0);
    i->EStack().push(Token{});
    i->EStack().top().swap(chosen);
    i->pop(3);
}

/*BeginDocumentation
Name: repeat - execute a procedure n times
Synopsis: n proc repeat
Description:
 repeat executes the supplied procedure n times.
 The loop can be left prematurely using exit.

Note: The interation counter is not available
 to the procedure. If this is desired, use for instead.

Examples:
SLI ] 3 { (Hello world) = } repeat
Hello world
Hello world
Hello world
SLI ]

SeeAlso: for, loop, exit
*/
void RepeatFunction::execute(SLIInterpreter *i) const
{
    SLIType * mark_t= i->get_type(sli3::marktype);
    SLIType * repeat_t= i->get_type(sli3::irepeattype);

    i->require_stack_load(2);
    i->require_stack_type(0,sli3::proceduretype);
    i->require_stack_type(1,sli3::integertype);

    TokenArray *proc= i->top().data_.array_val;
    // Axis I bundle step 4: dispatcher pre-popped /repeat. Push a
    // fresh mark + iter frame; pre-step-4 code overwrote the
    // /repeat slot's type to mark_t in place.
    i->EStack().push(Token(mark_t));
    i->EStack().push(i->pick(1));
    i->EStack().push(i->pick(0));
    i->EStack().push(i->new_token<sli3::integertype>(proc->size()));
    i->EStack().push(repeat_t);
    i->pop(2);
}

/*BeginDocumentation
Name: stopped - returns true if execution was stopped by stop
Synopsis:
  xobj stopped -> true;  if the object was aborted with stop
               -> false; otherwise.
Description:
  stopped is part of a pair of commands which implement the
  PostScript exception mechanism.
  stopped is applied to an executable object, most probably a
  procedure, and returns true if this object raised a stop
  signal and false otherwise.
  This is accomplished by first pushing an internal name which
  resolves to false and then executing the object.
  stop will pop the stack down to this name and return true. If
  stop is not called, the name is eventually executed and thus the
  return value false.

  Note that when the executable object was stopped by a call
  to raiseerror, the name of the routine that caused the
  error has can be found on the operand stack (see raiseerror).

  The stop/stopped pair is used to implement SLI's error handling
  capabilities.

Notes: stop, stopped is PostScript compatible
References:   The Red Book, sec. 3.10
SeeAlso: stop, raiseerror
*/
void StoppedFunction::execute(SLIInterpreter *i) const
{
    i->require_stack_load(1);
    // Axis I bundle step 4: dispatcher pre-popped /stopped.
    i->EStack().push(i->new_token<sli3::nametype>(i->istopped_name));
    i->EStack().push(i->top());
    i->pop();
}

/*BeginDocumentation
Name: stop - raise a stop signal
Synopsis:
  stop -> -

Desctiption:
  stop causes the execution stack to be popped until an
  enclosing stopped context is found. Effectively, the
  stopped/stop combination equals the catch/throw pair of
  C++.
  stop/stopped is used to implement SLI's error handling
  capabilities.
Notes: stop, stopped is PostScript compatible.
References: The Red Book, sec. 3.10
SeeAlso: stopped, raiseerror
*/
void StopFunction::execute(SLIInterpreter *i) const
{
    // Axis I bundle Phase 2b: new-ABI. Dispatcher pre-popped /stop,
    // so the scan starts at pick(0) and walks upward for the nearest
    // istopped_name sentinel (pushed by StoppedFunction). On hit:
    // push true onto the operand stack and pop(n + 1) the estack
    // down to and including the sentinel.
    size_t const l = i->EStack().load();
    size_t n = 0;
    bool found = false;
    while (n < l) {
        if (i->EStack().pick(n) == i->istopped_name) {
            found = true;
            break;
        }
        ++n;
    }
    if (!found) {
        i->message(30, "stop", "No stopped context was found! \n");
        i->EStack().clear();
        return;
    }
    i->push(true);
    i->EStack().pop(n + 1);
}

/*BeginDocumentation
Name: closeinput - Close current input file.
FirstVersion: 25 Jul 2005, Gewaltig
*/

void CloseinputFunction::execute(SLIInterpreter *i) const
{
  // Phase 5 new-ABI: dispatcher pre-popped /closeinput, so start
  // the scan at pick(0) (the call site) and look upward for the
  // nearest xistreamtype slot. On hit, pop(n) drops everything
  // from top down to and including the xistream.
  size_t const l = i->EStack().load();
  bool found = false;
  size_t n = 0;
  while (n < l && !(found = i->EStack().pick(n).is_of_type(sli3::xistreamtype)))
    ++n;

  if(!found)
  {
    i->message(30,"closeinput",
               "No active input file was found. \n  Restarting...");
    i->EStack().clear();
    i->EStack().push(i->new_token<sli3::nametype>(Name("start")));
    return;
  }

  // Drop everything from top down to and including the xistream slot.
  i->EStack().pop(n + 1);
}

/* BeginDocumentation
Name: currentname -  returns the most recently resolved name
Synopsis:
 currentname -> name true
             -> false
Description:
 currentname returns the most recently resolved name whose contents
 is still under execution.
 currentname is useful for error handling purposes where a procedure
 has to know the name with which it has been called.
Example:

 /divide
 {
    0 eq
    {
      currentname /DivisionByZero raiseerror
    } if
    div
} def

Note:
 This function is not foolproof, as it will fail for bound procedures.

 currentname evaluates certain debugging information on the execution
 stack. Note that the SLI interpreter can be compiled not to generate
 this information. In this case currentname fails, i.e. it will always
 return false
*/
void CurrentnameFunction::execute(SLIInterpreter *i) const
{
    // Stage 4.9: per the docstring, this returns the calling
    // function's Name with `true`, or just `false` if no caller
    // can be identified.
    //
    // NEST 2.20.2's implementation walks the e-stack looking for a
    // ::lookup marker, reading the name immediately below it. That
    // technique relied on NameType::execute pushing a [literal,
    // ::lookup] pair onto the e-stack -- but our dispatcher
    // REPLACES the name Token in place (sli_nametype.cpp), so the
    // breadcrumb trail isn't there. Without it, we can't recover
    // the caller name; return false so callers can detect the
    // "not available" case rather than reading garbage off the
    // stack.
    // Axis I bundle step 4: dispatcher pre-popped /currentname.
    i->push<bool>(false);
}


void DefFunction::execute(SLIInterpreter *i) const
{
    hot_op_def(i);  // shared with dispatcher's inline arm
}

/*BeginDocumentation
Name: Set - Define an association between a name and an object in the current dictionary
Synopsis:
  obj literal   Set -> -
  [... [obj_1 ...] ... obj_n] [... [literal_1 ...] ... literal_n] Set -> -
Description:
 In the first form Set is identical to def, except for the reversed parameters and
 creates or modifies an entry for the literal in the current dictionary. The new value
 assigned to the literal is obj.
 In the second form multiple simultaneous assignments are made to the literals contained in
 the second. The nesting of this array is arbitrary, indicated in the synopsis by the
 inner brackets, and the same object are taken from the identical positions in first array.


Examples:
 {1 2 add} /myproc Set
 [4 3] [/x /y] Set
 [[4 3 7] [-9 1]] [[/a /b /c] [/x /y]] Set
 [[4 9] 3] [/x /y] Set
 [[4 9] 3] [[/a /b] /y] Set

SeeAlso: def, undef, begin, end
*/
void SetFunction::execute(SLIInterpreter *i) const
{
    i->require_stack_load(2);
    i->require_stack_type(0,sli3::literaltype);

    Name name(i->pick(0).data_.name_val);

    i->def(name,i->pick(1));
    i->pop(2);
}

/*BeginDocumentation
Name: load - Search for a key in each dictionary on the dictionary stack.
Synopsis: /name load -> obj
Description: Load tries to find an association for /name in each dictionary
 on the dictionary stack, starting with the current (top) dictionary.
 If an association is found, load pushes the associated value on the
 stack. If no association is found, an UndefineName error is raised.
SeeAlso: lookup, def
*/
void LoadFunction::execute(SLIInterpreter *i) const
{
    i->require_stack_load(1);
    i->require_stack_type(0,sli3::literaltype);

    Name name(i->top().data_.name_val);

    i->top()=i->lookup(name);
}

/*BeginDocumentation
Name: lookup -  Search for a key in each dictionay on the dictionary stack.
Synopsis: /name lookup -> obj true
                       -> false
Description: lookup tries to find an association for /name in each dictionary
 on the dictionary stack, starting with the current (top) dictionary.
 If an association is found, lookup pushes the associated value on the
 stack followed by the boolean true.
 If no association is found, false is pushed.
SeeAlso: load, def
*/
void LookupFunction::execute(SLIInterpreter *i) const
{
    i->require_stack_load(1);
    i->require_stack_type(0,sli3::literaltype);

    Name name(i->top().data_.name_val);

    // Axis I bundle step 4: dispatcher pre-popped /lookup.
    i->pop();

    Token content;
    bool result =i->lookup(name, content);
    if(result)
	i->push(content);
    i->push(result);

}

// `dict key call -> ...` — look up `key` in `dict`, push `dict` onto
// the dictstack as a new namespace, execute the value, then pop `dict`
// back off. Mirrors NEST 2.20.2 sli/oosupport.cc CallMemberFunction.
//
// The dispatcher will execute the next item on the estack; we leave
// behind [..., end, /key] so that:
//   1. /key resolves under the new top dict (= the one we pushed)
//      and produces the bound value, executing it if it's executable.
//   2. /end pops the dict back off the dictstack.
class CallMemberFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(2);
        i->require_stack_type(1, sli3::dictionarytype);
        i->require_stack_type(0, sli3::literaltype);
        Token dict_tok = i->pick(1);   // copy (refcount via Token)
        Name key = i->top().data_.name_val;
        i->pop(2);
        // Axis I bundle step 4: dispatcher pre-popped /call.
        i->DStack().push(dict_tok);    // open namespace
        i->EStack().push(i->baselookup(i->end_name));  // schedule close
        i->EStack().push(i->new_token<sli3::nametype, Name>(key));
    }
};

CallMemberFunction call_member_fn;



/*BeginDocumentation
Name: for - execute a procedure for a sequence of numbers
Synopsis: n1 s n2 proc for -> -
Description:
   for repeatedly evaluates the supplied procedure for all
   values from n1 to n2 in steps of s. In each iteration
   proc is called with the current iteration counter as
   argument.
   The loop can be quit prematurely by calling exit.
   If the value of the iteration counter is not needed,
   use repeat instead.
Examples:
SLI ] 1 1 10 {=} for
1
2
3
4
5
6
7
8
9
10
SLI ]
SeeAlso: repeat, exit, loop
*/
void ForFunction::execute(SLIInterpreter *i) const
{

    SLIType * for_t=i->get_type(sli3::ifortype);
    SLIType * mark_t=i->get_type(sli3::marktype);

    i->require_stack_load(4);
    i->require_stack_type(0,sli3::proceduretype);
    i->require_stack_type(1,sli3::integertype);
    i->require_stack_type(2,sli3::integertype);
    i->require_stack_type(3,sli3::integertype);

    // Axis I bundle step 4: dispatcher pre-popped /for. Push a
    // fresh mark + iter frame.
    i->EStack().push(Token(mark_t));
    i->EStack().push(i->pick(2));      // increment
    i->EStack().push(i->pick(1));      // limit
    i->EStack().push(i->pick(3));      // counter
    i->EStack().push(i->pick(0));      // procedure
    i->EStack().push(i->new_token<sli3::integertype>(i->top().data_.array_val->size()));
    i->EStack().push(for_t); // %for
    i->pop(4);
}

/*
BeginDocumentation

   Name: forall - Call a procedure for each element of a list/string/dictionary

   Synopsis:
     [v1 ... vn] {f}                   forall ->  f(v1) ... f(vn)
     (c1...cn)   {f}                   forall ->  f(c1) ... f(cn)
     <</key1 val1 ... /keyn valn>> {f} forall ->  f(/key1 val1) ... f(/keyn valn)

   Parameters:
     [v1,...,vn]    - list of n arbitrary objects
     (c1...cn)      - arbitrary string
     <</keyi vali>> - arbitrary dictionary
     {f}            - function which can operate on the elements of the
                      array, or on key/value pairs of the dictionary.
                      f is not required to return a specific number
		      of values.

   Description:
     Arrays:
       For each element of the input array, forall calls f.
       forall is similar to Map, however, it does not construct a
       new list from the return values.

     Dictionaries:
       For each key/value pair of the dictionary, forall calls f.
       Order on the operand stack will be: key, value. (I.e. value on top.)

       *Note: The dictionary contents are copied before operation.
        This can be a potentially expensive operation.*

     Loops can be nested. The loop can be quit prematurely with exit.

   Examples:
     [1 2 3 4 5]  {=} forall -> - % Print all values of the list
     [1 2 3 4 5]  {} forall  -> 1 2 3 4 5
     (abc) {=} forall -> prints 97 98 99 on separate lines
     <</a 1 /b 2>> {== ==} forall -> prints 1 /a 2 /b on separate lines

   Author:
     Marc-Oliver Gewaltig, Ruediger Kupper (dictionary variant)

   References: The Red Book

   SeeAlso: Map, MapAt, MapIndexed, Table, forallindexed, NestList, FoldList, Fold, exit

*/

/******************************/
/* forall_di                  */
/*  wrapper around forall_a   */
/*  implemented in SLI        */
/*  see typeinit.sli          */
/******************************/

/******************************/
/* forall_a                   */
/*  call: obj proc forall     */
/*  pick   1    0             */
/******************************/
void Forall_aFunction::execute(SLIInterpreter *i) const
{
    // Look up the iterator function freshly each call. A function-local
    // `static Token` here outlived the SLIInterpreter and held a
    // dangling SLIType pointer once the engine was destroyed, causing
    // a heap-use-after-free in ~Token() during atexit cleanup.
    i->require_stack_load(2);
    i->require_stack_type(0,sli3::proceduretype);
    i->require_stack_type(1,sli3::arraytype);
    TokenArray *proc= i->top().data_.array_val;

    // Axis I bundle step 4: dispatcher pre-popped /forall_a.
    i->EStack().push(i->new_token<sli3::marktype>());
    i->EStack().push(i->pick(1));        // push object
    i->EStack().push(i->new_token<sli3::integertype>(0));          // push array counter
    i->EStack().push(i->pick(0));       // push procedure
    i->EStack().push(i->new_token<sli3::integertype>(proc->size()));          // push procedure counter
    i->EStack().push(i->baselookup(i->iforallarray_name));
    i->pop(2);
}


/*
BeginDocumentation

   Name: forallindexed - Call a procedure for each element of a list/string

   Synopsis:
     [v1,...,vn] {f} forallindexed ->  f(0,v1),...,f(n-1,vn)

   Parameters:
     [v1,...,vn] - list of n arbitrary objects or string
     {f}         - function which can operate on the elements of the
                   array. f is not required to return a specific number
		   of values.

   Description:
     For each element of the input array, forallindexed calls f with
     two arguments, the element and its index within the list/string.
     forallindexed is similar to forall, only that the index of the
     element is also passed to the function f.

     Alternatives: Functions forallindexed_a for a list, forallindexed_s
     for a string (both undocumented) -> behaviour and synopsis are the same.

   Examples:

   [(a) (b)]  {} forallindexed -> (a) 0 (b) 1

   Author:
    Marc-Oliver Gewaltig

   References: The Red Book

   SeeAlso: Map, MapIndexed, Table, forall, NestList, FoldList

*/

/******************************/
/* forallindexed_a            */
/*  call: obj proc forall     */
/*  pick   1    0             */
/******************************/
void Forallindexed_aFunction::execute(SLIInterpreter *i) const
{
    i->require_stack_load(2);
    i->require_stack_type(0,sli3::proceduretype);
    i->require_stack_type(1,sli3::arraytype);

    assert(i->top().data_.array_val != 0);

    // Phase 5: frame layout converted to body-walking form, parallel
    // to ifortype. Layout (bottom -> top):
    //   [mark, ad, lim, count, proc, pos=proclen, iforallindexedarraytype]
    // pos starts at proclen so body_walk's first iteration triggers
    // body_exhausted, which advances count, pushes ad[count]+count,
    // and resets pos=0.
    i->EStack().push(i->new_token<sli3::marktype>());
    i->EStack().push(i->pick(1));        // push object

    TokenArray  *ad= i->EStack().top().data_.array_val;
    assert(ad !=NULL);

    TokenArray  *proc= i->top().data_.array_val;
    i->EStack().push(i->new_token<sli3::integertype>(ad->size())); // limit
    i->EStack().push(i->new_token<sli3::integertype>(0));          // counter
    i->EStack().push(i->pick(0));                                  // proc
    i->EStack().push(i->new_token<sli3::integertype>(proc->size())); // pos
    i->EStack().push(Token(i->get_type(sli3::iforallindexedarraytype)));
    i->pop(2);
}

/******************************/
/* forallindexed_s            */
/*  call: obj proc forall     */
/*  pick   1    0             */
/******************************/
void Forallindexed_sFunction::execute(SLIInterpreter *i) const
{
    i->require_stack_load(2);
    i->require_stack_type(0,sli3::proceduretype);
    i->require_stack_type(1,sli3::stringtype);

    assert(i->top().data_.array_val != 0);

    // Phase 5: parallel to Forallindexed_a above. Layout:
    //   [mark, str, lim, count, proc, pos=proclen, iforallindexedstringtype]
    i->EStack().push(i->new_token<sli3::marktype>());
    i->EStack().push(i->pick(1));        // push object

    SLIString  *strd= i->EStack().top().data_.string_val;
    assert(strd !=NULL);

    TokenArray  *proc= i->top().data_.array_val;
    i->EStack().push(i->new_token<sli3::integertype>(strd->size())); // limit
    i->EStack().push(i->new_token<sli3::integertype>(0));            // counter
    i->EStack().push(i->pick(0));                                    // proc
    i->EStack().push(i->new_token<sli3::integertype>(proc->size())); // pos
    i->EStack().push(Token(i->get_type(sli3::iforallindexedstringtype)));
    i->pop(2);
}

/******************************/
/* forall_s                   */
/*  call: obj proc forall     */
/*  pick   1    0             */
/******************************/
void Forall_sFunction::execute(SLIInterpreter *i) const
{
    i->require_stack_load(2);
    i->require_stack_type(0,sli3::proceduretype);
    i->require_stack_type(1,sli3::stringtype);

    assert(i->top().data_.array_val != 0);

    // Phase 5: body-walking form, parallel to Forall_aFunction (which
    // uses iforalltype). Layout:
    //   [mark, str, idx=0, proc, pos=proclen, iforallstringtype]
    i->EStack().push(i->new_token<sli3::marktype>());
    i->EStack().push(i->pick(1));        // push object

    SLIString  *strd= i->EStack().top().data_.string_val;
    assert(strd !=NULL);
    (void)strd;  // size info now reached via body_exhausted handler

    TokenArray  *proc= i->top().data_.array_val;
    i->EStack().push(i->new_token<sli3::integertype>(0));            // idx
    i->EStack().push(i->pick(0));                                    // proc
    i->EStack().push(i->new_token<sli3::integertype>(proc->size())); // pos
    i->EStack().push(Token(i->get_type(sli3::iforallstringtype)));
    i->pop(2);
}

/* ForallFunction: compact /forall dispatcher. Replaces the
 * 3-arm trie formerly built by typeinit.sli. The C++ array
 * and string arms call the typed leaves directly; the dict
 * arm pushes the SLI-defined /forall_di procedure onto the
 * e-stack, where the procedure machinery picks it up on the
 * next iteration.
 *
 * Call: coll proc forall -> -
 * pick  1    0
 *
 * The forall_afunction / forall_sfunction instances are
 * defined further down in this file; forward-extern them so
 * we can call them here.
 */
extern Forall_aFunction         forall_afunction;
extern Forall_sFunction         forall_sfunction;
extern Forallindexed_aFunction  forallindexed_afunction;
extern Forallindexed_sFunction  forallindexed_sfunction;
extern DefFunction              deffunction;

void ForallFunction::execute(SLIInterpreter *i) const
{
    i->require_stack_load(2);
    if (i->pick(0).tag() != sli3::proceduretype) {
        i->raiseerror(i->ArgumentTypeError);
        return;
    }
    switch (i->pick(1).tag()) {
      case sli3::proceduretype:
      case sli3::litproceduretype:
        // Procedures share the TokenArray payload with arrays; flip
        // the SLIType pointer so forall_a sees an array. The slot is
        // popped inside forall_a, so the relabel doesn't outlive the
        // call.
        i->pick(1).type_ = i->get_type(sli3::arraytype);
        [[fallthrough]];
      case sli3::arraytype:
        forall_afunction.execute(i);
        return;
      case sli3::stringtype:
        forall_sfunction.execute(i);
        return;
      case sli3::dictionarytype: {
        // /forall_di is defined in typeinit.sli as an SLI
        // procedure. Look it up (baselookup cache hit after
        // first call) and let the procedure machinery run it.
        // forall_di consumes the dict+proc pair from the ostack
        // itself, so we leave them in place and just push the
        // procedure onto the estack -- after popping our own
        // function frame off the top.
        Token forall_di = i->baselookup(Name("forall_di"));
        i->EStack().push(forall_di);   // schedule the SLI proc
        return;
      }
    }
    i->raiseerror(i->ArgumentTypeError);
}

/* ForallindexedFunction: compact /forallindexed dispatcher.
 * Same 2-arm shape as /forall minus the dict arm. The trie
 * typeinit.sli formerly built selected forallindexed_a or
 * forallindexed_s based on the collection's type.
 *
 * Call: coll proc forallindexed -> -
 * pick  1    0
 */
void ForallindexedFunction::execute(SLIInterpreter *i) const
{
    i->require_stack_load(2);
    if (i->pick(0).tag() != sli3::proceduretype) {
        i->raiseerror(i->ArgumentTypeError);
        return;
    }
    switch (i->pick(1).tag()) {
      case sli3::arraytype:
        forallindexed_afunction.execute(i);
        return;
      case sli3::stringtype:
        forallindexed_sfunction.execute(i);
        return;
    }
    i->raiseerror(i->ArgumentTypeError);
}

/* DefDispatchFunction: compact /def dispatcher. Replaces the
 * 4-arm trie typeinit.sli formerly built.
 *
 * Call shapes:
 *   /lit obj             def -> -      (2 args)
 *   /lit [types] obj     def -> -      (3 args)
 *
 * The 2-arg form delegates to DefFunction (the C++ leaf).
 * The 3-arg form delegates to the SLI-defined /:def_ proc,
 * which validates the type array and then calls def_.
 *
 * Disambiguation: read the o-stack top backwards. If we have
 * three operands AND slot 2 is /lit AND slot 1 is an array,
 * that's the 3-arg form. Otherwise it's 2-arg (and the front
 * of the o-stack just happens to hold an array or whatever
 * unrelated).
 */
void DefDispatchFunction::execute(SLIInterpreter *i) const
{
    size_t load = i->OStack().load();
    if (load < 2) {
        i->require_stack_load(2);   // raises StackUnderflow
        return;
    }
    if (load >= 3
        && i->pick(2).tag() == sli3::literaltype
        && i->pick(1).tag() == sli3::arraytype)
    {
        // 3-arg typecheck form: /lit [/types] obj :def_
        Token def_colon = i->baselookup(Name(":def_"));
        i->EStack().push(def_colon);    // schedule SLI proc
        return;
    }
    if (i->pick(1).tag() == sli3::literaltype) {
        // 2-arg /lit obj def -> raw C++ def. Under step-4 contract,
        // the dispatcher pre-popped /def, so deffunction (new-ABI)
        // sees a clean e-stack top.
        deffunction.execute(i);
        return;
    }
    i->raiseerror(i->ArgumentTypeError);
}

/* BeginDocumentation
 Name: raiseerror - raise an error to the system
 Synopsis:
 /command /error raiserror -> /command (side-effects see below!)

 Description:
   raiseerror is a SLI interface to the interpreter's error
   handling mechanism (see The Red Book for details). If an error
   is raised, the following actions are performed:
   * the value of errordict /newerror is set to true
   * the value of errordict /commandname is set to the name of the
     command which raised the error
   * the name of the command which raised the error is pushed on
     the operand stack.
   * If the value of errordict /recorstack is true,
     the state of the interpreter is saved:
     - the operand stack is copied to errordict /ostack
     - the execution stack is copied to errordict /estack
     - the dictionary stack is copied to errordict /dstack
   * stop is called. Stop then tries to find an enclosing stopped
     context and calls the associated function.

   This mechanism is explained in detail in The PostScript Reference Manual.

   Please note that when raiserror is called, the state of the stacks
   should be restored to its initial state.

 Examples:
   /save_sqrt
   {
      0 gt % is the argument positive ?
      {
        sqrt
      }
      { % else, issue an error
        /save_sqrt /PositiveValueExpected raiseerror
      } ifelse
   } def

 Bugs: lets wait...
 Author: Gewaltig
 Remarks: not part of PostScript, but conform to the mechanism
 References: See the Red Book for PostScript's error handling facilities
 SeeAlso: raiseagain, stop, stopped, errordict
*/

void RaiseerrorFunction::execute(SLIInterpreter *i) const
{
        // pick :  2     1
        // call : /cmd /err raiseerror

    i->require_stack_load(2);
    i->require_stack_type(0,sli3::literaltype);
    i->require_stack_type(1,sli3::literaltype);

    Name errorname(i->top().data_.name_val);
    Name cmdname(i->pick(1).data_.name_val);

    i->pop(2);            // consume both literal args (matches NEST 2.x convention)
    // Axis I bundle step 4: dispatcher pre-popped /raiseerror; the
    // C++ raiseerror(N,N) does no estack pop itself.
    i->raiseerror(cmdname, errorname);
}

/* BeginDocumentation
 Name: print_error - print an error based on the errordict
 Synopsis:
 /command print_error -> --

 Description:
   print_error prints a message describing the content of
   the error dictionary. Please see the example below for
   information about how to use this function.
   Note: The errordict parameters command and message are
   reset after a call to print_error. The errorname is not
   reset, so every call to print_error will use the same
   errorname until this parameter is redefined.

 Examples:
   errordict /errorname /MyError put_d
   errordict /command /my_function put_d
   errordict /message (Something went wrong.) put_d
   /my_function print_error

 Bugs:
 Author:
 Remarks:
 References:
 SeeAlso: handleerror, raiseerror, raiseagain, stop, stopped, errordict
*/

void PrinterrorFunction::execute(SLIInterpreter *i) const
{
  // The name of the command that raised the error should be
  // placed at the top of the OStack.
    i->require_stack_load(1);

    // Call print_error function.
    i->print_error(i->top());

    i->pop();
}

/* BeginDocumentation
 Name: raiseagain - re-raise the last error
 Synopsis:  raiseagain

 Description:
   raiseagain re-raises a previously raised error. This is useful
   if an error handler cannot cope with a particular error (e.g. a signal)
   and wants to pass it to an upper level handler. Thus, nestet error handlers
   are possible.

 Bugs: lets wait...
 Author: Gewaltig
 Remarks: not part of PostScript
 References: See the Red Book for PostScript's error handling facilities
 SeeAlso: raiseerror, stop, stopped, errordict
*/

void RaiseagainFunction::execute(SLIInterpreter *i) const
{
    // Axis I bundle step 4: dispatcher pre-popped /raiseagain.
    i->raiseagain();
}

/*BeginDocumentation
Name: cycles - return the number of elapsed interpreter cycles
Synopsis: cycles -> n
*/
void CyclesFunction::execute(SLIInterpreter *i) const
{
    // Axis I bundle step 4: dispatcher pre-popped /cycles.
    i->push(i->cycles());
}

/*BeginDocumentation
Name: quit - leave the SLI interpreter, optionally return exit code

Synopsis:
           quit   -> -
  exitcode quit_i -> -

Description:
This function leaves the SLI interpreter, returning an exit code to
the calling shell. The first variant (quit) returns the value
EXIT_SUCCESS (usually 0). The second variant (quit_i) returns the
value given by the integer argument. All data which has not been saved
is lost.

Common exit codes and their reasons can be found in the dictionary
statusdict/exitcodes.

Parameters:
exitcode (integertype): exit code to return to the shell. If this
parameter is not given, statusdict/exitcode is used instead.

Examples:
quit      % quit SLI with exit code statusdict/exitcode.
23 quit_i % quit SLI with exit code 23.

Author: unknown, JM Eppler, R Kupper

FirstVersion: long time ago

Remarks:
This command should not be mixed up with exit.

Availability: SLI-2.0

References:
POSIX Programmer's Manual.

SeeAlso: exit
*/


/*BeginDocumentation
Name: exec - execute an object
Synopsis: any exec -> -
Description: exec tries to execute the object by moving it to
the execution stack.
Examples: {1 2 add} exec -> 3
SeeAlso: cvx
*/
void ExecFunction::execute(SLIInterpreter *i) const
{
    // new-ABI: dispatcher pre-popped /exec. Move the ostack top
    // (the object to execute) onto the estack via swap-out. The
    // vacated ostack slot is type_=0 so the trailing pop is a
    // no-op refcount-wise.
    i->require_stack_load(1);
    i->EStack().push(Token{});
    i->EStack().top().swap(i->top());
    i->pop();
}

// Phase 4: deep bind. Walks `body` once and replaces every nametype
// token whose dstack lookup resolves to a functiontype or trietype
// with that resolved Token. Recurses into nested procedure / litproc
// bodies in place. Names that resolve to something else (or are
// undefined) are preserved -- gs behaviour, and matches the SLI
// /bind definition in sli-init.sli line 261.
//
// The displaced name Tokens drop their name-handle refs cheaply.
// The replacement copies the resolved Token, which add_references
// the function/trie if needed (functiontype is a non-refcount type
// so this is a plain field copy; trietype is refcounted).
static void bind_body(SLIInterpreter *i, TokenArray *body)
{
    size_t const n = body->size();
    for (size_t k = 0; k < n; ++k)
    {
        Token &slot = body->get(k);
        unsigned const tag = slot.tag();
        if (tag == sli3::nametype)
        {
            Name nm(slot.data_.long_val);
            Token *resolved = nullptr;
            if (i->DStack().known(nm))
                resolved = &i->DStack().lookup(nm);
            if (resolved != nullptr)
            {
                unsigned const rt = resolved->tag();
                if (rt == sli3::functiontype || rt == sli3::trietype)
                    slot = *resolved;  // Token::operator= handles
                                       // refcounts on the displaced
                                       // name and the new payload.
            }
        }
        else if (tag == sli3::proceduretype
              || tag == sli3::litproceduretype)
        {
            bind_body(i, slot.data_.array_val);
        }
    }
}

/*BeginDocumentation
Name: bind - resolve names in a procedure body to their executable values
Synopsis: proc bind -> proc
Description:
  bind walks the procedure body and replaces every name token that
  resolves to an operator (functiontype) or trie with the resolved
  token directly. Nested procedures are recursed into. Names that
  resolve to user-defined procedures, constants, or undefined names
  are left as-is.

  After bind, the dispatcher's inner loop sees executable function
  tokens directly instead of having to look the name up in the
  dictionary stack at every iteration. For tight loops this is the
  difference between O(name lookups per token) and zero.

  This is the C++ replacement for the SLI /bind in sli-init.sli;
  identical semantics, ~50x faster per pass.

Examples:
  /sum { 1 2 add } bind def     -- /add resolves at bind-time

SeeAlso: trie, addtotrie
*/
void BindFunction::execute(SLIInterpreter *i) const
{
    // new-ABI: dispatcher pre-popped /bind. Body is at ostack pick(0).
    i->require_stack_load(1);
    unsigned const tag = i->top().tag();
    if (tag != sli3::proceduretype && tag != sli3::litproceduretype)
    {
        i->raiseerror(i->ArgumentTypeError);
        return;
    }
    bind_body(i, i->top().data_.array_val);
    // proc stays on the operand stack -- bind returns its argument.
}

/*BeginDocumentation
Name: typeinfo - return the type of an object
Synopsis: any type -> any literal
Description: typeinfo returns a literal name, which represents
 the type of the argument. The argument is left on the stack.
SeeAlso: typestack, type
*/
void TypeinfoFunction::execute(SLIInterpreter *i) const
{
    // `obj typeinfo -> obj /typename` — leaves the source on the
    // operand stack and pushes a LITERAL of its type-name on top.
    // Critical: the result MUST be literaltype (not nametype),
    // because callers (e.g. typeinit.sli's :def_) compare it with
    // /trietype using eq, which checks both type and value.
    SLIType *literal_t = i->get_type(sli3::literaltype);
    i->require_stack_load(1);
    if (i->top().type_ == nullptr)
    {
        i->raiseerror(i->ArgumentTypeError);
        return;
    }
    Token t(literal_t);
    t.data_.name_val = i->top().get_typename().toIndex();
    i->push(t);
}

void SwitchFunction::execute(SLIInterpreter *i) const
{
        // mark obj1 obj2 ... objn switch
        // Executes obj1 to obj2. If one object executes
        // exit, the execution of all other objects is
        // terminated.
    i->require_stack_load(1);
    // Axis I bundle step 4: /switch's own frame already popped by
    // the dispatcher. The recover/restore dance is gone.

    i->EStack().push(i->new_token<sli3::marktype>());
    i->EStack().push(i->baselookup(i->ipop_name));

    unsigned long depth = i->load();
    unsigned long pos = 0;
    unsigned int rewind=2;

    bool found =  (i->pick(pos).is_of_type(sli3::marktype));

    while( (pos < depth) && ! found )
    {
        i->EStack().push(i->pick(pos));
        found =  (i->pick(++pos).is_of_type(sli3::marktype));
	++rewind;
    }

    if(!found)
    {
	i->EStack().pop(rewind);
	i->raiseerror("UnmatchedMark");
	return;
    }
    i->pop(pos+1);
}

void SwitchdefaultFunction::execute(SLIInterpreter *i) const
{
  // mark obj1 obj2 ... objn defobj switchdefault
  // checks if n=0.
  // if so, executes defobj.
  // else pops defobj and calls switch to execute obj1..objn
  // If one object executes exit, the execution of all other
  // objects is terminated.
  // Axis I bundle step 4: /switchdefault frame already popped.

    i->EStack().push(i->new_token<sli3::marktype>());
    i->EStack().push(i->baselookup(i->ipop_name));
    unsigned int rewind=2;
    unsigned long depth = i->load();
    unsigned long pos = 0;

    if(depth == 0)
	throw TypeMismatch("At least 1 argument.", "Nothing.");

    if (depth>1 && not (i->pick(1).is_of_type(sli3::marktype))
	&& not i->pick(0).is_of_type(sli3::marktype))
	i->pop();                               // thus pop it!

    bool found =  (i->pick(pos).is_of_type(sli3::marktype));


    while( (pos < depth) && ! found )
    {
        i->EStack().push(i->pick(pos));
	++rewind;
        found =  (i->pick(++pos).is_of_type(sli3::marktype));
    }

    if(!found)
    {
	i->EStack().pop(rewind);
        i->raiseerror("UnmatchedMark");
	return;
    }

    i->pop(pos+1);
}

void CaseFunction::execute(SLIInterpreter *i) const
{
        // true  obj case -> obj
        // false obj case -> -
        // case is used in combinaion with the switch statement

    i->require_stack_load(2);

    // Axis I bundle step 4: dispatcher pre-popped /case.
    if(i->pick(1) == true)
    {
	i->top().swap(i->pick(1));
       i->pop();
    }
    else if(i->pick(1) == false)
    {
        i->pop(2);
    }
    else
    {
        i->raiseerror(i->ArgumentTypeError);
    }
}

/*BeginDocumentation
Name: counttomark - count number of objects on the stack from top to marker
Synopsis: mark obj1 ... objn counttomark -> mark obj1 ... objn n
SeeAlso: count
*/
void CounttomarkFunction::execute(SLIInterpreter *i) const
{
    long depth = i->load();
    long pos   = 0;
    bool found = false;

    while( (pos < depth) && !found)
    {
        found = (i->pick(pos).is_of_type(sli3::marktype));
        ++pos;
    }

    if(!found)
    {
        i->raiseerror("UnmatchedMark");
	return;
    }
    i->push(pos-1);
}

/* BeginDocumentation
 Name: pclocks - returns POSIX clocks for real, user, system time
 Synopsis:  pclocks -> [rclocks uclocks sclocks cuclocks csclocks]
 Description:
 Calls the POSIX times() function to obtain real, user,
 and system time clock counts, as well as user and system time clock
 counts for all children.  Real time clocks have arbitrary origin,
 i.e., only differences are meaningful.
 See man times(3) for details.

 Note: results for user and system time may not be reliable if more
 than one thread is used.

 Author: Hans Ekkehard Plesser
 FirstVersion: 2003-07-29, based on Ptimesfunction
 References: man 2 times
 SeeAlso: pclockspersec, ptimes, realtime, usertime, systemtime, tic, toc
*/

void PclocksFunction::execute(SLIInterpreter *i) const
{
   struct tms foo;
   const clock_t realtime = times(&foo);

   if ( realtime == (clock_t)(-1) )
   {
     i->message(sli3::M_ERROR, "PclocksFunction",
 	     "System function times() returned error!");
     i->raiseerror("SystemError");
     return;
   }

   Token result(i->get_type(sli3::arraytype));
   result.data_.array_val= new TokenArray();
   result.data_.array_val->reserve(5);
   result.data_.array_val->push_back(i->new_token<sli3::integertype>((long)realtime));
   result.data_.array_val->push_back(i->new_token<sli3::integertype>((long)foo.tms_utime));
   result.data_.array_val->push_back(i->new_token<sli3::integertype>((long)foo.tms_stime));
   result.data_.array_val->push_back(i->new_token<sli3::integertype>((long)foo.tms_cutime));
   result.data_.array_val->push_back(i->new_token<sli3::integertype>((long)foo.tms_cstime));
   i->push(result);

}

/* BeginDocumentation
Name: pclockspersec - POSIX clock ticks per second
Synopsis: pclockspersec -> clockticks
Description:
pclockspersec i an integer variable containing the number of
POSIX clock ticks per second.
Author: Hans Ekkehard Plesser
FirstVersion: 2003-07-29
Remarks: Replaces clockspersecond.
References: man 2 times
SeeAlso: pclocks, ptimes, realtime, usertime, systemtime, tic, toc
*/
void PclockspersecFunction::execute(SLIInterpreter *i) const
{
  const long cps = sysconf(_SC_CLK_TCK);

  if ( cps <= 0 )
  {
    i->message(sli3::M_ERROR, "PclockspersecFunction",
	     "This system does not support sysconf(_SC_CLK_TCK)!");
    i->raiseerror("FunctionUnsupported");
    return;
  }

  i->push(cps);
}

/* BeginDocumentation
 Name: pgetrusage - Get resource consumption information
 Synopsis:  pgetrusage - selfinfo childinfo
 Description:
 Calls the POSIX getrusage() function to obtain information on
 memory consumption, context switches, I/O operatin count, etc,
 for both the main process and its children.  Information is
 returned in dictionaries.

 Author: Hans Ekkehard Plesser
 FirstVersion: 2003-07-29
 Remarks: At least under Linux, child processes return 0 for all
 entries, while the main process seems to produce meaningfull data
 only for minflt and majflt, i.e., page reclaims and faults.
 References: man 2 getrusage
 SeeAlso: pclockspersec, ptimes, realtime, usertime, systemtime, tic, toc
*/

void PgetrusageFunction::execute(SLIInterpreter *i) const
{
  Dictionary *self= new Dictionary;
  Dictionary *children= new Dictionary;

  if ( !getinfo_(i, RUSAGE_SELF, self) )
  {
    i->message(sli3::M_ERROR, "PgetrusageFunction",
	     "System function getrusage() returned error for self!");
    i->raiseerror("SystemError");
    return;
  }

  if ( !getinfo_(i, RUSAGE_CHILDREN, children) )
  {
    i->message(sli3::M_ERROR, "PgetrusageFunction",
	     "System function getrusage() returned error for children!");
    i->raiseerror("SystemError");
    return;
  }
  i->push(self);
  i->push(children);
}

bool PgetrusageFunction::getinfo_(SLIInterpreter *i, int who, Dictionary *dict) const
{
  struct rusage data;

  if ( getrusage(who, &data) != 0 )
    return false;

  (*dict)["maxrss"] =   i->new_token<sli3::integertype>(data.ru_maxrss);     /* maximum resident set size */
  (*dict)["ixrss"] =    i->new_token<sli3::integertype>(data.ru_ixrss);      /* integral shared memory size */
  (*dict)["idrss"] =    i->new_token<sli3::integertype>(data.ru_idrss);      /* integral unshared data size */
  (*dict)["isrss"] =    i->new_token<sli3::integertype>(data.ru_isrss);      /* integral unshared stack size */
  (*dict)["minflt"] =   i->new_token<sli3::integertype>(data.ru_minflt);     /* page reclaims */
  (*dict)["majflt"] =   i->new_token<sli3::integertype>(data.ru_majflt);     /* page faults */
  (*dict)["nswap"] =    i->new_token<sli3::integertype>(data.ru_nswap);      /* swaps */
  (*dict)["inblock"] =  i->new_token<sli3::integertype>(data.ru_inblock);    /* block input operations */
  (*dict)["oublock"] =  i->new_token<sli3::integertype>(data.ru_oublock);    /* block output operations */
  (*dict)["msgsnd"] =   i->new_token<sli3::integertype>(data.ru_msgsnd);     /* messages sent */
  (*dict)["msgrcv"] =   i->new_token<sli3::integertype>(data.ru_msgrcv);     /* messages received */
  (*dict)["nsignals"] = i->new_token<sli3::integertype>(data.ru_nsignals);   /* signals received */
  (*dict)["nvcsw"] =    i->new_token<sli3::integertype>(data.ru_nvcsw);      /* voluntary context switches */
  (*dict)["nivcsw"] =   i->new_token<sli3::integertype>(data.ru_nivcsw);     /* involuntary context switches */

  return true;
}


/* BeginDocumentation
 Name: time - return wall clock time in s since 1.1.1970 0
 Synopsis:  time -> int
 Description: calls time() and returns seconds since 1.1.1970,
 00:00:00 UTC.  This is mainly meant as a tool to generate random seeds.
 Author: Hans E. Plesser
 FirstVersion: 2001-10-03
 SeeAlso: clock, usertime, tic, toc, sleep
*/

void TimeFunction::execute(SLIInterpreter *i) const
{
  long secs = time(0);
  i->push(secs);
}

/*BeginDocumentation
Name: realtime - Wall-clock time, in seconds, as a double.
Synopsis: realtime -> double
Description:
 Returns the current wall-clock time as a double in seconds since
 the epoch. Resolution is typically microsecond on POSIX (uses
 gettimeofday). Used by tic/toc/clock in sli-init.sli when the
 SLI definition (/realtime { ptimes 0 get } def) is shadowed.
*/
void RealtimeFunction::execute(SLIInterpreter *i) const
{
  struct timeval tv;
  if (gettimeofday(&tv, nullptr) != 0)
  {
    i->message(sli3::M_ERROR, "RealtimeFunction",
               "System function gettimeofday() returned error!");
    i->raiseerror("SystemError");
    return;
  }
  double const t =
      static_cast<double>(tv.tv_sec) +
      1.0e-6 * static_cast<double>(tv.tv_usec);
  i->push(t);
}

/*BeginDocumentation
Name: ptimes - process-time array as doubles in seconds
Synopsis: ptimes -> [rtime utime stime cutime cstime]
Description:
 rtime is wall-clock time (gettimeofday), utime/stime are
 self user/system times (getrusage RUSAGE_SELF), cutime/cstime
 are children user/system times (getrusage RUSAGE_CHILDREN).
 Used by /realtime, /usertime, /systemtime, /tic, /toc.
*/
void PtimesFunction::execute(SLIInterpreter *i) const
{
  struct timeval tv;
  if (gettimeofday(&tv, nullptr) != 0)
  {
    i->message(sli3::M_ERROR, "PtimesFunction",
               "gettimeofday() failed");
    i->raiseerror("SystemError");
    return;
  }
  double const rtime =
      static_cast<double>(tv.tv_sec) +
      1.0e-6 * static_cast<double>(tv.tv_usec);

  auto tv_to_d = [](struct timeval const& v) {
      return static_cast<double>(v.tv_sec) +
             1.0e-6 * static_cast<double>(v.tv_usec);
  };

  struct rusage self{}, kids{};
  getrusage(RUSAGE_SELF, &self);
  getrusage(RUSAGE_CHILDREN, &kids);

  TokenArray *arr = new TokenArray();
  arr->reserve(5);
  arr->push_back(i->new_token<sli3::doubletype>(rtime));
  arr->push_back(i->new_token<sli3::doubletype>(tv_to_d(self.ru_utime)));
  arr->push_back(i->new_token<sli3::doubletype>(tv_to_d(self.ru_stime)));
  arr->push_back(i->new_token<sli3::doubletype>(tv_to_d(kids.ru_utime)));
  arr->push_back(i->new_token<sli3::doubletype>(tv_to_d(kids.ru_stime)));

  Token result(i->get_type(sli3::arraytype));
  result.data_.array_val = arr;
  i->push(result);
}

/*BeginDocumentation
Name: sleep_i - suspends proces for n seconds
Synopsis:  n sleep_i -> -
Description:
Calls the POSIX select() function.
SeeAlso: clock, usertime, tic, toc
*/

void Sleep_iFunction::execute(SLIInterpreter *i) const
{
  i->require_stack_load(1);
  i->require_stack_type(0,sli3::integertype);
  const long sec    =  i->pick(0).data_.long_val;
  const long usec   = 0;
  struct timeval tv = { sec, usec };

  if (sec>0)
    select( 0, 0, 0, 0, &tv );

  i->pop();
}

/*BeginDocumentation
Name: sleep_i - suspends proces for x seconds
Synopsis:  x sleep_d -> -
Description:
Calls the POSIX select() function.
SeeAlso: clock, usertime, tic, toc
*/

void Sleep_dFunction::execute(SLIInterpreter *i) const
{
    i->require_stack_load(1);
    i->require_stack_type(0, sli3::doubletype);
    // Stage 5.5: read the double payload, not long_val. Previously
    // sleep_d read the bit pattern of the double interpreted as a
    // long and slept for an arbitrary amount of time.
    double const seconds = i->pick(0).data_.double_val;
    long const sec = static_cast<long>(seconds);
    long const usec_long =
        static_cast<long>((seconds - static_cast<double>(sec)) * 1.0e6);

    struct timeval tv;
    tv.tv_sec  = sec;
    tv.tv_usec = static_cast<suseconds_t>(usec_long);

    if (sec > 0 || usec_long > 0)
        select(0, 0, 0, 0, &tv);

    i->pop();
}


/*BeginDocumentation
Name: token_s - read a token from a string
Synopsis:  string token_s -> post any true
                            false
References: The Red Book
SeeAlso: token
*/

void  Token_sFunction::execute(SLIInterpreter *i) const
{
  // Stage 4.6: arity / type check BEFORE popping the e-stack frame.
  // If the precondition throws, raiseerror pops on its own;
  // popping here too would take away an unrelated frame.
  i->require_stack_load(1);
  i->require_stack_type(0, sli3::stringtype);
  // Axis I bundle step 4: dispatcher pre-popped /token_s.

  SLIString *sd = i->top().data_.string_val;
  assert(sd != NULL);
  std::istringstream in(sd->c_str());

  Token t;
  i->clear_parser_context(); // this clears the previously parsed strings.
  t = i->read_token(in);
  if (t == i->EndSymbol)
  {
    i->pop();
    i->push(false);
  }
  else
  {
      std::string s;
      char c;

      i->push(t);
      while (in.get(c))
        s += c;

      // Stage 4.6: copy-on-write when the SLIString is shared.
      // Otherwise a `dup token_s` (or any aliased holder) sees the
      // remainder string mutate under it. Refcount > 1 means
      // someone else holds a reference; rebuild a fresh SLIString
      // for the operand-stack slot.
      if (sd->references() == 1)
      {
          sd->str() = s;
      }
      else
      {
          // pick(1) is the original string Token (we pushed t on top).
          // Replace its payload with a new heap SLIString. Token
          // assignment will release the old shared pointer correctly.
          Token replaced(i->get_type(sli3::stringtype));
          replaced.data_.string_val = new SLIString(s);
          i->pick(1) = replaced;
      }
      i->push(true);
  }
}
/*BeginDocumentation
Name: token_is - read a token from an input stream
Synopsis:  istream token_is -> istream any true
                              istream false
References: The Red Book
SeeAlso: token
*/

void  Token_isFunction::execute(SLIInterpreter *i) const
{
  i->require_stack_load(1);
  i->require_stack_type(0,sli3::istreamtype);

  // Axis I bundle step 4: dispatcher pre-popped /token_is.
  SLIistream *sd= i->top().data_.istream_val;
  assert(sd != 0);
  assert(sd->get() != NULL);

  Token t;
  t=i->read_token(*sd->get());
  if(t == i->EndSymbol)
  {
      i->push(false);
  }
  else
  {
    i->push(t);
    i->push(true);
  }
}

/*BeginDocumentation
Name: symbol_s - read a symbol from a string
Synopsis:  string symbol_s -> post any true
                              false
SeeAlso: token
*/

void  Symbol_sFunction::execute(SLIInterpreter *i) const
{
  // Stage 4.10: implemented as a thin wrapper around read_token --
  // sli-init.sli's /css uses symbol_s only on strings that
  // tokenize cleanly as symbols (e.g. literal names), so the
  // narrower "readSymbol" semantics from NEST 2.x aren't needed
  // for the bootstrap path. If a future use case needs strict
  // symbol-only filtering, gate the result Token's typeid here.
  i->require_stack_load(1);
  i->require_stack_type(0, sli3::stringtype);
  // Axis I bundle step 4: dispatcher pre-popped /symbol_s.

  SLIString *sd = i->top().data_.string_val;
  assert(sd != NULL);
  std::istringstream in(sd->c_str());

  Token t;
  i->clear_parser_context();
  t = i->read_token(in);
  if (t == i->EndSymbol)
  {
    i->pop();
    i->push(false);
  }
  else
  {
    std::string remainder;
    char c;
    i->push(t);
    while (in.get(c))
      remainder += c;

    // Copy-on-write under shared payload (Stage 4.6 pattern).
    if (sd->references() == 1)
    {
        sd->str() = remainder;
    }
    else
    {
        Token replaced(i->get_type(sli3::stringtype));
        replaced.data_.string_val = new SLIString(remainder);
        i->pick(1) = replaced;
    }
    i->push(true);
  }
}





/* BeginDocumentation
 Name: setguard - limit the number of interpreter cycles
 Synopsis:  n setguard -> --
 Description: This command forces the interpreter to stop after
   it has performed n cycles. setguard is useful for testing programs
   with long running loops.

 Parameters: n : an integer argument greater than zero
 Examples:
 Bugs:
 Author: Gewaltig
 FirstVersion: ?
 Remarks: not part of PostScript
 References:
 SeeAlso: removeguard
*/

void SetGuardFunction::execute(SLIInterpreter *i) const
{
  i->require_stack_load(1);
  i->require_stack_type(0,sli3::integertype);
  long count= i->top().data_.long_val;
  i->setcycleguard(count);
  i->pop();
}


/* BeginDocumentation
 Name: removeguard - removes the limit on the number of interpreter cycles
 Synopsis:  removeguard -> --
 Description: This command removes the restriction on the number
   of possible interpreter cycles, imposed by setguard.

 Parameters: none
 Examples:
 Bugs:
 Author: Gewaltig
 FirstVersion: ?
 Remarks: not part of PostScript
 References:
 SeeAlso: setguard
*/
void RemoveGuardFunction::execute(SLIInterpreter *i) const
{
  i->removecycleguard();
}


/*
BeginDocumentation
Name: debugon - Start SLI level debugger.

Description:
The SLI Debug mode allows you to debug SLI programs by tracing
the execution of the individual commands.

In debug mode, each SLI instruction is stepped individually. After
each instruction, the user can see the next command, the contents of
the stack, the entire procedure being executed.

It is also possible to set a breakpoint, so that variables and stacks
in the procedure context can be examined and modified.


The debug mode is controled by the debug level. Whenever execution
enters a procedure or loop, the debug level is increased.
If the procedure is left, the debug level is decreased.
The user can specify a max debug level, beyond which the debug mode
is suspended.
This way, the user can skip over functions which are beyond a certain
level.

The following commands are available:

  next        - Trace (execute) next command.
  continue    - Continue this level without debugging
  step        - Step over deeper levels.
  list        - list current procedure or loop.
  where       - show backtrace of execution stack.
  stack       - show operand stack.
  estack      - show execution stack.
  edit        - enter interactive mode.
  stop        - raise an exception.
  help        - display this list.
  quit        - quit debug mode.
  show next   - show next command.
  show stack  - show operand stack.
  show backtrace- same as 'where'.
  show estack - show execution stack.
  toggle stack     - toggle stack display.
  toggle catch     - toggle debug on error.
  toggle backtrace - toggle stack backtrace on error.
  toggle tailrecursion - toggle tail-recursion optimisation.

Note: This mode is still experimental.
SeeAlso: debugoff, debug
*/
void DebugOnFunction::execute(SLIInterpreter *) const
{
  // SLI-level debugger was never wired up in sli3; this op is a stub.
}

/*
BeginDocumentation
Name: debugoff - Stop SLI level debugging mode.
Description:
debugoff is used to quit the debugging mode at a specific position in the code.
Example:

In this example, the parameter assignments as well as the
calculation will be dine in debug mode.

/myproc
{
   << >> begin
   debugon % Enter debugging
   /a Set  % store first parameter
   /b Set  % store second parameter
   a b add a b mul add
   debugoff
   end
} def

Note: This mode is still experimental.
SeeAlso: debugon, debug
*/
void DebugOffFunction::execute(SLIInterpreter *i) const
{
    // i->debug_mode_off();
}

/*BeginDocumentation
Name: debug - execute an object in debug mode.
Synopsis: any debug -> -
Description: debug tries to execute the object by moving it to
the execution stack.
Examples: {1 2 add} debug -> 3
SeeAlso: exec, debugon, debugoff
*/
void DebugFunction::execute(SLIInterpreter *i) const
{
    // if(i->load()==0)
    // {
    //   i->raiseerror(i->StackUnderflowError);
    //   return;
    // }

    // i->EStack().pop();
    // i->EStack().push(new NameDatum("debugoff"));
    // i->EStack().push_move(i->top());
    // i->EStack().push(new NameDatum("debugon"));
//    i->pop();
}

void SetVerbosityFunction::execute(SLIInterpreter *i) const
{
  i->require_stack_load(1);
  i->require_stack_type(0,sli3::integertype);
  long count= i->top().data_.long_val;
  i->set_verbosity(count);
  i->pop();
}

/*BeginDocumentation
Name: verbosity - return the current verbosity level for interpreter messages
Synopsis: verbosity -> n
SeeAlso: setverbosity, message
*/

void VerbosityFunction::execute(SLIInterpreter *i) const
{
    // Axis I bundle step 4: dispatcher pre-popped /verbosity.
    i->push(i->verbosity());
}

void StartFunction::execute(SLIInterpreter *i) const
{
    i->EStack().dump(std::cerr);
    i->EStack().clear();
    i->message(sli3::M_ERROR, "Start", "Something went wrong "
      "during initialisation of NEST or one of its modules. Probably "
      "there is a bug in the startup scripts. Please send the output "
      "of NEST to bugs@nest-initiative.org or contact the NEST mailing "
      "list for help. You can try to find the bug by re-starting NEST "
      "with the option: --debug");
}

void MessageFunction::execute(SLIInterpreter *i) const
{
        // call : level (from) (message) message
    i->require_stack_load(3);
    i->require_stack_type(2,sli3::integertype);
    i->require_stack_type(1,sli3::stringtype);
    i->require_stack_type(0,sli3::stringtype);
    long lev= i->pick(2).data_.long_val;
    SLIString  *frm= i->pick(1).data_.string_val;
    assert(frm != NULL);
    SLIString  *msg= i->pick(0).data_.string_val;
    assert(msg != NULL);

    i->message(lev,frm->c_str(),msg->c_str());
    i->pop(3);
}

/*BeginDocumentation
Name: noop - no operation function
Description: This function does nothing. It is used for benchmark purposes.
*/

void NoopFunction::execute(SLIInterpreter *i) const
{
}


 SetGuardFunction setguardfunction;
 RemoveGuardFunction removeguardfunction;


 Backtrace_onFunction      backtrace_onfunction;
 Backtrace_offFunction    backtrace_offfunction;
 OStackdumpFunction        ostackdumpfunction;
 EStackdumpFunction        estackdumpfunction;
 LoopFunction             loopfunction;
 ExitFunction             exitfunction;
 IfFunction               iffunction;
 IfelseFunction           ifelsefunction;
 RepeatFunction           repeatfunction;
 CloseinputFunction       closeinputfunction;
 StoppedFunction          stoppedfunction;
 StopFunction             stopfunction;
 CurrentnameFunction      currentnamefunction;
 DefFunction              deffunction;
 SetFunction              setfunction;
 LoadFunction             loadfunction;
 LookupFunction           lookupfunction;

 ForFunction              forfunction;
 Forall_aFunction         forall_afunction;
 ForallFunction           forallfunction;
 ForallindexedFunction    forallindexedfunction;
 DefDispatchFunction      defdispatchfunction;
 Forallindexed_aFunction  forallindexed_afunction;
 Forallindexed_sFunction  forallindexed_sfunction;
 Forall_sFunction         forall_sfunction;
 RaiseerrorFunction       raiseerrorfunction;
 PrinterrorFunction       printerrorfunction;
 RaiseagainFunction       raiseagainfunction;

 CyclesFunction           cyclesfunction;
 ExecFunction             execfunction;
 BindFunction             bindfunction;
 TypeinfoFunction         typeinfofunction;
 SwitchFunction           switchfunction;
 SwitchdefaultFunction    switchdefaultfunction;
 CaseFunction             casefunction;
 CounttomarkFunction      counttomarkfunction;
 PclocksFunction          pclocksfunction;
 PclockspersecFunction    pclockspersecfunction;
 PgetrusageFunction       pgetrusagefunction;
 TimeFunction             timefunction;
 RealtimeFunction         realtimefunction;
 PtimesFunction           ptimesfunction;
 Sleep_dFunction          sleep_dfunction;
 Sleep_iFunction          sleep_ifunction;

 Token_sFunction          token_sfunction;
 Token_isFunction         token_isfunction;

 Symbol_sFunction         symbol_sfunction;

 SetVerbosityFunction setverbosityfunction;
 VerbosityFunction    verbosityfunction;
 MessageFunction      messagefunction;
 NoopFunction         noopfunction;
 StartFunction        startfunction;
 DebugOnFunction      debugonfunction;
 DebugOffFunction     debugofffunction;
 DebugFunction        debugfunction;

/*BeginDocumentation
Name: mark - puts a mark on the stack

Description: A mark is a token which is lying on the stack and
can be found by the user. The mark is used :
1) by using it as command mark
2) by using it as [ when creating an array
3) by using it as << when creating a dict

Examples: [ 1 2 add ] -> [3]
mark 1 2 add ] -> [3]

Author: docu by Marc Oliver Gewaltig and Sirko Straube
SeeAlso: counttomark, arraystore, switch, case
 */

void  init_slicontrol(SLIInterpreter *i)
{
  // Define the built-in symbols
    i->def(i->true_name,  i->new_token<sli3::booltype>(true));
    i->def(i->false_name, i->new_token<sli3::booltype>(false));
    i->def(i->mark_name,  i->new_token<sli3::marktype>());
    i->def(Name("<<"),     i->new_token<sli3::marktype>());
    i->def(Name("["),     i->new_token<sli3::marktype>());
    i->def(i->istopped_name, i->new_token<sli3::booltype>(false));

  i->def(i->newerror_name,    i->new_token<sli3::booltype>(false));
  i->def(i->recordstacks_name,  i->new_token<sli3::booltype>(false));

  i->createcommand("backtrace_on",&backtrace_onfunction);
  i->createcommand("backtrace_off",&backtrace_offfunction);
  i->createcommand("estackdump",  &estackdumpfunction);
  i->createcommand("ostackdump",  &ostackdumpfunction);
  i->createcommand("loop",  &loopfunction);
  i->createcommand("exit",  &exitfunction);
  i->createcommand("if",    &iffunction);
  i->createcommand("ifelse",&ifelsefunction);
  i->createcommand("repeat",&repeatfunction);
  i->createcommand("closeinput",&closeinputfunction);
  i->createcommand("stop",&stopfunction);
  i->createcommand("stopped",&stoppedfunction);
  i->createcommand("currentname",&currentnamefunction);
  i->createcommand("start",     &startfunction);
  // The raw "def_" leaf is also exposed under its compound
  // name so user code that bypasses /def keeps working.
  i->createcommand("def_",&deffunction);
  // /def itself binds to the compact dispatcher that picks
  // 2-arg vs 3-arg form. Stage 9 batch 7 replaces the trie
  // typeinit.sli formerly built at /def.
  i->createcommand("def", &defdispatchfunction);
  i->createcommand("call", &call_member_fn);
  i->createcommand("Set",&setfunction);
  i->createcommand("load",&loadfunction);
  i->createcommand("lookup",&lookupfunction);
  i->createcommand("for",&forfunction);
  i->createcommand("forall_a",&forall_afunction);
  // Stage 9: compact /forall dispatcher replaces the trie
  // formerly built in typeinit.sli.
  i->createcommand("forall",  &forallfunction);
  i->createcommand("forallindexed_a",&forallindexed_afunction);
  i->createcommand("forallindexed_s",&forallindexed_sfunction);
  // Stage 9: compact /forallindexed dispatcher replaces the
  // 2-arm trie typeinit.sli formerly built.
  i->createcommand("forallindexed", &forallindexedfunction);
  i->createcommand("forall_s",&forall_sfunction);
  i->createcommand("raiseerror",&raiseerrorfunction);
  i->createcommand("print_error",&printerrorfunction);
  i->createcommand("raiseagain",&raiseagainfunction);
  i->createcommand("cycles",&cyclesfunction);
  i->createcommand("exec",&execfunction);
  // Phase 4: native /bind. The SLI definition in sli-init.sli will
  // redefine /bind in userdict; we expose this C++ leaf as /bind_
  // and sli-init.sli aliases /bind to it (replacing the SLI loop).
  i->createcommand("bind_",&bindfunction);
  i->createcommand("typeinfo",&typeinfofunction);
  i->createcommand("switch",&switchfunction);
  i->createcommand("switchdefault",&switchdefaultfunction);
  i->createcommand("case",  &casefunction);
  i->createcommand("counttomark",  &counttomarkfunction);
  i->createcommand("pclocks",         &pclocksfunction);
  i->createcommand("pclockspersec",         &pclockspersecfunction);
  i->createcommand("pgetrusage",         &pgetrusagefunction);
  i->createcommand("time",         &timefunction);
  i->createcommand("realtime",        &realtimefunction);
  i->createcommand("ptimes",          &ptimesfunction);
  i->createcommand("sleep_d",         &sleep_dfunction);
  i->createcommand("sleep_i",         &sleep_ifunction);

  i->createcommand("token_s",     &token_sfunction);
  i->createcommand("token_is",     &token_isfunction);

  i->createcommand("symbol_s",     &symbol_sfunction);

  i->createcommand("setguard", &setguardfunction);
  i->createcommand("removeguard", &removeguardfunction);
  i->createcommand("setverbosity_i", &setverbosityfunction);
  i->createcommand("verbosity", &verbosityfunction);
  i->createcommand("message_", &messagefunction);
  i->createcommand("noop", &noopfunction);
  i->createcommand("debug", &debugfunction);
  i->createcommand("debugon", &debugonfunction);
  i->createcommand("debugoff", &debugofffunction);

    // Axis I bundle step 3e: convert the trailing-pop ops in
    // sli_control.cpp to new ABI. The remaining ops in this file
    // (iter setups: RepeatFunction, ForFunction, Forall_*Function;
    // conditional: IfFunction, IfelseFunction, CaseFunction has
    // mid-body slot manipulation; control flow: LoopFunction,
    // StoppedFunction, ExecFunction, ExitFunction, StopFunction;
    // raiseerror / raiseagain; iter dispatchers ForallFunction,
    // ForallindexedFunction, DefDispatchFunction) all manipulate
    // their own e-stack slot mid-body and stay old ABI.
    backtrace_onfunction.set_new_abi();
    backtrace_offfunction.set_new_abi();
    deffunction.set_new_abi();
    // Axis II step 2: hot-op tag for inline dispatcher path.
    deffunction.set_hot_op(HOP_DEF);
    setfunction.set_new_abi();
    loadfunction.set_new_abi();
    printerrorfunction.set_new_abi();
    casefunction.set_new_abi();
    sleep_ifunction.set_new_abi();
    sleep_dfunction.set_new_abi();
    setguardfunction.set_new_abi();
    removeguardfunction.set_new_abi();
    debugonfunction.set_new_abi();
    debugofffunction.set_new_abi();
    debugfunction.set_new_abi();
    setverbosityfunction.set_new_abi();
    messagefunction.set_new_abi();
    noopfunction.set_new_abi();
    // Axis I bundle step 4: conditionals converted by dropping
    // their mid-body pop-self (dispatcher now pre-pops).
    iffunction.set_new_abi();
    // Axis II step 2: hot-op tag for inline dispatcher path.
    iffunction.set_hot_op(HOP_IF);
    ifelsefunction.set_new_abi();
    loopfunction.set_new_abi();
    repeatfunction.set_new_abi();
    stoppedfunction.set_new_abi();
    forfunction.set_new_abi();
    // Phase 2a/2b: exit + stop + exec migrated to new-ABI. Exit/stop
    // scan the estack from pick(0); exec moves the ostack top onto
    // the estack via swap-out. All three rely on the dispatcher
    // having pre-popped the call slot.
    exitfunction.set_new_abi();
    stopfunction.set_new_abi();
    execfunction.set_new_abi();
    bindfunction.set_new_abi();
    // Phase 5: the last old-ABI ops in this file.
    closeinputfunction.set_new_abi();
    startfunction.set_new_abi();
    forall_afunction.set_new_abi();
    forall_sfunction.set_new_abi();
    forallindexed_afunction.set_new_abi();
    forallindexed_sfunction.set_new_abi();
    forallfunction.set_new_abi();
    forallindexedfunction.set_new_abi();
    defdispatchfunction.set_new_abi();
    // Axis I bundle step 4 (second batch): trailing-pop ops and
    // the switch/case family. SwitchFunction / SwitchdefaultFunction
    // also had their recover/restore dance removed (dispatcher
    // already popped /switch, so there is nothing to put back).
    switchfunction.set_new_abi();
    switchdefaultfunction.set_new_abi();
    casefunction.set_new_abi();
    counttomarkfunction.set_new_abi();
    pclocksfunction.set_new_abi();
    ptimesfunction.set_new_abi();
    typeinfofunction.set_new_abi();
    // Axis I bundle step 4 (third batch): one-pop-self at body
    // start / body end. Each had a single trailing-style pop on
    // the success path -- removed and marked new-ABI.
    ostackdumpfunction.set_new_abi();
    estackdumpfunction.set_new_abi();
    currentnamefunction.set_new_abi();
    lookupfunction.set_new_abi();
    cyclesfunction.set_new_abi();
    pclockspersecfunction.set_new_abi();
    pgetrusagefunction.set_new_abi();
    timefunction.set_new_abi();
    realtimefunction.set_new_abi();
    token_sfunction.set_new_abi();
    token_isfunction.set_new_abi();
    symbol_sfunction.set_new_abi();
    verbosityfunction.set_new_abi();
    raiseerrorfunction.set_new_abi();
    raiseagainfunction.set_new_abi();
    call_member_fn.set_new_abi();
}

}
