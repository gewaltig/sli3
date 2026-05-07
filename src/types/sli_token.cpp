#include "sli_token.h"
#include "sli_type.h"
#include "sli_exceptions.h"
#include "sli_string.h"
namespace sli3
{


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
	return data_.string_val->str();
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
	return type_ and type_->get_typeid()==sli3::doubletype and data_.double_val==d;
    }

    bool Token::operator==(bool b) const
    {
	return type_ and type_->get_typeid()==sli3::booltype and data_.bool_val==b;
    }

    bool Token::operator==(Name n) const
    {
	// nametype / literaltype / symboltype all carry a Name handle in
	// data_.name_val; they differ only in execution semantics. The
	// parser returns a symboltype for EndSymbol, and Token_sFunction
	// compares the result against the EndSymbol Name to detect
	// end-of-input. Restricting to nametype here silently made that
	// comparison always false, so cst's tokenize loop never
	// terminated.
	if (!type_) return false;
	const auto tid = type_->get_typeid();
	return (tid == sli3::nametype
	        || tid == sli3::literaltype
	        || tid == sli3::symboltype)
	    && data_.name_val == n.toIndex();
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

    std::ostream & operator<<(std::ostream &out, const Token &t)
    {
	return t.print(out);
    }
    

}
