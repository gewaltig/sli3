// Wrap linenoise (vendored under src/util/linenoise/) as two SLI ops
// matching the names sli-init.sli expects:
//
//   `(prompt) GNUreadline -> (input) true | false`
//        Reads one edited line. Returns false on EOF (Ctrl-D) or
//        when the user hits Ctrl-C.
//   `(line)   GNUaddhistory -> -`
//        Appends `line` to the in-memory history ring (up-arrow, etc)
//        and saves the ring to $HOME/.sli3_history so it survives
//        across sessions.
//
// History persistence:
//   - On init we load $HOME/.sli3_history (if present) into the ring.
//   - On every GNUaddhistory we save the ring back to disk. Doing it
//     per-line costs one fast file write but means history survives
//     crashes / SIGKILL / `quit` paths that don't run an exithook.
//   - If $HOME isn't set or the file can't be opened we silently
//     run without persistence — matches GNU readline's behaviour.
//
// linenoise itself falls back to fgets when stdin is not a tty, so
// non-interactive (piped) input still works without changes.

#include "sli_readline.h"

#include "sli_function.h"
#include "sli_interpreter.h"
#include "sli_string.h"
#include "sli_token.h"
#include "sli_type.h"

#include "linenoise/linenoise.h"

#include <cstdlib>
#include <string>

namespace sli3
{
namespace
{

// Resolved at init time. Empty if $HOME wasn't set, in which case
// the load/save calls become no-ops.
std::string history_file;

// Maximum history entries kept in memory and saved to disk.
constexpr int kHistoryMaxLen = 1000;

class GNUreadlineFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        i->require_stack_type(0, sli3::stringtype);
        std::string prompt = i->top().data_.string_val->str();
        i->pop();
        char* line = linenoise(prompt.c_str());
        if (line == nullptr)
        {
            // EOF (Ctrl-D) or interrupt: tell the executive loop to
            // stop, matching readline's contract in sli-init.sli.
            i->push<bool>(false);
            i->EStack().pop();
            return;
        }
        std::string s(line);
        linenoiseFree(line);
        i->push(i->new_token<sli3::stringtype, std::string>(std::move(s)));
        i->push<bool>(true);
    }
};

class GNUaddhistoryFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        i->require_stack_type(0, sli3::stringtype);
        std::string const& s = i->top().data_.string_val->str();
        // Skip empty lines so they don't clutter the history ring.
        if (!s.empty())
        {
            linenoiseHistoryAdd(s.c_str());
            // Persist after every add so unexpected exits don't lose
            // recent history. If history_file is empty (no $HOME),
            // skip silently — this is best-effort, not load-bearing.
            if (!history_file.empty())
                linenoiseHistorySave(history_file.c_str());
        }
        i->pop();
    }
};

GNUreadlineFunction   gnureadline_fn;
GNUaddhistoryFunction gnuaddhistory_fn;

}  // anonymous namespace

void init_sli_readline(SLIInterpreter* i)
{
    i->createcommand("GNUreadline",   &gnureadline_fn);
    i->createcommand("GNUaddhistory", &gnuaddhistory_fn);

    // Axis I bundle step 3f: trailing-pop readline ops to new ABI.
    gnuaddhistory_fn.set_new_abi();
    gnureadline_fn.set_new_abi();

    // Configure the linenoise history ring before loading: the cap
    // applies to both the in-memory ring and the saved file.
    linenoiseHistorySetMaxLen(kHistoryMaxLen);

    // Resolve $HOME once and remember the path. On startup, prime the
    // ring from disk so up-arrow recalls commands from earlier
    // sessions. If the file doesn't exist yet, linenoiseHistoryLoad
    // returns -1 silently — the first GNUaddhistory will create it.
    char const* home = std::getenv("HOME");
    if (home && *home)
    {
        history_file = std::string(home) + "/.sli3_history";
        linenoiseHistoryLoad(history_file.c_str());
    }
}

}  // namespace sli3
