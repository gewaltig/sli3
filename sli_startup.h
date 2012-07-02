/*
 *  slistartup.h
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

#ifndef SLI_STARTUP_H
#define SLI_STARTUP_H

#include "sli_config.h"
#include "sli_module.h" 
#include "sli_token.h"
#include "sli_function.h"
#include "sli_name.h"
#include "compose.hpp"

#include "dirent.h"
#include "errno.h"
#include <string>

class SLIStartup: public SLIModule
{
  const std::string startupfilename;
  const std::string slilibpath;
  std::string slihomepath;
  std::string slidocdir;

  std::string locateSLIInstallationPath(void);
  bool checkpath(std::string const &, std::string &) const;
  std::string getenv(const std::string &) const;
  std::string checkenvpath(std::string const &, SLIInterpreter *, std::string) const;

  Token targs;
  int verbosity_;
  bool debug_;
  public:

  Name argv_name;
  Name prgname_name;
  Name exitcode_name;
  Name prgmajor_name;
  Name prgminor_name;
  Name prgpatch_name;
  Name prgbuilt_name;
  Name prefix_name;
  Name prgsourcedir_name;
  Name prgbuilddir_name;
  Name prgdatadir_name;
  Name prgdocdir_name;

  Name host_name;
  Name hostos_name;
  Name hostvendor_name;
  Name hostcpu_name;
  
  Name getenv_name;
  Name statusdict_name;
  Name start_name;

  Name intsize_name;
  Name longsize_name;
  Name havelonglong_name;
  Name longlongsize_name;
  Name doublesize_name;
  Name pointersize_name;
  Name architecturedict_name;

  Name have_mpi_name;
  Name ismpi_name;
  Name have_gsl_name;
  Name have_pthreads_name;
  Name havemusic_name;
  Name ndebug_name;

  Name exitcodes_name;
  Name exitcode_success_name;
  Name exitcode_scripterror_name;
  Name exitcode_abort_name;
  Name exitcode_segfault_name;
  Name exitcode_exception_name;
  Name exitcode_fatal_name;
  Name exitcode_unknownerror_name;
  
  class GetenvFunction: public SLIFunction
  {
    public:
    void execute(SLIInterpreter *) const;
  };

  GetenvFunction getenvfunction;

  SLIStartup(int, char**);
  ~SLIStartup(){}

  void init(SLIInterpreter *);

  const std::string name(void) const
    {
      return "SLIStartup";
    }
};

#endif
