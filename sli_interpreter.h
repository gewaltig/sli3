#ifndef SLI3_INTERPRETER_H
#define SLI3_INTERPRETER_H

#include "sli_type.h"
#include "sli_token.h"
#include "sli_allocator.h"
#include "sli_arraytype.h"
#include "sli_integertype.h"
#include "sli_tokenstack.h"
#include "sli_name.h"
#include "sli_string.h"
#include "sli_dictionary.h"
#include "sli_dictstack.h"
#include "sli_builtins.h"
#include "sli_module.h"
#include<vector>
#include <deque>

namespace sli3
{

    /**
     * Global flag to store system signals.
     */
    extern int signalflag;

    class Parser;

    enum message_level
    {
	M_ALL=0,
	M_DEBUG,
	M_STATUS,
	M_INFO,
	M_WARNING,
	M_ERROR,
	M_FATAL,
	M_QUIET,
	num_message_levels
    };
    
    enum exit_codes
    {
	success=0,
	ipc_signal=1, //!< an IPC signal was caught.
	restart_serial=4,
	unknown_error=10,
	exception=125,
	scripterror=126,
	fatal=127,
	abort=134,	    
	segfault=139,
	num_exitcodes
    };

   /**
    * Opcodes for internal functions.
    * These functions implement loops and other control structures.
    * Since they are often needed, the interpreter stores them in a table for
    * fast lookup.
    */
    enum opcode
    {
      i_lookup, // lookup a name
      i_move, // Move token to operand stack
      i_iterate, // iterate a procedure
      i_repeat, // iterate a repeat loop
      i_for, // iterate a for loop
      num_opcodes
    };
	
    class SLIInterpreter
    {
    public:

	SLIInterpreter();
	~SLIInterpreter();

	void init();
	void init_types();
	void init_dictionaries();
	void init_message_tags();
	void init_internal_functions();

	/**
	 * Initiates the interpreter's startup sequnce.
	 */
	int startup();

	template<class T>  void addmodule(void);
	void addmodule(SLIModule *);


	void clear_parser_context();

	Token read_token(std::istream &);
	/**
	 * Execute the commands, supplied as string.
	 */
	int execute(const std::string &);

	/**
	 * Start interpreter in input-evaluate loop.
	 * 
	 */
	int execute(int=0);
	
	Name get_current_name() const;

	void raiseerror(std::exception&);
	void raiseerror(Name cmd, Name err);
	void raiseerror(Name);
	void raiseerror(char const []);
	void raiseagain();
	void raisesignal(int);

	void print_error(Token);
	/**
	 * Execute until execution stack reaches level.
	 */
	int execute_(size_t level=0);
	int execute_debug_(size_t level=0);
	int execute_dispatch_(size_t level=0);

	void createdouble(Name , double);
	void createcommand(Name, SLIFunction *);
	void createconstant(Name, const Token&);



	/** Lookup a name searching all dictionaries on the stack.
	 *  The first occurrence is reported. If the Name is not found,
	 *  @a VoidToken is returned.
	 */
	bool lookup(Name n, Token &);
	
	
	/** Lookup a name searching all dictionaries on the stack.
	 *  The first occurrence is reported. If the Name is not found,
	 *  an UndefinedName exceptiopn is thrown.
	 */
	Token& lookup(Name n);
	
	/** Lookup a name searching only the bottom level dictionary.
	 *  If the Name is not found,
	 *  @a false is returned.
	 */
	bool baselookup(Name n, Token &);

	Token& baselookup(Name n);
	
	/** Test for a name searching all dictionaries on the stack.
	 */
	bool known(Name);
	
	/** Bind a Token to a Name.
	 *  The token is copied. This can be an expensive operation for large
	 *  objects. Also, if the token is popped off one of the stacks after
	 *  calling def, it is more reasonable to use SLIInterpreter::def_move.
	 */
	void def(Name , Token const &);
	
	/** Unbind a previously bound Token from a Name.
	 * Throws UnknownName Exception.
	 */
	void undef(Name);
	
	/** Bind a Token to a Name in the bottom level dictionary.
	 *  The Token is copied.
	 */
	void basedef(Name n, const Token &t);
	
	/**
	 * Push a piece of data to the operande stack.
	 */
	template<class T>
	void push(T);

	/**
	 * Push a token to the operand stack.
	 */
	void push(Token const &);

