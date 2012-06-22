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
 	      << "Dictionary: " << sizeof(sli3::Dictionary) <<'\n'
	      << "token::value: " << sizeof(sli3::Token::value) <<'\n';
    engine.execute();

    std::cerr << "Allocations: " << sli3::TokenArray::allocations << '\n';
    return 0;
}
