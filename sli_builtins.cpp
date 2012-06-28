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
  void IlookupFunction::execute(SLIInterpreter *i) const
  {
    i->EStack().pop(2);
  }
  
/*BeginDocumentation
Name: parsestdin - Read and execute tokens from standard input
Description: parsestdin repeatedly reads and executes SLI commands from
the standard input stream (cin) until an end-of-file symbol is excountered
or the command exit is executed.

*/
void IparsestdinFunction::execute(SLIInterpreter *i) const
{
    Token t= i->read_token(std::cin);
    if(t.is_of_type(sli3::symboltype))
        i->EStack().pop();
    else
      {
        i->EStack().push(t);
      }
}

void IparseFunction::execute(SLIInterpreter *i) const
{

        // Estack: handle  iparse
        // pick      1         0
    assert(i->EStack().load()>0);
    assert(i->EStack().top().is_of_type(sli3::xistreamtype));
    
    SLIistream *is= i->EStack().top().data_.istream_val;
    assert(is != NULL);

    Token t=i->read_token(**is);

    if(t.is_of_type(sli3::symboltype))
	i->EStack().pop(2);
    else
        i->EStack().push(t);

}

  
  void IiterateFunction::backtrace(SLIInterpreter *i, int p) const
  {
    /*
      assert(i->EStack().pick(p+2).is_of_type(sli3::proceduretype));
      TokenArray *proc=EStack().pick(p+2).data_.array_val;
      assert(i->EStack().pick(p+1).is_of_type(sli3::integertypetype));
      long pos= i->EStack().pick(p+1).data_.long_val;
      
      std::cerr << "In procedure:" << std::endl;
      
      proc->list(std::cerr,"   ",pos-1);
      std::cerr << std::endl;
    */
  }
  
  void IiterateFunction::execute(SLIInterpreter *i) const
  {
    /* 
       This function is responsible for executing a procedure
       object. Iiterate expects the procedure to execute as first
       and the iteration counter as second argument.
       
       Like in all internal function, no error checking is done.
       
    */
    
    /* Stack Layout:
       3       2       1
       <proc>  <pos>   %iterate
    */
    
    TokenArray *proc= i->EStack().pick(2).data_.array_val;   
    long &pos=i->EStack().pick(1).data_.long_val;
    
    while( proc->index_is_valid(pos))
      {
	const Token &t=proc->get(pos);
	++pos;
	if( t.is_executable())
	  {
	    i->EStack().push(t);
	    return;
	  }
	i->push(t);
      }
    
    i->EStack().pop(3);
    i->dec_call_depth();
  }
  
  void IloopFunction::execute(SLIInterpreter *i) const
  {
    // stack: mark procedure n   %loop
    // level:  4      3      2     1
    
    TokenArray *proc= i->EStack().pick(2).data_.array_val;   
    long &pos=i->EStack().pick(1).data_.long_val;
    
    while( proc->index_is_valid(pos))
      {
	const Token &t(proc->get(pos));
	++pos;
	if( t.is_executable())
	  {
	    i->EStack().push(t);
	    return;
	  }
	
	i->push(t);
      }
    
    pos =0;
  }
  
  void IloopFunction::backtrace(SLIInterpreter *i, int p) const
  {
    /*
      ProcedureDatum const *pd= dynamic_cast<ProcedureDatum *>(i->EStack().pick(p+2).datum());   
      assert(pd !=NULL);
      
      IntegerDatum   *id= dynamic_cast<IntegerDatum *>(i->EStack().pick(p+1).datum());
      assert(id != NULL);
      
      std::cerr << "During loop:" << std::endl;
      
      pd->list(std::cerr,"   ",id->get()-1);
      std::cerr <<std::endl;
    */
  }
  
  
  /**********************************************/
  /* %repeat                                    */
  /*  call: mark  count proc  n %repeat         */
  /*  pick   5      4     3   2    1            */        
  /**********************************************/
  void IrepeatFunction::execute(SLIInterpreter *i) const
  {
    TokenArray *proc= i->EStack().pick(2).data_.array_val;   
    long &pos=i->EStack().pick(1).data_.long_val;

    while( proc->index_is_valid(pos))
      {
	const Token &t=proc->get(pos);
	++pos;
	if( t.is_executable())
	  {
	    i->EStack().push(t);
	    return;
	  }
	i->push(t);
      }
    
    long &lc=i->EStack().pick(3).data_.long_val;
    if( lc > 0 )
      {
	pos=0;     // reset procedure iterator
	--lc;
      }
    else
      {
	i->EStack().pop(5);
	i->dec_call_depth();
      }
  }

