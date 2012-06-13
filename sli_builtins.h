/*
 *  slibuiltins.h
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

#ifndef SLI_BUILTINS_H
#define SLI_BUILTINS_H
/* 
    The interpreter's basic operators
*/
#include "sli_function.h"

namespace sli3
{
/*********************************************************
  This module contains only those functions which are
  needed by the intereter's default actions. All other
  built-in or user supplied functions must be defined
  either in builtins.{h,cc} or in user-defined modules
  *******************************************************/

  class IlookupFunction: public SLIFunction
  {
  public:
    IlookupFunction() {}
    void execute(SLIInterpreter *) const;
  };
  
  class IsetcallbackFunction: public SLIFunction
  {
  public:
    IsetcallbackFunction() {}
    void execute(SLIInterpreter *) const;
  };
  
  class IiterateFunction: public SLIFunction
  {
  public:
    IiterateFunction() {}
    void execute(SLIInterpreter *) const;
    void backtrace(SLIInterpreter *, int) const ;
  };
  
  class IloopFunction: public SLIFunction
  {
  public:
    IloopFunction() {}
    void execute(SLIInterpreter *) const;
    void backtrace(SLIInterpreter *, int) const ;
  };
  
  class IrepeatFunction: public SLIFunction
  {
  public:
    IrepeatFunction() {}
    void execute(SLIInterpreter *) const;
    void backtrace(SLIInterpreter *, int) const ;
    
  };
  
  class IforFunction: public SLIFunction
  {
  public:
    IforFunction() {}
    void execute(SLIInterpreter *) const;
    void backtrace(SLIInterpreter *, int) const ;
  };
  
  class IforallarrayFunction: public SLIFunction
  {
  public:
    IforallarrayFunction() {}
    void execute(SLIInterpreter *) const;
    void backtrace(SLIInterpreter *, int) const ;
  };
  
  class IforalliterFunction: public SLIFunction
  {
  public:
    IforalliterFunction() {}
    void execute(SLIInterpreter *) const;
    void backtrace(SLIInterpreter *, int) const ;
  };
  
  
  class IforallindexedarrayFunction: public SLIFunction
  {
  public:
    IforallindexedarrayFunction() {}
    void execute(SLIInterpreter *) const;
    void backtrace(SLIInterpreter *, int) const ;
  };
  
  class IforallindexedstringFunction: public SLIFunction
  {
  public:
    IforallindexedstringFunction() {}
    void execute(SLIInterpreter *) const;
    void backtrace(SLIInterpreter *, int) const ;
  };
  
  class IforallstringFunction: public SLIFunction
  {
  public:
    IforallstringFunction() {}
    void execute(SLIInterpreter *) const;
    void backtrace(SLIInterpreter *, int) const ;
  };
  
}
#endif
