/*
 *  slistack.h
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

#ifndef SLI_STACK_H
#define SLI_STACK_H
/* 
    Stack manipulation functions
*/
#include "sli_function.h"

namespace sli3
{
  class SLIInterpreter;


  void  init_slistack(SLIInterpreter *i);

    /************************************************
      Stack manipulation functions
      ********************************************/
  class PopFunction: public SLIFunction
  {
  public:
    PopFunction() {}
    void execute(SLIInterpreter *) const;
  };
  
  class NpopFunction: public SLIFunction
  {
  public:
    NpopFunction() {}
    void execute(SLIInterpreter *) const;
  };
  
  class DupFunction: public SLIFunction
  {
  public:
    DupFunction() {}
    void execute(SLIInterpreter *) const;
  };
  
  class ExchFunction: public SLIFunction
  {
  public:
    ExchFunction() {}
    void execute(SLIInterpreter *) const;
  };
  
  class IndexFunction: public SLIFunction
  {
  public:
    IndexFunction() {}
    void execute(SLIInterpreter *) const;
  };
  
  class RollFunction: public SLIFunction
  {
  public:
    RollFunction() {}
    void execute(SLIInterpreter *) const;
  };
  
  class RolluFunction: public SLIFunction
  {
  public:
    RolluFunction() {}
    void execute(SLIInterpreter *) const;
  };
  
  class RolldFunction: public SLIFunction
  {
  public:
    RolldFunction() {}
    void execute(SLIInterpreter *) const;
  };
  
  class RotFunction: public SLIFunction
  {
  public:
    RotFunction() {}
    void execute(SLIInterpreter *) const;
  };
  
  class OverFunction: public SLIFunction
  {
  public:
    OverFunction() {}
    void execute(SLIInterpreter *) const;
  };
  
  class CountFunction: public SLIFunction
  {
  public:
    CountFunction() {}
    void execute(SLIInterpreter *) const;
  };
  
  class CopyFunction: public SLIFunction
  {
  public:
    CopyFunction() {}
    void execute(SLIInterpreter *) const;
  };
  
  class ClearFunction: public SLIFunction
  {
  public:
    ClearFunction() {}
    void execute(SLIInterpreter *) const;
  };
  
  class ExecstackFunction: public SLIFunction
  {
  public:
    ExecstackFunction() {}
    void execute(SLIInterpreter *) const;
  };
  
  class RestoreestackFunction: public SLIFunction
  {
  public:
    RestoreestackFunction() {}
    void execute(SLIInterpreter *) const;
  };
  
  class RestoreostackFunction: public SLIFunction
  {
  public:
    RestoreostackFunction() {}
    void execute(SLIInterpreter *) const;
  };
  
  class OperandstackFunction: public SLIFunction
  {
  public:
    OperandstackFunction() {}
    void execute(SLIInterpreter *) const;
  };

}
#endif
  
