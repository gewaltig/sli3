// Slice 5 rewrite of the legacy slistartup.{h,cc}.
//
// Builds a minimal runtime environment for the SLI interpreter:
//   - cin / cout / cerr bound to std::{cin,cout,cerr} via SLIistream /
//     SLIostream "borrow" mode (the wrapper does not own the standard
//     stream).
//   - statusdict (compile-time version, runtime argv, exit codes,
//     paths) inserted into the system dictionary.
//   - errordict already exists from the interpreter constructor; we
//     register a small set of operators the bootstrap script expects
//     (getenv, evalstring, start).
//   - Locates sli-init.sli (env SLIDATADIR override, else compile-time
//     SLI3_DATADIR) and primes the execution stack with an XIstream
//     Token + iparse so the dispatcher will start parsing/executing it
//     once the user calls execute().

#include "sli_startup.h"

#include "config.h"
#include "sli_array.h"
#include "sli_dictionary.h"
#include "sli_dictstack.h"
#include "sli_function.h"
#include "sli_interpreter.h"
#include "sli_iostream.h"
#include "sli_iostreamtype.h"
#include "sli_name.h"
#include "sli_string.h"
#include "sli_token.h"
#include "sli_type.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace sli3
{
namespace
{

//------------------------------------------------------------------------
// Helpers
//------------------------------------------------------------------------

std::string read_env(char const* var)
{
    char const* v = std::getenv(var);
    return v ? std::string(v) : std::string();
}

// Build an arraytype Token from argv.
Token argv_to_array(SLIInterpreter& i, int argc, char** argv)
{
    TokenArray* a = new TokenArray();
    a->reserve(static_cast<size_t>(argc));
    for (int k = 0; k < argc; ++k)
    {
        std::string s = argv && argv[k] ? argv[k] : "";
        a->push_back(i.new_token<sli3::stringtype, std::string>(std::move(s)));
    }
    return i.new_token<sli3::arraytype>(a);
}

// Insert (literal-name, integer) into a dict.
void insert_int(SLIInterpreter& i, Dictionary& d, char const* key, long v)
{
    d.insert(Name(key), i.new_token<sli3::integertype>(v));
}

void insert_str(SLIInterpreter& i, Dictionary& d, char const* key, std::string v)
{
    d.insert(Name(key), i.new_token<sli3::stringtype, std::string>(std::move(v)));
}

void insert_bool(SLIInterpreter& i, Dictionary& d, char const* key, bool v)
{
    d.insert(Name(key), i.new_token<sli3::booltype, bool>(v));
}

//------------------------------------------------------------------------
// Builtins exposed by the startup module.
//------------------------------------------------------------------------

// `string getenv -> string true | false`
class GetenvFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        i->require_stack_type(0, sli3::stringtype);
        std::string key = static_cast<std::string&>(i->top());
        char const* v = std::getenv(key.c_str());
        i->pop();
        if (v)
        {
            i->push(i->new_token<sli3::stringtype, std::string>(std::string(v)));
            i->push<bool>(true);
        }
        else
        {
            i->push<bool>(false);
        }
        i->EStack().pop();
    }
};

// `string evalstring -> -` Parses and executes the given string. The
// existing SLIInterpreter::execute(const std::string&) path pushes a
// stringtype Token with the program source plus a /evalstring name; the
// dispatcher resolves the name to this function. We wrap the string as
// an XIstream and let iparse drive the execution.
class EvalstringFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        i->require_stack_type(0, sli3::stringtype);
        // Take the string off the operand stack.
        std::string src = static_cast<std::string&>(i->top());
        i->pop();
        // Set up an owning istringstream wrapped in an SLIistream.
        auto* iss = new std::istringstream(std::move(src));
        auto* wrap = new SLIistream(iss);  // owns iss
        // Replace ourselves on the estack with [xistream, iparse]: the
        // dispatcher will run iparse repeatedly over the stream until
        // EOF, executing each parsed token.
        i->EStack().pop();  // pop /evalstring entry
        Token x(i->get_type(sli3::xistreamtype));
        x.data_.istream_val = wrap;
        i->EStack().push(x);
        // x.execute() (XIstreamType::execute) would push iparse, but
        // we're already past the dispatch — push iparse manually.
        i->EStack().push(i->new_token<sli3::nametype, Name>(i->iparse_name));
    }
};

GetenvFunction      getenv_fn;
EvalstringFunction  evalstring_fn;

//------------------------------------------------------------------------
// Stream-output operators. These match the names typeinit.sli expects
// (`<-`, `<--`, `endl`) so the loaded init script can build print
// helpers like `=` and `==` on top.
//
// For `<-`: write the value's print() representation to the stream and
// leave the stream on the operand stack so the call can be chained
// (`cout obj <- endl`).
// For `<--`: same but uses pprint() (parser-readable form).
// For `endl`: writes '\n' and flushes.
//------------------------------------------------------------------------

