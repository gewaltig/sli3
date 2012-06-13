#include "sli_interpreter.h"
#include "sli_nametype.h"
#include "sli_dicttype.h"
#include "sli_functiontype.h"
#include "sli_string.h"
#include "sli_stringtype.h"


namespace sli3
{

    SLIInterpreter::SLIInterpreter()
	:types_(),
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

    void SLIInterpreter::push(const Token &t)
    {
      std::cout << operand_stack_.size() << '\n';
      operand_stack_.push(t);
      operand_stack_.dump(std::cerr);
    }

    template<>
    void SLIInterpreter::push<TokenRef>(TokenRef t)
    {
      std::cout << operand_stack_.size() << '\n';
      operand_stack_.push(t);
      operand_stack_.dump(std::cerr);
    }

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
    TokenRef SLIInterpreter::new_token<sli3::integertype>()
    {
	TokenRef t(types_[sli3::integertype]);
	t.data_.long_val= 0;
	return t;
    }

    template<>
    TokenRef SLIInterpreter::new_token<sli3::doubletype>()
    {
	TokenRef t(types_[sli3::doubletype]);
	t.data_.double_val= 0;
	return t;
    }

    template<>
    TokenRef SLIInterpreter::new_token<sli3::arraytype>()
    {
	TokenRef t(types_[sli3::arraytype]);
	t.data_.array_val= new TokenArray();
	return t;
    }

    template<>
    TokenRef SLIInterpreter::new_token<sli3::litproceduretype>()
    {
	TokenRef t(types_[sli3::litproceduretype]);
	t.data_.array_val= new TokenArray() ;
	return t;
    }

    template<>
    TokenRef SLIInterpreter::new_token<sli3::dictionarytype>()
    {
	Token t(types_[sli3::dictionarytype]);
	t.data_.dict_val= new Dictionary();
	return TokenRef(t);
    }

    template<>
    TokenRef SLIInterpreter::new_token<sli3::integertype,int>(int const &i)
    {
	Token t(types_[sli3::integertype]);
	t.data_.long_val= i;
	return TokenRef(t);
    }
    template<>
    TokenRef SLIInterpreter::new_token<sli3::integertype,long>(long const &l)
    {
	Token t(types_[sli3::integertype]);
	t.data_.long_val= l;
	return TokenRef(t);
    }
    template<>
    TokenRef SLIInterpreter::new_token<sli3::doubletype,double>(double const &d)
    {
	Token t(types_[sli3::integertype]);
	t.data_.double_val= d;
	return TokenRef(t);
    }
    template<>
    TokenRef SLIInterpreter::new_token<sli3::nametype,Name>(Name const &n)
    {
	Token t(types_[sli3::nametype]);
	t.data_.name_val= n.toIndex();;
	return TokenRef(t);
    }

    template<>
    TokenRef SLIInterpreter::new_token<sli3::literaltype,Name>(Name const &n)
    {
	Token t(types_[sli3::literaltype]);
	t.data_.name_val= n.toIndex();;
	return TokenRef(t);
    }

    template<>
    TokenRef SLIInterpreter::new_token<sli3::symboltype,Name>(Name const &n)
    {
	Token t(types_[sli3::symboltype]);
	t.data_.name_val= n.toIndex();;
	return TokenRef(t);
    }

    template<>
    TokenRef SLIInterpreter::new_token<sli3::stringtype,std::string>(std::string const &s)
    {
	TokenRef t(types_[sli3::stringtype]);
	t.data_.string_val= new SLIString(s) ;
	return t;
    }

}
