/*
 *  slicontrol.h
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

#ifndef SLI_CONTROL_H
#define SLI_CONTROL_H
/*
    SLI's control structures
*/
#include "sli_interpreter.h"

/**************************************
  All SLI control functions are
  defined in this module
  *************************************/
namespace sli3
{
  void init_slicontrol(SLIInterpreter *);
  
  class Backtrace_onFunction: public SLIFunction
  {
  public:
    Backtrace_onFunction(){}
    void execute(SLIInterpreter *) const;
  };
  
  class Backtrace_offFunction: public SLIFunction
  {
  public:
    Backtrace_offFunction(){}
    void execute(SLIInterpreter *) const;
  };
  
  class OStackdumpFunction: public SLIFunction
  {
  public:
    OStackdumpFunction() {}
    void execute(SLIInterpreter *) const;
  };
  
  class EStackdumpFunction: public SLIFunction
  {
  public:
    EStackdumpFunction() {}
    void execute(SLIInterpreter *) const;
  };
  
  class LoopFunction: public SLIFunction
  {
  public:
    LoopFunction() {}
    void execute(SLIInterpreter *) const;
  };
  
  class ExitFunction: public SLIFunction
  {
  public:
    ExitFunction() {}
    void execute(SLIInterpreter *) const;
  };

  /**
   * return - Python-style early return from the enclosing
   * procedure. Walks the e-stack from the top, skipping any loop
   * frames (irepeat / ifor / iforall / iloop / ...), and unwinds
   * down to and including the first iiteratetype frame — i.e. the
   * nearest plain procedure invocation. Loop frames have their own
   * marker tags, so the walk passes over their body's pos/proc/
   * state slots naturally. If no iiteratetype is found on the
   * stack, raises InvalidReturn (return called outside a
   * procedure).
   */
  class ReturnFunction: public SLIFunction
  {
  public:
    ReturnFunction() {}
    void execute(SLIInterpreter *) const;
  };


  class IfFunction: public SLIFunction
  {
  public:
    IfFunction() {}
    void execute(SLIInterpreter *) const;
  };
  
  class IfelseFunction: public SLIFunction
  {
  public:
    IfelseFunction() {}
    void execute(SLIInterpreter *) const;
  };
  
  class RepeatFunction: public SLIFunction
  {
  public:
    RepeatFunction() {}
    void execute(SLIInterpreter *) const;
  };
  
  class CloseinputFunction: public SLIFunction
  {
  public:
    CloseinputFunction() {}
    void execute(SLIInterpreter *) const;
  };
  
  class StoppedFunction: public SLIFunction
  {
  public:
    StoppedFunction() {}
    void execute(SLIInterpreter *) const;
  };
  
  class StopFunction: public SLIFunction
  {
  public:
    StopFunction() {}
    void execute(SLIInterpreter *) const;
  };
  
  class CurrentnameFunction: public SLIFunction
  {
  public:
    CurrentnameFunction() {}
    void execute(SLIInterpreter *) const;
  };
  
  
  class StartFunction: public SLIFunction
  {
  public:
    StartFunction() {}
    void execute(SLIInterpreter *) const;
  };
    
  class DefFunction: public SLIFunction
  {
  public:
    DefFunction() {}
    void execute(SLIInterpreter *) const;
  };
  
  class SetFunction: public SLIFunction
  {
  public:
    SetFunction() {}
    void execute(SLIInterpreter *) const;
  };
  
  class LoadFunction: public SLIFunction
  {
  public:
    LoadFunction() {}
    void execute(SLIInterpreter *) const;
  };
  
  class LookupFunction: public SLIFunction
  {
  public:
    LookupFunction() {}
    void execute(SLIInterpreter *) const;
  };
  
  class ForFunction: public SLIFunction
  {
  public:
    ForFunction() {}
    void execute(SLIInterpreter *) const;
  };
  
  class Forall_aFunction: public SLIFunction
  {
  public:
    Forall_aFunction() {}
    void execute(SLIInterpreter *) const;
  };

  class Forallindexed_aFunction: public SLIFunction
  {
  public:
    Forallindexed_aFunction() {}
    void execute(SLIInterpreter *) const;
  };
  
  class Forallindexed_sFunction: public SLIFunction
  {
  public:
    Forallindexed_sFunction() {}
    void execute(SLIInterpreter *) const;
  };
  
  class Forall_sFunction: public SLIFunction
  {
  public:
    Forall_sFunction() {}
    void execute(SLIInterpreter *) const;
  };

  /**
   * Compact /forall dispatcher. Switches on the collection
   * type and invokes the appropriate iterator setup:
   *   array  / proc -> Forall_aFunction::execute  (C++ leaf)
   *   string / proc -> Forall_sFunction::execute  (C++ leaf)
   *   dict   / proc -> the SLI-defined `/forall_di` proc
   * The dict arm has to do a name lookup at runtime because
   * forall_di is itself a procedure built in typeinit.sli;
   * the baselookup cache makes that cheap after the first
   * call. Replaces the trie that sli-init.sli formerly built.
   */
  class ForallFunction: public SLIFunction
  {
  public:
    ForallFunction() {}
    void execute(SLIInterpreter *) const;
  };