class StreamWriteFunction : public SLIFunction
{
    bool pprint_;
public:
    explicit StreamWriteFunction(bool pprint) : pprint_(pprint) {}
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(2);
        i->require_stack_type(1, sli3::ostreamtype);
        SLIostream* s = i->pick(1).data_.ostream_val;
        std::ostream* os = s ? s->get() : nullptr;
        if (!os)
        {
            i->raiseerror(i->BadIOError);
            return;
        }
        if (pprint_) i->top().pprint(*os);
        else         i->top().print(*os);
        i->pop();  // drop the value, leave the stream on top
        i->EStack().pop();
    }
};

class StreamEndlFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        i->require_stack_type(0, sli3::ostreamtype);
        SLIostream* s = i->top().data_.ostream_val;
        std::ostream* os = s ? s->get() : nullptr;
        if (!os)
        {
            i->raiseerror(i->BadIOError);
            return;
        }
        *os << '\n';
        os->flush();
        // Leave the stream on top.
        i->EStack().pop();
    }
};

class StreamFlushFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        i->require_stack_type(0, sli3::ostreamtype);
        SLIostream* s = i->top().data_.ostream_val;
        std::ostream* os = s ? s->get() : nullptr;
        if (!os)
        {
            i->raiseerror(i->BadIOError);
            return;
        }
        os->flush();
        i->EStack().pop();
    }
};

StreamWriteFunction  write_fn(false);
StreamWriteFunction  pwrite_fn(true);
StreamEndlFunction   endl_fn;
StreamFlushFunction  flush_fn;

//------------------------------------------------------------------------
// Minimal dictionary-stack operators that sli-init.sli needs to start
// (`userdict begin systemdict begin` on its first lines). More extensive
// dictionary surface will move to a dedicated module later.
//------------------------------------------------------------------------

class BeginFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        i->require_stack_type(0, sli3::dictionarytype);
        // DictionaryStack::push(Token) copies the Token; nothing to free.
        i->DStack().push(i->top());
        i->pop();
        i->EStack().pop();
    }
};

class EndFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        if (i->DStack().size() <= 2)
        {
            // The bottom two dictionaries (systemdict, userdict) are
            // the permanent base — refuse to pop them per PostScript
            // semantics.
            i->raiseerror(i->KernelError);
            return;
        }
        i->DStack().pop();
        i->EStack().pop();
    }
};

class DictFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        // `int dict -> dict`. The integer hint is ignored; std::map
        // grows as needed.
        i->require_stack_load(1);
        i->require_stack_type(0, sli3::integertype);
        auto* d = new Dictionary();
        Token t(i->get_type(sli3::dictionarytype));
        t.data_.dict_val = d;
        i->pop();
        i->push(t);
        i->EStack().pop();
    }
};

class CurrentdictFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        Dictionary* d = i->DStack().top();
        Token t(i->get_type(sli3::dictionarytype));
        t.data_.dict_val = d;
        i->push(t);
        i->EStack().pop();
    }
};

BeginFunction        begin_fn;
EndFunction          end_fn;
DictFunction         dict_fn;
CurrentdictFunction  currentdict_fn;

//------------------------------------------------------------------------
// Locate sli-init.sli. Search order:
//   1. $SLIDATADIR/sli-init.sli
//   2. SLI3_DATADIR/sli-init.sli (compile-time default)
//------------------------------------------------------------------------

bool file_readable(std::string const& path)
{
    std::ifstream in(path);
    return in.good();
}

std::string locate_sli_init()
{
    auto try_dir = [](std::string const& dir) -> std::string
    {
        if (dir.empty()) return std::string();
        std::string p = dir + "/sli-init.sli";
        return file_readable(p) ? p : std::string();
    };

    if (auto p = try_dir(read_env("SLIDATADIR")); !p.empty()) return p;
    if (auto p = try_dir(SLI3_DATADIR);            !p.empty()) return p;
    return std::string();
}

//------------------------------------------------------------------------
// Build the statusdict and bind it under /statusdict in systemdict.
//------------------------------------------------------------------------

