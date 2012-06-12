/*
 *  charcode.h
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

#ifndef SLI_CharCode_H
#define SLI_CharCode_H
/* 

    Character codes for the scanner
*/

/***************************************************************/
/* class CharCode                                              */
/***************************************************************/
#include <vector>

namespace sli3
{
  using std::size_t;
  
  class CharCode : public std::vector<size_t> {
  public:
    CharCode(size_t, size_t);
    
    void Range(size_t,  char,   char);
    void Group(size_t, const char*);
    size_t operator()( char) const;
  };
}


#endif
