#ifndef SLI_TOKEN_H
#define SLI_TOKEN_H
#include <iostream>
#include <string>
#include "sli_type.h"
#include "sli_exceptions.h"

/** 
 * Struct Datum is the smnallest memory unit. The linked type object decides
 * how this memory unit is interpreted.
 * The interpreter manages all memory for datums.
 * Compound objects are represented as follows:
 * array: array_type 1 datum with pointer to array_rep;
 * array_rep, string_rep
 *    n_ref ! len ! [ d1,...,d_len]
 * s_expr (for lists)
 *    n_ref ! lchild ! r_child
 */

namespace sli3
{
    class SLIType;
    class TokenArray;
    class SLIFunction;
    class Dictionary;
    class SLIString;

    class Token
    {
    public:
	Token();
	Token(SLIType *);
	Token(const Token &);
	virtual ~Token();
	

	/** 
	 * Initialize from a Token.
	 */
	Token & init(const Token &t);

	/**
	 * Initialize by assignment operation.
	 */
	Token & operator=(const Token &t);

	/**
	 * Move contents of t to this token.
	 * The after assigning the contents of t, t is cleared.
	 */
	Token & move(Token &t);

	/**
	 * Exchange the contents of t and this token.
	 */
	Token & swap(Token &t);
	
	/**
	 * Clear contents of token.
	 * 
	 */
	void clear();
	
	operator int() const;
	operator long() const;
	operator long&();
	operator double() const;
	operator double&();
	operator bool() const;
	operator bool&();
	operator std::string() const;
	operator std::string&();

	operator std::vector<long>() const;
	operator std::vector<double>() const;
	operator std::istream&();
	operator std::ostream&();
	operator TokenArray&();

	bool is_valid() const;

	refcount_t references() const;
	refcount_t add_reference() const;
	void remove_reference();
	
	bool operator==(const Token&) const;
	bool operator!=(const Token&) const;

	bool is_of_type(unsigned int) const;
	void require_type(unsigned int) const;

	Name get_typename() const;

	void execute();

	bool is_executable() const
	{
	  return (type_ and type_->is_executable());
	}
	std::ostream & print(std::ostream &) const;
	std::ostream & pprint(std::ostream &) const;
	
	SLIType *type_; //!< If NULL, the datum is unused.	
	union value
	{
	    double double_val;
	    long   long_val;   
	    bool   bool_val;
	    size_t  name_val;
	    TokenArray *array_val;
	    SLIFunction *func_val;
	    Dictionary *dict_val;
	    SLIString  *string_val;
	    void  *data_val;    //! Pointer to external resources or function. 
	} data_;
    };
    
    std::ostream & operator<<(std::ostream& , const Token&);

    inline
    bool Token::is_of_type(unsigned int id) const
    {
	return type_ and type_->is_type(id);
    }

    inline
      void Token::require_type(unsigned int id) const
    {
      if(not type_)
	throw InvalidToken();
      type_->require_type(id);
    }

    class TokenRef: public Token
    {
    public:
	TokenRef(): Token(){}
	TokenRef(TokenRef &tr)
	    {
		type_=tr.type_;
		data_=tr.data_;
	    }
	TokenRef(const Token&t)
	    {
		type_=t.type_;
		data_=t.data_;
	    }
	
	operator Token()
	    {
		Token t;
		t.type_= type_;
		t.data_= data_;
		return t;
	    }
    };
}
    
#endif
