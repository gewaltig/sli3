// Wrap linenoise (vendored under src/util/linenoise/) as two SLI ops
// matching the names sli-init.sli expects:
//
//   `(prompt) GNUreadline -> (input) true | false`
//        Reads one edited line. Returns false on EOF (Ctrl-D) or
//        when the user hits Ctrl-C.
//   `(line)   GNUaddhistory -> -`
//        Appends `line` to the in-memory history ring (up-arrow, etc).
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

#include <string>

namespace sli3
{
namespace
{

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
        i->EStack().pop();
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
            linenoiseHistoryAdd(s.c_str());
        i->pop();
        i->EStack().pop();
    }
};

GNUreadlineFunction   gnureadline_fn;
GNUaddhistoryFunction gnuaddhistory_fn;

}  // anonymous namespace

void init_sli_readline(SLIInterpreter* i)
{
    i->createcommand("GNUreadline",   &gnureadline_fn);
    i->createcommand("GNUaddhistory", &gnuaddhistory_fn);
}

}  // namespace sli3
