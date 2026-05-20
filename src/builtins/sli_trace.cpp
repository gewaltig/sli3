#include "sli_trace.h"

#include "sli_array.h"
#include "sli_dictionary.h"
#include "sli_interpreter.h"
#include "sli_name.h"
#include "sli_string.h"
#include "sli_token.h"
#include "sli_type.h"

#include <iostream>
#include <sstream>
#include <string>
#include <unordered_set>
#include <variant>
#include <vector>

namespace sli3
{

/* BeginDocumentation
   Name: trace - pretty-print the errordict /estack snapshot.

   Synopsis: trace -> -

   Description:
     trace reads the /estack entry stored in errordict by the most
     recent raiseerror and prints one line per execution-stack
     *frame* (rather than per slot). Each frame is decoded by its
     continuation-marker tag — proc, repeat, for, forall, loop, etc.
     — and annotated with the operator that was last dispatched
     (`body[pos-1]`).

     The innermost frame (the one that holds the failing op) is
     prefixed with `=>`; the rest are indented. Bottom-of-stack
     frames whose body matches a procedure reachable from
     /executive or /start (i.e. the REPL baseline) are collapsed
     into a single trailing `... <REPL>` line.

     If errordict has no /estack snapshot (recordstacks is off or
     no error has fired), trace prints a short notice and returns.

     By default the dispatcher tail-call-optimizes the last slot
     of a procedure body. To keep every frame for /trace, call
     /backtrace_on once *before* triggering the error; /backtrace_off
     restores the default.

   Examples:
     backtrace_on   %% keep full call chain on the next error
     (s) (t) add    %% raises ArgumentType, snapshots estack
     trace

   SeeAlso: raiseerror, resume, errordict, backtrace_on, backtrace_off
*/
namespace
{

struct ForState  { long counter, lim, inc; };
struct LoopState { long remaining; };
struct IdxState  { long idx, lim;
                   TokenArray* arr; SLIString* str; };
struct ParseState {};
struct NoState   {};

using FrameState = std::variant<NoState, ForState, LoopState,
                                IdxState, ParseState>;

struct Frame {
    unsigned int marker_tag = sli3::nulltype;
    TokenArray*  body       = nullptr;
    long         pos        = 0;
    long         marker_idx = -1;
    bool         is_stray   = false;
    Token        stray;
    FrameState   state;
};

struct MarkerDesc {
    unsigned int tag;
    char const*  label;
    long         slots;
    void       (*extract)(TokenArray const&, long k, Frame&);
};

void extract_iter(TokenArray const& s, long k, Frame& f) {
    f.pos   = s[k - 1].data_.long_val;
    f.body  = s[k - 2].data_.array_val;
    f.state = NoState{};
}

void extract_repeat(TokenArray const& s, long k, Frame& f) {
    f.pos   = s[k - 1].data_.long_val;
    f.body  = s[k - 2].data_.array_val;
    f.state = LoopState{ s[k - 3].data_.long_val };
}

void extract_for(TokenArray const& s, long k, Frame& f) {
    f.pos   = s[k - 1].data_.long_val;
    f.body  = s[k - 2].data_.array_val;
    f.state = ForState{ s[k - 3].data_.long_val,
                        s[k - 4].data_.long_val,
                        s[k - 5].data_.long_val };
}

void extract_forall_a(TokenArray const& s, long k, Frame& f) {
    f.pos  = s[k - 1].data_.long_val;
    f.body = s[k - 2].data_.array_val;
    TokenArray* arr = s[k - 4].data_.array_val;
    f.state = IdxState{ s[k - 3].data_.long_val,
                        arr ? static_cast<long>(arr->size()) : 0,
                        arr, nullptr };
}

void extract_forall_s(TokenArray const& s, long k, Frame& f) {
    f.pos  = s[k - 1].data_.long_val;
    f.body = s[k - 2].data_.array_val;
    SLIString* str = s[k - 4].data_.string_val;
    f.state = IdxState{ s[k - 3].data_.long_val,
                        str ? static_cast<long>(str->str().size()) : 0,
                        nullptr, str };
}

void extract_forallidx_a(TokenArray const& s, long k, Frame& f) {
    f.pos  = s[k - 1].data_.long_val;
    f.body = s[k - 2].data_.array_val;
    f.state = IdxState{ s[k - 3].data_.long_val,
                        s[k - 4].data_.long_val,
                        s[k - 5].data_.array_val, nullptr };
}

void extract_forallidx_s(TokenArray const& s, long k, Frame& f) {
    f.pos  = s[k - 1].data_.long_val;
    f.body = s[k - 2].data_.array_val;
    f.state = IdxState{ s[k - 3].data_.long_val,
                        s[k - 4].data_.long_val,
                        nullptr, s[k - 5].data_.string_val };
}

void extract_parse(TokenArray const&, long, Frame& f) {
    f.state = ParseState{};
}

MarkerDesc const markers[] = {
    { sli3::iiteratetype,             "proc",        3, extract_iter        },
    { sli3::ilooptype,                "loop",        3, extract_iter        },
    { sli3::irepeattype,              "repeat",      4, extract_repeat      },
    { sli3::ifortype,                 "for",         6, extract_for         },
    { sli3::iforalltype,              "forall",      5, extract_forall_a    },
    { sli3::iforallstringtype,        "forall_s",    5, extract_forall_s    },
    { sli3::iforallindexedarraytype,  "forallidx",   6, extract_forallidx_a },
    { sli3::iforallindexedstringtype, "forallidx_s", 6, extract_forallidx_s },
    { sli3::iparsetype,               "parse",       2, extract_parse       },
};

MarkerDesc const* find_marker(unsigned int tag) {
    for (auto const& m : markers)
        if (m.tag == tag) return &m;
    return nullptr;
}

// REPL baseline: procs reachable from /executive and /start. Re-resolved
// by Name each /trace call so a redefinition of /executive doesn't leave
// the trace machinery holding a dangling TokenArray*.
using BodySet = std::unordered_set<TokenArray*>;

void collect_proc_bodies(TokenArray* body, BodySet& out) {
    if (!body || out.count(body)) return;
    out.insert(body);
    for (size_t k = 0; k < body->size(); ++k) {
        Token const& s = (*body)[k];
        if (!s.type_) continue;
        unsigned int tag = s.tag();
        if ((tag == sli3::proceduretype || tag == sli3::litproceduretype) &&
            s.data_.array_val) {
            collect_proc_bodies(s.data_.array_val, out);
        }
    }
}

BodySet baseline_bodies(SLIInterpreter* i) {
    BodySet out;
    for (char const* seed : { "executive", "start" }) {
        Token t;
        if (!i->lookup(Name(seed), t)) continue;
        unsigned int tag = t.tag();
        if (tag != sli3::proceduretype && tag != sli3::litproceduretype)
            continue;
        collect_proc_bodies(t.data_.array_val, out);
    }
    return out;
}

bool is_baseline(Frame const& f, BodySet const& bodies) {
    if (std::holds_alternative<ParseState>(f.state)) return true;
    if (f.is_stray) return true;
    if (!f.body) return false;
    return bodies.count(f.body) > 0;
}

void print_token_compact(std::ostream& os, Token const& t) {
    if (!t.type_) { os << "/nulltoken"; return; }
    unsigned int tag = t.tag();
    if (tag == sli3::nametype) {
        os << Name(t.data_.name_val).toString();
        return;
    }
    if (tag == sli3::literaltype || tag == sli3::symboltype) {
        os << "/" << Name(t.data_.name_val).toString();
        return;
    }
    t.pprint(os);
}

void print_token_capped(std::ostream& os, Token const& t, size_t max_chars) {
    std::ostringstream tmp;
    print_token_compact(tmp, t);
    std::string s = tmp.str();
    if (s.size() <= max_chars) { os << s; return; }
    if (!s.empty() && s.front() == '{') { os << "{...}"; return; }
    if (!s.empty() && s.front() == '[') { os << "[...]"; return; }
    os << s.substr(0, max_chars) << "...";
}

void print_body_window(std::ostream& os, TokenArray* body, long focus,
                       long before, long after) {
    if (!body) { os << "<null>"; return; }
    long n = static_cast<long>(body->size());
    if (n == 0) return;
    if (focus < 0) focus = 0;
    if (focus >= n) focus = n - 1;
    long start = std::max<long>(0, focus - before);
    long end   = std::min<long>(n, focus + after + 1);
    if (start > 0) os << "... ";
    for (long k = start; k < end; ++k) {
        if (k > start) os << ' ';
        print_token_capped(os, (*body)[k], /*max_chars=*/20);
    }
    if (end < n) os << " ...";
}

std::vector<Frame> decode_snapshot(TokenArray const& snap) {
    std::vector<Frame> frames;
    long k = static_cast<long>(snap.size()) - 1;
    while (k >= 0) {
        Token const& top = snap[k];
        unsigned int tag = top.type_ ? top.tag() : static_cast<unsigned int>(sli3::nulltype);
        MarkerDesc const* m = find_marker(tag);
        if (m && k >= m->slots - 1) {
            Frame f;
            f.marker_tag = tag;
            f.marker_idx = k;
            m->extract(snap, k, f);
            k -= m->slots;
            frames.push_back(f);
            continue;
        }
        Frame s;
        s.is_stray   = true;
        s.stray      = top;
        s.marker_idx = k;
        s.state      = NoState{};
        frames.push_back(s);
        --k;
    }
    return frames;
}

// Frames are in most-recent-first order. The immediate predecessor in
// stack order is at index my_index - 1, plus any strays we need to skip.
std::string detect_branch(Frame const& f, std::vector<Frame> const& frames,
                          size_t my_index) {
    if (!f.body || f.pos < 1 || f.pos > static_cast<long>(f.body->size()))
        return "";
    long focus = f.pos - 1;
    Token const& called = (*f.body)[static_cast<size_t>(focus)];
    if (called.tag() != sli3::nametype) return "";
    std::string name = Name(called.data_.name_val).toString();
    bool is_if     = (name == "if");
    bool is_ifelse = (name == "ifelse");
    if (!is_if && !is_ifelse) return "";

    Frame const* above = nullptr;
    for (size_t k = my_index; k-- > 0; ) {
        Frame const& c = frames[k];
        if (c.is_stray) continue;
        if (std::holds_alternative<ParseState>(c.state)) continue;
        if (!c.body) continue;
        above = &c;
        break;
    }
    if (!above) return "";
    TokenArray* chosen = above->body;
    if (!chosen) return "";

    auto slot_proc_matches = [&](long idx) -> bool {
        if (idx < 0 || idx >= static_cast<long>(f.body->size())) return false;
        Token const& p = (*f.body)[static_cast<size_t>(idx)];
        if (p.tag() != sli3::proceduretype && p.tag() != sli3::litproceduretype)
            return false;
        return p.data_.array_val == chosen;
    };

    if (is_if) {
        return slot_proc_matches(focus - 1) ? "branch taken" : "";
    }
    if (slot_proc_matches(focus - 2)) return "true branch";
    if (slot_proc_matches(focus - 1)) return "false branch";
    return "";
}

void emit_frame(std::ostream& os, Frame const& f, bool is_top,
                std::string const& branch_info) {
    os << (is_top ? "=> " : "   ");

    if (f.is_stray) {
        os << "pending: ";
        if (f.stray.type_) f.stray.print(os);
        else               os << "/nulltoken";
        os << '\n';
        return;
    }

    if (std::holds_alternative<ParseState>(f.state)) {
        os << "In parse <input stream>\n";
        return;
    }

    std::ostringstream state;
    std::visit([&](auto const& s) {
        using T = std::decay_t<decltype(s)>;
        if constexpr (std::is_same_v<T, ForState>) {
            state << " (i=" << s.counter
                  << " lim=" << s.lim << " inc=" << s.inc << ")";
        } else if constexpr (std::is_same_v<T, LoopState>) {
            state << " (iter remaining=" << s.remaining << ")";
        } else if constexpr (std::is_same_v<T, IdxState>) {
            state << " (idx " << s.idx << "/" << s.lim << ")";
        }
    }, f.state);

    MarkerDesc const* m = find_marker(f.marker_tag);
    char const* label = m ? m->label : "?";

    if (f.body) {
        long total = static_cast<long>(f.body->size());
        os << "In step " << f.pos << "/" << total
           << " of " << label << state.str()
           << " { ";
        print_body_window(os, f.body, f.pos - 1, /*before=*/3, /*after=*/2);
        os << " }";
        if (f.pos >= 1 && f.pos <= total) {
            Token const& called = (*f.body)[static_cast<size_t>(f.pos - 1)];
            os << " calling: ";
            print_token_compact(os, called);
            if (!branch_info.empty()) os << " [" << branch_info << "]";
        } else if (f.pos == 0) {
            os << " (about to start)";
        }
    } else {
        os << "In " << label << state.str();
    }
    os << '\n';
}

}  // anonymous namespace

void TraceFunction::execute(SLIInterpreter* i) const {
    Dictionary& ed = i->error_dict();
    Token est_tok;
    if (!ed.lookup(Name("estack"), est_tok) ||
        est_tok.tag() != sli3::arraytype) {
        std::cout << "trace: no execution-stack snapshot recorded.\n"
                     "       Either recordstacks is off, or no error has fired yet.\n";
        return;
    }
    TokenArray* snap = est_tok.data_.array_val;
    if (!snap || snap->size() == 0) {
        std::cout << "trace: empty estack snapshot.\n";
        return;
    }

    BodySet bodies = baseline_bodies(i);
    std::vector<Frame> frames = decode_snapshot(*snap);

    size_t baseline_start = frames.size();
    while (baseline_start > 0 && is_baseline(frames[baseline_start - 1], bodies))
        --baseline_start;

    std::cout << "Execution stack at error (most recent first):\n";
    bool any = false;
    for (size_t k = 0; k < baseline_start; ++k) {
        std::string branch = detect_branch(frames[k], frames, k);
        emit_frame(std::cout, frames[k], !any, branch);
        any = true;
    }
    if (!any) {
        Token cmd_tok, err_tok;
        bool has_cmd = ed.lookup(Name("commandname"), cmd_tok);
        bool has_err = ed.lookup(Name("errorname"),   err_tok);
        if (has_cmd || has_err) {
            std::cout << "=> ";
            if (has_cmd && (cmd_tok.tag() == sli3::literaltype ||
                            cmd_tok.tag() == sli3::nametype ||
                            cmd_tok.tag() == sli3::symboltype)) {
                std::cout << "calling: " << Name(cmd_tok.data_.name_val).toString();
            }
            if (has_err && (err_tok.tag() == sli3::literaltype ||
                            err_tok.tag() == sli3::nametype ||
                            err_tok.tag() == sli3::symboltype)) {
                std::cout << "   (raised /" << Name(err_tok.data_.name_val).toString() << ")";
            }
            std::cout << "   [frame elided by tail-call;"
                         " run `backtrace_on` before the error to keep it]\n";
        }
    }
    if (baseline_start < frames.size())
        std::cout << "   ... <REPL>\n";
}

namespace {
TraceFunction trace_fn;
}

void init_trace(SLIInterpreter* i) {
    i->createcommand("trace", &trace_fn);
}

}  // namespace sli3
