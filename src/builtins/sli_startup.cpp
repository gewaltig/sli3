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

#include <csignal>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>

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

// Build an arraytype Token from argv. Always contains the program
// name as element 0 — `:commandline` in sli-init.sli does
// `statusdict /argv get Rest` (drops element 0 = program name) before
// scanning for options. With a truly empty argv, `Rest` raises
// RangeCheck via erase_a. NEST always gets argv[0] from main(), so
// matching that shape keeps the script verbatim-compatible.
Token argv_to_array(SLIInterpreter& i, int argc, char** argv)
{
    TokenArray* a = new TokenArray();
    a->reserve(static_cast<size_t>(argc > 0 ? argc : 1));
    if (argc <= 0 || argv == nullptr || argv[0] == nullptr)
    {
        a->push_back(i.new_token<sli3::stringtype, std::string>(
            std::string("sli3")));
    }
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
        // Phase 5 new-ABI: dispatcher already pre-popped the
        // /evalstring entry. Push [xistream, iparse] on top so the
        // dispatcher runs iparse repeatedly over the stream until EOF.
        Token x(i->get_type(sli3::xistreamtype));
        x.data_.istream_val = wrap;
        i->EStack().push(x);
        i->EStack().push(i->new_token<sli3::nametype, Name>(i->iparse_name));
    }
};

GetenvFunction      getenv_fn;
EvalstringFunction  evalstring_fn;

//------------------------------------------------------------------------
// Stub for operators that are referenced in typeinit.sli /
// mathematica.sli / etc. via `<X> load addtotrie` but are NOT yet
// implemented in C++. The trie machinery only requires the name to
// exist in systemdict at construction time; the trie body is consulted
// later when an operand of the matching type signature actually invokes
// the trie. So registering a stub satisfies trie construction; if a
// stubbed op is later actually called, the user gets a clear
// "Unimplemented" error with the operator's registered name.
//
// Each stub instance carries its own name (set by createcommand),
// which raiseerror reports as the failing command. We allocate one
// instance per registration so the names don't get clobbered.
//------------------------------------------------------------------------

class UnimplementedFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        // Print a clear diagnostic before raising, so the failure is
        // obvious in interactive use.
        std::string msg = "Operator '" + std::string(get_name().toString())
                          + "' is registered as a stub but not yet"
                            " implemented. Bootstrap referenced it via"
                            " `load addtotrie` so the trie surface could be"
                            " built; runtime calls land here.";
        i->message(M_ERROR, "Unimplemented", msg.c_str());
        i->raiseerror(get_name(), Name("Unimplemented"));
    }
};

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
    }
};

class EndFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        if (i->DStack().size() <= 3)
        {
            // PostScript Level-2 protects the 3 permanent base dicts
            // (bottom→top: systemdict, globaldict, userdict). The
            // dictionarystackunderflow condition is raised here as
            // KernelError to match the rest of the runtime's error
            // taxonomy; PLRM names it `dictstackunderflow`.
            i->raiseerror(i->KernelError);
            return;
        }
        i->DStack().pop();
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
        // Raw-assigned payload: bump the dict's refcount so the local
        // destructor's remove_reference is balanced. Without this, the
        // local's drop cancels push's add, leaving the ostack entry
        // unaccounted -- a later `end` then deletes the dict out from
        // under it. Same pattern as DictionaryStack::toArray.
        t.add_reference();
        i->push(t);
    }
};

// PostScript: push the number of dictionaries currently on the
// dictionary stack.
class CountdictstackFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->push(static_cast<long>(i->DStack().size()));
    }
};

// PostScript: push an array containing every dictionary currently
// on the dictionary stack, bottom first. Each entry shares the same
// payload pointer as the dictstack -- mutations via one are
// visible through the other. The returned array itself is the
// DictionaryStack's cached snapshot (refcount-shared); callers
// must treat it as read-only.
class DictstackFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        TokenArray *arr = i->DStack().snapshot(*i);
        Token t(i->get_type(sli3::arraytype));
        t.data_.array_val = arr;
        i->push(t);
    }
};

