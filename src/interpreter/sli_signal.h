#ifndef SLI3_SIGNAL_H
#define SLI3_SIGNAL_H

// POSIX-conforming signal handler for the SLI interpreter.
//
// On SIGINT / SIGUSR1 / SIGUSR2 the handler sets sli3::signalflag to
// the signal number. The dispatcher checks this flag each cycle and
// turns it into a SystemSignal raiseerror, so that surrounding
// `stopped { handleerror } if` brackets can recover gracefully
// instead of the process dying mid-computation.
//
// Mirrors NEST 2.20.2 sli/psignal.{h,c}.

namespace sli3 {

// Install handlers for SIGINT / SIGUSR1 / SIGUSR2. Signals whose
// disposition is already SIG_IGN (the typical case when sli3 runs as
// a background process) are left untouched. Safe to call once during
// SLIInterpreter construction.
void install_signal_handlers();

} // namespace sli3

#endif // SLI3_SIGNAL_H
