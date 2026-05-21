#include "sli_interpreter.h"
#include "sli_token.h"
#include "sli_dictionary.h"
#include <iostream>

int main(int argc, char** argv)
{
    // argv is plumbed through to statusdict /argv so that the SLI
    // bootstrap's /:commandline (lib/sli/sli-init.sli) can inspect
    // it -- this enables `sli3 [opts] file.sli ...` to read files
    // from the command line, `-c "code"` to evaluate inline, and
    // `-h` / `-v` / `--batch` etc. See :commandline in sli-init.sli
    // for the full option grammar.
    sli3::SLIInterpreter engine(argc, argv);
    engine.execute(2);

    if (engine.stats_enabled())
        engine.dump_stats(std::cerr);

    std::cerr << "Allocations: " << sli3::TokenArray::allocations << '\n';
    std::cerr << "Token: "<< sizeof(sli3::Token) << '\n';
    std::cerr << "Dictionary: "<< sizeof(sli3::Dictionary) << '\n';

    return 0;
}