Dictionary* build_statusdict(SLIInterpreter& i, int argc, char** argv,
                             std::string const& datadir)
{
    auto* d = new Dictionary();
    d->insert(Name("argv"), argv_to_array(i, argc, argv));
    insert_str(i, *d, "prgname",  "sli3");
    insert_str(i, *d, "version",  SLI3_VERSION);
    insert_int(i, *d, "prgmajor", SLI3_VERSION_MAJOR);
    insert_int(i, *d, "prgminor", SLI3_VERSION_MINOR);
    insert_str(i, *d, "prgpatch", SLI3_VERSION_PATCH);
    insert_str(i, *d, "built",    std::string(__DATE__) + " " + __TIME__);
    insert_str(i, *d, "prgdatadir", datadir);
    insert_int(i, *d, "exitcode", EXIT_SUCCESS);
    insert_bool(i, *d, "interactive", true);

    // Architecture sub-dict — sli-init.sli inspects it to print build
    // info. Keep entries minimal; add more as scripts demand them.
    auto* arch = new Dictionary();
    insert_int(i, *arch, "int",     sizeof(int));
    insert_int(i, *arch, "long",    sizeof(long));
    insert_int(i, *arch, "double",  sizeof(double));
    insert_int(i, *arch, "void *",  sizeof(void*));
    insert_int(i, *arch, "Token",   sizeof(Token));
    Token arch_tok(i.get_type(sli3::dictionarytype));
    arch_tok.data_.dict_val = arch;
    d->insert(Name("architecture"), arch_tok);

    // Standard exit codes consumed by quit_i.
    auto* codes = new Dictionary();
    insert_int(i, *codes, "success",      EXIT_SUCCESS);
    insert_int(i, *codes, "scripterror",  126);
    insert_int(i, *codes, "exception",    125);
    insert_int(i, *codes, "fatal",        127);
    insert_int(i, *codes, "unknownerror", 10);
    insert_int(i, *codes, "userabort",    134);
    Token codes_tok(i.get_type(sli3::dictionarytype));
    codes_tok.data_.dict_val = codes;
    d->insert(Name("exitcodes"), codes_tok);

    return d;
}

}  // anonymous namespace

void init_slistartup(SLIInterpreter* i, int argc, char** argv)
{
    // 1. Standard streams. We borrow std::cin/cout/cerr — they outlive
    //    the interpreter and must not be deleted.
    auto bind_stream = [&](Name n, SLIType* type, auto* sli_stream)
    {
        Token t(type);
        if constexpr (std::is_same_v<decltype(sli_stream), SLIistream*>)
            t.data_.istream_val = sli_stream;
        else
            t.data_.ostream_val = sli_stream;
        i->def(n, t);
    };
    bind_stream(Name("cin"),  i->get_type(sli3::istreamtype),
                new SLIistream(std::cin));
    bind_stream(Name("cout"), i->get_type(sli3::ostreamtype),
                new SLIostream(std::cout));
    bind_stream(Name("cerr"), i->get_type(sli3::ostreamtype),
                new SLIostream(std::cerr));

    // 2. Operators registered by this module.
    i->createcommand("getenv",     &getenv_fn);
    i->createcommand("evalstring", &evalstring_fn);
    i->createcommand("<-",         &write_fn);
    i->createcommand("<--",        &pwrite_fn);
    i->createcommand("endl",       &endl_fn);
    i->createcommand("flush",      &flush_fn);
    i->createcommand("begin",      &begin_fn);
    i->createcommand("end",        &end_fn);
    i->createcommand("dict",       &dict_fn);
    i->createcommand("currentdict",&currentdict_fn);

    // 3. Resolve sli-init.sli. If we can't find it, leave a clear
    //    message and skip the bootstrap — the interpreter still has
    //    its built-in functions and is usable for low-level testing.
    std::string init_path = locate_sli_init();

    // 4. Build statusdict. Done after locate so prgdatadir reflects
    //    the actual location used.
    std::string datadir;
    if (!init_path.empty())
    {
        // Strip "/sli-init.sli" off the resolved path.
        auto pos = init_path.find_last_of('/');
        datadir = (pos == std::string::npos) ? std::string() : init_path.substr(0, pos);
    }
    Dictionary* statusdict = build_statusdict(*i, argc, argv, datadir);
    Token status_tok(i->get_type(sli3::dictionarytype));
    status_tok.data_.dict_val = statusdict;
    i->def(Name("statusdict"), status_tok);

    // 5. Push sli-init.sli onto the execution stack so that as soon as
    //    the dispatcher starts, the script bootstrap runs.
    if (init_path.empty())
    {
        i->message(M_WARNING, "SLIStartup",
                   "sli-init.sli not found; bootstrap skipped. "
                   "Set SLIDATADIR to override the default search path.");
        return;
    }

    auto* in = new std::ifstream(init_path);
    if (!in->is_open())
    {
        delete in;
        i->message(M_ERROR, "SLIStartup",
                   ("Failed to open " + init_path).c_str());
        return;
    }
    auto* wrap = new SLIistream(in);  // takes ownership
    Token x(i->get_type(sli3::xistreamtype));
    x.data_.istream_val = wrap;
    // The dispatcher reaches xistreamtype on top -> XIstreamType::execute
    // pushes the stream + iparse to drive read-eval.
    i->EStack().push(x);
}

}  // namespace sli3