	/**
	 * Get a reference to the top element on the
	 * operand stack.
	 */
	template <class T>
	T& top();

	Token &top()
	    {
		return operand_stack_.top();
	    }

	Token const &top() const
	    {
		return top();
	    }

	/**
	 * Remove the top element from the operand stack.
	 */
	void pop()
	    {
		operand_stack_.pop();
	    }
	void pop(size_t n)
	    {
		operand_stack_.pop(n);
	    }

	Token& pick(size_t i)
	    {
		return operand_stack_.pick(i);
	    }

	void index(size_t i)
	    {
		operand_stack_.index(i);
	    }

	size_t load() const
	{
	  return operand_stack_.load();
	}

	/**
	 * Checks whether the operand stack holds at least n values.
	 * If the operand stack load is less than n, a StackUnderflow excetion is thrown.
	 */
	void require_stack_load(size_t n) const
	    {
		if(operand_stack_.load()<n)
		    throw StackUnderflow(n, operand_stack_.load() );
	    }

	/**
	 * Checks whether the operand stack holds a specific type at level l.
	 * If not, a TypeMismatch is thrown.
	 */
	void require_stack_type(int l, unsigned int t_id) const
	    {
		operand_stack_.pick(l).require_type(t_id);
	    }


	TokenStack& OStack()
	    {
		return operand_stack_;
	    }

	TokenStack& EStack()
	    {
		return execution_stack_;
	    }
	
	
	bool step_mode() const {return false;}

	/**
	 * Fill token with an object of the specified type.
	 */
	template <sli_typeid, class T>
	Token new_token(T);
	
	template <sli_typeid>
	Token new_token();

	SLIType* get_type(unsigned int id) const
	{
	  return types_[id];
	}

	bool is_initialized()
	    { return is_initialized_;}


	/** Display a message. 
	 *  @param level  The error level that shall be associated with the
	 *  message. You may use any poitive integer here. For conveniency,
	 *  there exist five predifined error levels:  \n
	 * (SLIInterpreter::M_ALL=0, for use with verbosity(int) only, see there), \n
	 *  SLIInterpreter::M_DEBUG=5, a debugging message \n
	 *  SLIInterpreter::M_DEBUG=7, a status message \n
	 *  SLIInterpreter::M_INFO=10, an informational message \n
	 *  SLIInterpreter::M_WARNING=20, a warning message \n
	 *  SLIInterpreter::M_ERROR=30, an error message \n
	 *  SLIInterpreter::M_FATAL=40, a failure message.
	 * (SLIInterpreter::M_QUIET=100, for use with verbosity(int) only, see there), \n
	 *  @param from   A string specifying the name of the function that
	 *  sends the message.
	 *  @param test   A string specifying the message text.
	 *
	 *  The message will ony be displayed if the current verbosity level
	 *  is greater than or equal to the specified level.
	 *  \n
	 *  If two or more messages are issued after each other, that have
	 *  the same <I>from</I> and <I>level</I> argument, the messages will 
	 *  be grouped toghether in the output.
	 *
	 *  @see verbosity(void), verbosity(int)
	 *  @ingroup SLIMessaging
	 */
	void set_verbosity(int);
	int verbosity() const;
	
	void message(int level, const char from[], const char text[], 
		     const char errorname[] = "") const;
	
	/** Function used by the message(int, const char*, const char*) function.
	 *  Prints a message to the specified output stream. 
	 *  @param out        output stream
	 *  @param levelname  name associated with input level
	 */
	void message(std::ostream& out, const char levelname[], 
		     const char from[], const char text[],
		     const char errorname[] = "") const;
	
	void terminate(int returnvalue=-1);

	void setcycleguard(size_t limit)
	    {
		cycle_guard_=true;
		cycle_restriction_= cycle_count_+ limit;
	    }
	
	void removecycleguard(void)
	    {
		cycle_guard_=false;
	    }

	unsigned long cycles(void) const
	    {
		return cycle_count_;
	    }
	
	  /**
	   * True, if a stack backtrace should be shown on error.
	   * Whenever an error or stop is raised, the execution stack is 
	   * unrolled up to the nearest stopped context.
	   * In this process it is possible to display a stack backtrace 
	   * which allows the user to diagnose the origin and possible 
	   * cause of the error.
	   * For applications which handle themselfs, this backtrace may be
	   * disturbing. So it is possible to switch this behavior on and 
	   * off. 
	   */
	bool show_backtrace() const
	    {
		return show_backtrace_;
	    }
	
