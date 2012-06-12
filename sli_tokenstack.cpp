/*
 *  tokenstack.cc
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

#include "sli_tokenstack.h"
/* 
    tokenstack.cc
*/
namespace sli3
{

void TokenStack::dump(std::ostream &out) const
{
    out << '\n';
    out << " --> ";
    for(size_t i = 0; i<load() ; i++)
    {
	if (i!=0)
	    out << "     ";
	pick(i).print(out);
	out << '\n';
    }
    out << "   " << "--------------------\n";
    out << '\n';
}    


}
