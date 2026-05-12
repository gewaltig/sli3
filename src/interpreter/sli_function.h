/*
 *  slifunction.h
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

#ifndef SLI_FUNCTION_H
#define SLI_FUNCTION_H
/* 
    Base class for all SLI functions.
*/

#include "sli_name.h"

#include <cstdint>

namespace sli3
{
  class SLIInterpreter;

  // Axis II: hot-op inlining (doc/compact_procedure_spec.md §Axis II).
  //
  // The dispatcher's `functiontype` path switches on
  // SLIFunction::hot_op() before falling back to the virtual
  // fn->execute(this). For hot ops the body is inlined into the
  // dispatcher's switch -- one vcall + per-call refcount-on-the-
  // type elided per dispatch, body folds into the body-walk
  // fast path so the compiler can keep operand-stack pointers in
  // registers across the call.
  //
  // Default is HOP_NONE; subclasses opt in by setting hot_op_id_
  // in their constructor. HOP_NONE keeps the virtual-call path.
  //
  // Enum is append-only inside the value range; values are
  // not a wire contract (no serialization depends on them), so
  // re-ordering is safe -- but additions should stay before
  // HOP_count to keep the enumerator dense for the dispatcher's
  // jump table.
  enum HotOpId : uint8_t {
      HOP_NONE = 0,
      HOP_POP,
      HOP_DUP,
      HOP_EXCH,
      HOP_ADD_II,
      HOP_ADD,      // step 2: poly-add dispatcher (int+int, dd, di, id)
      HOP_SUB,      // step 2: poly-sub dispatcher
      HOP_IF,       // step 2: bool proc -> push proc if true
      HOP_DEF,      // step 2: /lit obj -> def in current dict
      HOP_count
  };

  class SLIFunction
  {
  public:
      SLIFunction(){}
      SLIFunction(Name n):name_(n){}
      virtual void execute(SLIInterpreter *) const = 0;
      virtual ~SLIFunction(){}
      
      /**
       * Show stack backtrace information on error.
       * This function tries to extract and display useful
       * information from the execution stack if an error occurs.
       * This function should be implemented for all functions which
       * store administrative information on the execution stack.
       * Examples are: loops and procedure iterations.
       * backtrace() is only called, if the interpreter flag
       * show_backtrace is set.
       */
      virtual void backtrace(SLIInterpreter *, int) const
	  {}
      
      Name get_name() const
	  {
	      return name_;
	  }
      
      void set_name(Name n)
	  {
	      name_=n;
	  }

      // Axis I bundle step 3: when true, the dispatcher pops this
      // operator's e-stack slot itself after fn->execute returns
      // (in the main functiontype case) or skips the sentinel push
      // entirely (in iter cases). When false, the operator manages
      // its own slot -- either via a trailing i->EStack().pop()
      // (most ops, pre-conversion) or by special manipulation
      // (Repeat / For / Loop / Iparse and other control-flow
      // setups, which push iter frames or stay-in-place to
      // re-iterate).
      //
      // Default is false (today's ABI). Per-file step-3 commits
      // call set_new_abi() in the file's init function for each
      // converted operator (whose trailing i->EStack().pop() in
      // execute() has been removed). Step 4 of the bundle flips
      // the default to true and drops the flag once the
      // dispatcher's old-ABI path can be deleted.
      bool uses_new_abi() const { return new_abi_; }
      void set_new_abi() { new_abi_ = true; }

      // Axis II: hot-op tag. Subclasses set this in their
      // constructor (see Pop/Dup/Exch/Add_ii). HOP_NONE keeps
      // the virtual-call path. The setter is intentionally
      // public so the existing leaf SLIFunction subclasses can
      // self-register without needing a templated ctor argument.
      HotOpId hot_op() const { return hot_op_id_; }
      void set_hot_op(HotOpId id) { hot_op_id_ = id; }

  private:
      Name name_;
      bool new_abi_ = false;
      HotOpId hot_op_id_ = HOP_NONE;
  };
}
#endif