  /**
   * Compact /forallindexed dispatcher. 2-arm trie collapsed:
   *   array  / proc -> Forallindexed_aFunction::execute
   *   string / proc -> Forallindexed_sFunction::execute
   *
   * Mirrors /forall's shape but without a dict arm, since
   * the original trie only had array and string variants.
   */
  class ForallindexedFunction: public SLIFunction
  {
  public:
    ForallindexedFunction() {}
    void execute(SLIInterpreter *) const;
  };

  /**
   * Compact /def dispatcher. Replaces the 4-arm trie
   * typeinit.sli used to build:
   *
   *   /lit obj           def  -> -    (DefFunction; 2 args)
   *   /lit proc          def  -> -    (DefFunction; 2 args, same C++ leaf)
   *   /lit [types] obj   def  -> -    (SLI :def_; 3 args, typecheck wrapper)
   *   /lit [types] proc  def  -> -    (SLI :def_; 3 args)
   *
   * Two outcomes:
   *   - 2-arg form -> dispatch to DefFunction (the C++ leaf).
   *     The trie used /def_ for [lit any] and /def for
   *     [lit proc] but both names are aliased to the same
   *     DefFunction body in typeinit.sli line 32.
   *   - 3-arg form (typecheck array in slot 1) -> look up
   *     :def_ (an SLI proc) and push it to the e-stack.
   */
  class DefDispatchFunction: public SLIFunction
  {
  public:
    DefDispatchFunction() {}
    void execute(SLIInterpreter *) const;
  };
  
  class RaiseerrorFunction: public SLIFunction
  {
  public:
    RaiseerrorFunction() {}
    void execute(SLIInterpreter *) const;
  };
  
  class PrinterrorFunction: public SLIFunction
  {
  public:
    PrinterrorFunction() {}
    void execute(SLIInterpreter *) const;
  };

  // TraceFunction lives in src/builtins/sli_trace.{h,cpp}.

  class RaiseagainFunction: public SLIFunction
  {
  public:
    RaiseagainFunction() {}
    void execute(SLIInterpreter *) const;
  };
  
  class ExecFunction: public SLIFunction
  {
  public:
    ExecFunction() {}
    void execute(SLIInterpreter *) const;
  };

  // Phase 4: deep `/bind` that walks a procedure body and replaces
  // every nametype token whose dstack lookup resolves to a function
  // or trie with the resolved Token directly. Names that resolve to
  // anything else (or are undefined) are left as-is, mirroring
  // PostScript bind semantics. Nested procedures (litproctype /
  // proctype) are recursed into in place. Re-binding is a no-op
  // (idempotent) -- the SLI-level bind in sli-init.sli has the same
  // shape but is interpreted and ~50x slower per pass.
  class BindFunction: public SLIFunction
  {
  public:
    BindFunction() {}
    void execute(SLIInterpreter *) const;
  };
  
  
  class TypeinfoFunction: public SLIFunction
  {
  public:
    TypeinfoFunction() {}
    void execute(SLIInterpreter *) const;
  };
  
  /* BeginDocumentation
     Name: switch - finish a case ... switch structure
     Synopsis: mark proc1...procn switch-> -
     Description:
     switch executes proc1 ... procn and removes the mark. If any executed
     proc containes an exit command, switch will remove the other procs without
     execution. switch is used together with case.
     Parameters:
     proc1...procn: executable procedure tokens.
     Examples:
     
     mark
     false {1 == exit} case
     false {2 == exit} case
     true  {3 == exit} case
     false {4 == exit} case
     switch
     --> 3
     
     mark {1 ==} {2 ==} switch -->  1 2
     
     Author: Gewaltig
     FirstVersion: ??
     
     SeeAlso: case, switchdefault, exit, mark
  */
  
  
  class SwitchFunction: public SLIFunction
  {
  public:
    SwitchFunction() {}
    void execute(SLIInterpreter *) const;
  };
  
  /* BeginDocumentation
     Name: switchdefault - finish a case ... switchdefault structure
     Synopsis: mark proc1...procn procdefault switchdefault -> -
     Description:
     Like switch, switchdefault executes any of proc1...procn.
     If an execution it meets an exit command, no further procs are executed.
     If n=0, e.g. no procedure other than procdefault is found, procdefault
     will be executed. Thus, procdefault will be skipped if any other proc
     exists.
     Parameters:
     proc1...procn: executable procedure tokens.
     procdefault  : execulable procedure called if no other proc is present.
     Examples:
     
     mark
     false {1 == exit} case
     false {2 == exit} case
     true  {3 == exit} case
     false {4 == exit} case
     {(default) ==}
     switchdefault
     -->  3
     
     mark
     false {1 == exit} case
     false {2 == exit} case
     false {3 == exit} case
     false {4 == exit} case
     {(default) ==}
     switchdefault
     --> default
     
     Author: Hehl
     FirstVersion: April 16, 1999
     
     SeeAlso: case, switch, exit, mark
  */
  
