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
    class SLIInterpreter
    {
    public:
	SLIInterpreter();
	~SLIInterpreter();

	void init();
	void init_types();

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

	Token &top();

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
	    
	Token& index(size_t);

	Token const& index(size_t i) const
	    {
		return index(i);
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


    private:
	TokenStack operand_stack_;
	TokenStack execution_stack_;
	DictionaryStack dictionary_stack_;
	sli3::pool token_memory;       //!< Memory allocator for token
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
      TokenRef SLIInterpreter::new_token<sli3::nametype,Name>(Name const &);
    template<>
      TokenRef SLIInterpreter::new_token<sli3::literaltype,Name>(Name const &);
    template<>
      TokenRef SLIInterpreter::new_token<sli3::symboltype,Name>(Name const &);
    template<>
      TokenRef SLIInterpreter::new_token<sli3::stringtype,std::string>(std::string const &);
    
}
#endif
