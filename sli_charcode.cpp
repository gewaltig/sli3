/*
 *  charcode.cc
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

#include "sli_charcode.h"
#include <cassert>
namespace sli3
{
  CharCode::CharCode(size_t n, size_t def)
    : std::vector<size_t>(n+1,def)
  {
  }
  
  void CharCode::Range(size_t code, char lc,  char uc)
  {
    unsigned char lower=lc;
    unsigned char upper=uc;
    
    assert(lower <= upper);
    assert(upper < size());
    
    for(size_t i=lower; i<=upper; ++i)
      (*this)[i]=code;
  }
  
  void CharCode::Group(size_t code, const char* g)
  {
    while (*g)
      {
	unsigned char c=*g++;
	assert(c < size());
	(*this)[c]=code;
      }
  }
  
  size_t CharCode::operator()(char c) const
  {
    unsigned char chr= static_cast<unsigned char>(c);
    assert(chr < size());
    
    return (*this)[chr];
  }
  
}

