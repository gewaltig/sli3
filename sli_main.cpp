#include "sli_interpreter.h"
#include "sli_token.h"
#include "sli_dictionary.h"
#include "sli_parser.h"
#include "sli_tokenutils.h"
#include <iostream>

int main()
{
    sli3::SLIInterpreter engine;
    engine.execute(2);

    std::cerr << "Allocations: " << sli3::TokenArray::allocations << '\n';
    std::cerr << "Token: "<< sizeof(sli3::Token) << '\n';
    std::cerr << "Dictionary: "<< sizeof(sli3::Dictionary) << '\n';

    return 0;
}
