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
#include "scanner.h"
#include "parser.h"
#include "arraydatum.h"
#include "symboldatum.h"
#include "namedatum.h"
#include "config.h"

/*****************************************************************/
/* parse                                                         */
/* --------            Token --> Token                           */
/*                                                               */
/* Errors:                                                       */
/*                                                               */
/*                                                               */
/*                                                               */
/*****************************************************************/

// kann der ParseStack ein Stack von ArrayDatums sein ?????
// Token ist besser weil dann verschoben werden kann
void Parser::init(std::istream &is)
{
    s=new Scanner(&is);
    
    arraytoken=ArrayDatum();
}

Parser::Parser(std::istream &is)
        :s(NULL), ParseStack(128)
{
    init(is);
    assert(s !=NULL);
}

Parser::Parser(void)
        :s(NULL), ParseStack(128)
{
    init(std::cin);
    assert(s !=NULL);
    
}


bool Parser::operator()(Token& t)
{
    assert(s != NULL);
    
    Token pt;

    bool ok;
    ParseResult result=scancontinue;

    do
    {
      if(result == scancontinue)
	ok=(*s)(t);
      else ok=true;
        
        
      if (ok) 
      {
            
	if (t.contains(s->BeginProcedureSymbol))
	{
	  ParseStack.push(new LitprocedureDatum());
	  ParseStack.top()->set_executable();
	  result=scancontinue;
	}
	else if (t.contains(s->BeginArraySymbol))
	{
#ifdef PS_ARRAYS
	  Token cb(new NameDatum("["));
	  t.move(cb);
	  result=tokencontinue;
#else
	  ParseStack.push(new ArrayDatum());
	  result=scancontinue;
#endif
	}
	else if (t.contains(s->EndProcedureSymbol))
	{
	  if (!ParseStack.empty())
	  {
	    ParseStack.pop_move(pt);
	    if (pt->isoftype(SLIInterpreter::Litproceduretype))
	    {
	      t.move(pt);                    // procedure completed
	      result=tokencontinue;
	    }
	    else result=endarrayexpected;
	  }
	  else result=noopenproc;
	}
	else if (t.contains(s->EndArraySymbol))
	{
#ifdef PS_ARRAYS
	  Token ob(new NameDatum("]"));
	  t.move(ob);
	  result=tokencontinue;
#else
	  if (!ParseStack.empty())
	  {
	    ParseStack.pop_move(pt);
	    if (pt->isoftype(SLIInterpreter::Arraytype))
	    {
	      t.move(pt);                     // array completed
	      result=tokencontinue;
	    }
	    else result=endprocexpected;
	  }
	  else result=noopenarray;
#endif
	}
	else if (t.contains(s->EndSymbol))
	{
	  if (!ParseStack.empty())
	  {
	    result=unexpectedeof;
	    ParseStack.clear();
	  }
	  else
	    result=tokencompleted;
	}
	else
	{
	  // Now we should be left with a "simple" Token
	  assert(! t->isoftype(SLIInterpreter::Symboltype));
	  if (!ParseStack.empty())
	  {
	    // append token to array on stack
	    ParseStack.pop_move(pt);
	    if(pt->isoftype(SLIInterpreter::Arraytype))
	    {
	      ArrayDatum *pa=dynamic_cast<ArrayDatum*>(pt.datum());
	      assert(pa!=NULL);
	      pa->push_back(t);
	    }
	    else // now it must be a procedure
	    {
	      LitprocedureDatum *pp=
		dynamic_cast<LitprocedureDatum*>(pt.datum());
	      assert(pp!=NULL);
	      pp->set_executable();
	      pp->push_back(t);
	    }
	    ParseStack.push_move(pt);
	    result=scancontinue;
	  }
	  else result=tokencompleted;
	}
	
      } // if(ok)
//      else std::cerr << "<Scanner> : unable to scan input, Result:" << ok << '\n';
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
      t= s->EndSymbol; // clear erroneous input
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

