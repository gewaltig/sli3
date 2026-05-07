// Error handling: every SLIException subclass surfaces in
// errordict /errorname under its own Name, not the generic
// "DictError" parent label.
//
// Today, sli_exceptions.h:225 has DictError(char const *) ignore
// its argument, so UndefinedName / EntryTypeMismatch / etc. all
// end up as /DictError. Stage 3.1 forwards the argument to the
// SLIException base, fixing the collapse.
//
// Mechanism: push a marker onto the e-stack so raiseerror's
// "pop the failing op's frame" doesn't underflow, then call
// raiseerror(exc) directly. Inspect error_dict() afterwards.

#include "sli_dictionary.h"
#include "sli_exceptions.h"
#include "sli_interpreter.h"
#include "sli_name.h"
#include "sli_token.h"
#include "sli_tokenstack.h"

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>

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

// Push a placeholder Token onto the e-stack so raiseerror's
// pop has something to consume. quittype is convenient because
// new_token<quittype>() is specialized and the test never enters
// the dispatcher (so the quit-as-sentinel semantics doesn't fire).
void push_dummy_frame(SLIInterpreter& i)
{
    i.EStack().push(i.new_token<sli3::quittype>());
}

void clear_after_error(SLIInterpreter& i)
{
    i.OStack().clear();
    i.EStack().clear();
    // Wipe error_dict_ so the next call observes a fresh state.
    i.error_dict().clear();
    i.set_call_depth(0);
}

// Throw an SLIException of the given type via raiseerror, then
// look up errordict /errorname. Returns the resolved Name string.
template <class Exc>
std::string trigger_and_get_errorname(SLIInterpreter& i, Exc&& e)
{
    push_dummy_frame(i);
    try
    {
        // raiseerror(std::exception&) reads execution_stack_.top()
        // for /commandname (so the dummy frame must be there).
        i.raiseerror(e);
    }
    catch (...)
    {
        // raiseerror itself doesn't throw, but be defensive.
        std::cerr << "raiseerror threw, unexpected\n";
        std::exit(1);
    }

    // Look up errordict /errorname.
    Token const& t = i.error_dict().lookup(Name("errorname"));
    if (t.is_of_type(sli3::literaltype) ||
        t.is_of_type(sli3::nametype) ||
        t.is_of_type(sli3::symboltype))
    {
        return Name(t.data_.name_val).toString();
    }
    return "<not-a-name>";
}

// ---------- per-exception errorname survival ----------------------------

void test_undefined_name(SLIInterpreter& i)
{
    UndefinedName e("/foo");
    auto name = trigger_and_get_errorname(i, e);
    CHECK(name == "UndefinedName");   // <-- collapsed to "DictError" today
    clear_after_error(i);
}

void test_entry_type_mismatch(SLIInterpreter& i)
{
    EntryTypeMismatch e("integertype", "doubletype");
    auto name = trigger_and_get_errorname(i, e);
    CHECK(name == "EntryTypeMismatch");
    clear_after_error(i);
}

void test_unaccessed_dictionary_entry(SLIInterpreter& i)
{
    UnaccessedDictionaryEntry e("/k");
    auto name = trigger_and_get_errorname(i, e);
    CHECK(name == "UnaccessedDictionaryEntry");
    clear_after_error(i);
}

// Non-DictError subclasses already surface their name correctly.
// These tests lock in the existing behaviour so a future refactor
// of SLIException / InterpreterError doesn't accidentally
// reintroduce the same forwarding bug.

void test_range_check(SLIInterpreter& i)
{
    RangeCheck e(0);
    auto name = trigger_and_get_errorname(i, e);
    CHECK(name == "RangeCheck");
    clear_after_error(i);
}

void test_argument_type(SLIInterpreter& i)
{
    ArgumentType e(2);
    auto name = trigger_and_get_errorname(i, e);
    CHECK(name == "ArgumentType");
    clear_after_error(i);
}

void test_stack_underflow(SLIInterpreter& i)
{
    StackUnderflow e(3, 1);
    auto name = trigger_and_get_errorname(i, e);
    CHECK(name == "StackUnderflow");
    // Stage 3 also fixes a missing space in the rendered message:
    // "more arguments than 1" not "argumentsthan 1".
    auto msg = e.message();
    CHECK(msg.find("argumentsthan") == std::string::npos);
    clear_after_error(i);
}

void test_type_mismatch(SLIInterpreter& i)
{
    TypeMismatch e("integertype", "stringtype");
    auto name = trigger_and_get_errorname(i, e);
    CHECK(name == "TypeMismatch");
    clear_after_error(i);
}

// ---------- estack discipline ------------------------------------------
//
// raiseerror(Name) pops the e-stack first; raiseerror(std::exception&)
// must do the same so both error-entry paths leave the stack at the
// same depth. Stage 3.3.

void test_estack_consistent_between_paths(SLIInterpreter& i)
{
    // Path 1: Name overload.
    push_dummy_frame(i);
    size_t before = i.EStack().load();
    i.raiseerror(Name("ChosenError"));
    size_t after_name = i.EStack().load();
    clear_after_error(i);

    // Path 2: exception overload.
    push_dummy_frame(i);
    CHECK(i.EStack().load() == before);
    UndefinedName e("/k");
    i.raiseerror(e);
    size_t after_exc = i.EStack().load();
    clear_after_error(i);

    // Both paths push /cmd onto ostack and `stop` onto estack;
    // both should pop the failing op's frame first. So the
    // delta should be the same: -1 (frame popped) +1 (stop pushed).
    CHECK(after_name == after_exc);
}

}  // namespace

int main()
{
    SLIInterpreter i;

    test_undefined_name(i);
    test_entry_type_mismatch(i);
    test_unaccessed_dictionary_entry(i);
    test_range_check(i);
    test_argument_type(i);
    test_stack_underflow(i);
    test_type_mismatch(i);

    test_estack_consistent_between_paths(i);

    std::cout << "test_errors: ok\n";
    return 0;
}
