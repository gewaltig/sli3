#include "sli_interpreter.h"
#include "sli_nametype.h"
#include "sli_dicttype.h"
#include "sli_functiontype.h"
#include "sli_string.h"
#include "sli_stringtype.h"
#include "compose.hpp"
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

namespace sli3
{

    int signalflag =0;

    SLIInterpreter::SLIInterpreter()
	: is_initialized_(false),
	  debug_mode_(false),
	  show_stack_(false),
	  show_backtrace_(false),
	  catch_errors_(false),
	  opt_tailrecursion_(true),
	  cycle_guard(false),
	  call_depth_(0),
	  max_call_depth_(10),
	  cycle_count_(0),
	  cycle_restriction_(0),
	  verbosity_level_(M_INFO),
	  system_dict_(0),
	  user_dict_(0),
	  status_dict_(0),
	  error_dict_(0),
	  parser_(0),
	  ilookup_name("::lookup"),
	  ipop_name("::pop"),
	  isetcallback_name("::setcallback"),
	  iiterate_name("::executeprocedure"),
	  iloop_name("::loop"),
	  irepeat_name("::repeat"),
	  ifor_name("::for"),
	  iforallarray_name("::forall_a"),
	  iforalliter_name("::forall_iter"),
	  iforallindexedarray_name("::forallindexed_a"),
	  iforallindexedstring_name("::forallindexed_s"),
	  iforallstring_name("::forall_s"),
	  pi_name("Pi"),    
	  e_name("E"),
	  iparse_name("::parse"),
	  stop_name("stop"),
	  end_name("end"),
	  null_name("null"),
	  true_name("true"),
	  false_name("false"),
	  mark_name("mark"),
	  istopped_name("::stopped"),
	  systemdict_name("systemdict"),
	  userdict_name("userdict"),
	  errordict_name("errordict"),
	  quitbyerror_name("quitbyerror"),
	  newerror_name("newerror"),
	  errorname_name("errorname"),
	  commandname_name("commandname"),
	  signo_name("sys_signo"),
	  recordstacks_name("recordstacks"),
	  estack_name("estack"),
	  ostack_name("ostack"),
	  dstack_name("dstack"),
	  commandstring_name("moduleinitializers"),
	  interpreter_name("SLIInterpreter::execute"),
	  
	  ArgumentTypeError("ArgumentType"),
	  StackUnderflowError("StackUnderflow"),
	  UndefinedNameError("UndefinedName"),
	  WriteProtectedError("WriteProtected"),
	  DivisionByZeroError("DivisionByZero"),
	  RangeCheckError("RangeCheck"),
	  PositiveIntegerExpectedError("PositiveIntegerExpected"),
	  BadIOError("BadIO"),
	  StringStreamExpectedError("StringStreamExpected"),
	  CycleGuardError("AllowedCyclesExceeded"),
	  SystemSignal("SystemSignal"),
	  BadErrorHandler("BadErrorHandler"),
	  KernelError("KernelError"),
	  InternalKernelError("InternalKernelError"),
	  types_(),
	  token_memory(),
	  operand_stack_(100),
	  execution_stack_(100)
    {
	init();
    }
    
    SLIInterpreter::~SLIInterpreter()
    {
	operand_stack_.clear();
	execution_stack_.clear();

	for(size_t t=0; t<types_.size();++t)
	    delete types_[t];
    }

    void SLIInterpreter::init()
    {
	init_types();
    }
    
    void SLIInterpreter::init_types()
    {
	types_.clear();
	/*
	  The order in which these types are created must match the
	  order in which the lables in the enum typeid (sli_type.h) are
	  defined.
	*/
	types_.push_back(new IntegerType(this,"integertype",sli3::integertype));
	types_.push_back(new DoubleType(this,"doubletype",sli3::doubletype));
	types_.push_back(new BoolType(this,"booltype",sli3::booltype));
	types_.push_back(new LiteralType(this,"nametype",sli3::literaltype));
	types_.push_back(new NameType(this,"nametype",sli3::nametype));
	types_.push_back(new SymbolType(this,"symboltype",sli3::symboltype));
	types_.push_back(new StringType(this,"stringtype",sli3::stringtype));
	types_.push_back(new ArrayType(this,"arraytype",sli3::arraytype));
	types_.push_back(new LitprocedureType(this,"litproceduretype",sli3::litproceduretype));
	types_.push_back(new ProcedureType(this,"proceduretype",sli3::proceduretype));
	types_.push_back(new DictionaryType(this,"dictionarytype",sli3::dictionarytype));
	types_.push_back(new FunctionType(this,"functiontype",sli3::functiontype));
    }

