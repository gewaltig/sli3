#include "sli_interpreter.h"
#include "compose.hpp"
#include "sli_array_module.h"
#include "sli_container_ops.h"
#include "sli_control.h"
#include "sli_dicttype.h"
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
      call_depth_(0), max_call_depth_(10), cycle_count_(0),
      cycle_restriction_(0), verbosity_level_(M_INFO), system_dict_(0),
      user_dict_(0), status_dict_(0), error_dict_(0), parser_(0),
      operand_stack_(1000), execution_stack_(1000), types_() {
  init();
  parser_ = new Parser();
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
  for (size_t t = 0; t < types_.size(); ++t)
    delete types_[t];
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
  */
  types_[sli3::nulltype] = (new OperatorType<sli3::nulltype>(this, "nulltype"));
  types_[sli3::anytype] = (new OperatorType<sli3::anytype>(this, "anytype"));
  types_[sli3::integertype] =
      (new IntegerType(this, "integertype", sli3::integertype));
  types_[sli3::doubletype] =
      (new DoubleType(this, "doubletype", sli3::doubletype));
  types_[sli3::booltype] = (new BoolType(this, "booltype", sli3::booltype));
  types_[sli3::literaltype] =
      (new LiteralType(this, "literaltype", sli3::literaltype));
  types_[sli3::marktype] = (new MarkType(this, "marktype", sli3::marktype));
  types_[sli3::nametype] = (new NameType(this, "nametype", sli3::nametype));
  types_[sli3::symboltype] =
      (new SymbolType(this, "symboltype", sli3::symboltype));
  types_[sli3::stringtype] =
      (new StringType(this, "stringtype", sli3::stringtype));
  types_[sli3::arraytype] = (new ArrayType(this, "arraytype", sli3::arraytype));
  // Name "literalproceduretype" matches what NEST 2.x's
  // typeinit.sli / sli-init.sli use when keying type tries.
  types_[sli3::litproceduretype] = (new LitprocedureType(
      this, "literalproceduretype", sli3::litproceduretype));
  types_[sli3::proceduretype] =
      (new ProcedureType(this, "proceduretype", sli3::proceduretype));
  types_[sli3::dictionarytype] =
      (new DictionaryType(this, "dictionarytype", sli3::dictionarytype));
  types_[sli3::functiontype] =
      (new FunctionType(this, "functiontype", sli3::functiontype));
  types_[sli3::iiteratetype] =
      (new OperatorType<sli3::iiteratetype>(this, "proc_continue"));
  types_[sli3::irepeattype] =
      (new OperatorType<sli3::irepeattype>(this, "repeat_continue"));
  types_[sli3::ifortype] =
      (new OperatorType<sli3::ifortype>(this, "for_continue"));
  types_[sli3::quittype] = (new OperatorType<sli3::quittype>(this, "quit"));
  types_[sli3::iforalltype] =
      (new OperatorType<sli3::iforalltype>(this, "forall_continue"));
  types_[sli3::trietype] = (new TrieType(this, "trietype", sli3::trietype));
  types_[sli3::istreamtype] =
      (new IstreamType(this, "istreamtype", sli3::istreamtype));
  types_[sli3::xistreamtype] =
      (new XIstreamType(this, "xistreamtype", sli3::xistreamtype));
  types_[sli3::ostreamtype] =
      (new OstreamType(this, "ostreamtype", sli3::ostreamtype));
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
  createcommand(iparse_name, &iparsefunction);
  createcommand(iparsestdin_name, &iparsestdinfunction);
  createcommand(ilookup_name, &ilookupfunction);
  createcommand(ipop_name, &ilookupfunction);
  createcommand(iiterate_name, &iiteratefunction);
  createcommand(iloop_name, &iloopfunction);
  system_dict_->insert(irepeat_name, Token(types_[sli3::irepeattype]));
  system_dict_->insert(Name("quit"), Token(types_[sli3::quittype]));
  system_dict_->insert(iforallarray_name, Token(types_[sli3::iforalltype]));
  createcommand(ifor_name, &iforfunction);
  createcommand(iforallindexedstring_name, &iforallindexedstringfunction);
  createcommand(iforallindexedarray_name, &iforallindexedarrayfunction);
  createcommand(iforallstring_name, &iforallstringfunction);
  createcommand("]", &arraycreatefunction);
  createcommand(">>", &dictconstructfunction);

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

  if (!is_initialized_ and execution_stack_.load() > 0) {
    // Use the same dispatcher as the main run so trie / for /
    // forall / iiterate / etc. dispatch correctly during the
    // sli-init.sli bootstrap. exitlevel=0 → run until the
    // execution stack drains.
    exitcode = execute_dispatch_(0);
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
  if (execution_stack_.top().is_of_type(sli3::functiontype))
    return execution_stack_.top().data_.func_val->get_name();
  //	TrieDatum *trie=dynamic_cast<TrieDatum *>(EStack.top().datum());
  //	if (trie !=NULL)
  //	    return(trie->getname());
  return interpreter_name;
}

void SLIInterpreter::raiseerror(Name err) {
  Name caller = get_current_name();
  execution_stack_.pop();
  raiseerror(caller, err);
}

void SLIInterpreter::raiseerror(char const err[]) { raiseerror(Name(err)); }

void SLIInterpreter::raiseerror(std::exception &err) {
  Name caller = get_current_name();
  Name command_n("command");

  assert(error_dict_ != NULL);
  // Read the failing op (top of e-stack) into /command BEFORE
  // popping. The Name overload pops in raiseerror(Name); we mirror
  // that here so both error-entry paths leave the e-stack at the
  // same depth (Stage 3.3). Without the pop, the failing op stays
  // on the estack with /cmd and stop on top -- when stop unwinds,
  // the op would be re-executed.
  error_dict_->insert(command_n, execution_stack_.top());
  execution_stack_.pop();

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
          if (t.type_ && (t.type_->get_typeid() == sli3::nametype ||
                          t.type_->get_typeid() == sli3::literaltype ||
                          t.type_->get_typeid() == sli3::symboltype)) {
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
          if (t.type_ && (t.type_->get_typeid() == sli3::nametype ||
                          t.type_->get_typeid() == sli3::literaltype ||
                          t.type_->get_typeid() == sli3::symboltype)) {
            std::cerr << " = " << Name(t.data_.name_val).toString();
          } else if (t.type_ &&
                     (t.type_->get_typeid() == sli3::proceduretype ||
                      t.type_->get_typeid() == sli3::litproceduretype) &&
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
  return execute_(); // run the interpreter
}

int SLIInterpreter::execute(int v) {
  startup();
  execution_stack_.push(new_token<sli3::quittype>());
  // /start is the SLI-defined entry point set up by sli-init.sli:
  // it inspects argv (via :commandline) and either calls executive
  // (interactive) or runs the named scripts and quits. executive
  // drives the prompt+readline+cst+exec loop.
  execution_stack_.push(new_token<sli3::nametype>(Name("start")));
  switch (v) {
  case 0:
    return execute_(); // run the interpreter
  case 1:
    return execute_debug_(); // run the interpreter in debug mode
  case 2:
    return execute_dispatch_();
  default:
    return -1;
  }
}

int SLIInterpreter::execute_(size_t exitlevel) {
  int exitcode = 0;

  if (sli3::signalflag != 0) {
    return sli3::unknown_error;
  }

  try {
    do { // loop1  this double loop to keep the try/catch outside the inner loop
      try {
        while (!sli3::signalflag and
               execution_stack_.load() > exitlevel) // loop 2
        {
          ++cycle_count_;
          if (cycle_guard_ and cycle_count_ > cycle_restriction_) {
            return 0;
          }
          execution_stack_.top().execute();
        }
      } catch (std::exception &exc) {
        // message(M_FATAL, "SLIInterpreter","A C++ library exception was
        // caught.");
        raiseerror(exc);
      }
    } while (execution_stack_.load() > exitlevel);
  } catch (std::exception &e) {
    message(M_FATAL, "SLIInterpreter", "A C++ library exception occured.");
    operand_stack_.dump(std::cerr);
    execution_stack_.dump(std::cerr);
    message(M_FATAL, "SLIInterpreter", e.what());
    terminate(sli3::exception);
  } catch (...) {
    message(M_FATAL, "SLIInterpreter", "An unknown c++ exception occured.");
    operand_stack_.dump(std::cerr);
    execution_stack_.dump(std::cerr);
    terminate(sli3::exception);
  }

  // Token &exit_tk=	status_dict_->lookup(Name("exitcode")); // This throws
  // an exception if the entry is not found. exitcode = exit_tk.data_.long_val;

  // if (exitcode != 0)
  //   error_dict_->insert(quitbyerror_name,new_token<sli3::booltype>(true));

  return exitcode;
}

int SLIInterpreter::execute_debug_(size_t exitlevel) {
  int exitcode;

  if (sli3::signalflag != 0) {
    return sli3::unknown_error;
  }

  try {
    do { // loop1  this double loop to keep the try/catch outside the inner loop
      try {
        while (!sli3::signalflag and
               execution_stack_.load() > exitlevel) // loop 2
        {
          ++cycle_count_;
          if (cycle_guard_ and cycle_count_ > cycle_restriction_) {
            return 0;
          }
          if (operand_stack_.load() > 0)
            operand_stack_.dump(std::cerr);
          execution_stack_.top().execute();
        }
      } catch (std::exception &exc) {
        message(M_ERROR, "SLIInterpreter",
                "A C++ library exception was caught.");
        operand_stack_.dump(std::cerr);
        execution_stack_.dump(std::cerr);
        message(M_ERROR, "SLIInterpreter", exc.what());
        // raiseerror(exc);
        execution_stack_.pop();
      }
    } while (execution_stack_.load() > exitlevel);
  } catch (std::exception &e) {
    message(M_FATAL, "SLIInterpreter", "A C++ library exception occured.");
    operand_stack_.dump(std::cerr);
    execution_stack_.dump(std::cerr);
    message(M_FATAL, "SLIInterpreter", e.what());
    terminate(sli3::exception);
  } catch (...) {
    message(M_FATAL, "SLIInterpreter", "An unknown c++ exception occured.");
    operand_stack_.dump(std::cerr);
    execution_stack_.dump(std::cerr);
    terminate(sli3::exception);
  }

  Token &exit_tk = status_dict_->lookup(
      Name("exitcode")); // This throws an exception if the entry is not found.
  exitcode = exit_tk.data_.long_val;

  if (exitcode != 0)
    error_dict_->insert(quitbyerror_name, new_token<sli3::booltype>(true));

  return exitcode;
}

int SLIInterpreter::execute_dispatch_(size_t exitlevel) {
  int exitcode = 0;
  const Name exitcode_name("exitcode");
  const Token null_val = new_token<sli3::integertype>(0);
  // Locals must hold the *tagged* SLIType*, not raw types_[i],
  // because they are written into Token::type_ later. With pointer
  // tagging, every type_ value has the typeid in its top byte;
  // raw types_[i] has no tag and would dispatch as typeid 0.
  SLIType *iiterate_t = get_type(sli3::iiteratetype);
  SLIType *proc_type  = get_type(sli3::proceduretype);
  if (status_dict_)
    (*status_dict_)[exitcode_name] = null_val;

  if (sli3::signalflag != 0) {
    return sli3::unknown_error;
  }

  // Pull cycle_count_ into a register-resident local. The
  // per-iteration ++cycle_count_ on the member would be a write
  // through to L1 every cycle; a local is a register increment.
  // We sync back on loop exit / exception so the SLI `cycles`
  // operator (which reads the member) still observes correct
  // values between dispatcher invocations.
  size_t local_cycles = cycle_count_;
  try {
    do {  // loop1 -- recover from raiseerror without re-entering the
          // per-iteration try block. NEST 2.20 uses the same pattern
          // (interpret.cc execute_); zero-cost exceptions are zero-cost
          // for happy-path code, but moving the try out of the inner
          // while keeps the unwind tables out of the dispatcher's
          // hottest loop and lets the compiler hoist register state
          // more aggressively.
      try {
        while (execution_stack_.load() > exitlevel) {
          ++local_cycles;

        /*
          - The following switch handles most of the interpreter's internal
          functions.
          - Placing them all in one large piece of code allows the compiler to
          better optimize the code.
          - The case lables are the type ids which are in strict ascending
          order. This allows the compiler to contruct a jump table.
          - Not all case lables have break statements, but continue to the next
          label.
          - Some case-lables use gotos to implement loops that depend on the
          stack contents. Since we must use break to exit the case-label, we
          cannot use loops without awkward rechecks of the loop's exit
          condition.
          - quit is now also implemented as an operator.
        */
        // Pointer-tag fast path: tag() returns the typeid via a
        // shift on the already-loaded type_ pointer. No chained
        // load through SLIType::id_ -- saves one cache line read
        // per dispatcher iteration.
        switch (execution_stack_.top().tag()) {
          /*
            We first send all plain data types to the operand stack.
          */
        case sli3::integertype:
        case sli3::doubletype:
        case sli3::booltype:
        case sli3::literaltype:
        case sli3::marktype:
        case sli3::stringtype:
        case sli3::arraytype:
        case sli3::dictionarytype:
          operand_stack_.push(execution_stack_.top());
          execution_stack_.pop();
          break;
          /*
            Next we deal with the executable types.
          */
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
          operand_stack_.push(execution_stack_.top());
          execution_stack_.pop();
          break;

        case sli3::functiontype: {
          // Optional dispatch tracing for debugging the bootstrap.
          // Enabled when the SLI3_TRACE environment variable is set
          // and non-empty. Cached on first dispatch.
          static int trace = -1;
          if (trace == -1) {
            char const *env = std::getenv("SLI3_TRACE");
            trace = env && env[0] ? 1 : 0;
          }
          if (trace) {
            SLIFunction *fn = execution_stack_.top().data_.func_val;
            std::cerr << "[fn] " << fn->get_name().toString()
                      << " ostack=" << operand_stack_.load();
            size_t lim = std::min<size_t>(operand_stack_.load(), 4);
            for (size_t k = 0; k < lim; ++k) {
              std::cerr << " [" << k << "]=";
              operand_stack_.pick(k).pprint(std::cerr);
            }
            std::cerr << '\n';
          }
          if (__builtin_expect(count_calls_, 0))
            ++call_counts_[execution_stack_.top().data_.func_val];
          execution_stack_.top().data_.func_val->execute(this);
          break;
        }

        case sli3::proceduretype:
          execution_stack_.push(null_val);
          execution_stack_.push(iiterate_t);

        case sli3::iiteratetype: {
          // Axis I Slice 3: hoist proc + pos_p out of the per-
          // iteration label. See irepeat case for rationale.
          TokenArray *proc = execution_stack_.pick(2).data_.array_val;
          long *pos_p = &execution_stack_.pick(1).data_.long_val;
        start_iterate:
          if (proc->index_is_valid(*pos_p)) {
            const Token &t = proc->get((*pos_p)++);
            if (not t.is_executable()) {
              operand_stack_.push(t);
              if (not proc->index_is_valid(*pos_p)) {
                execution_stack_.pop(3);
                break;
              }
              goto start_iterate;
            }
            if (not proc->index_is_valid(*pos_p)) {
              proc->add_reference();
              execution_stack_.pick(2) = t;
              proc->remove_reference();
              execution_stack_.pop(2);
              break;
            }
            // Inline fast path: functiontype calls execute directly,
            // then loop back. See irepeat's case for the same trick.
            if (t.tag() == sli3::functiontype) {
              execution_stack_.push(t);
              if (__builtin_expect(count_calls_, 0))
                ++call_counts_[t.data_.func_val];
              t.data_.func_val->execute(this);
              if (execution_stack_.top().tag() == sli3::iiteratetype) {
                proc = execution_stack_.pick(2).data_.array_val;
                pos_p = &execution_stack_.pick(1).data_.long_val;
                goto start_iterate;
              }
              break;
            }
            // Inline name -> function dispatch (un-bound procs).
            if (t.tag() == sli3::nametype) {
              const Token &resolved = lookup(t.data_.long_val);
              if (resolved.tag() == sli3::functiontype) {
                execution_stack_.push(resolved);
                if (__builtin_expect(count_calls_, 0))
                  ++call_counts_[resolved.data_.func_val];
                resolved.data_.func_val->execute(this);
                if (execution_stack_.top().tag() == sli3::iiteratetype) {
                  proc = execution_stack_.pick(2).data_.array_val;
                  pos_p = &execution_stack_.pick(1).data_.long_val;
                  goto start_iterate;
                }
                break;
              }
              if (resolved.is_executable()) {
                execution_stack_.push(resolved);
                break;
              }
              operand_stack_.push(resolved);
              if (not proc->index_is_valid(*pos_p)) {
                execution_stack_.pop(3);
                break;
              }
              goto start_iterate;
            }
            execution_stack_.push(t);
            break;
          }
          // This point is only reached for empty procedures
          execution_stack_.pop(3);
          break;
        }

        case sli3::irepeattype: {
          // Axis I Slice 3: hoist proc and pos capture out of the
          // per-iteration label. Both held as raw pointers/values
          // (not references). Only `execution_stack_.push()` can
          // reallocate the e-stack vector and invalidate `pos_p`;
          // `operand_stack_.push()` cannot (separate vector). So
          // the literal-push and operand-push paths can re-enter
          // start_repeat without re-deriving `pos_p`. Re-derive
          // after every fn->execute that left us in the same iter
          // frame.
          TokenArray *proc = execution_stack_.pick(2).data_.array_val;
          long *pos_p = &execution_stack_.pick(1).data_.long_val;
        start_repeat:
          if (proc->index_is_valid(*pos_p)) {
            const Token &t = proc->get(*pos_p);
            ++*pos_p;
            if (t.is_executable()) {
              // Inline fast path for functiontype tokens: push the
              // function token (raiseerror reads the operator name
              // from estack top), call execute() directly, then loop
              // back to start_repeat instead of returning to the
              // outer switch. The function's body pops its own
              // estack frame. If the function nested any deeper
              // execution (pushed onto estack and didn't pop net),
              // we fall back to the outer dispatcher by breaking.
              if (t.tag() == sli3::functiontype) {
                execution_stack_.push(t);
                if (__builtin_expect(count_calls_, 0))
                  ++call_counts_[t.data_.func_val];
                t.data_.func_val->execute(this);
                if (execution_stack_.top().tag() == sli3::irepeattype) {
                  // proc + pos_p may have moved (estack reallocation)
                  proc = execution_stack_.pick(2).data_.array_val;
                  pos_p = &execution_stack_.pick(1).data_.long_val;
                  goto start_repeat;
                }
                break;
              }
              // Inline name resolution + function dispatch. Two
              // outer round-trips collapse into one case body --
              // matters for un-bound procedures where the body
              // still holds nametype tokens.
              if (t.tag() == sli3::nametype) {
                const Token &resolved = lookup(t.data_.long_val);
                if (resolved.tag() == sli3::functiontype) {
                  execution_stack_.push(resolved);
                  if (__builtin_expect(count_calls_, 0))
                    ++call_counts_[resolved.data_.func_val];
                  resolved.data_.func_val->execute(this);
                  if (execution_stack_.top().tag() == sli3::irepeattype) {
                    proc = execution_stack_.pick(2).data_.array_val;
                    pos_p = &execution_stack_.pick(1).data_.long_val;
                    goto start_repeat;
                  }
                  break;
                }
                if (resolved.is_executable()) {
                  execution_stack_.push(resolved);
                  break;
                }
                operand_stack_.push(resolved);
                goto start_repeat;
              }
              execution_stack_.push(t);
              break;
            }
            operand_stack_.push(t);
            goto start_repeat; // we must use goto here, so that break exits the
                               // case label rather than the while loop.
          }

          long &lc = execution_stack_.pick(3).data_.long_val;
          if (lc > 0) {
            *pos_p = 0; // reset procedure iterator
            --lc;
          } else {
            execution_stack_.pop(5);
          }
          break;
        }

        case sli3::ifortype: {
          // Axis I Slice 3: hoist proc + pos_p out of the per-
          // iteration label. See irepeat case for rationale.
          TokenArray *proc = execution_stack_.pick(2).data_.array_val;
          long *pos_p = &execution_stack_.pick(1).data_.long_val;
        start_for:
          if (proc->index_is_valid(*pos_p)) {
            const Token &t = proc->get(*pos_p);
            ++*pos_p;
            if (t.is_executable()) {
              // Inline functiontype: see irepeat case.
              if (t.tag() == sli3::functiontype) {
                execution_stack_.push(t);
                if (__builtin_expect(count_calls_, 0))
                  ++call_counts_[t.data_.func_val];
                t.data_.func_val->execute(this);
                if (execution_stack_.top().tag() == sli3::ifortype) {
                  proc = execution_stack_.pick(2).data_.array_val;
                  pos_p = &execution_stack_.pick(1).data_.long_val;
                  goto start_for;
                }
                break;
              }
              // Inline name -> function dispatch.
              if (t.tag() == sli3::nametype) {
                const Token &resolved = lookup(t.data_.long_val);
                if (resolved.tag() == sli3::functiontype) {
                  execution_stack_.push(resolved);
                  if (__builtin_expect(count_calls_, 0))
                    ++call_counts_[resolved.data_.func_val];
                  resolved.data_.func_val->execute(this);
                  if (execution_stack_.top().tag() == sli3::ifortype) {
                    proc = execution_stack_.pick(2).data_.array_val;
                    pos_p = &execution_stack_.pick(1).data_.long_val;
                    goto start_for;
                  }
                  break;
                }
                if (resolved.is_executable()) {
                  execution_stack_.push(resolved);
                  break;
                }
                operand_stack_.push(resolved);
                goto start_for;
              }
              execution_stack_.push(t);
              break;
            }
            operand_stack_.push(t);
            goto start_for; // We use goto so that 'break' exits 'case' rather
                            // than 'while'
          }

          long &count = execution_stack_.pick(3).data_.long_val;
          long &lim = execution_stack_.pick(4).data_.long_val;
          long &inc = execution_stack_.pick(5).data_.long_val;

          if (((inc > 0) && (count <= lim)) || ((inc < 0) && (count >= lim))) {
            *pos_p = 0; // reset procedure interator

            operand_stack_.push(
                execution_stack_.pick(3)); // push counter to user
            count += inc;                  // then increment it
            break;
          } else
            execution_stack_.pop(7);
          break;
        }
        case sli3::iforalltype: {
          // Axis I Slice 3: hoist proc + pos_p out of the per-
          // iteration label. See irepeat case for rationale.
          TokenArray *proc = execution_stack_.pick(2).data_.array_val;
          long *pos_p = &execution_stack_.pick(1).data_.long_val;
        start_forall:
          if (proc->index_is_valid(*pos_p)) {
            const Token &t = proc->get(*pos_p);
            ++*pos_p;
            if (t.is_executable()) {
              // Inline functiontype: see irepeat case.
              if (t.tag() == sli3::functiontype) {
                execution_stack_.push(t);
                if (__builtin_expect(count_calls_, 0))
                  ++call_counts_[t.data_.func_val];
                t.data_.func_val->execute(this);
                if (execution_stack_.top().tag() == sli3::iforalltype) {
                  proc = execution_stack_.pick(2).data_.array_val;
                  pos_p = &execution_stack_.pick(1).data_.long_val;
                  goto start_forall;
                }
                break;
              }
              // Inline name -> function dispatch.
              if (t.tag() == sli3::nametype) {
                const Token &resolved = lookup(t.data_.long_val);
                if (resolved.tag() == sli3::functiontype) {
                  execution_stack_.push(resolved);
                  if (__builtin_expect(count_calls_, 0))
                    ++call_counts_[resolved.data_.func_val];
                  resolved.data_.func_val->execute(this);
                  if (execution_stack_.top().tag() == sli3::iforalltype) {
                    proc = execution_stack_.pick(2).data_.array_val;
                    pos_p = &execution_stack_.pick(1).data_.long_val;
                    goto start_forall;
                  }
                  break;
                }
                if (resolved.is_executable()) {
                  execution_stack_.push(resolved);
                  break;
                }
                operand_stack_.push(resolved);
                goto start_forall;
              }
              execution_stack_.push(t);
              break;
            }
            operand_stack_.push(t);
            goto start_forall;
          }

          TokenArray *ad = execution_stack_.pick(4).data_.array_val;
          long &idx = execution_stack_.pick(3).data_.long_val;

          if (ad->index_is_valid(idx)) {
            *pos_p = 0; // reset procedure interator

            operand_stack_.push(ad->get(idx)); // push object to user
            ++idx;
          } else {
            execution_stack_.pop(6);
          }
          break;
        }
        case sli3::quittype:
          goto exit_interpreter;
        default:
          execution_stack_.top().execute();
        }
        } // inner while -- ran to completion or break
      } catch (std::exception &exc) {
        // Funnel C++ exceptions through the SLI error machinery so
        // a surrounding stopped/handleerror context can react.
        // raiseerror pushes /cmd onto the operand stack and `stop`
        // onto the execution stack.
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
  // Sync the local cycle counter back to the member. The
  // case sli3::quittype branches here via goto from inside the
  // try; both paths land at this label.
  cycle_count_ = local_cycles;

  // Read exit code from statusdict if it's been set up by the
  // bootstrap; otherwise default to 0. (During the very first call
  // to the dispatcher the script may not have populated the entry
  // yet.)
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
  delete this;
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
