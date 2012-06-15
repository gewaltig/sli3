#include "sli_interpreter.h"
#include "sli_token.h"
#include "sli_dictionary.h"
#include "sli_parser.h"
#include "sli_tokenutils.h"
#include <iostream>

int main()
{
    sli3::SLIInterpreter engine;
    sli3::Dictionary dict=new Dictionary();
    
    Name n1("A");
    Name n2("B");

    sli3::Token t1(engine.new_token<sli3::integertype>(10));
    assert(t1.type_ == engine.get_type(sli3::integertype));
    assert(t1.data_.long_val==10);

    sli3::Token t2;
    t2=t1;
    assert(t2.type_ == engine.get_type(sli3::integertype));
    assert(t2.data_.long_val==10);
    assert(t1==t2);

    dict->insert(n1,t1);
    assert(dict->known(n1));
    dict->insert(n2,t2);
    assert(dict->known(n2));
    dict->dump(std::cerr);

    return 0;

}