void IrepeatFunction::backtrace(SLIInterpreter *i, int p) const
{
  /*
  IntegerDatum   *count= static_cast<IntegerDatum *>(i->EStack().pick(p+3).datum());
  assert(count != NULL);

  ProcedureDatum const *pd= static_cast<ProcedureDatum *>(i->EStack().pick(p+2).datum());   
  assert(pd!= NULL);

  IntegerDatum   *id= static_cast<IntegerDatum *>(i->EStack().pick(p+1).datum());
  assert(id != NULL);

  std::cerr << "During repeat with " << count->get() << " iterations remaining." << std::endl;
  
  pd->list(std::cerr,"   ",id->get()-1);
  std::cerr <<std::endl;
  */
}

/*****************************************************/
/* %for                                              */
/*  call: mark incr limit count proc  n  %for        */
/*  pick   6     5    4     3    2    1    0         */        
/*****************************************************/
void IforFunction::execute(SLIInterpreter *i) const
{
  TokenArray *proc= i->EStack().pick(2).data_.array_val;   
  long &pos=i->EStack().pick(1).data_.long_val;
  
  while( proc->index_is_valid(pos))
    {
      const Token &t= proc->get(pos);
      ++pos;
      if( t.is_executable())
	{
	  i->EStack().push(t);
	  return;
	}
      i->push(t);
    }
  
  long &count=i->EStack().pick(3).data_.long_val;
  long &lim=i->EStack().pick(4).data_.long_val;
  long &inc=i->EStack().pick(5).data_.long_val;
        
   if(( (inc> 0) && (count <= lim)) ||
      ( (inc< 0) && (count >= lim)))
    {
	pos=0;                        // reset procedure interator
	
	i->push(i->EStack().pick(3)); // push counter to user
	count += inc;                 // increment loop counter
    }
  else
  {
      i->EStack().pop(7);
      i->dec_call_depth();
  }
}
  
void IforFunction::backtrace(SLIInterpreter *i, int p) const
{
  /* 
 IntegerDatum   *count= static_cast<IntegerDatum *>(i->EStack().pick(p+3).datum());
  assert(count!=NULL);
  ProcedureDatum const *pd= static_cast<ProcedureDatum *>(i->EStack().pick(p+2).datum());   
  assert(pd != NULL);
  IntegerDatum   *id= static_cast<IntegerDatum *>(i->EStack().pick(p+1).datum());
  assert(id != NULL);

  std::cerr << "During for at iterator value " << count->get() << "." << std::endl;
  
  pd->list(std::cerr,"   ",id->get()-1);
  std::cerr <<std::endl;
  */
}

/*********************************************************/
/* %forallarray                                          */
/*  call: mark object count proc n %forallarray      */
/*  pick    5     4    3     2    1    0               */        
/*********************************************************/
void IforallarrayFunction::execute(SLIInterpreter *i) const
{
  TokenArray *proc= i->EStack().pick(2).data_.array_val;   
  long &pos=i->EStack().pick(1).data_.long_val;

  while( proc->index_is_valid(pos))
    {
      const Token &t= proc->get(pos);
      ++pos;
      if( t.is_executable())
	{
	  i->EStack().push(t);
	  return;
	}
      i->push(t);
    }
 
  TokenArray *ad= i->EStack().pick(4).data_.array_val;   
  long &idx=i->EStack().pick(3).data_.long_val;
        
  if(ad->index_is_valid(idx))
    {
      pos=0; // reset procedure interator
      
      i->push(ad->get(idx)); // push counter to user
      ++idx;
    }
  else
    {
      i->EStack().pop(6);
      i->dec_call_depth();
    }
}


void IforallarrayFunction::backtrace(SLIInterpreter *i, int p) const
{
  /*
  IntegerDatum   *count= static_cast<IntegerDatum *>(i->EStack().pick(p+3).datum());
  assert(count!=NULL);

  std::cerr << "During forall (array) at iteration " << count->get() << "." << std::endl;
  */
}


/*********************************************************/
/* %foralliter                                          */
/*  call: mark iterator proc %foralliter                */
/*  pick   3     2    1      0                          */        
/*********************************************************/
void IforalliterFunction::execute(SLIInterpreter *i) const
{
  /*
  IteratorDatum *iter=
    static_cast<IteratorDatum *>(i->EStack().pick(2).datum());

    
   if(iter->pos() < iter->end())
    {

      i->OStack.push(iter->pos());  // push current element to operand stack
      iter->incr();
      i->EStack().push(i->EStack().pick(1));
      if(i->step_mode())
      {
	std::cerr << "foralliter:"
		  << " Limit: " << iter->end()
		  << " Pos: "   << iter->pos() ;
	i->OStack.pick(0).pprint(std::cerr);
	std::cerr << std::endl;
      }
    }
    else
    {
      i->EStack().pop(4);
      i->dec_call_depth();
    }
  */
}

