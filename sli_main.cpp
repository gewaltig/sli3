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
    return 0;
}