// Restore the canonical PS Level-2 dictstack: bottom-to-top
// systemdict / globaldict / userdict. Any other dicts currently on
// the stack (local frames from `begin` / `dict begin`, etc.) are
// dropped. This is a stronger reset than just trimming the top —
// it also restores the permanent layout if `end` had popped one
// of the three default dicts off.
class CleardictstackFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->reset_dictstack();
    }
};

// `array restoredstack -> -`
// Replace the dictionary stack with the dictionaries listed in the
// argument array. Inverse of `dictstack`: array[0] is the bottom
// dict, array[size-1] is the top. Used by debug.sli/sli-init.sli to
// restore the dstack from the snapshot in errordict /dstack.
class RestoredstackFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        i->require_stack_type(0, sli3::arraytype);
        TokenArray const* arr = i->top().data_.array_val;
        // Validate the whole array before touching the dstack: on a
        // type error, both stacks must look as they did on entry so
        // the error handler can introspect them.
        for (size_t k = 0; k < arr->size(); ++k)
        {
            if (!(*arr)[k].is_of_type(sli3::dictionarytype))
            {
                i->raiseerror(i->ArgumentTypeError);
                return;
            }
        }
        // DictionaryStack::restore_from has an identity fast path for
        // the common `dictstack ... restoredstack` save/restore case
        // and skips redundant per-key cache invalidation on the slow
        // path. See sli_dictstack.h for the contract.
        i->DStack().restore_from(*arr);
        i->pop();
    }
};

BeginFunction           begin_fn;
EndFunction             end_fn;
DictFunction            dict_fn;
CurrentdictFunction     currentdict_fn;
CountdictstackFunction  countdictstack_fn;
DictstackFunction       dictstack_fn;
CleardictstackFunction  cleardictstack_fn;
RestoredstackFunction   restoredstack_fn;

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

