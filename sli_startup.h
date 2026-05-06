// Slice 5 rewrite of the legacy slistartup.{h,cc}.
//
// Locates `sli-init.sli`, populates statusdict with a minimal set of
// runtime parameters, defines cin/cout/cerr in the system dictionary,
// pushes the startup file as an XIstream + iparse onto the execution
// stack, and registers a small bootstrap surface (getenv, evalstring).
//
// Single entry point: init_slistartup(SLIInterpreter*, int argc, char** argv).
// Called from SLIInterpreter::init_internal_functions so the interpreter
// is ready to bootstrap itself once execute() is invoked.

#ifndef SLI_STARTUP_H
#define SLI_STARTUP_H

namespace sli3
{
class SLIInterpreter;

// `argc`/`argv` are forwarded to statusdict::argv. Pass {0, nullptr}
// when no command-line is available (e.g. unit tests).
void init_slistartup(SLIInterpreter*, int argc, char** argv);

}  // namespace sli3

#endif
