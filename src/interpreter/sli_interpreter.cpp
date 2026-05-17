#include "sli_interpreter.h"
#include "compose.hpp"
#include "sli_access_ops.h"
#include "sli_array_module.h"
#include "sli_container_ops.h"
#include "sli_control.h"
#include "sli_dicttype.h"
#include "sli_function.h"
#include "sli_op_bodies.h"
#include "sli_functiontype.h"
#include "sli_io_ops.h"
#include "sli_state_ops.h"
#include "sli_readline.h"
#include "sli_iostreamtype.h"
#include "sli_math.h"
#include "sli_module.h"
#include "sli_nametype.h"
#include "sli_numerics.h"
#include "sli_parser.h"
#include "sli_stack.h"
#include "sli_startup.h"
#include "sli_string.h"
#include "sli_stringtype.h"
#include "sli_trietype.h"
#include <algorithm>
#include <ostream>
#include <sstream>
#include "sli_typecheck.h"
#include <time.h>

/* BeginDocumentation
 Name: Pi - Value of the constant Pi= 3.1415...
 Synopsis:  Pi -> double
 Description: Pi yields an approximation with a precision of 12 digits.
 Author: Diesmann, Hehl
 FirstVersion: 10.6.99
 References:
 SeeAlso: E, sin, cos
*/

/* BeginDocumentation
 Name: E - Value of the Euler constant E=2.718...
 Synopsis:  E -> double
 Description: E is the result of the builtin function std::exp(1).
 The precision of this value is therefore system-dependent.

 Author: Diesmann, Hehl
 FirstVersion: 10.6.99
 SeeAlso: exp
*/

/* BeginDocumentation
 Name: errordict - pushes error dictionary on operand stack
 Synopsis: errordict -> dict
 Description:
  Pushes the dictionary object errordict on the operand stack.
  errordict is not an operator; it is a name in systemdict associated
  with the dictionary object.

  The flag newerror helps to distinguish
  between interrupts caused by call of
  stop and interrupts raised by raiseerror.

  The name command contains the name of the command which
  caused the most recent error.

  The flag recordstacks decides whether the state of the interpreter
  is saved on error.
  If reckordstacks is true, the following state objects are saved

  Operand stack    -> ostack
  Dictionary stack -> dstack
  Execution stack  -> estack

 Parameters: none
 Examples: errordict info -> shows errordict
 Remarks: commented  1.4.1999, Diesmann
 SeeAlso: raiseerror, raiseagain, info
 References: The Red Book 2nd. ed. p. 408
*/