// Search for sli-init.sli under <datadir>/sli/sli-init.sli, mirroring
// the NEST install layout where the data root contains a sli/
// subdirectory of init scripts. Returns the resolved path or "".
std::string locate_sli_init()
{
    auto try_dir = [](std::string const& dir) -> std::string
    {
        if (dir.empty()) return std::string();
        std::string p = dir + "/sli/sli-init.sli";
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
    // /host -- consumed by sli-init.sli /:commandline's `-v` branch
    // (the version banner reads `... built ( for ) host`). NEST 2.x
    // populated this from configure (NEST_HOST); sli3 reads it at
    // runtime via gethostname(2) so there's no configure-time
    // dependency.
    {
        char hn[256];
        std::string host = "unknown";
        if (gethostname(hn, sizeof(hn)) == 0) {
            hn[sizeof(hn) - 1] = '\0';
            host = hn;
        }
        insert_str(i, *d, "host", host);
    }
    insert_str(i, *d, "prgdatadir", datadir);
    // prgdocdir is consumed by helpinit.sli (HelpRoot, HelpdeskURL).
    // We don't ship help docs yet — point at datadir as a harmless
    // default so the lookups succeed and helpinit.sli loads.
    insert_str(i, *d, "prgdocdir",  datadir);
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
    i->createcommand("countdictstack", &countdictstack_fn);
    i->createcommand("dictstack",      &dictstack_fn);
    i->createcommand("cleardictstack", &cleardictstack_fn);
    i->createcommand("restoredstack",  &restoredstack_fn);

    // 2b. Stubs for unimplemented operators that typeinit.sli /
    //     mathematica.sli / etc. reference via `<X> load addtotrie`.
    //     The trie can be built; calling a stubbed op at runtime
    //     raises Unimplemented. This list is the audited set of
    //     names that appear in `addtotrie` calls but have neither a
    //     C++ leaf nor an .sli definition before their use site.
    //     When an operator graduates to a real implementation, drop
    //     its entry here.
    static const char* unimplemented_ops[] = {
        ":resize_a", ":resize_s",
        "capacity_a", "capacity_s",
        "cvlp_p", "cvn_s",
        "doublevector2array",
        "intvector2array",
        "references_a",
        "shrink_a",
        // FormattedIO.sli — typed-stream read helpers, not on the
        // interactive-prompt path.
        "ReadDouble", "ReadInt", "ReadWord",
    };
    for (const char* name : unimplemented_ops)
    {
        i->createcommand(name, new UnimplementedFunction());
    }

    // 3. Resolve sli-init.sli. If we can't find it, leave a clear
    //    message and skip the bootstrap — the interpreter still has
    //    its built-in functions and is usable for low-level testing.
    std::string init_path = locate_sli_init();

    // 4. Build statusdict. Done after locate so prgdatadir reflects
    //    the actual location used.
    // statusdict::prgdatadir is the data ROOT (the parent of sli/),
    // so that sli-init.sli's `[ statusdict /prgdatadir get_d (/sli)
    // join_s ]` produces the correct SLISearchPath entry.
    std::string datadir;
    if (!init_path.empty())
    {
        // init_path looks like "<datadir>/sli/sli-init.sli". Strip the
        // last two path components.
        auto p1 = init_path.find_last_of('/');
        if (p1 != std::string::npos)
        {
            auto p2 = init_path.find_last_of('/', p1 - 1);
            if (p2 != std::string::npos)
                datadir = init_path.substr(0, p2);
        }
    }
    // sli-init.sli runs `moduleinitializers { initialize_module } forall`
    // late in the bootstrap. In NEST 2.x both are populated by C++
    // modules via SLIModule::commandstring; sli3 has no concrete
    // SLIModule subclasses (per the scope decision), so bind harmless
    // empties and the forall becomes a no-op. iterate-over-empty-string
    // matches NEST's data shape (commandstring is a string).
    i->def(Name("moduleinitializers"),
           i->new_token<sli3::stringtype, std::string>(std::string()));
    {
        TokenArray* p = new TokenArray();
        Token init_proc = i->new_token<sli3::proceduretype, TokenArray*>(p);
        i->def(Name("initialize_module"), init_proc);
    }

    Dictionary* statusdict = build_statusdict(*i, argc, argv, datadir);
    Token status_tok(i->get_type(sli3::dictionarytype));
    status_tok.data_.dict_val = statusdict;
    i->def(Name("statusdict"), status_tok);

    // signaldict maps POSIX signal NAMES to their numeric values, so
    // SLI code can write `signaldict /SIGINT get` instead of hard-
    // coding the platform-specific integer. raisesignal stores the
    // delivered number under /sys_signo in errordict; matching it
    // against signaldict is how scripts decide what to do.
    auto* sigd = new Dictionary();
#define SLI_REG_SIG(name) insert_int(*i, *sigd, #name, name)
    SLI_REG_SIG(SIGABRT);
    SLI_REG_SIG(SIGALRM);
    SLI_REG_SIG(SIGFPE);
    SLI_REG_SIG(SIGHUP);
    SLI_REG_SIG(SIGILL);
    SLI_REG_SIG(SIGINT);
    SLI_REG_SIG(SIGKILL);
    SLI_REG_SIG(SIGPIPE);
    SLI_REG_SIG(SIGQUIT);
    SLI_REG_SIG(SIGSEGV);
    SLI_REG_SIG(SIGTERM);
    SLI_REG_SIG(SIGUSR1);
    SLI_REG_SIG(SIGUSR2);
    SLI_REG_SIG(SIGCHLD);
    SLI_REG_SIG(SIGCONT);
    SLI_REG_SIG(SIGSTOP);
    SLI_REG_SIG(SIGTSTP);
    SLI_REG_SIG(SIGTTIN);
    SLI_REG_SIG(SIGTTOU);
#undef SLI_REG_SIG
    Token sig_tok(i->get_type(sli3::dictionarytype));
    sig_tok.data_.dict_val = sigd;
    i->def(Name("signaldict"), sig_tok);

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
