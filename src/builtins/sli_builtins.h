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
  // Phase 5 cleanup: all 11 legacy iter-helper SLIFunction classes
  // have been replaced by TYPE markers. The dispatcher's body_walk
  // handles iiterate / irepeat / ifor / iforall / iloop /
  // iforallstring / iforallindexedarray / iforallindexedstring
  // inline at body_exhausted; iparse has its own outer-switch case.
  // See sli_interpreter.cpp:execute_dispatch_.

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
