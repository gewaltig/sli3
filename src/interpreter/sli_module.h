/*
 *  slimodule.h
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

#ifndef SLI_MODULE_H
#define SLI_MODULE_H
#include <iostream>
#include <string>

namespace sli3
{
  class SLIInterpreter;
  class Dictionary;

  /**
   * Base class for all SLI Interpreter modules.
   */
  class SLIModule
  {
  public:
    virtual ~SLIModule(){}
    
    /**
     * Initialise the module.
     * When this function is called, most of the
     * interpreter's fascilities are up and running.
     * However, depending on where in the interpreter's 
     * bootstrap sequence the module is initialised, not 
     * all services may be available.
     */
    virtual void init(SLIInterpreter *) =0;
    
    /**
     * Return name of the module.
     */
    virtual const std::string name(void) const=0;
    
    /**
     * Return sli command sequence to be executed for initialisation.
     * Inlined so the class has no out-of-line virtual definition: the
     * C++ ABI's "key function" rule would otherwise emit typeinfo only
     * in the TU defining this method, and the corresponding .cpp lives
     * under unported/ (no concrete SLIModule subclass exists in sli3).
     * Linux UBSan emits dynamic-type checks that reference SLIModule's
     * typeinfo from sli_interpreter.cpp; with no key function the
     * typeinfo gets vague linkage and the link succeeds.
     */
    virtual const std::string commandstring(void) const { return std::string(); }

    /**
     * Print installation message via interpreter message command.
     * Declared but never defined — no caller exists in sli3 (the only
     * use site is the SLIInterpreter::addmodule<T> template, which is
     * never instantiated).
     */
    void install(std::ostream &, SLIInterpreter *);
  };
}
#endif
