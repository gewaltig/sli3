/*
 *  slibuiltins.cc
 *
 *  This file is part of NEST
 *
 *  Copyright (C) 2004 by
 *  The NEST Initiative
 *
 *  See the file AUTHORS for details.
 *
 *  Permission is granted to compile and modify
 *  this file for non-commercial use.
 *  See the file LICENSE for details.
 *
 */

/* 
    Interpreter builtins
*/
#include "sli_builtins.h"
#include "sli_interpreter.h"
#include "sli_array.h"
#include "sli_string.h"
#include "sli_iostream.h"

namespace sli3
{
// Phase 5 (final): the 5 remaining iter-helper SLIFunction classes
// (IparseFunction, IloopFunction, IforallstringFunction,
// IforallindexedarrayFunction, IforallindexedstringFunction) joined
// the 6 previously-dead ones. All 11 are replaced by TYPE markers:
//   iparsetype, ilooptype, iforallstringtype,
//   iforallindexedarraytype, iforallindexedstringtype
// (plus the original iiteratetype, irepeattype, ifortype,
// iforalltype). The dispatcher handles them inline in body_walk
// (for the body-walking ones) or in a dedicated outer-switch case
// (for iparse, which reads from a stream rather than walking a
// proc body). See sli_interpreter.cpp:execute_dispatch_.

void ArraycreateFunction::execute(SLIInterpreter *i) const
{
//  call: mark t1 ... tn  arraycreate -> array
    size_t depth = i->load();
    size_t n   = 0;
    bool found = false;

    while( (n < depth) && !found)
    {
	found = (i->pick(n++).is_of_type(sli3::marktype));
    }

    if(found)
    {
	TokenArray *ad= new TokenArray();
	ad->resize(n-1);
	for(size_t l=2; l <= n; ++l)
	    (*ad)[l-2].init((i->pick(n-l)));
	i->pop(n);
	i->push(i->new_token<sli3::arraytype>(ad));
	// new-ABI: dispatcher pre-popped the /] slot.
    }
    else
    {
	i->message(sli3::M_ERROR, "arraycreate","Opening bracket missing.");
	i->raiseerror("SyntaxError");
	return;
    }
}

// `mark k1 v1 ... kn vn  >>  -> dict` — close-dict operator. Pairs
// keys and values down to the mark, builds a fresh Dictionary, replaces
// everything from the mark up. Mirrors NEST 2.20.2 sli/slidict.cc
// DictconstructFunction (lines 631-681).
void DictconstructFunction::execute(SLIInterpreter *i) const
{
    size_t depth = i->load();
    size_t n = 0;
    bool found = false;
    while ((n < depth) && !found)
    {
        found = i->pick(n++).is_of_type(sli3::marktype);
    }
    if (!found)
    {
        i->message(sli3::M_ERROR, ">>", "Opening << missing.");
        i->raiseerror("SyntaxError");
        return;
    }
    // n now counts pick(0)..pick(n-1) where pick(n-1) is the mark.
    // The pairs occupy pick(0)..pick(n-2); needs (n-1) % 2 == 0.
    size_t pairs_count = n - 1;
    if (pairs_count % 2 != 0)
    {
        i->message(sli3::M_ERROR, ">>",
                   "Initializer must be /key value pairs.");
        i->raiseerror(i->ArgumentTypeError);
        return;
    }
    Dictionary* d = new Dictionary();
    // Walk top-down in pairs of (val, key). The key sits ABOVE the
    // value when reading top-down on the operand stack:
    //   stack: ... mark key1 val1 key2 val2 (top)
    //   pick(0)=val2, pick(1)=key2, pick(2)=val1, pick(3)=key1
    // So pick(2*k)   is val_{n-k},  pick(2*k+1) is key_{n-k}.
    for (size_t k = 0; k < pairs_count / 2; ++k)
    {
        Token& val = i->pick(2 * k);
        Token& key = i->pick(2 * k + 1);
        if (!key.is_of_type(sli3::literaltype))
        {
            i->message(sli3::M_ERROR, ">>",
                       "Literal expected as dictionary key.");
            i->raiseerror(i->ArgumentTypeError);
            delete d;
            return;
        }
        d->insert(key.data_.name_val, val);
    }
    i->pop(n);
    Token dict_tok(i->get_type(sli3::dictionarytype));
    dict_tok.data_.dict_val = d;
    i->push(dict_tok);
    // new-ABI: dispatcher pre-popped the />> slot.
}


}
