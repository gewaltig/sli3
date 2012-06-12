/*
 *  parser.h
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

#ifndef SLI_PARSER_H
#define SLI_PARSER_H
/* 
    SLI's parser.
*/

#include "sli_token.h"
#include "sli_tokenstack.h"
#include <typeinfo>
#include <iostream>

namespace sli3
{
  class Scanner;
  class SLIInterpreter;
  class Parser 
  {
      Scanner* s;
      
      Token arraytoken;
      Token proctoken;
      TokenStack ParseStack; 
      
      enum ParseResult   {
	  tokencontinue,
	  scancontinue,
	  tokencompleted,
	  noopenproc,
	  endprocexpected,
	  noopenarray,
	  endarrayexpected,
	  unexpectedeof
      };
      
      void init(std::istream &);
      
  public:    
      Parser(void);
      Parser(std::istream &);
      
      bool operator()(SLIInterpreter &, Token&);
      bool readToken(SLIInterpreter &sli, std::istream &is, Token &t)
	  {
	      s->source(sli, &is);
	      return operator()(sli, t);
	  }
      
      bool readSymbol(SLIInterpreter &sli, std::istream &is, Token &t)
	  {
	      s->source(sli, &is);
	      return s->operator()(sli, t);
	  }
      
      Scanner const* scan(void) const
	  {
	      return s;
	  }
      
      void clear_context()
	  {
	      if(s !=NULL)
	      {
		  s->clear_context();
	      }
	  }
  };
    
    bool operator==(Parser const &, Parser const &);
    
    std::ostream& operator<<(std::ostream&, const Parser&);
    
}
    

#endif













