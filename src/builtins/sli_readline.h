// linenoise-based line editing for the SLI REPL.
//
// Registers GNUreadline and GNUaddhistory. sli-init.sli's /executive
// gates on `systemdict /GNUreadline known` — if these names exist by
// the time sli-init.sli loads, the script automatically picks the
// editing-aware variant (see sli-init.sli around line 1647). So
// init_sli_readline must run BEFORE init_slistartup.

#ifndef SLI_READLINE_H
#define SLI_READLINE_H

namespace sli3
{
class SLIInterpreter;
void init_sli_readline(SLIInterpreter *);
}

#endif
