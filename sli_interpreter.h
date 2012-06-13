#ifndef SLI3_INTERPRETER_H
#define SLI3_INTERPRETER_H

#include "sli_type.h"
#include "sli_token.h"
#include "sli_allocator.h"
#include "sli_arraytype.h"
#include "sli_integertype.h"
#include "sli_tokenstack.h"
#include "sli_name.h"
#include "sli_dictionary.h"
#include "sli_dictstack.h"
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

    class SLIInterpreter
    {
    public:

	SLIInterpreter();
	~SLIInterpreter();

	void init();
	void init_types();
	void init_dictionaries();
	void init_message_tags();

	/**
	 * Initiates the interpreter's startup sequnce.
	 */
	int startup();

	/**
	 * Execute the commands, supplied as string.
	 */
	int execute(const std::string &);

	/**
	 * Start interpreter in input-evaluate loop.
	 * 
	 */
	int execute(int=0);
	
	void raiseerror(std::exception&){}
	/**
	 * Execute until execution stack reaches level.
	 */
	int execute_(size_t level=0);

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
	void lookup2(Name n, Token &);
	
	/** Lookup a name searching only the bottom level dictionary.
	 *  If the Name is not found,
	 *  @a false is returned.
	 */
	bool baselookup(Name n, Token &);
	
	/** Test for a name searching all dictionaries on the stack.
	 */
	bool known(Name);
	
	/** Test for a name in the bottom level dictionary.
	 */
	bool baseknown(Name);
	
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
	void pop(size_t n=1)
	    {
		operand_stack_.pop(n);
	    }

	Token& pick(size_t);

	Token& index(size_t);

	Token const& index(size_t i) const
	    {
		return index(i);
	    }


	void estack_pop(size_t n=1)
	    {
		execution_stack_.pop(n);
	    }


	Token & estack_top(size_t n=1)
	    {
		return execution_stack_.top();
	    }

	Token & estack_pick(size_t n=0)
	    {
		return execution_stack_.pick(n);
	    }

	void estack_push(const Token &t)
	    {
		execution_stack_.push(t);
	    }


	/**
	 * Fill token with an object of the specified type.
	 */
	template <sli_typeid, class T>
	  TokenRef new_token(T const&);

	template <sli_typeid>
	  TokenRef new_token();

	SLIType* get_type(sli_typeid id) const
	{
	  return types_.at(id);
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


	    // Names of basics functions
	Name ilookup_name;
	Name ipop_name;
	Name isetcallback_name;
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
	
	Name iparse_name;
	Name stop_name;
	Name end_name;
	
	// Names of symbols and objects
	Name null_name;
	Name true_name;
	Name false_name;
	Name mark_name;
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
	
    private:
	bool is_initialized_;
	bool debug_mode_;       //!< True, if SLI level debugging is enabled.
	bool show_stack_;       //!< Show stack in debug mode.
	bool show_backtrace_;   //!< Show stack-backtrace on error.
	bool catch_errors_;     //!< Enter debugger on error.
	bool opt_tailrecursion_;//!< Optimize tailing recursion.
	bool cycle_guard;

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

	TokenStack operand_stack_;
	TokenStack execution_stack_;
	DictionaryStack dictionary_stack_;
	sli3::pool token_memory;       //!< Memory allocator for token
	std::vector<std::string> message_tag_;
	std::vector<SLIType *> types_; // must be last, so it is deleted last
    };

    template<>
    void SLIInterpreter::push<TokenRef>(TokenRef);

    template<>
    void SLIInterpreter::push<int>(int);

    template<>
    void SLIInterpreter::push<long>(long);

    template<>
    void SLIInterpreter::push<double>(double);

    template<>
    void SLIInterpreter::push<Name>(Name);

    template<>
      TokenRef SLIInterpreter::new_token<sli3::integertype>();

    template<>
      TokenRef SLIInterpreter::new_token<sli3::doubletype>();

    template<>
      TokenRef SLIInterpreter::new_token<sli3::arraytype>();

    template<>
      TokenRef SLIInterpreter::new_token<sli3::litproceduretype>();

    template<>
      TokenRef SLIInterpreter::new_token<sli3::dictionarytype>();

    template<>
      TokenRef SLIInterpreter::new_token<sli3::integertype,int>(int const&);

    template<>
      TokenRef SLIInterpreter::new_token<sli3::integertype,long>(long const &);

    template<>
      TokenRef SLIInterpreter::new_token<sli3::doubletype,double>(double const &);
    template<>
      TokenRef SLIInterpreter::new_token<sli3::booltype,bool>(bool const &);

    template<>
      TokenRef SLIInterpreter::new_token<sli3::nametype,Name>(Name const &);
    template<>
      TokenRef SLIInterpreter::new_token<sli3::literaltype,Name>(Name const &);
    template<>
      TokenRef SLIInterpreter::new_token<sli3::symboltype,Name>(Name const &);
    template<>
      TokenRef SLIInterpreter::new_token<sli3::stringtype,std::string>(std::string const &);
    

    inline
    bool SLIInterpreter::lookup(Name n, Token &t)
    {
	return dictionary_stack_.lookup(n, t);
    }

    inline
    void SLIInterpreter::lookup2(Name n, Token &t)
    {
	dictionary_stack_.lookup2(n, t);
    }

    inline
    bool SLIInterpreter::baselookup(Name n, Token &t)
    {
	return dictionary_stack_.baselookup(n,t);
    }

    inline
    bool SLIInterpreter::known(Name n)
    {
	Token t;
	return dictionary_stack_.lookup(n,t);
    }

    inline
    bool SLIInterpreter::baseknown(Name n) 
    {
	Token t;
	return dictionary_stack_.baselookup(n,t);
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

}
#endif
