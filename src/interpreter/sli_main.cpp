#include "sli_interpreter.h"
#include "sli_token.h"
#include "sli_dictionary.h"
#include <iostream>

int main()
{
    sli3::SLIInterpreter engine;
    engine.execute(2);

    if (engine.stats_enabled())
        engine.dump_stats(std::cerr);

    std::cerr << "Allocations: " << sli3::TokenArray::allocations << '\n';
    std::cerr << "Token: "<< sizeof(sli3::Token) << '\n';
    std::cerr << "Dictionary: "<< sizeof(sli3::Dictionary) << '\n';

    return 0;
}