void IforalliterFunction::backtrace(SLIInterpreter *i, int p) const
{
  /*
  IteratorDatum   *iter= static_cast<IteratorDatum *>(i->EStack().pick(p+2).datum());

  std::cerr << "During forall (iterator) at iteration " << iter->pos() << "." << std::endl;
  */
}



/*********************************************************/
/* %forallindexedarray                                   */
/*  call: mark object limit count proc forallindexedarray  */
/*  pick   5      4    3     2    1      0         */        
/*********************************************************/
void IforallindexedarrayFunction::execute(SLIInterpreter *i) const
{
  long &count = i->EStack().pick(2).data_.long_val;
  long &limit = i->EStack().pick(3).data_.long_val;

  if( count< limit)
    {
      TokenArray *obj=i->EStack().pick(4).data_.array_val;

      i->push(obj->get(count));  // push element to user
      i->push(count);        // push index to user
      ++count;
      i->EStack().push(i->EStack().pick(1));
      // if(i->step_mode())
      // {
      // 	std::cerr << "forallindexed:"
      // 		  << " Limit: " << limit->get()
      // 		  << " Pos: " << count->get() -1
      // 		  << " Iterator: "; 
      // 	i->OStack.pick(1).pprint(std::cerr);
      // 	std::cerr << std::endl;
      // }
    }
    else
    {
      i->EStack().pop(6);
      i->dec_call_depth();
    }

}

void IforallindexedarrayFunction::backtrace(SLIInterpreter *i, int p) const
{
  /*
  IntegerDatum   *count= static_cast<IntegerDatum *>(i->EStack().pick(p+2).datum());
  assert(count!=NULL);

  std::cerr << "During forallindexed (array) at iteration " << count->get()-1 << "." << std::endl;
  */
}

void IforallindexedstringFunction::backtrace(SLIInterpreter *i, int p) const
{
  /*
  IntegerDatum   *count= static_cast<IntegerDatum *>(i->EStack().pick(p+2).datum());
  assert(count!=NULL);

  std::cerr << "During forallindexed (string) at iteration " << count->get()-1 << "." << std::endl;
  */
}

/*********************************************************/
/* %forallindexedarray                                   */
/*  call: mark object limit count proc forallindexedarray  */
/*  pick   5      4    3     2    1      0         */        
/*********************************************************/
void IforallindexedstringFunction::execute(SLIInterpreter *i) const
{
  long &count=i->EStack().pick(2).data_.long_val;
  long &limit=i->EStack().pick(3).data_.long_val;

  if(count < limit)
    {
      SLIString *obj= i->EStack().pick(4).data_.string_val;
      
      i->push((*obj)[count]);  // push element to user
      i->push(count);          // push index to user
      ++count;
      i->EStack().push(i->EStack().pick(1));
      if(i->step_mode())
      {
	std::cerr << "forallindexed:"
		  << " Limit: " << limit
		  << " Pos: " << count
		  << " Iterator: "; 
	i->pick(1).pprint(std::cerr);
	std::cerr << std::endl;
      }
    }
    else
    {
      i->EStack().pop(6);	    
      i->dec_call_depth();
    }

}

void IforallstringFunction::backtrace(SLIInterpreter *i, int p) const
{
  /*
  IntegerDatum   *count= static_cast<IntegerDatum *>(i->EStack().pick(p+2).datum());
  assert(count!=NULL);

  std::cerr << "During forall (string) at iteration " << count->get()-1 << "." << std::endl;
  */
}
/*********************************************************/
/* %forallstring                                         */
/*  call: mark object limit count proc %forallarray  */
/*  pick   5      4    3     2    1      0         */        
/*********************************************************/
void IforallstringFunction::execute(SLIInterpreter *i) const
{
  long &count=i->EStack().pick(2).data_.long_val;
  long &limit=i->EStack().pick(3).data_.long_val;

  if(count < limit)
    {
      SLIString const *obj= i->EStack().pick(4).data_.string_val;
      i->push<long>((*obj)[count]);  // push element to user
      ++count;
      i->EStack().push(i->EStack().pick(1));
      if(i->step_mode())
      {
	std::cerr << "forall:"
		  << " Limit: " << limit
		  << " Pos: " << count
		  << " Iterator: "; 
	i->top().pprint(std::cerr);
	std::cerr << std::endl;
      }
    }
    else
    {
      i->EStack().pop(6);
      i->dec_call_depth();
    }

}

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
	i->EStack().pop();
    }
    else
    {
	i->message(sli3::M_ERROR, "arraycreate","Opening bracket missing.");
	i->raiseerror("SyntaxError");
	return;
    }
}


}
