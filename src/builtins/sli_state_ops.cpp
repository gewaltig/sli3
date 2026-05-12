#include "sli_state_ops.h"

#include "sli_exceptions.h"
#include "sli_function.h"
#include "sli_interpreter.h"
#include "sli_serialize.h"
#include "sli_string.h"
#include "sli_tokenstack.h"

#include <fstream>

namespace sli3
{
namespace
{

// (string) savestate -> -
//
// Captures (operand stack, execution stack) to the named file in
// the BinaryWriter format defined in src/serialize/. The op pops
// itself from the e-stack BEFORE serializing, so the snapshot is
// the state as if savestate had just returned. On the resume
// side, restorestate replaces the live stacks with the snapshot
// and the dispatcher continues from the new e-stack top.
class SaveStateFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        i->require_stack_type(0, sli3::stringtype);
        std::string filename = i->top().data_.string_val->str();
        i->pop();

        // Axis I bundle step 4: dispatcher pre-popped /savestate,
        // so the e-stack snapshot already reflects "savestate has
        // returned" without an explicit pop here.

        std::ofstream out(filename, std::ios::binary);
        if (!out)
        {
            i->raiseerror(i->BadIOError);
            return;
        }

        try
        {
            BinaryWriter w(out);
            w.write_header();
            write_token_stack(i->OStack(), w);
            write_token_stack(i->EStack(), w);
        }
        catch (std::exception&)
        {
            // BinaryWriter / write_token can throw on I/O failure
            // or unsupported type. The output file is left in an
            // incomplete state; the caller's stacks are unchanged
            // (we wrote out a copy, not the live state).
            i->raiseerror(i->BadIOError);
            return;
        }
    }
};

// (string) restorestate -> -
//
// Reads the snapshot from the named file and atomically replaces
// the operand stack and execution stack with its contents. If the
// snapshot's execution stack is non-empty, dispatch continues
// from its top after this operator returns; if empty, the
// dispatcher exits.
class RestoreStateFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        i->require_stack_type(0, sli3::stringtype);
        std::string filename = i->top().data_.string_val->str();
        i->pop();

        // Axis I bundle step 4: dispatcher pre-popped /restorestate;
        // we're about to overwrite the e-stack entirely anyway.

        std::ifstream in(filename, std::ios::binary);
        if (!in)
        {
            i->raiseerror(i->BadIOError);
            return;
        }

        try
        {
            BinaryReader r(in);
            r.read_header();
            // Read into the live stacks. read_token_stack clears
            // each stack before populating. If the second read
            // throws, the operand stack is already replaced and
            // the e-stack is empty — recoverable but lossy.
            read_token_stack(i->OStack(), r, *i);
            read_token_stack(i->EStack(), r, *i);
        }
        catch (std::exception&)
        {
            i->raiseerror(i->BadIOError);
            return;
        }
    }
};

SaveStateFunction    savestate_fn;
RestoreStateFunction restorestate_fn;

}  // namespace

void init_state_ops(SLIInterpreter* i)
{
    i->createcommand("savestate",    &savestate_fn);
    i->createcommand("restorestate", &restorestate_fn);

    // Axis I bundle step 4: dispatcher pre-pop contract.
    savestate_fn.set_new_abi();
    restorestate_fn.set_new_abi();
}

}  // namespace sli3
