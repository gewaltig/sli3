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
  // Phase 5 cleanup: IparsestdinFunction, IlookupFunction,
  // IiterateFunction, IrepeatFunction, IforFunction, and
  // IforallarrayFunction were dead in dispatch mode (the
  // dispatcher handles proceduretype, ifortype, iforalltype
  // inline via body_walk; ::lookup, ::pop, ::parsestdin were
  // createcommand'd but no caller baselookup'd them). All six
  // have been deleted -- see sli_builtins.cpp for the audit.

  class IparseFunction: public SLIFunction
  {
  public:
    IparseFunction() {}
    void execute(SLIInterpreter *) const;
  };

  class IloopFunction: public SLIFunction
  {
  public:
    IloopFunction() {}
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
 

  class ArraycreateFunction: public SLIFunction
  {
    public:
    void execute(SLIInterpreter *) const;
  };

  class DictconstructFunction: public SLIFunction
  {
    public:
    void execute(SLIInterpreter *) const;
  };

}
#endif
