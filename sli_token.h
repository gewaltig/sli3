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
    class SLIistream;
    class SLIostream;
    class TypeNode;

    class Token
    {
    public:
	Token();
	Token(SLIType *);
	Token(const Token &);
	~Token();
	

	/** 
	 * Initialize from a Token. This function assumes that the object is uninitialized.
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
	bool operator==(Name) const;
	bool operator==(int) const;
	bool operator==(unsigned int) const;
	bool operator==(long) const;
	bool operator==(unsigned long) const;
	bool operator==(double) const;
	bool operator==(bool) const;
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
	    SLIistream *istream_val;
	    SLIostream *ostream_val;
	    TypeNode *trie_val;
	    SLIType *type_val;
	} data_;
    };
    
    std::ostream & operator<<(std::ostream& , const Token&);


    inline
    Token::Token()
	:type_(0), data_()
    {}

    inline
    Token::Token(SLIType *t)
	:type_(t), data_()
    {}

    inline
    Token::Token(Token const& s)
	:type_(s.type_), 
	 data_(s.data_)
    {
	if(type_ != 0)
	    type_->add_reference(*this);
    }

    inline
    Token::~Token()
    {
      remove_reference();
    }

    inline
    void Token::clear()
    {
      remove_reference();
      type_=0;
      //data_=value();
    }

    inline
    Token& Token::init(const Token&t)
    {
      t.add_reference();
      type_=t.type_;
      data_=t.data_;
      return *this;
    }

    inline
    Token& Token::move( Token&t)
    {
      clear();
      init(t);
      t.clear();
      return *this;
    }

    inline
    Token& Token::swap(Token&t)
    {
	SLIType *tmp_type=type_;
	value   tmp_data=data_;

	type_=t.type_;
	data_=t.data_;
	t.type_=tmp_type;
	t.data_=tmp_data;

	return *this;
    }

    inline
    Token& Token::operator=(const Token &t)
    {
      clear();
      return init(t);
    }

    inline
    bool Token::is_valid() const
    {
	return type_ !=0;
    }
    
    inline
    refcount_t Token::references() const
    {
	return (is_valid()) ? type_->references(*this):1;
    }
    
    inline
    refcount_t Token::add_reference() const
    {
	return (is_valid()) ? type_->add_reference(*this) : 1;
    }

    inline
    void Token::remove_reference()
    {
	if(is_valid())
	  type_->remove_reference(*this);
    }

    inline
    Token::operator int() const
    {
	require_type(sli3::integertype);
	return data_.long_val;
    }

 
    inline
    Token::operator long() const
    {
	require_type(sli3::integertype);
	return data_.long_val;
    }

    inline
    Token::operator long&() 
    {
	require_type(sli3::integertype);
	return data_.long_val;
    }

    inline
   Token::operator double&()
    {
	require_type(sli3::doubletype);
	return data_.double_val;
    }

    inline
   Token::operator double() const
    {
	require_type(sli3::doubletype);
	return data_.double_val;
    }

    inline
    Token::operator bool() const
    {
	require_type(sli3::booltype);
	return data_.bool_val;
    }

    inline
    Token::operator bool&() 
    {
	require_type(sli3::booltype);
	return data_.bool_val;
    }


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

    inline
    void Token::execute()
    {
      if(type_)
	{
	  type_->execute(*this);
	  return;
	}
      throw InvalidToken();
    }


}
    
#endif