	/**
	 * Switch stack backtrace on.
	 * Whenever an error or stop is raised, the execution stack is 
	 * unrolled up to the nearest stopped context.
	 * In this process it is possible to display a stack backtrace 
	 * which allows the user to diagnose the origin and possible 
	 * cause of the error.
	 * For applications which handle themselfs, this backtrace may be
	 * disturbing. So it is possible to switch this behavior on and 
	 * off. 
	 */
	void backtrace_on()
	    { show_backtrace_=true;}
	
	
	/**
	 * Switch stack backtrace off.
	 * Whenever an error or stop is raised, the execution stack is 
	 * unrolled up to the nearest stopped context.
	 * In this process it is possible to display a stack backtrace 
	 * which allows the user to diagnose the origin and possible 
	 * cause of the error.
	 * For applications which handle themselfs, this backtrace may be
	 * disturbing. So it is possible to switch this behavior on and 
	 * off. 
	 */
	void backtrace_off()
	    { show_backtrace_=false;}

	
	/**
	 * Increment call depth level.
	 * The value of call_depth_ is used to control
	 * the step mode. 
	 * Step mode is disabled for call_depth_ >= max_call_depth_.
	 * This gives the user the opportunity to skip over nested 
	 * calls during debugging.
	 */
	void inc_call_depth()
	    {
		++call_depth_;
	    }
	
	/**
	 * Decrement call depth level.
	 * The value of call_depth_ is used to control
	 * the step mode. 
	 * Step mode is disabled for call_depth_ >= max_call_depth_.
	 * This gives the user the opportunity to skip over nested 
	 * calls during debugging.
	 */
	void dec_call_depth()
	    {
		--call_depth_;
	    }
	
	/**
	 * Set call depth level to a specific value.
	 * The value of call_depth_ is used to control
	 * the step mode. 
	 * Step mode is disabled for call_depth_ >= max_call_depth_.
	 * This gives the user the opportunity to skip over nested 
	 * calls during debugging.
	 */
	void set_call_depth(int l)
	    {
		call_depth_=l;
	    }
	
	/**
	 * Return current call depth level.
	 * The value of call_depth_ is used to control
	 * the step mode. 
	 * Step mode is disabled for call_depth_ >= max_call_depth_.
	 * This gives the user the opportunity to skip over nested 
	 * calls during debugging.
	 */
	int get_call_depth() const
	    {
		return call_depth_;
	    }
	
	/**
	 * Set maximal call depth level to a specific value.
	 * The value of call_depth_ is used to control
	 * the step mode. 
	 * Step mode is disabled for call_depth_ >= max_call_depth_.
	 * This gives the user the opportunity to skip over nested 
	 * calls during debugging.
	 */
	void set_max_call_depth(int d)
	    {
		max_call_depth_=d;
	    }
	
	/**
	 * Return value of maximal call depth level.
	 * The value of call_depth_ is used to control
	 * the step mode. 
	 * Step mode is disabled for call_depth_ >= max_call_depth_.
	 * This gives the user the opportunity to skip over nested 
	 * calls during debugging.
	 */
	int get_max_call_depth() const
	    {
		return max_call_depth_;
	    }
	
	
	// Names of basics functions
	Name mark_name;
	Name iparse_name;
	Name iparsestdin_name;
	Name ilookup_name;
	Name ipop_name;
	Name iiterate_name;
	Name iloop_name;
	Name irepeat_name;
	Name ifor_name;
	Name iforallarray_name;
	Name iforalliter_name;
	Name iforallindexedarray_name;
	Name iforallindexedstring_name;
	Name iforallstring_name;
	
	Name pi_name;
	Name e_name;
	
	Name stop_name;
	Name end_name;
	Name EndSymbol;

	// Names of symbols and objects
	Name null_name;
	Name true_name;
	Name false_name;
	Name istopped_name; 
	Name systemdict_name;
	Name userdict_name;
	Name errordict_name;
	Name quitbyerror_name;
	Name newerror_name;
	Name errorname_name;
	Name commandname_name;
	Name signo_name;
	Name recordstacks_name;
	Name estack_name;
	Name ostack_name;
	Name dstack_name;
	Name commandstring_name;
	Name interpreter_name;
	
