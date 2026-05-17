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

  // Hot-op inlining.
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
      HOP_GT,       // step 3: poly-gt dispatcher (numeric + string)
      HOP_LT,       // step 3: poly-lt dispatcher
      HOP_GEQ,      // step 3: poly-geq dispatcher
      HOP_LEQ,      // step 3: poly-leq dispatcher
      HOP_EQ,       // step 3: any/any equality via Token::operator==
      HOP_NEQ,      // step 3: any/any inequality
      HOP_GET,      // step 3: container key -> elem (multi-arm)
      HOP_PUT,      // step 3: container key value -> [container]
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

      // Axis II: hot-op tag. Subclasses set this in their
      // constructor (see Pop/Dup/Exch/Add_ii). HOP_NONE keeps
      // the virtual-call path. The setter is intentionally
      // public so the existing leaf SLIFunction subclasses can
      // self-register without needing a templated ctor argument.
      HotOpId hot_op() const { return hot_op_id_; }
      void set_hot_op(HotOpId id) { hot_op_id_ = id; }

  private:
      Name name_;
      HotOpId hot_op_id_ = HOP_NONE;
  };
}
#endif