  class SwitchdefaultFunction: public SLIFunction
  {
  public:
    SwitchdefaultFunction() {}
    void execute(SLIInterpreter *) const;
  };
  
  /* BeginDocumentation
     Name: case - like if, but test a series of conditions.
     Synopsis: bool proc case --> proc
     -
     Description:
     case tests the bool and pushes proc if true, else does nothing.
     Parameters:
     bool : condition for case to test
     proc : procedure to be executed if case is true
     Examples:
     
     true {(hello) ==} case --> hello
     false {(hello) ==} case --> -
     
     1 0 gt {(1 bigger than 0) ==} case --> 1 bigger than 0
     1 0 lt {(0 bigger than 1) ==} case --> -
     
     mark
     false {1 == exit} case
     false {2 == exit} case
     true  {3 == exit} case
     false {4 == exit} case
     switch
     --> 3
     
     Author: Gewaltig
     FirstVersion: ??
     Remarks: Use exit to make sure that switch is exited.
     SeeAlso: switch, switchdefault, exit, mark, if
  */
  
  class CaseFunction: public SLIFunction
  {
  public:
    CaseFunction() {}
    void execute(SLIInterpreter *) const;
  };
  
  class CounttomarkFunction: public SLIFunction
  {
  public:
    CounttomarkFunction() {}
    void execute(SLIInterpreter *) const;
  };
  
  class PclocksFunction: public SLIFunction
  {
  public:
    PclocksFunction() {}
    void execute(SLIInterpreter *) const;
  };
  
  class PclockspersecFunction: public SLIFunction
  {
  public:
    PclockspersecFunction() {}
    void execute(SLIInterpreter *) const;
  };
  
  class PgetrusageFunction: public SLIFunction
  {
  public:
    PgetrusageFunction() {}
    void execute(SLIInterpreter *) const;
    
  private:
    bool getinfo_(SLIInterpreter *, int, Dictionary*) const;
  };
  
  class TimeFunction: public SLIFunction
  {
  public:
    TimeFunction() {}
    void execute(SLIInterpreter *) const;
  };

  /**
   * realtime - high-resolution wall-clock time as a double, in
   * seconds since the epoch. tic/toc/clock in sli-init.sli are
   * defined on top of this; without it, every elapsed-time helper
   * raises UndefinedName at runtime.
   */
  class RealtimeFunction: public SLIFunction
  {
  public:
    RealtimeFunction() {}
    void execute(SLIInterpreter *) const;
  };

  /**
   * ptimes -> [rtime utime stime cutime cstime]   (all doubletype, seconds)
   *
   * sli-init.sli's /ptimes is `pclocks pclockspersec cvd div` --
   * but the div trie does not have an [arraytype, doubletype]
   * overload, so the SLI version raises UndefinedName at runtime.
   * Registering a C++ leaf bypasses the broken chain and lets
   * /realtime / /usertime / /systemtime / /tic / /toc all work.
   */
  class PtimesFunction: public SLIFunction
  {
  public:
    PtimesFunction() {}
    void execute(SLIInterpreter *) const;
  };
  
  class Sleep_iFunction: public SLIFunction
  {
  public:
    Sleep_iFunction() {}
    void execute(SLIInterpreter *) const;
  };  
  
  class Sleep_dFunction: public SLIFunction
  {
  public:
    Sleep_dFunction() {}
    void execute(SLIInterpreter *) const;
  };
  
  class Token_sFunction: public SLIFunction
  {
  public:
    Token_sFunction() {}
    void execute(SLIInterpreter *) const;
  };
  
  class Token_isFunction: public SLIFunction
  {
  public:
    Token_isFunction() {}
    void execute(SLIInterpreter *) const;
  };
  
  class Symbol_sFunction: public SLIFunction
  {
  public:
    Symbol_sFunction() {}
    void execute(SLIInterpreter *) const;
  };
  
  class MessageFunction: public SLIFunction
  {
  public:
    MessageFunction() {}
    void execute(SLIInterpreter *) const;
  };
  
  class SetVerbosityFunction: public SLIFunction
  {
  public:
    SetVerbosityFunction() {}
    void execute(SLIInterpreter *) const;
  };
  
  class VerbosityFunction: public SLIFunction
  {
  public:
    VerbosityFunction() {}
    void execute(SLIInterpreter *) const;
  };
  
  
  class NoopFunction: public SLIFunction
  {
  public:
    NoopFunction() {}
    void execute(SLIInterpreter *) const;
  };
  
  class DebugOnFunction: public SLIFunction
  {
  public:
    DebugOnFunction() {}
    void execute(SLIInterpreter *) const;
  };
  
  class DebugOffFunction: public SLIFunction
  {
  public:
    DebugOffFunction() {}
    void execute(SLIInterpreter *) const;
  };
  
  class DebugFunction: public SLIFunction
  {
  public:
    DebugFunction() {}
    void execute(SLIInterpreter *) const;
  };
  
}
#endif
