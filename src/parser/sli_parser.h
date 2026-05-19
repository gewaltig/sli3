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
#include "sli_name.h"
#include "sli_tokenstack.h"
#include "sli_scanner.h"
#include <typeinfo>
#include <iostream>
#include <utility>
#include <vector>

namespace sli3
{
  class Scanner;
  class SLIInterpreter;
  class Parser
  {
      Scanner* s;
      TokenStack stack_;
      // Parallel to stack_: line/col where each unmatched `{` was
      // opened. Pushed when BeginProcedureSymbol is seen, popped
      // when EndProcedureSymbol matches. On unexpectedeof, the
      // outermost (front) entry pinpoints the brace whose body
      // ran past EOF -- usually the actual bug.
      std::vector<std::pair<unsigned long, unsigned long>> open_brace_positions_;
      Name open_array_;
      Name close_array_;
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
      ~Parser();

      bool operator()(SLIInterpreter &, Token&);
      bool readToken(SLIInterpreter &sli, std::istream &is, Token &t)
	  {
	      s->source(&is);
	      return operator()(sli, t);
	  }
      
      bool readSymbol(SLIInterpreter &sli, std::istream &is, Token &t)
	  {
	      s->source(&is);
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













