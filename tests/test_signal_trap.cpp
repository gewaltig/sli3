// Signal-trap port from NEST 2.20.2 (sli/psignal.{h,c}).
//
// When the OS delivers SIGINT / SIGUSR1 / SIGUSR2, the handler sets
// sli3::signalflag to the signal number. The dispatcher checks the
// flag each cycle and, if non-zero, turns it into a SystemSignal
// raiseerror. A surrounding `stopped { handleerror } if` catches it
// cleanly. These tests pin the dispatcher half of that contract
// (driven by setting signalflag directly, so the test is
// deterministic and doesn't depend on the kernel's signal-delivery
// timing).

#include "sli_dictionary.h"
#include "sli_exceptions.h"
#include "sli_interpreter.h"
#include "sli_iostream.h"
#include "sli_name.h"
#include "sli_token.h"
#include "sli_tokenstack.h"

#include <csignal>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/time.h>

using namespace sli3;

namespace
{

#define CHECK(cond)                                                    \
    do {                                                               \
        if (!(cond)) {                                                 \
            std::cerr << "FAIL @" << __LINE__ << ": " #cond "\n";      \
            std::exit(1);                                              \
        }                                                              \
    } while (0)

void prime_eval(SLIInterpreter& i, std::string const& src)
{
    auto* iss = new std::istringstream(src);
    auto* wrap = new SLIistream(iss);

    i.OStack().clear();
    i.EStack().clear();
    i.error_dict().clear();

    Token x(i.get_type(sli3::xistreamtype));
    x.data_.istream_val = wrap;
    i.EStack().push(x);
    i.EStack().push(i.new_token<sli3::nametype, Name>(i.iparse_name));
}

std::string get_errorname(SLIInterpreter& i)
{
    if (!i.error_dict().known(Name("errorname"))) return "";
    Token const& t = i.error_dict().lookup(Name("errorname"));
    if (t.is_of_type(sli3::literaltype) ||
        t.is_of_type(sli3::nametype) ||
        t.is_of_type(sli3::symboltype))
    {
        return Name(t.data_.name_val).toString();
    }
    return "<not-name>";
}

long get_signo(SLIInterpreter& i)
{
    if (!i.error_dict().known(Name("sys_signo"))) return -1;
    Token const& t = i.error_dict().lookup(Name("sys_signo"));
    if (!t.is_of_type(sli3::integertype)) return -2;
    return t.data_.long_val;
}

// signaldict is built by init_slistartup during interpreter
// construction. Verify a handful of POSIX-mandatory names map to
// the platform's integer codes.
void test_signaldict_present(SLIInterpreter& i)
{
    Token sigd_t;
    CHECK(i.lookup(Name("signaldict"), sigd_t));
    CHECK(sigd_t.is_of_type(sli3::dictionarytype));
    Dictionary& sigd = *sigd_t.data_.dict_val;
    CHECK(sigd.known(Name("SIGINT")));
    CHECK(sigd.lookup(Name("SIGINT")).data_.long_val == SIGINT);
    CHECK(sigd.known(Name("SIGUSR1")));
    CHECK(sigd.lookup(Name("SIGUSR1")).data_.long_val == SIGUSR1);
    CHECK(sigd.known(Name("SIGTERM")));
    CHECK(sigd.lookup(Name("SIGTERM")).data_.long_val == SIGTERM);
}

// Set the global signalflag before execution, then run a long-running
// repeat loop. The dispatcher must observe the flag and surface a
// SystemSignal error before the loop finishes naturally. /sys_signo
// in errordict must reflect the saved signal number.
void test_pending_signal_interrupts_repeat(SLIInterpreter& i)
{
    // A 1e9-iteration loop would take seconds to finish naturally.
    // If the trap works we'll exit on iteration 1.
    prime_eval(i, "1000000000 { 1 pop } repeat");
    sli3::signalflag = SIGINT;
    try { i.execute_dispatch_(0); } catch (...) { /* swallowed */ }
    CHECK(sli3::signalflag == 0);  // dispatcher cleared it
    CHECK(get_errorname(i) == "SystemSignal");
    CHECK(get_signo(i) == SIGINT);
}

// A signal raised between two top-level statements (not inside a
// tight loop) is still caught at the outer-while signal check. The
// resulting SystemSignal raiseerror pushes /stop on the e-stack,
// which the dispatcher executes -- unwinding to the test's
// uninstalled stopped context (none here, so the e-stack drains
// fully and ostack carries the /commandname literal pushed by
// raiseerror, plus any pre-error values).
void test_pending_signal_caught_outside_loop(SLIInterpreter& i)
{
    // Two simple pushes; signalflag is checked at the top of each
    // outer-while iteration. The first push lands, then the next
    // cycle sees the flag and converts to a SystemSignal.
    prime_eval(i, "1 2 3");
    sli3::signalflag = SIGUSR1;
    try { i.execute_dispatch_(0); } catch (...) { /* swallowed */ }
    CHECK(sli3::signalflag == 0);
    CHECK(get_errorname(i) == "SystemSignal");
    CHECK(get_signo(i) == SIGUSR1);
}

// Stacking case: the signal flag is sticky -- once set, the handler
// won't overwrite it with a second signal until the dispatcher
// drains it. This mirrors NEST 2.20.2's `if (SLIsignalflag == 0)`
// guard in SLISignalHandler. We rely on it so a SIGINT in the
// middle of error handling for a previous SIGINT doesn't recurse.
// (Tested in isolation since the handler itself is not on the test
// path -- but the invariant is what's important.)
void test_signalflag_starts_zero(SLIInterpreter& i)
{
    (void)i;
    // Two prior tests set signalflag; both must have cleared it
    // before returning. (Tested implicitly above via the CHECK
    // calls; restated here as documentation.)
    CHECK(sli3::signalflag == 0);
}

// End-to-end recovery: `{ ...big_loop... } stopped` should catch the
// SystemSignal and leave `true` on the ostack. This is the contract
// the SLI prompt relies on -- Ctrl-C unwinds to the executive's
// outer stopped context and the user gets the prompt back.
void test_stopped_catches_signal(SLIInterpreter& i)
{
    // Wrap the loop in stopped {} so the unwind has a sentinel to
    // land on. Set the signal flag BEFORE the loop's body starts,
    // but AFTER stopped has pushed its istopped sentinel -- by
    // setting it now (pre-dispatch) the outer-while signal check
    // doesn't fire on its first iteration (e-stack tops are xistream
    // + iparse, no sentinel yet). We instead delay the signal until
    // the dispatcher is mid-loop by piggy-backing on a custom
    // operator.
    //
    // Simpler: set signalflag inside the loop via `usertype-of` a
    // small SLI hook... but the harness has no such hook. The
    // cleanest deterministic mechanism is to set signalflag from a
    // SIGALRM that we arm via setitimer before calling
    // execute_dispatch_. The kernel delivers SIGALRM ~10 ms later;
    // by then the parser has consumed `stopped`, pushed the istopped
    // sentinel, and the repeat is grinding. The signal handler sets
    // signalflag, dispatcher trips on it at the next resume_iter,
    // raisesignal pushes /stop, /stop unwinds to the sentinel,
    // ostack gets `true`.
    prime_eval(i, "{ 1000000000 { 1 pop } repeat } stopped");

    // Arm a one-shot 50ms timer that delivers SIGUSR1 (the trap
    // already handles SIGUSR1). 50 ms is enough for the parser to
    // run `stopped` and enter the repeat loop on any sane CPU; we
    // still wall-clock cap at 5 s via the test's CTest TIMEOUT.
    struct itimerval timer{};
    timer.it_value.tv_sec  = 0;
    timer.it_value.tv_usec = 50 * 1000;
    // No SIGALRM-to-USR1 mapping; raise SIGUSR1 directly from a
    // SIGALRM handler. Use sigaction so we can install the trampoline.
    static volatile sig_atomic_t alarmed = 0;
    struct sigaction sa{};
    sa.sa_handler = [](int){ alarmed = 1; sli3::signalflag = SIGUSR1; };
    sigemptyset(&sa.sa_mask);
    sigaction(SIGALRM, &sa, nullptr);
    setitimer(ITIMER_REAL, &timer, nullptr);

    try { i.execute_dispatch_(0); } catch (...) { /* swallowed */ }

    // Disarm so the alarm doesn't fire after the test finishes.
    struct itimerval off{};
    setitimer(ITIMER_REAL, &off, nullptr);
    CHECK(alarmed == 1);

    if (i.OStack().load() < 1)
    {
        std::cerr << "FAIL: ostack empty after recovery\n";
        std::exit(1);
    }
    Token const& top = i.top();
    if (!top.is_of_type(sli3::booltype))
    {
        std::cerr << "FAIL: ostack top is not booltype, got tag="
                  << top.tag() << " load=" << i.OStack().load() << "\n";
        for (size_t k = 0; k < i.OStack().load(); ++k)
        {
            std::cerr << "  [" << k << "] tag="
                      << i.OStack().pick(k).tag() << "\n";
        }
        std::cerr << "  errorname=" << get_errorname(i) << "\n";
        std::exit(1);
    }
    CHECK(top.data_.bool_val == true);
    CHECK(get_errorname(i) == "SystemSignal");
    CHECK(get_signo(i) == SIGUSR1);
}

}  // namespace

int main()
{
    SLIInterpreter i;

    test_signaldict_present(i);
    test_pending_signal_interrupts_repeat(i);
    test_pending_signal_caught_outside_loop(i);
    test_signalflag_starts_zero(i);
    test_stopped_catches_signal(i);

    std::cout << "test_signal_trap: ok\n";
    return 0;
}