    // Names of basic errors
	Name ArgumentTypeError;
	Name StackUnderflowError;
	Name UndefinedNameError;
	Name WriteProtectedError;
	Name DivisionByZeroError;
	Name RangeCheckError;
	Name PositiveIntegerExpectedError;
	Name BadIOError;
	Name StringStreamExpectedError;
	Name CycleGuardError;
	Name SystemSignal;
	Name BadErrorHandler;
	Name KernelError;
	Name InternalKernelError;
	
	Token execbarrier_token; 
	
	IparseFunction       iparsefunction;
	IparsestdinFunction  iparsestdinfunction;
	IlookupFunction      ilookupfunction;
	IiterateFunction     iiteratefunction;
	IloopFunction        iloopfunction;
	IrepeatFunction      irepeatfunction;
	IforFunction         iforfunction;
	IforallarrayFunction iforallarrayfunction;
	IforalliterFunction  iforalliterfunction;
	IforallindexedarrayFunction iforallindexedarrayfunction;
	IforallindexedstringFunction iforallindexedstringfunction;
	IforallstringFunction iforallstringfunction;
	ArraycreateFunction arraycreatefunction;

    private:
	bool is_initialized_;
	bool debug_mode_;       //!< True, if SLI level debugging is enabled.
	bool show_stack_;       //!< Show stack in debug mode.
	bool show_backtrace_;   //!< Show stack-backtrace on error.
	bool catch_errors_;     //!< Enter debugger on error.
	bool opt_tailrecursion_;//!< Optimize tailing recursion.
	bool cycle_guard_;

	size_t  call_depth_;       //!< Current depth of procedure calls.
	size_t  max_call_depth_;   //!< Depth until which procedure calls are debugged.

	size_t cycle_count_;
	size_t cycle_restriction_; 
	int verbosity_level_;

	Dictionary *system_dict_;
	Dictionary *user_dict_;
	Dictionary *status_dict_;
	Dictionary *error_dict_;
	Parser *parser_;

    public:
	TokenStack operand_stack_;
	TokenStack execution_stack_;
    private:
	std::vector<SLIModule *> modules_;
	DictionaryStack dictionary_stack_;
	std::vector<std::string> message_tag_;
	std::vector<SLIFunction *> functions_; //!< Table with internal functions.
	std::vector<SLIType *> types_;         //!< must be last, so it is deleted last
    };

//    template<>
//    void SLIInterpreter::push<TokenRef>(TokenRef);

    template<>
    void SLIInterpreter::push<int>(int);

    template<>
    void SLIInterpreter::push<char>(char);

    template<>
    void SLIInterpreter::push<long>(long);

    template<>
    void SLIInterpreter::push<unsigned long>(unsigned long);

    template<>
    void SLIInterpreter::push<double>(double);

   template<>
    void SLIInterpreter::push<bool>(bool);

    template<>
    void SLIInterpreter::push<Name>(Name);

    template<>
    void SLIInterpreter::push<TokenArray&>(TokenArray&);

    template<>
    void SLIInterpreter::push<TokenArray *>(TokenArray *);

   template<>
    void SLIInterpreter::push<Dictionary *>(Dictionary *);

    template<>
      Token SLIInterpreter::new_token<sli3::quittype>();

    template<>
      Token SLIInterpreter::new_token<sli3::integertype>();

    template<>
      Token SLIInterpreter::new_token<sli3::doubletype>();

    template<>
      Token SLIInterpreter::new_token<sli3::arraytype>();

    template<>
      Token SLIInterpreter::new_token<sli3::litproceduretype>();

    template<>
      Token SLIInterpreter::new_token<sli3::dictionarytype>();

    template<>
      Token SLIInterpreter::new_token<sli3::marktype>();

    template<>
      Token SLIInterpreter::new_token<sli3::integertype,int>(int);

    template<>
      Token SLIInterpreter::new_token<sli3::integertype,long>(long);

    template<>
      Token SLIInterpreter::new_token<sli3::integertype,unsigned long>(unsigned long);

    template<>
      Token SLIInterpreter::new_token<sli3::doubletype,double>(double);
    template<>
      Token SLIInterpreter::new_token<sli3::booltype,bool>(bool);

    template<>
      Token SLIInterpreter::new_token<sli3::nametype,Name>(Name);
    template<>
      Token SLIInterpreter::new_token<sli3::literaltype,Name>(Name);
    template<>
      Token SLIInterpreter::new_token<sli3::symboltype,Name>(Name);
 
