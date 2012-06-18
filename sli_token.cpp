#include "sli_token.h"
#include "sli_type.h"
#include "sli_exceptions.h"
#include "sli_string.h"
namespace sli3
{

    Token::Token()
	:type_(0), data_()
    {}

    Token::Token(SLIType *t)
	:type_(t), data_()
    {}

    Token::Token(Token const& s)
	:type_(s.type_), 
	 data_(s.data_)
    {
	if(type_ != 0)
	    type_->add_reference(*this);
    }

    Token::~Token()
    {
	if(type_)
	    type_->remove_reference(*this);
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
	require_type(sli3::integertype);
	return data_.long_val;
    }

 
    Token::operator long() const
    {
	require_type(sli3::integertype);
	return data_.long_val;
    }

    Token::operator long&() 
    {
	require_type(sli3::integertype);
	return data_.long_val;
    }

   Token::operator double&()
    {
	require_type(sli3::doubletype);
	return data_.double_val;
    }

   Token::operator double() const
    {
	require_type(sli3::doubletype);
	return data_.double_val;
    }

    Token::operator bool() const
    {
	require_type(sli3::booltype);
	return data_.bool_val;
    }

    Token::operator bool&() 
    {
	require_type(sli3::booltype);
	return data_.bool_val;
    }

    Token::operator TokenArray&() 
    {
  	if( not is_valid())
	    throw InvalidToken();
	unsigned int id=type_->get_typeid();
	if(id==sli3::arraytype or
	   id==sli3::proceduretype or
	   id==sli3::litproceduretype)
	{
	    return *data_.array_val;
	}
	
	throw TypeMismatch();

    }

    Token::operator std::string&()
    {
	require_type(sli3::stringtype);
	return *data_.string_val;
    }

    std::ostream & Token::print(std::ostream &out) const
    {
	if(type_ != 0)
	    return type_->print(out,*this);
	else
	    return out << "/nulltoken";
    }

    std::ostream & Token::pprint(std::ostream &out) const
    {
	if(type_ != 0)
	    return type_->pprint(out,*this);
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

    bool Token::operator==(int i) const
    {
	return type_ and type_->get_typeid()==sli3::integertype and data_.long_val==i;
    }

    bool Token::operator==(unsigned int i) const
    {
	return type_ and type_->get_typeid()==sli3::integertype and static_cast<unsigned int>(data_.long_val)==i;
    }

    bool Token::operator==(long l) const
    {
	return type_ and type_->get_typeid()==sli3::integertype and data_.long_val==l;
    }

    bool Token::operator==(unsigned long l) const
    {
	return type_ and type_->get_typeid()==sli3::integertype and static_cast<unsigned long>(data_.long_val)==l;
    }

    bool Token::operator==(double d) const
    {
	return type_ and type_->get_typeid()==sli3::integertype and data_.double_val==d;
    }

    bool Token::operator==(bool b) const
    {
	return type_ and type_->get_typeid()==sli3::integertype and data_.bool_val==b;
    }


    bool Token::operator!=(const Token &t) const
    {
	return not operator==(t);
    }

    Name Token::get_typename() const
    {
	if(type_ == 0)
	    throw InvalidToken();
	return type_->get_typename();
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
