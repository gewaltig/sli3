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
    
    
    engine.push(10);
    engine.push(1.5);
    engine.push(20);
    engine.push(30);
    engine.pop();
    engine.push(sli3::Name("Hello"));

    engine.push(engine.new_token<sli3::arraytype>());
    sli3::TokenArray *d=sli3::token_value<sli3::arraytype,sli3::TokenArray*>(engine.top());
    std::cerr << *d << '\n';
    engine.push(engine.new_token<sli3::dictionarytype>());
    engine.push(engine.new_token<sli3::stringtype>(std::string("Hello World")));
    sli3::Parser parse(std::cin);
    for(int i=0; i<10; ++i)
      {
	std::cerr << "sli3 >";
	sli3::Token t;
	parse(engine,t);
	engine.push(t);
      }
    
    return 0;
}
