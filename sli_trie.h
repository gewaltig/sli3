/*
 *  slitypecheck.h
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

#ifndef SLITYPECHECK_H
#define SLITYPECHECK_H
#include <typeinfo>
#include "slifunction.h"

class TrieFunction: public SLIFunction
{
public:
  TrieFunction() {}
    void execute(SLIInterpreter *) const;
};

class TrieInfoFunction: public SLIFunction
{
public:
  TrieInfoFunction() {}
  void execute(SLIInterpreter *) const;
};

class AddtotrieFunction: public SLIFunction
{
public:
AddtotrieFunction() {}
    void execute(SLIInterpreter *) const;
};

class Cva_tFunction: public SLIFunction
{
public:
Cva_tFunction() {}
    void execute(SLIInterpreter *) const;
};

class Cvt_aFunction: public SLIFunction
{
public:
Cvt_aFunction() {}
    void execute(SLIInterpreter *) const;
};

class TypeFunction: public SLIFunction
{
public:
TypeFunction() {}
    void execute(SLIInterpreter *) const;
};


void init_slitypecheck(SLIInterpreter *);

#endif