    template<>
      Token SLIInterpreter::new_token<sli3::stringtype, std::string>(std::string);
    template<>
      Token SLIInterpreter::new_token<sli3::stringtype, std::string *>(std::string *);

    template<>
    Token SLIInterpreter::new_token<sli3::arraytype,sli3::TokenArray>(sli3::TokenArray);
    template<>
    Token SLIInterpreter::new_token<sli3::proceduretype,sli3::TokenArray>(sli3::TokenArray);

    template<>
    Token SLIInterpreter::new_token<sli3::arraytype,sli3::TokenArray *>(sli3::TokenArray *);

    template<>
    Token SLIInterpreter::new_token<sli3::proceduretype,sli3::TokenArray *>(sli3::TokenArray *);

    template<>
    Token SLIInterpreter::new_token<sli3::dictionarytype,sli3::Dictionary *>(sli3::Dictionary *);
    
    inline
    bool SLIInterpreter::lookup(Name n, Token &t)
    {
	return dictionary_stack_.lookup(n, t);
    }

    inline
    Token& SLIInterpreter::lookup(Name n)
    {
	return dictionary_stack_.lookup(n);
    }

    inline
    Token& SLIInterpreter::baselookup(Name n)
    {
	return dictionary_stack_.baselookup(n);
    }

    inline
    bool SLIInterpreter::known(Name n)
    {
	Token t;
	return dictionary_stack_.lookup(n,t);
    }

    inline
    void SLIInterpreter::def(Name n, Token const &t)
    {
	dictionary_stack_.def(n,t);
    }
    
    inline
    void SLIInterpreter::undef(Name n)
    {
	dictionary_stack_.undef(n);
    }
    
    inline
    void SLIInterpreter::basedef(Name n, Token const &t)
    {
	dictionary_stack_.basedef(n,t);
    }

    inline
    void SLIInterpreter::push(const Token &t)
    {
	operand_stack_.push(t);
    }

    template<>
    inline
    void SLIInterpreter::push<int>(int l)
    {
      operand_stack_.push(types_[sli3::integertype]);
      operand_stack_.top().data_.long_val=l;
    }

    template<>
    inline
    void SLIInterpreter::push<long>(long l)
    {
	operand_stack_.push(types_[sli3::integertype]);
	operand_stack_.top().data_.long_val=l;
    }

     template<>
    inline
   void SLIInterpreter::push<unsigned long>(unsigned long ul)
    {
	operand_stack_.push(types_[sli3::integertype]);
	operand_stack_.top().data_.long_val=static_cast<long>(ul);
    }

     template<>
    inline
    void SLIInterpreter::push<char>(char c)
    {
      operand_stack_.push(types_[sli3::integertype]);
      operand_stack_.top().data_.long_val=c;
    }

    template<>
    inline
    void SLIInterpreter::push<double>(double d)
    {
	operand_stack_.push(types_[sli3::doubletype]);
	operand_stack_.top().data_.double_val=d;
    }

    template<>
    inline
    void SLIInterpreter::push<bool>(bool b)
    {
	operand_stack_.push(types_[sli3::booltype]);
	operand_stack_.top().data_.bool_val=b;
    }

    template<>
    inline
    void SLIInterpreter::push<Name>(Name n)
    {
	operand_stack_.push(types_[sli3::nametype]);
	operand_stack_.top().data_.name_val=n.toIndex();
    }

    template<>
    inline
    void SLIInterpreter::push<TokenArray const&>(TokenArray const& a)
    {
	operand_stack_.push(types_[sli3::arraytype]);
	operand_stack_.top().data_.array_val= new TokenArray(a);
    }

   template<>
    inline
    void SLIInterpreter::push<TokenArray *>(TokenArray * a)
    {
	operand_stack_.push(types_[sli3::arraytype]);
	operand_stack_.top().data_.array_val= a;
    }

   template<>
    inline
    void SLIInterpreter::push<Dictionary *>(Dictionary * d)
    {
	operand_stack_.push(types_[sli3::dictionarytype]);
	operand_stack_.top().data_.dict_val= d;
    }


    template<>
    inline
    Token SLIInterpreter::new_token<sli3::quittype>()
    {
	return Token(types_[sli3::quittype]);
    }