    void SLIInterpreter::init_message_tags()
    {
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

    void SLIInterpreter::init_dictionaries()
    {
	system_dict_= new Dictionary();
	error_dict_= new Dictionary();
	user_dict_= new Dictionary();
	status_dict_= new Dictionary();

	Token dict(types_[sli3::dictionarytype]);
	dict.data_.dict_val=system_dict_;
	system_dict_->insert(Name("systemdict"),dict);
	dictionary_stack_.push(dict);
	dictionary_stack_.set_basedict();
	dict.data_.dict_val=error_dict_;
	system_dict_->insert(Name("errordict"),dict);
	dict.data_.dict_val=user_dict_;
	system_dict_->insert(Name("userdict"),dict);
	dictionary_stack_.push(dict);
	dict.data_.dict_val=status_dict_;
	system_dict_->insert(Name("statusdict"),dict);
	dict.type_=0; // This prevents the token data from being cleared.
    }

    int SLIInterpreter::startup()
    {
	int exitcode=EXIT_SUCCESS;

	if (! is_initialized_ and execution_stack_.load()>0)
	{
	    exitcode=execute_();
	    is_initialized_=true;
	}
	return exitcode;
    }

    int SLIInterpreter::execute(const std::string &cmdline)
    {
	int exitcode=startup();
	if(exitcode !=EXIT_SUCCESS)
	    return -1;
	
	operand_stack_.push(new_token<sli3::stringtype>(cmdline));
	execution_stack_.push(new_token<sli3::nametype>(Name("evalstring")));
	return execute_(); // run the interpreter
    }

    int SLIInterpreter::execute(int v)
    {
	startup();
	execution_stack_.push(new_token<sli3::nametype>(Name("start")));
	switch(v)
	{
	case 0:
	case 1:
	    return execute_(); // run the interpreter
	case 2:
	default:
	    return -1;
	}
    }

    int SLIInterpreter::execute_(size_t exitlevel)
    {
	int exitcode;
    
	if(sli3::signalflag !=0)
	{
	    return sli3::unknown_error;
	}
    
	try
	{
	    do { //loop1  this double loop to keep the try/catch outside the inner loop
		try
		{ 
		    while(execution_stack_.load() > exitlevel) // loop 2
		    {
			++cycle_count_;
			execution_stack_.top().execute();
		    }
		}
		catch(std::exception &exc)
		{
		    raiseerror(exc);
		}
	    } while(execution_stack_.load() > exitlevel);
	}
	catch(std::exception &e)
	{
	    message(M_FATAL, "SLIInterpreter","A C++ library exception occured.");
	    operand_stack_.dump(std::cerr);
	    execution_stack_.dump(std::cerr);
	    message(M_FATAL, "SLIInterpreter",e.what());
	    terminate(sli3::exception);
	}
	catch(...)
	{
	    message(M_FATAL, "SLIInterpreter","An unknown c++ exception occured.");
	    operand_stack_.dump(std::cerr);
	    execution_stack_.dump(std::cerr);
	    terminate(sli3::exception);
	} 
	
	Token &exit_tk=	status_dict_->lookup(Name("exitcode")); // This throws an exception if the entry is not found.
	exitcode = exit_tk.data_.long_val;
	
	if (exitcode != 0)
	    error_dict_->insert(quitbyerror_name,new_token<sli3::booltype>(true));
	
	return exitcode;
    }

    /** Define a function in the current dictionary.
     *  This function defines a SLI function in the current dictionary. 
     *  Note that you may also pass a string as the first argument, as
     *  there is an implicit type conversion operator from string to Name. 
     *  Use the Name when a name object for this function already
     *  exists.
     */
    void SLIInterpreter::createcommand(Name n, SLIFunction *fn)
    {
	if ( dictionary_stack_.known(n) )
	    throw NamingConflict("A function called '" + std::string(n.toString()) 
				 + "' exists already.\n"
				 "Please choose a different name!");
  
	TokenRef t;
	t.type_=types_[sli3::functiontype];
	t.data_.func_val= fn;
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
    void SLIInterpreter::createconstant(Name n, Token const & val)
    {
	dictionary_stack_.def(n, val);
    }
    
    void SLIInterpreter::push(const Token &t)
    {
	std::cout << operand_stack_.size() << '\n';
	operand_stack_.push(t);
	operand_stack_.dump(std::cerr);
    }

    void SLIInterpreter::set_verbosity(int l)
    {
	verbosity_level_ = l;
    }
    
    int SLIInterpreter::verbosity(void) const
    {
	return verbosity_level_;
    }
    
    void SLIInterpreter::terminate(int returnvalue)
    {
	if (returnvalue == -1)
	{
	    returnvalue = sli3::fatal;
	}
  
	message(sli3::M_FATAL, "SLIInterpreter","Exiting.");
	delete this;
	std::exit(returnvalue);
    }

    void SLIInterpreter::message(int level, const char from[], 
				 const char text[],
				 const char errorname[]) const
    {
	#ifdef HAVE_PTHREADS
	static Mutex barrier;
	
	barrier.lock(); // Only one thread may write at a time.
	#endif

	if(level >= verbosity_level_)
	{
	    if (level >= M_FATAL)
	    {   
		// Take care that std::cerr and std::cout don't 
		// send output to the same source. This would 
		// lead to duplication of the message.
		message(std::cout, message_tag_[M_FATAL].c_str(), from, text, errorname);
	    }
	    else if (level >= M_ERROR)
	    {         
		message(std::cout, message_tag_[M_ERROR].c_str(), from, text, errorname);
		
	    }
	    else if (level >= M_WARNING)
	    { 
		message(std::cout, message_tag_[M_WARNING].c_str(), from, text, errorname);
	    }
	    else if (level >= M_INFO)
	    {
		message(std::cout, message_tag_[M_INFO].c_str(), from, text, errorname);
	    }
	    else if (level >= M_STATUS)
	    {  
		message(std::cout, message_tag_[M_STATUS].c_str(), from, text, errorname);
	    }
	    else if (level >= M_DEBUG)
	    {   
		message(std::cout, message_tag_[M_DEBUG].c_str(), from, text, errorname);
	    }
	    else
	    {
		message(std::cout, message_tag_[M_ALL].c_str(), from, text, errorname);
	    }
	}
	#ifdef HAVE_PTHREADS
	barrier.unlock(); // Now the next thread my write.
	#endif
    }

void SLIInterpreter::message(std::ostream& out, const char levelname[], 
			     const char from[], const char text[],
			     const char errorname[]) const
{
  const unsigned buflen=30;
  char timestring[buflen+1]="";
  const time_t tm = std::time(NULL);

  std::strftime(timestring,buflen,"%b %d %H:%M:%S",std::localtime(&tm));

  std::string msg = 
    String::compose("%1 %2 [%3]: ", timestring, from, levelname); 
  out << std::endl << msg << errorname;
        
  //Set the preferred line indentation.
  const size_t indent = 4;

  // Get size of the output window. The message text will be
  // adapted to the width of the window.
  //
  // The COLUMNS variable should preferably be extracted
  // from the environment dictionary set up by the
  // Processes class. getenv("COLUMNS") works only on 
  // the created NEST executable (not on the messages
  // printed by make install).
  char const * const columns = std::getenv("COLUMNS");
  size_t max_width = 78;
  if ( columns )
    max_width = std::atoi(columns);
  if ( max_width < 3 * indent )
    max_width = 3 * indent;
  const size_t width = max_width - indent; 

  // convert char* to string to be able to use the string functions
  std::string text_str(text); 
 
  // Indent first message line
  if(text_str.size() != 0)
    {
      std::cout << std::endl << std::string(indent, ' '); 
    }

  size_t pos = 0;

  for(size_t i = 0; i<text_str.size(); ++i)
    {
      if( text_str.at(i) == '\n' && i != text_str.size()-1)  
	{
	  // Print a lineshift followed by an indented whitespace
	  // Manually inserted lineshift at the end of the message 
	  // are suppressed.
	  out << std::endl << std::string(indent, ' ');
	  pos = 0;
	}
      else
	{
	  // If we've reached the width of the output we'll print
	  // a lineshift regardless of whether '\n' is found or not.
	  // The printing is done so that no word splitting occurs.
	  size_t space = text_str.find(' ', i) < text_str.find('\n') ? 
	    text_str.find(' ', i) : text_str.find('\n');
	  // If no space is found (i.e. the last word) the space 
	  // variable is set to the end of the string.
	  if(space == std::string::npos)
	    {
	      space = text_str.size();
	    }

	  // Start on a new line if the next word is longer than the 
	  // space available (as long as the word is shorter than the
	  // total width of the printout). 
	  if(i != 0 && 
	     text_str.at(i-1) == ' ' &&
	     static_cast<int>(space-i) > static_cast<int>(width-pos))
	    {
	      out << std::endl << std::string(indent, ' ');
	      pos = 0;
	    }

	  // Only print character if we're not at the end of the 
	  // line and the last character is a space.
	  if(!(width-pos == 0 && text_str.at(i) == ' '))
	    {
	      // Print the actual character.
	      out << text_str.at(i);
	    }

	  ++pos;
	}
    }
  out << std::endl;
}



/* Template specializations only beyond this point */

    template<>
    void SLIInterpreter::push<int>(int l)
    {
      std::cout << operand_stack_.size() << '\n';
      operand_stack_.push(types_[sli3::integertype]);
      operand_stack_.top().data_.long_val=l;
      operand_stack_.dump(std::cerr);
    }

    template<>
    void SLIInterpreter::push<long>(long l)
    {
	operand_stack_.push(types_[sli3::integertype]);
	operand_stack_.top().data_.long_val=l;
	operand_stack_.dump(std::cerr);
    }

    template<>
    void SLIInterpreter::push<double>(double d)
    {
	operand_stack_.push(types_[sli3::doubletype]);
	operand_stack_.top().data_.double_val=d;
	operand_stack_.dump(std::cerr);
    }

    template<>
    void SLIInterpreter::push<Name>(Name n)
    {
	operand_stack_.push(types_[sli3::nametype]);
	operand_stack_.top().data_.name_val=n.toIndex();
	operand_stack_.dump(std::cerr);
    }

    template<>
    void SLIInterpreter::push<TokenArray&>(TokenArray& a)
    {
	operand_stack_.push(types_[sli3::arraytype]);
	operand_stack_.top().data_.array_val= &a;
	a.add_reference();
	operand_stack_.dump(std::cerr);
    }


    template<>
    Token SLIInterpreter::new_token<sli3::integertype>()
    {
	Token t(types_[sli3::integertype]);
	t.data_.long_val= 0;
	return t;
    }

    template<>
    Token SLIInterpreter::new_token<sli3::doubletype>()
    {
	Token t(types_[sli3::doubletype]);
	t.data_.double_val= 0;
	return t;
    }

    template<>
    Token SLIInterpreter::new_token<sli3::arraytype>()
    {
	Token t(types_[sli3::arraytype]);
	t.data_.array_val= new TokenArray();
	return t;
    }

    template<>
    Token SLIInterpreter::new_token<sli3::litproceduretype>()
    {
	Token t(types_[sli3::litproceduretype]);
	t.data_.array_val= new TokenArray() ;
	return t;
    }

    template<>
    Token SLIInterpreter::new_token<sli3::dictionarytype>()
    {
	Token t(types_[sli3::dictionarytype]);
	t.data_.dict_val= new Dictionary();
	return Token(t);
    }

    template<>
    Token SLIInterpreter::new_token<sli3::integertype,int>(int const &i)
    {
	Token t(types_[sli3::integertype]);
	t.data_.long_val= i;
	return Token(t);
    }
    template<>
    Token SLIInterpreter::new_token<sli3::integertype,long>(long const &l)
    {
	Token t(types_[sli3::integertype]);
	t.data_.long_val= l;
	return Token(t);
    }
    template<>
    Token SLIInterpreter::new_token<sli3::doubletype,double>(double const &d)
    {
	Token t(types_[sli3::doubletype]);
	t.data_.double_val= d;
	return Token(t);
    }

    template<>
    Token SLIInterpreter::new_token<sli3::booltype,bool>(bool const &b)
    {
	Token t(types_[sli3::booltype]);
	t.data_.bool_val= b;
	return Token(t);
    }
    template<>
    Token SLIInterpreter::new_token<sli3::nametype,Name>(Name const &n)
    {
	Token t(types_[sli3::nametype]);
	t.data_.name_val= n.toIndex();;
	return Token(t);
    }

    template<>
    Token SLIInterpreter::new_token<sli3::literaltype,Name>(Name const &n)
    {
	Token t(types_[sli3::literaltype]);
	t.data_.name_val= n.toIndex();;
	return Token(t);
    }

    template<>
    Token SLIInterpreter::new_token<sli3::symboltype,Name>(Name const &n)
    {
	Token t(types_[sli3::symboltype]);
	t.data_.name_val= n.toIndex();;
	return Token(t);
    }

    template<>
    Token SLIInterpreter::new_token<sli3::stringtype,std::string>(std::string const &s)
    {
	Token t(types_[sli3::stringtype]);
	t.data_.string_val= new SLIString(s) ;
	return t;
    }





}
