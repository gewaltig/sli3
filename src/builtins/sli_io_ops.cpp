// Minimal file I/O for the bootstrap. Slice 6 will rewrite this as a
// full modern-C++ stream layer; for now we only register what
// sli-init.sli + typeinit.sli actually call: ifstream / ofstream /
// file / close / eof / getline_is / cvx_f.

#include "sli_io_ops.h"

#include "sli_dictionary.h"
#include "sli_function.h"
#include "sli_interpreter.h"
#include "sli_iostream.h"
#include "sli_iostreamtype.h"
#include "sli_name.h"
#include "sli_string.h"
#include "sli_token.h"
#include "sli_type.h"

#include <fstream>
#include <istream>
#include <ostream>
#include <sstream>
#include <string>

namespace sli3
{
namespace
{

// Open a file for reading. The SLI search-path logic lives in
// sli-init.sli's `searchifstream` / `searchfile`; here we just open
// whatever literal path the caller gives us.
std::ifstream* open_for_read(SLIInterpreter&, std::string const& path)
{
    auto* in = new std::ifstream(path);
    if (in->is_open()) return in;
    delete in;
    return nullptr;
}

// `string ifstream -> istream true | false`. Opens for reading; on
// failure leaves only `false` on the stack (the original string is
// consumed, matching NEST 2.x behaviour).
class IfstreamFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        i->require_stack_type(0, sli3::stringtype);
        std::string path = i->top().data_.string_val->str();
        i->pop();
        std::ifstream* in = open_for_read(*i, path);
        if (!in)
        {
            i->push<bool>(false);
        }
        else
        {
            auto* wrap = new SLIistream(in);  // owns
            Token t(i->get_type(sli3::istreamtype));
            t.data_.istream_val = wrap;
            i->push(t);
            i->push<bool>(true);
        }
    }
};

// `string ofstream -> ostream true | false`. Opens for writing
// (truncate). Same convention as ifstream.
class OfstreamFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        i->require_stack_type(0, sli3::stringtype);
        std::string path = i->top().data_.string_val->str();
        i->pop();
        auto* out = new std::ofstream(path);
        if (!out->is_open())
        {
            delete out;
            i->push<bool>(false);
        }
        else
        {
            auto* wrap = new SLIostream(out);  // owns
            Token t(i->get_type(sli3::ostreamtype));
            t.data_.ostream_val = wrap;
            i->push(t);
            i->push<bool>(true);
        }
    }
};

// `file` is intentionally NOT registered as a C++ op: sli-init.sli
// defines it as a trie at line ~1156, dispatching to ifstream/ofstream
// + searchifstream and reporting FileOpenError on failure. Keeping the
// trie as the single source of truth lets the SLI script control the
// search-path semantics.

// `istream cvx_f -> xistream` — flip the type tag so the dispatcher
// treats the stream as executable (parse-and-exec).
class CvxFFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        // Accept either istreamtype or xistreamtype already.
        if (i->top().is_of_type(sli3::istreamtype))
            i->top().type_ = i->get_type(sli3::xistreamtype);
        else if (!i->top().is_of_type(sli3::xistreamtype))
        {
            i->raiseerror(i->ArgumentTypeError);
            return;
        }
    }
};

class CloseStreamFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        Token& top = i->top();
        // Closing is implicit on Token destruction (when refcount hits
        // zero). Just drop the stream. new-ABI: dispatcher pre-popped
        // /close's slot, no self-pop needed.
        if (top.is_of_type(sli3::istreamtype) ||
            top.is_of_type(sli3::xistreamtype) ||
            top.is_of_type(sli3::ostreamtype))
        {
            i->pop();
        }
        else
        {
            i->raiseerror(i->ArgumentTypeError);
        }
    }
};

class EofStreamFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        Token& top = i->top();
        bool eof = true;
        if (top.is_of_type(sli3::istreamtype) ||
            top.is_of_type(sli3::xistreamtype))
        {
            std::istream* s = top.data_.istream_val
                              ? top.data_.istream_val->get() : nullptr;
            eof = !s || s->eof();
        }
        else if (top.is_of_type(sli3::ostreamtype))
        {
            // ostreams are never "eof"; report false.
            eof = false;
        }
        else
        {
            i->raiseerror(i->ArgumentTypeError);
            return;
        }
        i->pop();
        i->push<bool>(eof);
    }
};

