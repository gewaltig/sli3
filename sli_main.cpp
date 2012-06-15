#include "sli_interpreter.h"
#include "sli_token.h"
#include "sli_dictionary.h"
#include "sli_parser.h"
#include "sli_tokenutils.h"
#include <iostream>

int main()
{
    sli3::SLIInterpreter engine;
    std::cout << "int: " << sizeof(int) << '\n'
	      << "unsigned int: " << sizeof(unsigned int) <<'\n'
 	      << "long: " << sizeof(long) <<'\n'
	      << "Token: " << sizeof(sli3::Token) << '\n'
	      << "Array: " << sizeof(sli3::TokenArray) <<'\n'
 	      << "Dictionary: " << sizeof(sli3::Dictionary) <<'\n';

    sli3::Dictionary *d= new sli3::Dictionary();
    sli3::Name n1("n1");
    sli3::Name n2("n2");
    
    d->insert(n1, engine.new_token<sli3::integertype>(10));
    d->insert(n2, engine.new_token<sli3::integertype>(20));
    d->info(std::cerr);
    delete d;
    return 0;
    
    // engine.push(10);
    // engine.push(1.5);
    // engine.push(20);
    // engine.push(30);
    // engine.pop();
    // engine.push(sli3::Name("Hello"));

    // engine.push(engine.new_token<sli3::arraytype>());
    // sli3::Token t=engine.top();
    // t.require_type(sli3::arraytype);
    // sli3::TokenArray *d= t.data_.array_val;
    // d->reserve(10);
    // for(int i=0;i<10;++i)
    // 	d->push_back(engine.new_token<sli3::integertype>(i));
    // std::cerr << '[' << (*d) << "]\n";
    // engine.push(t);
    // engine.push(engine.new_token<sli3::dictionarytype>());
    // engine.push(engine.new_token<sli3::stringtype>(std::string("Hello World")));
    // sli3::Parser parse(std::cin);
    // for(int i=0; i<10; ++i)
    //   {
    // 	std::cerr << "sli3 >";
    // 	sli3::Token t;
    // 	parse(engine,t);
    // 	engine.push(t);
    //   }
    
    // return 0;
}
