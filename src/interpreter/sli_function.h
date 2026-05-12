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

namespace sli3
{
  class SLIInterpreter;

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
      // remove the trailing pop from each execute() AND call
      // set_new_abi() in the file's init function, gradually
      // flipping operators to the new ABI. Step 4 of the bundle
      // removes the flag once all converted.
      bool uses_new_abi() const { return new_abi_; }
      void set_new_abi() { new_abi_ = true; }

  private:
      Name name_;
      bool new_abi_ = false;
  };
}
#endif

