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
// Phase 5: IlookupFunction and IparsestdinFunction were dead --
// ::lookup / ::pop / ::parsestdin were createcommand'd but no
// caller ever baselookup'd them. Removed; the classes are gone
// from sli_builtins.h.

void IparseFunction::execute(SLIInterpreter *i) const
{
    // EStack layout:
    //   pick(0) = iparse function (top, dispatched right now)
    //   pick(1) = xistream (the stream we read from)
    assert(i->EStack().load() >= 2);
    assert(i->EStack().pick(1).is_of_type(sli3::xistreamtype));

    SLIistream *is = i->EStack().pick(1).data_.istream_val;
    assert(is != nullptr);
    assert(is->get() != nullptr);

    Token t = i->read_token(*is->get());

    if (t.is_of_type(sli3::symboltype))
    {
        // EOF (or other terminator) -> pop iparse and the stream.
        i->EStack().pop(2);
    }
    else
    {
        // Push the parsed token on top so the dispatcher executes it
        // next; control returns to iparse afterwards.
        i->EStack().push(t);
    }
}

  // Phase 5: IiterateFunction was dead. ProcedureType::execute
  // (sli_arraytype.cpp) used to push baselookup(iiterate_name) so
  // this function would be invoked, but ProcedureType::execute is
  // itself unreachable in dispatch mode (the dispatcher's outer
  // `case proceduretype:` handles proceduretype directly). Removed.
  // The body_walk `case sli3::iiteratetype:` arm provides the
  // same semantics inline.

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
  
  
  // Phase 5: IrepeatFunction, IforFunction, IforallarrayFunction
  // were dead. The dispatcher's body_walk handles irepeattype,
  // ifortype, iforalltype directly; the entry ops (RepeatFunction,
  // ForFunction, Forall_aFunction) push the type marker on top of
  // their iter frame, not these legacy function tokens.

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

      i->push(obj->str()[count]);  // push element to user
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
      i->push<long>(obj->str()[count]);  // push element to user
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
