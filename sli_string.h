/*
 *  lockptr.h
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

#ifndef SLI_STRING_H
#define SLI_STRING_H

#include <cassert>
#include <string>

/**

*/
namespace sli3
{
  class SLIString: public std::string
    {
      mutable size_t refs_;
    public:
      SLIString(){}
      SLIString(const std::string &s)
	:std::string(s){}

      
      size_t add_reference() const
      {
	return ++refs_;
      }
      
      size_t references() const
      {
	return refs_;
      }
      
      size_t remove_reference()
      {
	if( --refs_==0)
	  {
	    delete this;
	    return 0;
	  }
	return refs_;
      }
    };
}

#endif
