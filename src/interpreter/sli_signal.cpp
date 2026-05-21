#include "sli_signal.h"
#include "sli_interpreter.h"

#include <csignal>
#include <cstring>

namespace sli3 {

extern "C" {
static void sli_signal_handler(int sig) {
  if (signalflag == 0)
    signalflag = sig;
}
}

namespace {

// POSIX.1 conforming signal-install. Equivalent to NEST's posix_signal.
// SA_RESTART so non-fatal calls (read/write/wait) auto-restart on EINTR
// instead of returning failure mid-script.
using Sigfunc = void(int);
Sigfunc* posix_signal(int signo, Sigfunc* func) {
  struct sigaction act;
  struct sigaction oact;
  std::memset(&act, 0, sizeof(act));
  act.sa_handler = func;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;
#ifdef SA_RESTART
  act.sa_flags |= SA_RESTART;
#endif
  if (sigaction(signo, &act, &oact) < 0)
    return SIG_ERR;
  return oact.sa_handler;
}

} // namespace

void install_signal_handlers() {
  // Skip any signal whose disposition is already SIG_IGN — that's
  // the convention for background processes ("we explicitly do not
  // want this signal observed"). For the others, install our handler.
  for (int sig : {SIGINT, SIGUSR1, SIGUSR2}) {
    if (posix_signal(sig, SIG_IGN) != SIG_IGN)
      posix_signal(sig, &sli_signal_handler);
  }
}

} // namespace sli3
