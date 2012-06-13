#include "sli_token.h"
#include "sli_type.h"
#include "sli_exceptions.h"
namespace sli3
{

    Token::Token()
	:type_(0), data_()
    {}

    Token::Token(SLIType *t)
	:type_(t), data_()
    {}

    Token::Token(Token const& s)
	:type_(s.type_), data_(s.data_)
    {
      if(type_ != 0)
	type_->add_reference(s);
    }

    Token::~Token()
    {
      clear();
    }

    void Token::clear()
    {
	if (type_)
	    type_->clear(*this);
	type_=0;
    }

    Token& Token::init(const Token&t)
    {
      t.add_reference();
      remove_reference();
      type_=t.type_;
      data_=t.data_;
      return *this;
    }

    Token& Token::move( Token&t)
    {
      t.add_reference();
      remove_reference();
      type_=t.type_;
      data_=t.data_;
      t.clear();
      return *this;
    }

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

    Token& Token::operator=(const Token &t)
    {
	return init(t);
    }

    bool Token::is_valid() const
    {
	return type_ !=0;
    }
    
    refcount_t Token::references() const
    {
	return (is_valid()) ? type_->references(*this):1;
    }
    
    refcount_t Token::add_reference() const
    {
	return (is_valid()) ? type_->add_reference(*this) : 1;
    }

    void Token::remove_reference()
    {
	if(is_valid())
	  type_->remove_reference(*this);
    }

    Token::operator int() const
    {
	if(type_ && type_->is_type(sli3::integertype))
	    return data_.long_val;
	else
	    throw TypeMismatch();
    }

    Token::operator long() const
    {
	if(type_ && type_->is_type(sli3::integertype))
	    return data_.long_val;
	else
	    throw TypeMismatch();
    }

    Token::operator double() const
    {
	if(type_ && type_->is_type(sli3::doubletype))
	    return data_.double_val;
 	else
	    throw TypeMismatch();
   }

    Token::operator bool() const
    {
	if(type_ && type_->is_type(sli3::booltype))
	    return data_.bool_val;
 	else
	    throw TypeMismatch();
   }

    long& Token::operator=(long val)
    {
	if(type_ && type_->is_type(sli3::integertype))
	    return data_.long_val=val;
  	else
	    throw TypeMismatch();
    }

    double& Token::operator=(double val)
    {
	if(type_ && type_->is_type(sli3::doubletype))
	    return data_.double_val=val;
  	else
	    throw TypeMismatch();
    }

    std::ostream & Token::print(std::ostream &out) const
    {
	if(type_ != 0)
	    return type_->print(out,*this);
	else
	    return out << "/nulltoken";
    }

    bool Token::operator==(const Token &t) const
    {
	if(type_ != t.type_)
	    return false;
	if(!type_)
	    return true;
	return type_->compare(*this,t);
    }

    bool Token::operator!=(const Token &t) const
    {
	return not operator==(t);
    }

    std::string Token::get_typename() const
    {
	if (type_)
	    return type_->get_typename();
	return "/nulltype";
    }

    void Token::execute()
    {
	if(type_==0)
	    throw InvalidToken();
	type_->execute(*this);
    }

    std::ostream & operator<<(std::ostream &out, const Token &t)
    {
	return t.print(out);
    }
    

}