namespace sli3 {

int signalflag = 0;

// Hybrid hot-op dispatch (2026-05-14): split into an inline
// ultra-hot switch and a function-pointer table for the rest. A
// previous 16-arm inline switch caused the dispatcher's jump table
// to grow enough to regress B2b by +12 % even though B2b only uses
// POP and ADD. The split keeps the 5 most-hit ops as direct inlined
// cases and pushes the warm 11 through one indirect call.
//
// Each hot_op_* helper is `static inline` in op_bodies.h; taking
// its address in *this* TU forces the compiler to emit per-TU
// out-of-line copies here. Other TUs (sli_math.cpp etc.) still
// inline the bodies in their *Function::execute() methods.
namespace {
using HotOpFn = void(*)(SLIInterpreter*);
constexpr HotOpFn hot_op_table[HOP_count] = {
    nullptr,            // HOP_NONE
    hot_op_pop,
    hot_op_dup,
    hot_op_exch,
    hot_op_add_ii,
    hot_op_add,
    hot_op_sub,
    hot_op_if,
    hot_op_def,
    hot_op_gt,
    hot_op_lt,
    hot_op_geq,
    hot_op_leq,
    hot_op_eq,
    hot_op_neq,
    hot_op_get,
    hot_op_put,
};
static_assert(sizeof(hot_op_table) / sizeof(hot_op_table[0]) == HOP_count,
              "hot_op_table out of sync with HotOpId");
} // namespace

SLIInterpreter::SLIInterpreter()
    : mark_name("mark"), iparse_name("::parse"),
      iparsestdin_name("::parsestdin"), ilookup_name("::lookup"),
      ipop_name("::pop"), iiterate_name("::executeprocedure"),
      iloop_name("::loop"), irepeat_name("::repeat"), ifor_name("::for"),
      iforallarray_name("::forall_a"),
      iforallindexedarray_name("::forallindexed_a"),
      iforallindexedstring_name("::forallindexed_s"),
      iforallstring_name("::forall_s"), pi_name("Pi"), e_name("E"),
      stop_name("stop"), end_name("end"), EndSymbol("EndSymbol"),
      null_name("null"), true_name("true"), false_name("false"),
      istopped_name("::stopped"), systemdict_name("systemdict"),
      userdict_name("userdict"), errordict_name("errordict"),
      quitbyerror_name("quitbyerror"), newerror_name("newerror"),
      errorname_name("errorname"), commandname_name("commandname"),
      signo_name("sys_signo"), recordstacks_name("recordstacks"),
      estack_name("estack"), ostack_name("ostack"), dstack_name("dstack"),
      commandstring_name("moduleinitializers"),
      interpreter_name("SLIInterpreter::execute"),
      ArgumentTypeError("ArgumentType"), StackUnderflowError("StackUnderflow"),
      UndefinedNameError("UndefinedName"),
      WriteProtectedError("WriteProtected"),
      DivisionByZeroError("DivisionByZero"), RangeCheckError("RangeCheck"),
      PositiveIntegerExpectedError("PositiveIntegerExpected"),
      BadIOError("BadIO"), StringStreamExpectedError("StringStreamExpected"),
      CycleGuardError("AllowedCyclesExceeded"), SystemSignal("SystemSignal"),
      BadErrorHandler("BadErrorHandler"), KernelError("KernelError"),
      InternalKernelError("InternalKernelError"), is_initialized_(false),
      debug_mode_(false), show_stack_(false), show_backtrace_(false),
      catch_errors_(false), opt_tailrecursion_(true), cycle_guard_(false),
      cycle_count_(0),
      cycle_restriction_(0), verbosity_level_(M_INFO), system_dict_(0),
      user_dict_(0), status_dict_(0), error_dict_(0), parser_(0),
      operand_stack_(1000), execution_stack_(1000), types_() {
  parser_ = new Parser();
  init();
  // Operator-usage statistics: SLI3_STATS=1 (or any non-empty,
  // non-"0" value) toggles per-call counting at construction.
  // The dump is invoked from sli_main when the program exits.
  if (char const *env = std::getenv("SLI3_STATS"))
    count_calls_ = env[0] && env[0] != '0';
}

SLIInterpreter::~SLIInterpreter() {
  for (size_t m = 0; m < modules_.size(); ++m)
    delete modules_[m];

  operand_stack_.clear();
  execution_stack_.clear();
  dictionary_stack_.clear();

  // Types must be deleted *after* all tokens are deleted.
  // otherwhise the Token desctructor will crash.
  // types_[] holds typeid-tagged pointers; strip the tag for `delete`
  // (passing a tagged pointer to operator delete is UB on x86-64
  // and a noise source for sanitizers).
  for (size_t t = 0; t < types_.size(); ++t)
    delete Token::unpack_type(types_[t]);
}

void SLIInterpreter::init() {
  init_types();
  init_message_tags();
  init_dictionaries();
  init_internal_functions();
  Token dict(types_[sli3::dictionarytype]);
  dict.data_.dict_val = user_dict_;
  dictionary_stack_.push(dict);
}

void SLIInterpreter::init_types() {
  types_.resize(num_sli_types, 0);
  /*
    The order in which these types are created must match the
    order in which the lables in the enum typeid (sli_type.h) are
    defined.

    Each freshly-allocated SLIType* is tagged with its typeid (top
    byte) before storage. Downstream code reads the tag via
    Token::tag() / SLIType::get_typeid(); only the destructor strips
    it back off for delete.
  */
  auto reg = [this](sli_typeid id, SLIType* raw) {
    types_[id] = Token::pack_type(raw, id);
  };
  reg(sli3::nulltype, new OperatorType<sli3::nulltype>(this, "nulltype"));
  reg(sli3::anytype, new OperatorType<sli3::anytype>(this, "anytype"));
  reg(sli3::integertype,
      new IntegerType(this, "integertype", sli3::integertype));
  reg(sli3::doubletype,
      new DoubleType(this, "doubletype", sli3::doubletype));
  reg(sli3::booltype, new BoolType(this, "booltype", sli3::booltype));
  reg(sli3::literaltype,
      new LiteralType(this, "literaltype", sli3::literaltype));
  reg(sli3::marktype, new MarkType(this, "marktype", sli3::marktype));
  reg(sli3::nametype, new NameType(this, "nametype", sli3::nametype));
  reg(sli3::symboltype,
      new SymbolType(this, "symboltype", sli3::symboltype));
  reg(sli3::stringtype,
      new StringType(this, "stringtype", sli3::stringtype));
  reg(sli3::arraytype, new ArrayType(this, "arraytype", sli3::arraytype));
  // Name "literalproceduretype" matches what NEST 2.x's
  // typeinit.sli / sli-init.sli use when keying type tries.
  reg(sli3::litproceduretype,
      new LitprocedureType(this, "literalproceduretype", sli3::litproceduretype));
  reg(sli3::proceduretype,
      new ProcedureType(this, "proceduretype", sli3::proceduretype));
  reg(sli3::dictionarytype,
      new DictionaryType(this, "dictionarytype", sli3::dictionarytype));
  reg(sli3::functiontype,
      new FunctionType(this, "functiontype", sli3::functiontype));
  reg(sli3::iiteratetype,
      new OperatorType<sli3::iiteratetype>(this, "proc_continue"));
  reg(sli3::irepeattype,
      new OperatorType<sli3::irepeattype>(this, "repeat_continue"));
  reg(sli3::ifortype,
      new OperatorType<sli3::ifortype>(this, "for_continue"));
  reg(sli3::quittype, new OperatorType<sli3::quittype>(this, "quit"));
  reg(sli3::iforalltype,
      new OperatorType<sli3::iforalltype>(this, "forall_continue"));
  // Phase 5: five iter helpers converted to TYPE markers (parallel
  // to iiteratetype / ifortype / iforalltype). body_walk handles
  // them inline; their entry ops push the type instead of a function
  // token, and the SLIFunction classes were deleted.
  reg(sli3::ilooptype,
      new OperatorType<sli3::ilooptype>(this, "loop_continue"));
  reg(sli3::iparsetype,
      new OperatorType<sli3::iparsetype>(this, "parse_continue"));
  reg(sli3::iforallstringtype,
      new OperatorType<sli3::iforallstringtype>(this, "forallstring_continue"));
  reg(sli3::iforallindexedarraytype,
      new OperatorType<sli3::iforallindexedarraytype>(this, "forallindexedarray_continue"));
  reg(sli3::iforallindexedstringtype,
      new OperatorType<sli3::iforallindexedstringtype>(this, "forallindexedstring_continue"));
  reg(sli3::trietype, new TrieType(this, "trietype", sli3::trietype));
  reg(sli3::istreamtype,
      new IstreamType(this, "istreamtype", sli3::istreamtype));
  reg(sli3::xistreamtype,
      new XIstreamType(this, "xistreamtype", sli3::xistreamtype));
  reg(sli3::ostreamtype,
      new OstreamType(this, "ostreamtype", sli3::ostreamtype));
}

void SLIInterpreter::init_message_tags() {
  message_tag_.clear();
  message_tag_.reserve(sli3::num_message_levels);
  message_tag_.push_back("M_ALL");
  message_tag_.push_back("M_DEBUG");
  message_tag_.push_back("M_STATUS");
  message_tag_.push_back("M_INFO");
  message_tag_.push_back("M_WARNING");
  message_tag_.push_back("M_ERROR");
  message_tag_.push_back("M_FATAL");
  message_tag_.push_back("M_QUIET");
}

void SLIInterpreter::init_dictionaries() {
  system_dict_ = new Dictionary();
  error_dict_ = new Dictionary();
  user_dict_ = new Dictionary();
  status_dict_ = new Dictionary();

  Token dict(types_[sli3::dictionarytype]);
  dict.data_.dict_val = system_dict_;
  system_dict_->insert(Name("systemdict"), dict);
  dictionary_stack_.push(dict);
  dictionary_stack_.set_basedict();
  dict.data_.dict_val = error_dict_;
  system_dict_->insert(Name("errordict"), dict);
  dict.data_.dict_val = user_dict_;
  system_dict_->insert(Name("userdict"), dict);
  dict.data_.dict_val = status_dict_;
  system_dict_->insert(Name("statusdict"), dict);
  for (unsigned int t_id = sli3::nulltype; t_id < sli3::symboltype; ++t_id) {
    SLIType *type = types_[t_id];
    if (type == 0)
      continue;
    Name t_name = type->get_typename();
    system_dict_->insert(t_name, new_token<sli3::integertype>(t_id));
  }
  dict.type_ = 0; // This prevents the token data from being cleared.
}

void SLIInterpreter::init_internal_functions(void) {
  // Phase 5: all iter helpers are now TYPE markers handled inline
  // by the dispatcher (body_walk for iiterate/ifor/iforall/iloop/
  // iforall_s/iforallindexed_a/iforallindexed_s; outer-switch case
  // for iparse). Each name binds to its type.
  system_dict_->insert(iparse_name, Token(types_[sli3::iparsetype]));
  system_dict_->insert(iloop_name, Token(types_[sli3::ilooptype]));
  system_dict_->insert(irepeat_name, Token(types_[sli3::irepeattype]));
  system_dict_->insert(Name("quit"), Token(types_[sli3::quittype]));
  system_dict_->insert(iforallarray_name, Token(types_[sli3::iforalltype]));
  system_dict_->insert(iforallstring_name, Token(types_[sli3::iforallstringtype]));
  system_dict_->insert(iforallindexedarray_name, Token(types_[sli3::iforallindexedarraytype]));
  system_dict_->insert(iforallindexedstring_name, Token(types_[sli3::iforallindexedstringtype]));
  createcommand("]", &arraycreatefunction);
  createcommand(">>", &dictconstructfunction);
  // Phase 5: both moved to new-ABI (their bodies were updated to
  // drop the trailing self-pop).
  arraycreatefunction.set_new_abi();
  dictconstructfunction.set_new_abi();

  createdouble(pi_name, numerics::pi);
  createdouble(e_name, numerics::e);
  init_slicontrol(this);
  init_slistack(this);
  init_slimath(this);
  init_slitypecheck(this);
  init_sliarray(this);
  init_container_ops(this);
  init_io_ops(this);
  init_state_ops(this);
  init_access_ops(this);
  // GNUreadline / GNUaddhistory must be registered BEFORE sli-init.sli
  // loads — the script's /executive definition gates on
  // `systemdict /GNUreadline known` and only picks the line-editing
  // branch if the names are present at that time.
  init_sli_readline(this);
  // Startup must run AFTER the other modules so it can prime the
  // execution stack with sli-init.sli — that script depends on the
  // operators registered above.
  init_slistartup(this, 0, nullptr);
}

int SLIInterpreter::startup() {
  int exitcode = EXIT_SUCCESS;

  if (!is_initialized_) {
    if (execution_stack_.load() > 0) {
      // Use the same dispatcher as the main run so trie / for /
      // forall / iiterate / etc. dispatch correctly during the
      // sli-init.sli bootstrap. exitlevel=0 → run until the
      // execution stack drains.
      exitcode = execute_dispatch_(0);
    }
    // Always flip the flag — a no-op startup (e.g. sli-init.sli
    // not found, so init_slistartup pushed nothing onto the
    // e-stack) still counts as initialized; otherwise
    // is_initialized() would lie about the interpreter state.
    is_initialized_ = true;
  }
  return exitcode;
}

void SLIInterpreter::clear_parser_context() {
  if (parser_)
    parser_->clear_context();
}

void SLIInterpreter::dump_stats(std::ostream &out) const {
  if (!count_calls_ || call_counts_.empty()) {
    out << "operator stats: none recorded "
        << "(set SLI3_STATS=1 to enable)\n";
    return;
  }
  // Walk system_dict and user_dict; pull names for any
  // functiontype / trietype value whose payload pointer was
  // counted. The same SLIFunction* may be bound under several
  // names (e.g. /pop and /;); list each binding separately.
  struct Row { std::string name; uint64_t count; unsigned tag; };
  std::vector<Row> rows;
  rows.reserve(call_counts_.size());
  std::unordered_map<void const*, uint64_t> remaining = call_counts_;

  auto scan = [&](Dictionary const *d) {
    if (!d) return;
    for (auto it = d->begin(); it != d->end(); ++it) {
      Token const &t = it->second;
      void const *key = nullptr;
      unsigned tag = t.tag();
      if (tag == sli3::functiontype) key = t.data_.func_val;
      else if (tag == sli3::trietype) key = t.data_.trie_val;
      else continue;
      auto found = remaining.find(key);
      if (found == remaining.end()) continue;
      rows.push_back({it->first.toString(), found->second, tag});
      remaining.erase(found);
    }
  };
  scan(system_dict_);
  scan(user_dict_);

  // Anything still in `remaining` was counted but no longer
  // resolves to a name (e.g. a function bound only via internal
  // tries, or a name overridden after counting). Emit as
  // <unbound:0xXXXX>.
  for (auto const &kv : remaining) {
    std::ostringstream os;
    os << "<unbound:" << kv.first << ">";
    rows.push_back({os.str(), kv.second, 0});
  }

  std::sort(rows.begin(), rows.end(),
            [](Row const &a, Row const &b) { return a.count > b.count; });

  out << "# operator usage (" << rows.size() << " bindings)\n";
  out << "#   count    name                              kind\n";
  for (auto const &r : rows) {
    char const *kind = (r.tag == sli3::functiontype) ? "fn"
                     : (r.tag == sli3::trietype)     ? "trie"
                                                     : "raw";
    out.width(10); out << r.count << "  ";
    out.width(34); out.setf(std::ios::left);
    out << r.name;
    out.unsetf(std::ios::left);
    out << "  " << kind << '\n';
  }
}

Token SLIInterpreter::read_token(std::istream &in) {
  Token t;
  if (not parser_->readToken(*this, in, t))
    throw SyntaxError();
  return t;
}

Name SLIInterpreter::get_current_name(void) const {
  // Axis I bundle step 1: prefer the dispatcher's transient
  // current_op_ record over the e-stack read. Today (step 1)
  // current_op_ stays nullptr because the dispatcher doesn't
  // write it yet; we fall through to the e-stack-top read
  // unchanged. Step 2 of the bundle will populate current_op_
  // before each fn->execute call so this branch becomes the
  // primary path.
  if (current_op_)
    return current_op_->get_name();
  if (execution_stack_.top().is_of_type(sli3::functiontype))
    return execution_stack_.top().data_.func_val->get_name();
  return interpreter_name;
}

void SLIInterpreter::raiseerror(Name err) {
  Name caller = get_current_name();
  // Phase 5: dispatcher always pre-pops the fn slot before
  // fn->execute, so we never pop here.
  raiseerror(caller, err);
}

void SLIInterpreter::raiseerror(char const err[]) { raiseerror(Name(err)); }

void SLIInterpreter::raiseerror(std::exception &err) {
  Name caller = get_current_name();
  Name command_n("command");

  assert(error_dict_ != NULL);
  // Phase 5: dispatcher pre-pops the fn slot. /command stays
  // whatever was there previously; new-ABI ops are uniquely
  // identified via /commandname (set in raiseerror(Name,Name)).

  // SLIException provide addtional information
  SLIException *slierr = dynamic_cast<SLIException *>(&err);

  if (slierr) {
    // err is a SLIException
    error_dict_->insert(Name("message"),
                        new_token<sli3::stringtype>(slierr->message()));
    raiseerror(caller, slierr->what());
  } else {
    // plain std::exception: turn what() output into message
    error_dict_->insert(Name("message"),
                        new_token<sli3::stringtype>(std::string(err.what())));
    raiseerror(caller, "C++Exception");
  }
}

void SLIInterpreter::raiseerror(Name cmd, Name err) {

  // All error related symbols are now in their correct dictionary,
  // the error dictionary $errordict ( see Bug #4)

  assert(error_dict_ != NULL);

  // Read newerror as a value, not just presence — the original code
  // used `lookup() == false` which means "name not found", but that
  // flag stays present after the first error and would mark every
  // subsequent independent error as a handler failure. Treat
  // "not present" or "present-and-false" as a normal error.
  Token nerr_tok;
  bool already_in_error = false;
  if (error_dict_->lookup(newerror_name, nerr_tok) &&
      nerr_tok.is_of_type(sli3::booltype) && nerr_tok.data_.bool_val) {
    already_in_error = true;
  }

  if (!already_in_error) {
    // Diagnostic: only fires at M_DEBUG verbosity, so the default
    // (M_INFO) stays clean. The bootstrap-debug bump is one
    // set_verbosity(M_DEBUG) call away. Originally was unconditional
    // and spammed stderr on every error.
    if (verbosity_level_ <= M_DEBUG) {
      std::cerr << "[raiseerror] " << cmd.toString() << " : " << err.toString()
                << "\n  ostack:";
      {
        size_t lim = std::min<size_t>(operand_stack_.load(), 5);
        for (size_t k = 0; k < lim; ++k) {
          std::cerr << "\n   [" << k << "] ";
          Token const &t = operand_stack_.pick(k);
          t.pprint(std::cerr);
          if (t.type_ && (t.tag() == sli3::nametype ||
                          t.tag() == sli3::literaltype ||
                          t.tag() == sli3::symboltype)) {
            std::cerr << " = " << Name(t.data_.name_val).toString();
          }
        }
      }
      std::cerr << "\n  estack:";
      {
        size_t lim = std::min<size_t>(execution_stack_.load(), 8);
        for (size_t k = 0; k < lim; ++k) {
          std::cerr << "\n   [" << k << "] ";
          Token const &t = execution_stack_.pick(k);
          t.pprint(std::cerr);
          // Resolve name handle to its string for readability.
          if (t.type_ && (t.tag() == sli3::nametype ||
                          t.tag() == sli3::literaltype ||
                          t.tag() == sli3::symboltype)) {
            std::cerr << " = " << Name(t.data_.name_val).toString();
          } else if (t.type_ &&
                     (t.tag() == sli3::proceduretype ||
                      t.tag() == sli3::litproceduretype) &&
                     t.data_.array_val) {
            std::cerr << " body: ";
            t.print(std::cerr);
          }
        }
      }
      std::cerr << '\n';
    }
    error_dict_->insert(newerror_name, new_token<sli3::booltype>(true));
    error_dict_->insert(errorname_name, new_token<sli3::literaltype>(err));
    error_dict_->insert(commandname_name, new_token<sli3::literaltype>(cmd));

    Token rs_tok;
    if (error_dict_->lookup(recordstacks_name, rs_tok) &&
        rs_tok.is_of_type(sli3::booltype) && rs_tok.data_.bool_val) {
      TokenArray old_dict_stack;
      dictionary_stack_.toArray(*this, old_dict_stack);

      error_dict_->insert(
          estack_name, new_token<sli3::arraytype>(execution_stack_.toArray()));
      error_dict_->insert(ostack_name,
                          new_token<sli3::arraytype>(operand_stack_.toArray()));
      error_dict_->insert(dstack_name,
                          new_token<sli3::arraytype>(old_dict_stack));
    }

    operand_stack_.push(new_token<sli3::literaltype>(cmd));
    execution_stack_.push(baselookup(stop_name));
  } else {
    // Error inside an error handler. Don't recurse — emit a
    // diagnostic, mark the error as consumed, and let `stop`
    // unwind to whatever stopped context (or the top-level
    // dispatcher).
    error_dict_->insert(newerror_name, new_token<sli3::booltype>(false));
    message(M_ERROR, "raiseerror",
            ("Error during error handling: " + std::string(err.toString()))
                .c_str());
    operand_stack_.push(new_token<sli3::literaltype>(BadErrorHandler));
    execution_stack_.push(baselookup(stop_name));
  }
}

void SLIInterpreter::raiseagain(void) {
  assert(error_dict_ != NULL);

  if (error_dict_->known(commandname_name)) {
    Token cmd_t = error_dict_->lookup(commandname_name);
    error_dict_->insert(newerror_name, new_token<sli3::booltype>(true));
    operand_stack_.push(cmd_t);
    execution_stack_.push(baselookup(stop_name));
  } else
    raiseerror(Name("raiseagain"), BadErrorHandler);
}

void SLIInterpreter::raisesignal(int sig) {
  Name caller = get_current_name();

  error_dict_->insert(signo_name, new_token<sli3::integertype>(sig));

  raiseerror(caller, SystemSignal);
}

void SLIInterpreter::print_error(Token cmd) {
  // Declare the variables where the information
  // about the error is stored.
  std::string errorname;
  std::ostringstream msg;

  // Read errorname from dictionary. raiseerror() stores it as a
  // literaltype Token (see raiseerror(Name,Name) in this file), so
  // resolve via the Name handle. Casting to std::string& would throw
  // TypeMismatch — and we are CALLED from handleerror in a stopped
  // context, so that secondary error becomes BadErrorHandler.
  //
  // Take Token copies up-front (Stage 3.5). The error_dict_->erase
  // calls below would otherwise invalidate references taken from
  // the dict; copying detaches the lifetime concern.
  if (error_dict_->known(errorname_name)) {
    Token t = error_dict_->lookup(errorname_name);
    if (t.is_of_type(sli3::stringtype)) {
      errorname = static_cast<std::string&>(t);
    } else if (t.is_of_type(sli3::literaltype) ||
               t.is_of_type(sli3::nametype) ||
               t.is_of_type(sli3::symboltype)) {
      errorname = Name(t.data_.name_val).toString();
    }
  }

  // Find the correct message for the errorname.

  // If errorname is equal to SystemError no message string
  // is printed. The if-else branching below follows the
  // syntax of the lib/sli/sli-init.sli function
  // /:print_error
  if (errorname == "SystemError") {
  } else if (errorname == "BadErrorHandler") {
    msg << ": The error handler of a stopped context "
        << "contained itself an error.";
  } else {
    // Read a pre-defined message from dictionary.
    if (error_dict_->known(Name("message"))) {
      Token msg_tok = error_dict_->lookup(Name("message"));
      msg << msg_tok;
      error_dict_->erase(Name("message"));
    }

    // Read /command but currently nothing else uses it -- the
    // legacy "print candidates for trietype" block was deleted in
    // Stage 3.6 (it referenced TrieDatum, the pre-rewrite trie API).
    if (error_dict_->known(Name("command"))) {
      error_dict_->erase(Name("command"));
    }
  }

  // Error message header is defined as "$errorname in $cmd". cmd is
  // pushed by raiseerror() as a literaltype Token holding the
  // command Name; resolve it the same way as errorname above.
  std::string from;
  if (cmd.is_of_type(sli3::stringtype)) {
    from = static_cast<std::string&>(cmd);
  } else if (cmd.is_of_type(sli3::literaltype) ||
             cmd.is_of_type(sli3::nametype) ||
             cmd.is_of_type(sli3::symboltype)) {
    from = Name(cmd.data_.name_val).toString();
  }

  // Print error.
  message(M_ERROR, from.c_str(), msg.str().c_str(), errorname.c_str());
}

int SLIInterpreter::execute(const std::string &cmdline) {
  int exitcode = startup();
  if (exitcode != EXIT_SUCCESS)
    return -1;

  operand_stack_.push(new_token<sli3::stringtype>(cmdline));
  execution_stack_.push(new_token<sli3::nametype>(Name("evalstring")));
  return execute_dispatch_();
}

int SLIInterpreter::execute(int /*unused*/) {
  startup();
  execution_stack_.push(new_token<sli3::quittype>());
  // /start is the SLI-defined entry point set up by sli-init.sli:
  // it inspects argv (via :commandline) and either calls executive
  // (interactive) or runs the named scripts and quits. executive
  // drives the prompt+readline+cst+exec loop.
  execution_stack_.push(new_token<sli3::nametype>(Name("start")));
  // execute_dispatch_ is the only execution mode. The pre-Phase-1
  // execute_ (plain) and execute_debug_ alternates have been
  // retired; the legacy `int` parameter is kept for source
  // compatibility but ignored.
  return execute_dispatch_();
}

int SLIInterpreter::execute_dispatch_(size_t exitlevel) {
  int exitcode = 0;
  static const Name exitcode_name("exitcode");
  const Token null_val = new_token<sli3::integertype>(0);
  SLIType *iiterate_t = get_type(sli3::iiteratetype);
  SLIType *proc_type  = get_type(sli3::proceduretype);
  // Phase 5 (final): the `sentinel` Token (nulltype placeholder for
  // old-ABI ops to absorb their self-pop) is gone. All ops are
  // new-ABI; the dispatcher always pre-pops the fn slot.
  if (status_dict_)
    (*status_dict_)[exitcode_name] = null_val;

  if (sli3::signalflag != 0) {
    return sli3::unknown_error;
  }

  size_t local_cycles = cycle_count_;
  try {
    do {
      try {
        while (execution_stack_.load() > exitlevel) {
          ++local_cycles;

        switch (execution_stack_.top().tag()) {
        case sli3::integertype:
        case sli3::doubletype:
        case sli3::booltype:
        case sli3::literaltype:
        case sli3::marktype:
        case sli3::stringtype:
        case sli3::arraytype:
        case sli3::dictionarytype:
          operand_stack_.push_move(execution_stack_.top());
          execution_stack_.pop();
          break;
        case sli3::nametype: {
          const Token &t = lookup(execution_stack_.top().data_.long_val);
          if (t.is_executable())
            execution_stack_.top() = t;
          else {
            operand_stack_.push(t);
            execution_stack_.pop();
          }
          break;
        }
        case sli3::trietype: {
          if (__builtin_expect(count_calls_, 0))
            ++call_counts_[execution_stack_.top().data_.trie_val];
          const Token &t =
              execution_stack_.top().data_.trie_val->lookup(operand_stack_);
          if (t.is_executable())
            execution_stack_.top() = t;
          else {
            operand_stack_.push(t);
            execution_stack_.pop();
          }
          break;
        }
        case sli3::litproceduretype:
          execution_stack_.top().type_ = proc_type;
          operand_stack_.push_move(execution_stack_.top());
          execution_stack_.pop();
          break;

        case sli3::functiontype: {
          SLIFunction* fn = execution_stack_.top().data_.func_val;
          if (__builtin_expect(count_calls_, 0))
            ++call_counts_[fn];
          current_op_ = fn;
          // Phase 5: all ops are new-ABI; pre-pop unconditionally.
          execution_stack_.pop();
          fn->execute(this);
          current_op_ = nullptr;
          break;
        }

        // ----------------------------------------------------------
        // Unified body-walk for all four iter types (iiterate /
        // irepeat / ifor / iforall). proc/pos_p are case-local so
        // the compiler register-allocates them across fn->execute
        // calls.
        //
        // Two design wins over the legacy four-case dispatcher
        // (deleted in slice 8 step 4):
        //   D1 cross-iter-type resume: after fn->execute the new
        //     top can be ANY iter type, not just the one we were
        //     walking — e.g. `for { repeat { } }`, when the inner
        //     repeat returns the outer for resumes inline.
        //   D1 multi-level body-exit cascade: when a body
        //     exhausts and the new top is itself an iter frame at
        //     body-exhaust, the cascade unwinds all stacked frames
        //     in one tight loop — no outer-switch round-trip per
        //     frame.
        // ----------------------------------------------------------
        case sli3::proceduretype:
          execution_stack_.push(null_val);
          execution_stack_.push(iiterate_t);
          // fall through to the unified body-walk case below.
          // [[fallthrough]]
        case sli3::iiteratetype:
        case sli3::irepeattype:
        case sli3::ifortype:
        case sli3::iforalltype:
        case sli3::ilooptype:                 // Phase 5
        case sli3::iforallstringtype:         // Phase 5
        case sli3::iforallindexedarraytype:   // Phase 5
        case sli3::iforallindexedstringtype: {// Phase 5
          TokenArray *proc = execution_stack_.pick(2).data_.array_val;
          long *pos_p = &execution_stack_.pick(1).data_.long_val;
        body_walk:
          if (proc->index_is_valid(*pos_p)) {
            const Token &t = proc->get((*pos_p)++);
            if (not t.is_executable()) {
              operand_stack_.push(t);
              if (proc->index_is_valid(*pos_p))
                goto body_walk;
              goto body_exhausted;
            }
            // Single-level TCO: only for iiterate
            // (control_flow_spec.md §3.3). irepeat/ifor/iforall
            // need their proc slot preserved across iterations.
            if (not proc->index_is_valid(*pos_p)
                && execution_stack_.pick(0).tag() == sli3::iiteratetype) {
              proc->add_reference();
              execution_stack_.pick(2) = t;
              proc->remove_reference();
              execution_stack_.pop(2);
              break;
            }
            if (t.tag() == sli3::functiontype) {
              SLIFunction* fn = t.data_.func_val;
              if (__builtin_expect(count_calls_, 0))
                ++call_counts_[fn];
              current_op_ = fn;
              // Hybrid dispatch (see hot_op_table comment above):
              //   - 5 ultra-hot ops handled inline (zero call cost)
              //   - HOP_NONE falls through to virtual-call path
              //   - everything else goes through hot_op_table[hop]
              //     (one indirect call; keeps the inline switch's
              //     jump table tight)
              switch (fn->hot_op()) {
                case HOP_POP:    hot_op_pop(this);    break;
                case HOP_DUP:    hot_op_dup(this);    break;
                case HOP_EXCH:   hot_op_exch(this);   break;
                case HOP_ADD_II: hot_op_add_ii(this); break;
                case HOP_ADD:    hot_op_add(this);    break;
                case HOP_NONE:   fn->execute(this); break;
                default:
                  hot_op_table[fn->hot_op()](this);
                  break;
              }
              current_op_ = nullptr;
              goto resume_iter;
            }
            if (t.tag() == sli3::nametype) {
              const Token &resolved = lookup(t.data_.long_val);
              if (resolved.tag() == sli3::functiontype) {
                SLIFunction* fn = resolved.data_.func_val;
                if (__builtin_expect(count_calls_, 0))
                  ++call_counts_[fn];
                current_op_ = fn;
                switch (fn->hot_op()) {
                  case HOP_POP:    hot_op_pop(this);    break;
                  case HOP_DUP:    hot_op_dup(this);    break;
                  case HOP_EXCH:   hot_op_exch(this);   break;
                  case HOP_ADD_II: hot_op_add_ii(this); break;
                  case HOP_ADD:    hot_op_add(this);    break;
                  case HOP_NONE:   fn->execute(this); break;
                  default:
                    hot_op_table[fn->hot_op()](this);
                    break;
                }
                current_op_ = nullptr;
                goto resume_iter;
              }
              if (resolved.is_executable()) {
                execution_stack_.push(resolved);
                break;
              }
              operand_stack_.push(resolved);
              goto body_walk;
            }
            // litproctype short-circuit: a literal procedure in
            // a body is data -- push it directly to the operand
            // stack as a proceduretype value, skipping the
            // estack round-trip through the outer switch. Saves
            // one ArrayType::add_reference / remove_reference
            // pair per litproc. Marked unlikely to keep the
            // compiler from re-laying-out the dispatcher's hot
            // path for benchmarks that have no litprocs in their
            // body (B2b is the canary -- a 1% regression appears
            // here without the hint).
            if (__builtin_expect(t.tag() == sli3::litproceduretype, 0)) {
              operand_stack_.push(t);
              operand_stack_.top().type_ = proc_type;
              if (proc->index_is_valid(*pos_p))
                goto body_walk;
              goto body_exhausted;
            }
            // proceduretype / trietype / other executable types:
            // push to e-stack and break to the outer switch. On
            // return (when the dispatcher re-enters via our case
            // label) proc/pos_p are re-loaded.
            execution_stack_.push(t);
            break;
          }

        body_exhausted:
          // Body has no more tokens. Dispatch the iter-type-
          // specific exhaust handler by reading the marker tag
          // at pick(0).
          switch (execution_stack_.pick(0).tag()) {
            case sli3::iiteratetype:
              execution_stack_.pop(3);
              break;
            case sli3::irepeattype: {
              long &lc = execution_stack_.pick(3).data_.long_val;
              if (lc > 0) {
                *pos_p = 0;
                --lc;
              } else {
                execution_stack_.pop(5);
              }
              break;
            }
            case sli3::ifortype: {
              long &counter = execution_stack_.pick(3).data_.long_val;
              long &lim     = execution_stack_.pick(4).data_.long_val;
              long &inc     = execution_stack_.pick(5).data_.long_val;
              if (((inc > 0) && (counter <= lim))
               || ((inc < 0) && (counter >= lim))) {
                *pos_p = 0;
                operand_stack_.push(execution_stack_.pick(3));
                counter += inc;
              } else {
                execution_stack_.pop(7);
              }
              break;
            }
            case sli3::iforalltype: {
              TokenArray *ad = execution_stack_.pick(4).data_.array_val;
              long &idx      = execution_stack_.pick(3).data_.long_val;
              if (ad->index_is_valid(idx)) {
                *pos_p = 0;
                operand_stack_.push(ad->get(idx));
                ++idx;
              } else {
                execution_stack_.pop(6);
              }
              break;
            }
            // Phase 5: loop / forall_s / forallindexed_a / forallindexed_s
            // converted from old-ABI iter functions to TYPE markers in
            // body_walk's exhaust arm.
            case sli3::ilooptype:
              // loop: reset pos and walk the body forever; only exit /
              // stop unwinds the frame.
              *pos_p = 0;
              break;
            case sli3::iforallstringtype: {
              // Frame: [mark, str, idx, proc, pos, iforallstringtype].
              SLIString *str = execution_stack_.pick(4).data_.string_val;
              long &idx      = execution_stack_.pick(3).data_.long_val;
              if (idx < static_cast<long>(str->str().size())) {
                *pos_p = 0;
                operand_stack_.push(get_type(sli3::integertype));
                operand_stack_.top().data_.long_val =
                    static_cast<long>(str->str()[idx]);
                ++idx;
              } else {
                execution_stack_.pop(6);
              }
              break;
            }
            case sli3::iforallindexedarraytype: {
              // Frame: [mark, ad, lim, count, proc, pos, type].
              TokenArray *ad = execution_stack_.pick(5).data_.array_val;
              long &count    = execution_stack_.pick(3).data_.long_val;
              long &lim      = execution_stack_.pick(4).data_.long_val;
              if (count < lim) {
                *pos_p = 0;
                operand_stack_.push(ad->get(count));
                operand_stack_.push(get_type(sli3::integertype));
                operand_stack_.top().data_.long_val = count;
                ++count;
              } else {
                execution_stack_.pop(7);
              }
              break;
            }
            case sli3::iforallindexedstringtype: {
              // Frame: [mark, str, lim, count, proc, pos, type].
              SLIString *str = execution_stack_.pick(5).data_.string_val;
              long &count    = execution_stack_.pick(3).data_.long_val;
              long &lim      = execution_stack_.pick(4).data_.long_val;
              if (count < lim) {
                *pos_p = 0;
                operand_stack_.push(get_type(sli3::integertype));
                operand_stack_.top().data_.long_val =
                    static_cast<long>(str->str()[count]);
                operand_stack_.push(get_type(sli3::integertype));
                operand_stack_.top().data_.long_val = count;
                ++count;
              } else {
                execution_stack_.pop(7);
              }
              break;
            }
            default: __builtin_unreachable();
          }
          // Fall through to resume_iter. The exhaust handler may
          // have:
          //   (a) popped the frame fully -- new top might be
          //       another iter frame (cascade), or anything else;
          //   (b) reset pos for next iteration -- top is the same
          //       iter marker; the cascade re-loads proc/pos_p
          //       to the same values and goto body_walk continues.

        resume_iter:
          // D1 cascade. After fn->execute or body-exhaust, the
          // new top might be any iter marker (same as we were,
          // or different). Re-load proc/pos_p and continue
          // walking. The switch is the only iter-type check;
          // matching any of the four jumps directly to body_walk
          // without an outer-switch round-trip.
          switch (execution_stack_.top().tag()) {
            case sli3::iiteratetype:
            case sli3::irepeattype:
            case sli3::ifortype:
            case sli3::iforalltype:
            case sli3::ilooptype:                 // Phase 5
            case sli3::iforallstringtype:         // Phase 5
            case sli3::iforallindexedarraytype:   // Phase 5
            case sli3::iforallindexedstringtype:  // Phase 5
              proc  = execution_stack_.pick(2).data_.array_val;
              pos_p = &execution_stack_.pick(1).data_.long_val;
              goto body_walk;
            default:
              break;
          }
          break;
        }
        case sli3::quittype:
          goto exit_interpreter;
        case sli3::iparsetype: {
          // Phase 5: was IparseFunction. The parser drives one token
          // off the stream and either pushes it (executable) or
          // dispatches it (data). When the stream returns
          // symboltype (EOF), pop the [xistream, iparsetype] frame.
          // Frame: pick(0) = iparsetype, pick(1) = xistream.
          SLIistream *is = execution_stack_.pick(1).data_.istream_val;
          Token t = read_token(*is->get());
          if (t.is_of_type(sli3::symboltype)) {
            execution_stack_.pop(2);
          } else {
            // Push the parsed token on top; the dispatcher runs it
            // (if executable) or pushes it to ostack (if data).
            // Either way, control returns to iparsetype on the next
            // cycle and we read the next token.
            execution_stack_.push(t);
          }
          break;
        }
        default:
          execution_stack_.top().execute();
        }
        }
      } catch (std::exception &exc) {
        cycle_count_ = local_cycles;
        raiseerror(exc);
        local_cycles = cycle_count_;
      }
    } while (execution_stack_.load() > exitlevel);
  } catch (std::exception &e) {
    cycle_count_ = local_cycles;
    message(M_FATAL, "SLIInterpreter", "A C++ library exception occured.");
    operand_stack_.dump(std::cerr);
    execution_stack_.dump(std::cerr);
    message(M_FATAL, "SLIInterpreter", e.what());
    terminate(sli3::exception);
  } catch (...) {
    cycle_count_ = local_cycles;
    message(M_FATAL, "SLIInterpreter", "An unknown C++ exception occured.");
    operand_stack_.dump(std::cerr);
    execution_stack_.dump(std::cerr);
    terminate(sli3::exception);
  }

exit_interpreter:
  cycle_count_ = local_cycles;

  if (status_dict_) {
    Token exit_tk = (*status_dict_)["exitcode"];
    if (exit_tk.is_of_type(sli3::integertype))
      exitcode = exit_tk.data_.long_val;
  }

  if (exitcode != 0 && error_dict_)
    error_dict_->insert(quitbyerror_name, new_token<sli3::booltype>(true));

  return exitcode;
}

void SLIInterpreter::createdouble(Name n, double val) {
  dictionary_stack_.def(n, new_token<sli3::doubletype>(val));
}

/** Define a function in the current dictionary.
 *  This function defines a SLI function in the current dictionary.
 *  Note that you may also pass a string as the first argument, as
 *  there is an implicit type conversion operator from string to Name.
 *  Use the Name when a name object for this function already
 *  exists.
 */
void SLIInterpreter::createcommand(Name n, SLIFunction *fn) {
  if (dictionary_stack_.known(n))
    throw NamingConflict("A function called '" + std::string(n.toString()) +
                         "' exists already.\n"
                         "Please choose a different name!");

  Token t(types_[sli3::functiontype]);
  t.data_.func_val = fn;
  fn->set_name(n);
  dictionary_stack_.def(n, t);
}

/**
    Define a constant in the current dictionary.
    This function defines a SLI constant in the current dictionary.
    Note that you may also pass a string as the first argument, as
    there is an implicit type conversion operator from string to Name.
    Use the Name when a name object for this function already
    exists.
*/
void SLIInterpreter::createconstant(Name n, Token const &val) {
  dictionary_stack_.def(n, val);
}

void SLIInterpreter::set_verbosity(int l) { verbosity_level_ = l; }

int SLIInterpreter::verbosity(void) const { return verbosity_level_; }

void SLIInterpreter::terminate(int returnvalue) {
  if (returnvalue == -1) {
    returnvalue = sli3::fatal;
  }

  message(sli3::M_FATAL, "SLIInterpreter", "Exiting.");
  // Do NOT `delete this` before std::exit. The process is about
  // to die anyway; the OS reclaims memory. Self-deleting here
  // leaves any static destructor that touches the interpreter
  // (or the Meyers singletons in sli_name.h that the destructor
  // chain unwinds) chasing a freed pointer.
  std::exit(returnvalue);
}

void SLIInterpreter::message(int level, const char from[], const char text[],
                             const char errorname[]) const {
  if (level >= verbosity_level_) {
    if (level >= M_FATAL) {
      // Take care that std::cerr and std::cout don't
      // send output to the same source. This would
      // lead to duplication of the message.
      message(std::cout, message_tag_[M_FATAL].c_str(), from, text, errorname);
    } else if (level >= M_ERROR) {
      message(std::cout, message_tag_[M_ERROR].c_str(), from, text, errorname);

    } else if (level >= M_WARNING) {
      message(std::cout, message_tag_[M_WARNING].c_str(), from, text,
              errorname);
    } else if (level >= M_INFO) {
      message(std::cout, message_tag_[M_INFO].c_str(), from, text, errorname);
    } else if (level >= M_STATUS) {
      message(std::cout, message_tag_[M_STATUS].c_str(), from, text, errorname);
    } else if (level >= M_DEBUG) {
      message(std::cout, message_tag_[M_DEBUG].c_str(), from, text, errorname);
    } else {
      message(std::cout, message_tag_[M_ALL].c_str(), from, text, errorname);
    }
  }
}

void SLIInterpreter::message(std::ostream &out, const char levelname[],
                             const char from[], const char text[],
                             const char errorname[]) const {
  const unsigned buflen = 30;
  char timestring[buflen + 1] = "";
  const time_t tm = ::time(NULL);

  ::strftime(timestring, buflen, "%b %d %H:%M:%S", ::localtime(&tm));

  std::string msg =
      String::compose("%1 %2 [%3]: ", timestring, from, levelname);
  out << std::endl << msg << errorname;

  // Set the preferred line indentation.
  const size_t indent = 4;

  // Get size of the output window. The message text will be
  // adapted to the width of the window.
  //
  // The COLUMNS variable should preferably be extracted
  // from the environment dictionary set up by the
  // Processes class. getenv("COLUMNS") works only on
  // the created NEST executable (not on the messages
  // printed by make install).
  char const *const columns = std::getenv("COLUMNS");
  size_t max_width = 78;
  if (columns)
    max_width = std::atoi(columns);
  if (max_width < 3 * indent)
    max_width = 3 * indent;
  const size_t width = max_width - indent;

  // convert char* to string to be able to use the string functions
  std::string text_str(text);

  // Indent first message line
  if (text_str.size() != 0) {
    std::cout << std::endl << std::string(indent, ' ');
  }

  size_t pos = 0;

  for (size_t i = 0; i < text_str.size(); ++i) {
    if (text_str.at(i) == '\n' && i != text_str.size() - 1) {
      // Print a lineshift followed by an indented whitespace
      // Manually inserted lineshift at the end of the message
      // are suppressed.
      out << std::endl << std::string(indent, ' ');
      pos = 0;
    } else {
      // If we've reached the width of the output we'll print
      // a lineshift regardless of whether '\n' is found or not.
      // The printing is done so that no word splitting occurs.
      size_t space = text_str.find(' ', i) < text_str.find('\n')
                         ? text_str.find(' ', i)
                         : text_str.find('\n');
      // If no space is found (i.e. the last word) the space
      // variable is set to the end of the string.
      if (space == std::string::npos) {
        space = text_str.size();
      }

      // Start on a new line if the next word is longer than the
      // space available (as long as the word is shorter than the
      // total width of the printout).
      if (i != 0 && text_str.at(i - 1) == ' ' &&
          static_cast<int>(space - i) > static_cast<int>(width - pos)) {
        out << std::endl << std::string(indent, ' ');
        pos = 0;
      }

      // Only print character if we're not at the end of the
      // line and the last character is a space.
      if (!(width - pos == 0 && text_str.at(i) == ' ')) {
        // Print the actual character.
        out << text_str.at(i);
      }

      ++pos;
    }
  }
  out << std::endl;
}

} // namespace sli3
