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

  private:
    Name name_;
  };
}
#endif

