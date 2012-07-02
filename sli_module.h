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
     */
    virtual const std::string commandstring(void) const;
    
    /**
     * Print installation message via interpreter message command.
     */
    void install(std::ostream &, SLIInterpreter *);
  };
}
#endif