// `istream getline_is -> istream string true | istream false`
class GetlineIsFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        Token& top = i->top();
        if (!(top.is_of_type(sli3::istreamtype) ||
              top.is_of_type(sli3::xistreamtype)))
        {
            i->raiseerror(i->ArgumentTypeError);
            return;
        }
        std::istream* s = top.data_.istream_val
                          ? top.data_.istream_val->get() : nullptr;
        if (!s || !s->good())
        {
            i->push<bool>(false);
            // Axis I bundle step 4: /getline_is frame already popped.
            return;
        }
        std::string line;
        if (!std::getline(*s, line))
        {
            i->push<bool>(false);
        }
        else
        {
            i->push(i->new_token<sli3::stringtype, std::string>(std::move(line)));
            i->push<bool>(true);
        }
    }
};

// `ostrstream -> ostream true | false` — open an in-memory output
// string-stream. NEST 2.x sli_io.cc OstrstreamFunction. Always
// succeeds in modern C++ — std::ostringstream's no-arg ctor doesn't
// fail. We still push the (true) success flag for compatibility with
// callers that pop and branch on it.
class OstrstreamFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        auto* oss = new std::ostringstream();
        auto* wrap = new SLIostream(oss);  // takes ownership
        Token t(i->get_type(sli3::ostreamtype));
        t.data_.ostream_val = wrap;
        i->push(t);
        i->push<bool>(true);
    }
};

// `src dst CopyFile -> --` — file-system copy. Mirrors NEST 2.20.2
// sli/filesystem.cc CopyFileFunction. Pops both arguments on success;
// raises BadIO on failure — critically, also pops them in the error
// path so the operand stack is clean for the surrounding stopped
// handler. The previous "CopyFile : DictError" trace left two
// strings stranded on the stack, which then polluted every later
// dispatch and made the executive REPL unusable.
class CopyFileFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(2);
        i->require_stack_type(1, sli3::stringtype);
        i->require_stack_type(0, sli3::stringtype);
        std::string src = i->pick(1).data_.string_val->str();
        std::string dst = i->top().data_.string_val->str();
        i->pop(2);

        std::ifstream in(src, std::ios::binary);
        if (!in)
        {
            i->message(M_ERROR, "CopyFile",
                       ("Could not open source file: " + src).c_str());
            i->raiseerror(i->BadIOError);
            return;
        }
        std::ofstream out(dst, std::ios::binary);
        if (!out)
        {
            i->message(M_ERROR, "CopyFile",
                       ("Could not create destination file: " + dst).c_str());
            i->raiseerror(i->BadIOError);
            return;
        }
        out << in.rdbuf();
        if (!out)
        {
            i->message(M_ERROR, "CopyFile", "Error during copy.");
            i->raiseerror(i->BadIOError);
            return;
        }
    }
};

// `ostream str -> string` — extract the accumulated contents of an
// ostrstream-style output stream. Only meaningful for streams backed
// by std::ostringstream; for other ostreamtype values it returns the
// empty string. The stream is left consumed (closed/empty).
class StrFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        i->require_stack_type(0, sli3::ostreamtype);
        SLIostream* s = i->top().data_.ostream_val;
        std::ostream* os = s ? s->get() : nullptr;
        std::string out;
        if (auto* oss = dynamic_cast<std::ostringstream*>(os))
        {
            out = oss->str();
        }
        i->pop();
        i->push(i->new_token<sli3::stringtype, std::string>(std::move(out)));
    }
};

IfstreamFunction       ifstream_fn;
OfstreamFunction       ofstream_fn;
CvxFFunction           cvx_f_fn;
CloseStreamFunction    close_fn;
EofStreamFunction      eof_fn;
GetlineIsFunction      getline_is_fn;
OstrstreamFunction     ostrstream_fn;
StrFunction            str_fn;
CopyFileFunction       copyfile_fn;

}  // anonymous namespace

void init_io_ops(SLIInterpreter* i)
{
    i->createcommand("ifstream",   &ifstream_fn);
    i->createcommand("ofstream",   &ofstream_fn);
    i->createcommand("cvx_f",      &cvx_f_fn);
    i->createcommand("close",      &close_fn);
    i->createcommand("eof",        &eof_fn);
    i->createcommand("getline_is", &getline_is_fn);
    i->createcommand("ostrstream", &ostrstream_fn);
    i->createcommand("str",        &str_fn);
    i->createcommand("CopyFile",   &copyfile_fn);
}

}  // namespace sli3