    template<>
    inline
    Token SLIInterpreter::new_token<sli3::integertype>()
    {
	Token t(types_[sli3::integertype]);
	t.data_.long_val= 0;
	return t;
    }

    template<>
    inline
    Token SLIInterpreter::new_token<sli3::doubletype>()
    {
	Token t(types_[sli3::doubletype]);
	t.data_.double_val= 0;
	return t;
    }

    template<>
    inline
    Token SLIInterpreter::new_token<sli3::arraytype>()
    {
	Token t(types_[sli3::arraytype]);
	t.data_.array_val= new TokenArray();
	return t;
    }

    template<>
    inline
    Token SLIInterpreter::new_token<sli3::litproceduretype>()
    {
	Token t(types_[sli3::litproceduretype]);
	t.data_.array_val= new TokenArray() ;
	return t;
    }

    template<>
    inline
    Token SLIInterpreter::new_token<sli3::dictionarytype>()
    {
	Token t(types_[sli3::dictionarytype]);
	t.data_.dict_val= new Dictionary();
	return t;
    }

    template<>
    inline
    Token SLIInterpreter::new_token<sli3::marktype>()
    {
	Token t(types_[sli3::marktype]);
	return t;
    }

    template<>
    inline
    Token SLIInterpreter::new_token<sli3::integertype,int>(int i)
    {
	Token t(types_[sli3::integertype]);
	t.data_.long_val= i;
	return t;
    }

    template<>
    inline
    Token SLIInterpreter::new_token<sli3::integertype,long>(long l)
    {
	Token t(types_[sli3::integertype]);
	t.data_.long_val= l;
	return t;
    }

    template<>
    inline
    Token SLIInterpreter::new_token<sli3::integertype,unsigned long>(unsigned long ul)
    {
	Token t(types_[sli3::integertype]);
	t.data_.long_val= static_cast<long>(ul);
	return t;
    }

    template<>
    inline
    Token SLIInterpreter::new_token<sli3::doubletype,double>(double d)
    {
	Token t(types_[sli3::doubletype]);
	t.data_.double_val= d;
	return t;
    }

    template<>
    inline
    Token SLIInterpreter::new_token<sli3::booltype,bool>(bool b)
    {
	Token t(types_[sli3::booltype]);
	t.data_.bool_val= b;
	return t;
    }

    template<>
    inline
    Token SLIInterpreter::new_token<sli3::nametype,Name>(Name n)
    {
	Token t(types_[sli3::nametype]);
	t.data_.name_val= n.toIndex();
	return t;
    }

    template<>
    inline
    Token SLIInterpreter::new_token<sli3::literaltype,Name>(Name n)
    {
	Token t(types_[sli3::literaltype]);
	t.data_.name_val= n.toIndex();
	return t;
    }

    template<>
    inline
    Token SLIInterpreter::new_token<sli3::symboltype,Name>(Name n)
    {
	Token t(types_[sli3::symboltype]);
	t.data_.name_val= n.toIndex();;
	return t;
    }

    template<>
    inline
    Token SLIInterpreter::new_token<sli3::arraytype, TokenArray>(TokenArray a)
    {
	Token t(types_[sli3::arraytype]);
	t.data_.array_val= new TokenArray(a);
	return t;
    }

    template<>
    inline
    Token SLIInterpreter::new_token<sli3::arraytype, TokenArray *>(TokenArray * a)
    {
	Token t(types_[sli3::arraytype]);
	t.data_.array_val= a;
	return t;
    }

    template<>
    inline
    Token SLIInterpreter::new_token<sli3::stringtype, std::string >(std::string s)
    {
	Token t(types_[sli3::stringtype]);
	t.data_.string_val= new SLIString(s) ;
	return t;
    }

    template<>
    inline
    Token SLIInterpreter::new_token<sli3::stringtype, std::string *>(std::string *s)
    {
	Token t(types_[sli3::stringtype]);
	t.data_.string_val= new SLIString(*s) ;
	return t;
    }

    template<>
    inline
    Token SLIInterpreter::new_token<sli3::dictionarytype, sli3::Dictionary *>(sli3::Dictionary *d)
    {
	Token t(types_[sli3::dictionarytype]);
	t.data_.dict_val= d;
	return t;
    }

    template<class T>
    void SLIInterpreter::addmodule(void)
    {
	SLIModule *m=new T();
	
	modules_.push_back(m);
	m->install(std::cout,this);
    }

}
#endif
