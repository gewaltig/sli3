/*
 *  slimodule.cc
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

#include "sli_module.h"
#include "sli_interpreter.h"

void sli3::SLIModule::install(std::ostream &, SLIInterpreter *i)
{
  // Output stream is now set by the message level.
  // i->message(out, 5, name().c_str(), "Initializing.");
    i->message(sli3::M_INFO, name().c_str(), "Initializing.");
    init(i);
}

const sli3::std::string SLIModule::commandstring(void) const
{
    return std::string();
}

