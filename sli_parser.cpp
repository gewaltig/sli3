/*
 *  parser.cc
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
    parser.cc
*/
#include "sli_interpreter.h"
#include "sli_scanner.h"
#include "sli_parser.h"
#include "sli_arraytype.h"
#include "sli_nametype.h"
//#include "config.h"
namespace sli3
{
/*****************************************************************/
/* parse                                                         */
/* --------            Token --> Token                           */
/*                                                               */
/* Errors:                                                       */
/*                                                               */
/*                                                               */
/*                                                               */
/*****************************************************************/

void Parser::init(std::istream &is)
{
    s=new Scanner(&is);
}

Parser::Parser(std::istream &is)
  :s(NULL), stack_(128),
   open_array_("["),
   close_array_("]")
{
    init(is);
    assert(s !=NULL);
}

Parser::Parser(void)
  :s(NULL), stack_(128),
   open_array_("["),
   close_array_("]")   
{
    init(std::cin);
    assert(s !=NULL);
    
}

inline
  bool contains_symbol(Token const &t, Name n)
  {
    if( !t.is_valid())
      return false;
    
    return t.type_->is_type(sli3::symboltype) and
      t.data_.name_val == n.toIndex();
  }

  bool Parser::operator()(SLIInterpreter &sli, Token& t)
  {
    assert(s != NULL);
    
    Token pt;
    
    bool ok;
    ParseResult result=scancontinue;
    
    do
      {
	if(result == scancontinue)
	  ok=(*s)(sli, t);
	else ok=true;
        
        
	if (ok) 
	  {
	      if (contains_symbol(t,s->BeginProcedureSymbol))
	      {
		  stack_.push(sli.new_token<sli3::litproceduretype>());
		  result=scancontinue;
	      }
	    else if (contains_symbol(t,s->BeginArraySymbol))
	      {
		t=sli.new_token<sli3::nametype>(open_array_);
		result=tokencontinue;
	      }
	    else if (contains_symbol(t, s->EndProcedureSymbol))
	      {
		if (!stack_.empty())
		{
		    t=stack_.top();
		    if (t.is_of_type(sli3::litproceduretype))
		    {
			stack_.pop();
			result=tokencontinue;
		    }
		    else result=endarrayexpected;
		}
		else result=noopenproc;
	      }
	    else if (contains_symbol(t,s->EndArraySymbol))
	      {
		t=sli.new_token<sli3::nametype>(close_array_);
		result=tokencontinue;
	      }
	    else if (contains_symbol(t, s->EndSymbol))
	      {
		if (!stack_.empty())
		  {
		    result=unexpectedeof;
		    stack_.clear();
		  }
		else
		  result=tokencompleted;
	      }
	    else
	      {
		// Now we should be left with a "simple" Token
		assert(! t.is_of_type(sli3::symboltype));
		if (!stack_.empty())
		  {
		    // append token to array on stack
		    Token &pt=stack_.top();
		    pt.data_.array_val->push_back(t);
		    result=scancontinue;
		  }
		else result=tokencompleted;
	      }
	    
	  } // if(ok)
      } while ( (result==tokencontinue) || (result==scancontinue));
    
    if( result != tokencompleted)
      {
	switch (result)
	  {
	  case noopenproc: 
	    s->print_error("Open brace missing.");
	    break;
	  case endprocexpected:
	    s->print_error("Closed brace missing.");
	    break;
	  case noopenarray: 
	    s->print_error("Open bracket missing.");
	    break;
	  case endarrayexpected:
	    s->print_error("Closed bracket missing.");
	    break;
	  case unexpectedeof:
	    s->print_error("Unexpected end of input.");
	    break;
	  default: break;
	  }
	t=sli.new_token<sli3::symboltype>(s->EndSymbol); // clear erroneous input
	return false;
      }
    return (result==tokencompleted);
  }
  
bool operator==(Parser const &p1, Parser const &p2)
{
  return &p1 == &p2;
}

std::ostream& operator<<(std::ostream& out, const Parser& p)
{
  out << "Parser(" << p.scan() << ')' << std::endl;
  return out;    
}

}
