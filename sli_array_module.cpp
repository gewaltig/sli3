/*
 *  sliarray.cc
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
    SLI's data access functions
*/
#include "sli_numerics.h"
#include "sli_array.h"
#include "sli_array_module.h"
#include "sli_tokenutils.h"
#include "config.h"
#include <vector>
#include <cmath>

const std::string  SLIArrayModule::commandstring(void) const
{
  return std::string("/arraymodule /C++ ($Revision: 9458 $) provide-component  /arraymodule /SLI (1.7) require-component");
  return std::string("(arraymodule.sli) run");
}

const std::string SLIArrayModule::name(void) const
{
  return std::string("arraymodule");
}

void SLIArrayModule::RangeFunction::execute(SLIInterpreter *i) const
{
//  call:  array Range -> array

  i->require_stack_load(1);
  i->require_stack_type(0,sli::arraytype);
  TokenArray   *ad = i->top().data_.array_val;

  if(ad->size() == 1) // Construct an array of elements 1 ... N
  {
      if(ad->get(0).is_of_type(sli3::integertype))
	 {
	     long n= ad->get(0).data_.long_val;
	     ad->remove_reference();
	     ad = new TokenArray(n);
	     if(n >0)
	     {
		 SLIType *tp=i->get_type(sli3::integertype);
		 for(long j=1; j<=n; ++j)
		 {
		     (*ad)[j].type=tp;
		     (*ad)[j].data_.long_val=j;
		 }
	     }
	 }
	 else
	 {
	     double d=ad->get(0); // This may throw TypeMismatch
	     ad->remove_reference();
	     long n= (long) std::floor(d);
	     if(n >0)
	     {
		 SLIType *tp=i->get_type(sli3::doubletype);
		 ad= new TokenArray(n);
		 for(long j=1; j<=n; ++j)
		 {
		     (*ad)[j].type_=static_cast<double>(j) ;
		     (*ad)[j].data_.double_val=static_cast<double>(j) ;
		 }
	     }
	 }
	 i->top().data_.array_val=ad;
	 i->EStack().pop();
	 return;
  }

  if(ad->size() == 2) // [n1 n2]
  {
    if( ad->get(0).is_of_type(sli3::arraytype) and ad->get(1).is_of_type(sli3::integertype))
    {
	long start = ad->get(0).data_.long_val;
	long stop  = ad->get(1).data_.long_val;
	long n = 1 + stop -  start;

	ad->remove_reference();
	ad= new TokenArray(n);
	SLIType *tp=i->get_type(sli3::integertype);

	for(long j=start; j<=stop; ++j)
	{
	    (*ad)[j].type_=tp;
	    (*ad)[j].data_.long_val=j;

	}
    }
    else
    {
	double start = ad->get(0); // This may throw TypeMismatch
	double stop  = ad->get(1); // This may throw TypeMismatch

	long n = 1 + static_cast<long>(stop -  start);

	ad->remove_reference();
	ad= new TokenArray(n);
	SLIType *tp=i->get_type(sli3::doubletype);

	for(double j=start; j<=stop; ++j)
	{
	    (*ad)[j].type_=tp;
	    (*ad)[j].data_.double_val=j;
	}
    } 
    i->top().data_.array_val=ad;
    i->EStack().pop();
    return;
  }

  if(ad->size() == 3) // [n1 n2 dn]
  {
      if( ad->get(0).is_of_type(sli3::integertype) and  ad->get(1).is_of_type(sli3::integertype) ad->get(2).is_of_type(sli3::integertype))
      {
	  long di =  ad->get(2).data_.long_val;
	  long start = ad->get(0).data_.long_val;
	  long stop  = ad->get(1).data_.long_val;
	  if(di != 0)
	  {
	      long n= 1 + (stop-start)/di;
	      ad->remove_reference();
	      ad= new TokenArray(n);
	      if(n > 0)
	      {
		  SLIType *tp=i->get_type(sli3::integertype);
		  long s=start;
		  for(long j=0; j<n; ++j, s+=di)
		  {
		      (*ad)[j].type_=tp;
		      (*ad)[j].data_.long_val=s;
		  }
	      }
	  }
	  else throw DivisionByZero();
	  
      }
      else
      {
	  double di = ad->get(2);
	  double start=ad->get(1);
	  double stop=ad->get(0);
	  
	  if(di != 0)
	  {
	      long n= 1 + static_cast<long>((stop-start)/di);
	      ad->remove_reference();
	      if(n>0)
	      {
		  ad->reserve(n);
		  ad= new TokenArray(n);
		  SLIType *tp=i->get_type(sli3::doubletype);
		  for(long j=0; j<n; ++j)
		  {
		      (*ad)[j].type_=tp;
		      (*ad)[j].data_.double_val= start+j*di;
		  }
	      }
	  }
	  else throw DivisionByZero();
      }
      i->top().data_.array_val=ad;
      i->EStack().pop();
      return;
  }
  else throw RangeCheck();
}


void SLIArrayModule::ReverseFunction::execute(SLIInterpreter *i) const
{
//  call:  array reverse -> t1 ... tn n
  i->require_stack_load(1);
  i->require_stack_type(0,sli3::arraytype);
  TokenArray *ad = i->top().data_.array_val= i->top().data_.array_val->clone();
  ad->reverse();
  i->EStack().pop();
}

void SLIArrayModule::RotateFunction::execute(SLIInterpreter *i) const
{
  i->require_stack_load(2);
  i->require_stack_type(1,sli3::arraytype);
  i->require_stack_type(0,sli3::integertype);

  long &n =i->pick(0).data_.long_val;
  TokenArray *ad = i->pick(1).data_.array_val=i->pick(1).data_.array_val->clone();
  ad->rotate(n);
  i->pop();
  i->EStack().pop();
}


void SLIArrayModule::FlattenFunction::execute(SLIInterpreter *i) const
{
//  call:  array Flatten -> array
  i->require_stack_load(1);
  i->require_stack_type(0,sli3::arraytype);
  TokenArray *ad = i->top().data_.array_val;
  ArrayDatum *ta = new TokenArray();

  size_t size=0;

  for(Token const*t= ta->begin(); t != ta->end(); ++t)
  {
      if( t->is_of_type(sli3::arraytype))
	  size += t->data_.array_val->size();
      else
	  ++size;
  }

  ta->reserve(size);

  for(Token const*t= ad->begin(); t != ad->end(); ++t)
  {
      if ( t->is_of_type(sli3::arraytype))
      {
	  TokenArray *ad1= t->data_.array_val;
	  for(Token const*t1= ad1->begin(); t1 != ad1->end(); ++t1)
	      ta->push_back(*t1);
      }
      else
	  ta->push_back(*t);
  }

  ad->revome_reference();
  i->top().array_val=ta;

  i->EStack().pop();
}

/*BeginDocumentation
Name: Sort - Sorts a homogeneous array of doubles, ints, or strings.
Synopsis:
 array Sort -> array
Parameters:
 array of doubles, ints, or strings
Description:
 The present implementation is restricted to doubles, ints, and strings.
Examples:
 [8. 4. 3. 6. 9. 5.] Sort --> [3. 4. 5. 6. 8. 9.]
Author: Diesmann, Eppler
SeeAlso: Max, Min
*/
void SLIArrayModule::SortFunction::execute(SLIInterpreter *i) const
{
    i->require_stack_load(1);
    i->require_stack_type(sli3::arraytype);
    TokenArray *td = i->top().data_.array_val;

    try
    {
	std::vector<double> vd;
	td.toVector(vd);
	std::sort(vd.begin(),vd.end());
	i->pop();
	i->push(vd);
	i->EStack().pop();
	return;
    }
    catch (TypeMismatch) {;}
    
    try
    {
	std::vector<long> vd;
	td.toVector(vd);
	std::sort(vd.begin(),vd.end());
	i->pop();
	i->push(vd);
	i->EStack().pop();
	return;
    }
    catch (TypeMismatch) {;}
    
    try
    {
	std::vector<std::string> vd;
	td.toVector(vd);
	std::sort(vd.begin(),vd.end());
	i->pop();
	TokenArray* output = new TokenArray();
	output->reserve(vd.size());
	for(size_t c = 0; c < vd.size(); ++c)
	{
	    SLIString *sd= new SLIString(vd[c]);
	    output->push_back(i->new_token<sli3::stringtype>(sd));
	}
	i->push(i->new_token<sli3::arraytype>(output));
	i->EStack().pop();
	return;
    }
    catch (TypeMismatch) {;}
    
    i->message(SLIInterpreter::M_ERROR, "Sort", "argument array may only contain doubles, ints, or strings");
    i->raiseerror(i->ArgumentTypeError);
}


/*
BeginDocumentation

   Name: Transpose - Transposes the first two levels of its argument

   Synopsis:
      array Transpose -> array

   Description:
     Transpose gives the usual transpose of a matrix.
     Acting on a tensor Tijkl... Transpose gives the tensor Tjikl...

   Parameters:

   Examples:
     [ [3 4 5] [6 7 8] ] Transpose  -> [[3 6] [4 7] [5 8]]

   Bugs:
     protected for non-rectangular shapes by assert().
     Transpose should raise
       /NonRectangularShapeError error
     and message
      "The first two levels of the one-dimensional list cannot be transposed."
   Author: Markus Diesmann, July 9, 2000
   FirstVersion: June, 2000
   References:   [1] The Mathematica Book V4.0 "Transpose"
   SeeAlso: Flatten, Partition

*/
void SLIArrayModule::TransposeFunction::execute(SLIInterpreter *i) const
{
//  call:  array Transpose -> array
    i->require_stack_load(1);
    i->require_stack_type(sli3::arraytype,0);
    
    TokenArray *sd = i->top().data_.array_val;
    size_t m=sd->size();
    if(m ==0 )
    {
	i->EStack().pop();
	i->pop();
	return;
    }

    Token *iter=sd->begin();
    iter->require_type(sli3::arraytype);

    // size of source second level
    size_t n =  iter->data_.array_val->size();

    for(; iter != sd->end(); ++iter)
    {
	iter->require_type(sli3::arraytype);
	if(iter->data_.array_val->size() != n)
	    throw DimesionMismatch();
    }
    // Now we know that we have a transposable structure

    TokenArray *td = new TokenArray();
    td->resize(n);
    SLIType *array_type=i->get_type(sli3::arraytype);
    for (size_t j = 0; j< n; j++)
    {
	(*td)[j].type_=array_type;
	(*td)[j].data_.array_val=new TokenArray(m);
    }
    for(k=0; k<m; k++)
    {
	TokenArray &source=*((*sd)[k].data_.array_val);
	for(l=0; l<n; l++)
	{
	    (*((*td)[l].data_array_val))[k]=source[l];
	}
    }

    sd->remove_reference();
    i->top().data_.array_val=td;
    i->EStack().pop();
}



void SLIArrayModule::PartitionFunction::execute(SLIInterpreter *i) const
{
//  call:  array n d Partition -> array
  assert(i->load()>2);

  IntegerDatum *dd = dynamic_cast<IntegerDatum *>(i->pick(0).datum());
  assert(dd != NULL);
  IntegerDatum *nd = dynamic_cast<IntegerDatum *>(i->pick(1).datum());
  assert(nd != NULL);
  TokenArray *source = dynamic_cast<TokenArray *>(i->pick(2).datum());
  assert(source !=0);
  TokenArray *target = new TokenArray;

  long n = nd->get();
  long d = dd->get();

  if (n>0)
  {
    if (d>0)
    {
      size_t na= source->size();
      if(na >0)
      {
	long max = (na - n + d)/d;

	target->reserve( (max > 0) ? max : 0 );
	Token *b = source->begin();
	Token *e = source->end();

	for(Token *pt=b; pt < e-n+1; pt+=d)
	{
	  TokenArray *ad= new TokenArray;
	  ad->reserve(n);
	  for(long i = 0; i < n ; ++i)
	  {
	    assert(pt+i < e);
	    ad->push_back(*(pt+i));
	  }
	  target->push_back(ad);
	}
      }
      // need to pop ourselves, arguments, push (empty) target
      // even if argument array was empty --- HEP 2001-10-22
      i->EStack().pop();
      i->pop(3);
      i->push(target);
    }
    else
      i->raiseerror("RangeError");
  }
  else
    i->raiseerror("RangeError");
}

/*
BeginDocumentation

   Name: arrayload - pushes array elements followed by number of elements

   Synopsis:
      array Transpose ->  array arrayload -> t1 ... tn n

   Description:
    The stack is invariant under the sequences
       arrayload arraystore
       arraystore arrayload  .
    arrayload is the SLI version of PostScript operator aload.
    In contrast to PostScript SLI arrays are dynamic therefore
    the syntax of aload and astore is obsolete in SLI.
    If used aload and astore issue a warning message.

   Examples:
        [ 5 4 2 ] arrayload  --> 5 4 2   3

   Author: Marc-Oliver Gewaltig, Markus Diesmann
   Remarks: There are two obsolete versions existing called aload and astore.
   SeeAlso: arraystore
*/
void SLIArrayModule::ArrayloadFunction::execute(SLIInterpreter *i) const
{
//  call:  array arrayload -> t1 ... tn n
  assert(i->load()>0);

  Token at; at.move(i->top());
  i->pop();
  TokenArray *ad = dynamic_cast<TokenArray *>(at.datum());
  assert(ad !=0);
  i->EStack().pop();
  int  arraysize=ad->size();
  i->reserve_token(arraysize);

  if(ad->references()==1)
    for(Token *ti=ad->begin(); ti != ad->end(); ++ti)
      i->push_move(*ti);
  else
    for(Token *ti=ad->begin(); ti != ad->end(); ++ti)
      i->push(*ti);

  i->push(arraysize);
}

/*
BeginDocumentation

   Name: arraystore - pops the first n elements of the stack into an array

   Synopsis:
     t1 ... tn n  arraystore -->  array

   Description:
    The stack is invariant under the sequences
       arrayload arraystore
       arraystore arrayload  .
    arraystore is the SLI version of PostScript operator astore.
    In contrast to PostScript SLI arrays are dynamic therefore
    the syntax of aload and astore is obsolete in SLI.
    If used aload and astore issue a warning message.

   Parameters:

   Examples:
      5 4 2   3  arraystore  -->   [ 5 4 2 ]

   Bugs:
   Author: Marc-Oliver Gewaltig, Markus Diesmann
   Remarks: There are two obsolete versions existing called aload and astore.
   SeeAlso: arrayload
*/
void SLIArrayModule::ArraystoreFunction::execute(SLIInterpreter *i) const
{
//  call: t1 ... tn n  arraystore -> array
//  assert(i->load()>0);

  IntegerDatum *id=dynamic_cast<IntegerDatum *>(i->top().datum());
  assert(id != NULL);

  long n=id->get();
  if(n>=0)
  {
    if(i->load()>static_cast<size_t>(n))
    {
      i->pop();
      TokenArray *ad= new TokenArray();
      ad->reserve(n);
      Token at(ad);
      for(long l=1; l<=n; ++l)
	ad->push_back_move(i->pick(n-l));
      i->pop(n);
      i->push_move(at);
      i->EStack().pop();
    } else i->raiseerror(i->StackUnderflowError);
  } else i->raiseerror(i->RangeCheckError);
}


void SLIArrayModule::IMapFunction::backtrace(SLIInterpreter *i, int p) const
{
  IntegerDatum   *id= static_cast<IntegerDatum *>(i->EStack().pick(p+3).datum());
  assert(id!=NULL);

  IntegerDatum   *count= static_cast<IntegerDatum *>(i->EStack().pick(p+2).datum());
  assert(count==NULL);

  ProcedureDatum const *pd= static_cast<ProcedureDatum *>(i->EStack().pick(p+1).datum());
  assert(pd !=NULL);

  std::cerr << "During Map at iteration " << count->get() << "." << std::endl;


  pd->list(std::cerr,"   ",id->get()-1);
  std::cerr <<std::endl;

}

/**********************************************/
/* % IMap                                     */
/*  call: array mark procc count proc %map   */
/*  pick   5     4    3     2    1      0     */
/**********************************************/
void SLIArrayModule::IMapFunction::execute(SLIInterpreter *i) const
{
    ProcedureDatum *proc=static_cast<ProcedureDatum *>(i->EStack().pick(1).datum());
    size_t proclimit=proc->size();
    IntegerDatum *count= static_cast<IntegerDatum *>(i->EStack().pick(2).datum());
    size_t iterator= count->get();
    IntegerDatum *procc= static_cast<IntegerDatum *>(i->EStack().pick(3).datum());
    size_t pos= procc->get();
    TokenArray *array=static_cast<TokenArray *>(i->EStack().pick(5).datum());
    size_t limit=array->size();

    // Do we  start a new iteration ?
    if(pos==0)
    {
      if(iterator < limit) // Is Iteration is still running
      {
	if(iterator>0)
	{
	  // In this case we have to put the result of
	  // the last procedure call into the array
	  if(i->load()==0)
	  {
	    i->dec_call_depth();
	    i->raiseerror(i->StackUnderflowError);
	    return;
	  }
	  array->assign_move(iterator-1,i->top());
	  i->pop();
	}

	i->push(array->get(iterator));  // push element to user
	if(i->step_mode())
	{
	  std::cerr << "Map:"
		    << " Limit: " << limit
		    << " Pos: " << iterator
		    << " Iterator: ";
	  i->pick(0).pprint(std::cerr);
	  std::cerr << std::endl;
	}

	++(count->get());
	// We continue after this if-branch and do the commands
      }
      else
      {
	if(iterator>0)
	{
	  // In this case we have to put the result of
	  // the last procedure call into the array
	  if(i->load()==0)
	  {
	    i->raiseerror(i->StackUnderflowError);
	    return;
	  }
	  array->assign_move(iterator-1,i->top());
	  i->pop();
	}
	i->push_move(i->EStack().pick(5)); // push array
	i->EStack().pop(6);
	i->dec_call_depth();
	return;
      }
    }

    if((size_t)procc->get() < proclimit)
    {
      /* we are still evaluating the procedure. */
      i->EStack().push(proc->get(pos));  // get next command from the procedure
      ++(procc->get());                 // increment the counter and

      if(i->step_mode())
      {
	std::cerr << std::endl;
	do{
	  char cmd=i->debug_commandline(i->EStack().top());
	  if(cmd=='l') // List the procedure
	  {
	    if(proc !=NULL)
	    {
	      proc->list(std::cerr,"   ",pos);
	      std::cerr <<std::endl;
	    }
	  }
	  else
	    break;
	} while (true);
      }

    }
    if((size_t)procc->get() >= proclimit)
      (*procc)=0;


}

/********************************/
/* Map                          */
/*  call: array proc Map -> array */
/*  pick   1    0               */
/********************************/
/*
BeginDocumentation

   Name: Map - Apply a procedure to each element of a list or string

   Synopsis:
     [v1 ... vn] {f} Map -> [ f(v1) ... f(vn) ]
     (c1 ... cn) {f} Map -> (f(c1)...f(vn))

   Parameters:
     [v1 ... vn] - list of n arbitrary objects
     (c1 ... cn) - string with n characters

     {f}         - function which can operate on the elements of [array].
                   This function must return exaclty one value.

   Description:
     Map works like the corresponding Mathematica function.
     For each element of the input array, Map calls f and replaces
     the element with the result of f.
     Note that f must return exactly one value! The result of Map
     is a list with the same number of values as the argument list.
     If f does not return a value, Map fails.
     If f returns more than one value, the result of Map is undefined.

   Examples:

   [1 2 3 4 5]  {2 mul} Map -> [2 4 6 8 10]
   (abc) {1 add} Map  -> (bcd)

   Diagnostics:

   Bugs:

   Author:
    Marc-Oliver Gewaltig

   Remarks: Map is not part of PostScript
   References: The Mathematica Book
   SeeAlso: MapAt, MapIndexed, Table, forall, forallindexed, NestList

*/

void SLIArrayModule::MapFunction::execute(SLIInterpreter *i) const
{
    i->EStack().pop();
    ProcedureDatum *proc=
        dynamic_cast<ProcedureDatum *>(i->top().datum());
    assert(proc !=NULL);

    if(proc->size()==0)
    {
      // If the procedure is empty, just leave the array as it is.
      i->pop();
      return;
    }

    i->EStack().push_move(i->pick(1));        // push array
    i->EStack().push(i->baselookup(i->mark_name));

    i->EStack().push_by_pointer(new IntegerDatum(0));          // push procedure counter
    i->EStack().push_by_pointer(new IntegerDatum(0));          // push initial counter
    i->EStack().push_move(i->pick(0));       // push procedure

    i->EStack().push(i->baselookup(sli::imap));
    i->inc_call_depth();
    i->pop(2);
}

void SLIArrayModule::ValidFunction::execute(SLIInterpreter *i) const
{
  assert(i->load() > 0);
  TokenArray *ad = dynamic_cast<TokenArray*>(i->top().datum());
  assert(ad != NULL);
  i->push( ad->valid());

  i->EStack().pop();
}

void SLIArrayModule::IMapIndexedFunction::backtrace(SLIInterpreter *i, int p) const
{
  IntegerDatum   *id= static_cast<IntegerDatum *>(i->EStack().pick(p+3).datum());
  assert(id !=NULL);

  IntegerDatum   *count= static_cast<IntegerDatum *>(i->EStack().pick(p+2).datum());
  assert(count!=NULL);

  ProcedureDatum const *pd= static_cast<ProcedureDatum *>(i->EStack().pick(p+1).datum());
  assert(pd !=NULL);


  std::cerr << "During MapIndexed at iteration " << count->get() << "." << std::endl;


  pd->list(std::cerr,"   ",id->get()-1);
  std::cerr <<std::endl;

}

/**********************************************/
/* % IMapIndexed                              */
/*  call: array mark procc count proc %map    */
/*  pick   5     4    3     2    1      0     */
/**********************************************/
void SLIArrayModule::IMapIndexedFunction::execute(SLIInterpreter *i) const
{
    ProcedureDatum *proc=static_cast<ProcedureDatum *>(i->EStack().pick(1).datum());
    size_t proclimit=proc->size();
    IntegerDatum *count= static_cast<IntegerDatum *>(i->EStack().pick(2).datum());
    size_t iterator= count->get();
    IntegerDatum *procc= static_cast<IntegerDatum *>(i->EStack().pick(3).datum());
    size_t pos= procc->get();
    TokenArray *array=static_cast<TokenArray *>(i->EStack().pick(5).datum());
    size_t limit=array->size();

    // Do we  start a new iteration ?
    if(pos==0)
    {
      if(iterator < limit) // Is Iteration is still running
      {
	if(iterator>0)
	{
	  // In this case we have to put the result of
	  // the last procedure call into the array
	  if(i->load()==0)
	  {
	    i->raiseerror(i->StackUnderflowError);
	    return;
	  }
	  array->assign_move(iterator-1,i->top());


	  i->pop();
	}

	i->push(array->get(iterator));  // push element to user
	i->push(*count);             // push iterator to user
	++(count->get());

	if(i->step_mode())
	{

	  std::cerr << "MapIndexed:"
		    << " Limit: " << limit
		    << " Pos: " << iterator
		    << " Iterator: ";
	  i->pick(1).pprint(std::cerr);
	  std::cerr << std::endl;
	}

	// We continue after this if-branch and do the commands
      }
      else
      {
	if(iterator>0)
	{
	  // In this case we have to put the result of
	  // the last procedure call into the array
	  if(i->load()==0)
	  {
	    i->raiseerror(i->StackUnderflowError);
	    return;
	  }
	  array->assign_move(iterator-1,i->top());
	  i->pop();
	}
	i->push_move(i->EStack().pick(5)); // push array
	i->EStack().pop(6);
	i->dec_call_depth();
	return;
      }
    }

    if((size_t)procc->get() < proclimit)
    {
      /* we are still evaluating the procedure. */
      i->EStack().push(proc->get(pos));  // get next command from the procedure
      ++(procc->get());                 // increment the counter and
      if(i->step_mode())
      {
	std::cerr << std::endl;
	do{
	  char cmd=i->debug_commandline(i->EStack().top());
	  if(cmd=='l') // List the procedure
	  {
	    if(proc !=NULL)
	    {
	      proc->list(std::cerr,"   ",pos);
	      std::cerr <<std::endl;
	    }
	  }
	  else
	    break;
	} while (true);
      }
    }
    if((size_t)procc->get() >= proclimit)
      (*procc)=0;


}


void SLIArrayModule::MapIndexedFunction::execute(SLIInterpreter *i) const
{
    i->EStack().pop();
    ProcedureDatum *proc=
        dynamic_cast<ProcedureDatum *>(i->top().datum());
    assert(proc !=NULL);

    if(proc->size()==0)
    {
      // If the procedure is empty, just leave the array as it is.
      i->pop();
      return;
    }

    i->EStack().push_move(i->pick(1));        // push array
    i->EStack().push(i->baselookup(i->mark_name));

    i->EStack().push(new IntegerDatum(0));          // push procedure counter
    i->EStack().push(new IntegerDatum(0));          // push initial counter
    i->EStack().push_move(i->pick(0));       // push procedure

    i->EStack().push(i->baselookup(sli::imapindexed));
    i->inc_call_depth();
    i->pop(2);
}

void SLIArrayModule::IMapThreadFunction::backtrace(SLIInterpreter *i, int p) const
{
  IntegerDatum   *id= static_cast<IntegerDatum *>(i->EStack().pick(p+3).datum());
  assert(id !=NULL);

  IntegerDatum   *count= static_cast<IntegerDatum *>(i->EStack().pick(p+2).datum());
  assert(count!=NULL);

  ProcedureDatum const *pd= static_cast<ProcedureDatum *>(i->EStack().pick(p+1).datum());
  assert(pd !=NULL);


  std::cerr << "During MapThread at iteration " << count->get() << "." << std::endl;


  pd->list(std::cerr,"   ",id->get()-1);
  std::cerr <<std::endl;

}

//******************************************************
// % IMapThread
//  call: mark  lim  tarray sarray procc count proc %map
//  pick   7     6      5      4      3     2   1    0
//*******************************************************
void SLIArrayModule::IMapThreadFunction::execute(SLIInterpreter *i) const
{
    ProcedureDatum *procd=static_cast<ProcedureDatum *>(i->EStack().pick(1).datum());
//    assert(procd != NULL);

    size_t proclimit=procd->size();

    IntegerDatum *argcountd= static_cast<IntegerDatum *>(i->EStack().pick(2).datum());
    //  assert(argcountd != NULL);

    size_t argcount= argcountd->get();

    IntegerDatum *proccountd= static_cast<IntegerDatum *>(i->EStack().pick(3).datum());
    //assert(proccountd != NULL);
    size_t proccount= proccountd->get();

    TokenArray   *sarray= static_cast<TokenArray *>(i->EStack().pick(4).datum());
    //assert(sarray != NULL);
    TokenArray   *tarray= static_cast<TokenArray *>(i->EStack().pick(5).datum());
    //assert(tarray != NULL);

    IntegerDatum *limitd= static_cast<IntegerDatum *>(i->EStack().pick(6).datum());
    //assert(limitd != NULL);

    size_t args= sarray->size(); // number of argument arrays
    size_t limit= limitd->get(); // number of arguments per array

    // first we check whether we start anew with the iteration of the procedure
    // this is the case when proccount==0
    if(proccount==0)
    {
      if(argcount < limit) // Is Iteration is still running
      {
	if(argcount>0)
	{
	  // In this case we have to put the result of
	  // the last procedure call into the array
	  if(i->load()==0)
	  {
	    i->raiseerror(i->StackUnderflowError);
	    return;
	  }
	  tarray->assign_move(argcount-1,i->top());
	  i->pop();
	}

	// Make a loop over all argument arrays and push the next element
	for(size_t j=0; j< args; ++j)
	{
	  TokenArray *ad= static_cast<TokenArray *>(sarray->get(j).datum());
	  i->push(ad->get(argcount));  // push element to user
	}
	assert(i->load()>= args);
	++(argcountd->get());

	// We continue after this if-branch and do the commands
	if(i->step_mode())
	{

	  std::cerr << "MapThread:"
		    << " Limit: " << limit
		    << " Pos: " << argcount
		    << " Args: " << args
		    << std::endl;
	}
      }
      else
      {
        assert(argcount >= limit);
	if(argcount>0) // should be obsolete
	{
	  // In this case we have to put the result of
	  // the last procedure call into the array
	  if(i->load()==0)
	  {
	    i->raiseerror(i->StackUnderflowError);
	    return;
	  }
	  tarray->assign_move(argcount-1,i->top());
	  i->pop();
	}
	i->push_move(i->EStack().pick(5)); // push result array
	i->EStack().pop(8);
	i->dec_call_depth();
	return;
      }
    }

    if((size_t)proccountd->get() < proclimit)
    {
      /* we are still evaluating the procedure. */
      i->EStack().push(procd->get(proccount));  // get next command from the procedure
      ++(proccountd->get());                 // increment the counter and

      if(i->step_mode())
      {
	std::cerr << std::endl;
	do{
	  char cmd=i->debug_commandline(i->EStack().top());
	  if(cmd=='l') // List the procedure
	  {
	    if(procd !=NULL)
	    {
	      procd->list(std::cerr,"   ", proccount);
	      std::cerr <<std::endl;
	    }
	  }
	  else
	    break;
	} while (true);
      }
    }
    if((size_t)proccountd->get() >= proclimit)
      (*proccountd)=0;
}

/* BeginDocumentation
Name: MapThread - apply a procedure to corresponding elements of n arrays
Synopsis: [[a11 ... a1n]...[am1 ... amn]] {f} MapThread ->
                                  [f(a11, a21,... am1)...f(a1n, a2n,...,amn)]
Description: MapThread is like a multidimensional Map. It applies the function of
             to corresponding elements of m argument arrays.

Parameters: the first parameter is a list of m arrays of equal size n.
            The second parameter is a procedure which takes m arguments and returns
            a single value.
Examples:    [[1 2][3 4]] {add} MapThread -> [4 6]
            [[1 2 3 4] [1 1 1 1]] {add} MapThread -> [2 3 4 5]

References: This function implements the simple version of Mathematica's MapThread
SeeAlso: Map, MapIndexed, NestList, FoldList, ScanThread
*/

void SLIArrayModule::MapThreadFunction::execute(SLIInterpreter *i) const
{
  assert(i->load()>=2 );
  ProcedureDatum *proc=
    dynamic_cast<ProcedureDatum *>(i->top().datum());
  assert(proc !=NULL);

  if(proc->size()==0)
  {
    // If the procedure is empty, just leave the array as it is.
    i->pop();
    i->EStack().pop();
    return;
  }

  TokenArray  *ad= dynamic_cast<TokenArray *>(i->pick(1).datum());
  assert(ad !=NULL);

  if(ad->size() >0)
  {
    // check if the components are arrays of equal length.
    TokenArray *ad1= dynamic_cast<TokenArray *>(ad->get(0).datum());
    if (ad1 == NULL)
    {
      i->raiseerror(i->ArgumentTypeError);
      return;
    }

    for(size_t j=1; j< ad->size(); ++ j)
    {
      TokenArray *ad2= dynamic_cast<TokenArray *>(ad->get(j).datum());
      if (ad2 == NULL)
      {
	i->raiseerror(i->ArgumentTypeError);
	return;
      }

      if(ad2->size() != ad1->size())
      {
	i->raiseerror(i->RangeCheckError);
	return;
      }
    }

    i->EStack().pop(); // remove MapThread object
    i->EStack().push(i->baselookup(i->mark_name));    //  mark
    i->EStack().push(new IntegerDatum(ad1->size()));  //  limit
    i->EStack().push(new TokenArray(*ad1));           //  target array (copy)
    i->EStack().push_move(i->pick(1));          //  argument array
    i->EStack().push(new IntegerDatum(0));            //  procedure counter
    i->EStack().push(new IntegerDatum(0));            //  initial counter
    i->EStack().push_move(i->top());           //  procedure

    i->EStack().push(i->baselookup(Name("::MapThread")));
    i->pop(2);
    i->inc_call_depth();
  }
  else // size > 0
  {
    i->pop();
    i->EStack().pop();
  }

}


// Put a token to a nested array.
void SLIArrayModule::Put_a_a_tFunction::execute(SLIInterpreter *i) const
{
  if(i->load()<3 )
  {
    i->message(SLIInterpreter::M_ERROR, "Put","Too few parameters supplied.");
    i->message(SLIInterpreter::M_ERROR, "Put","Usage: [array] [d1 ...dn] obj Put -> [array]");
    i->raiseerror(i->StackUnderflowError);
    return;
  }

  TokenArray *source = dynamic_cast<TokenArray *>(i->pick(2).datum());
  if(source == NULL)
  {
    i->message(SLIInterpreter::M_ERROR, "Put","First argument must be an array.");
    i->message(SLIInterpreter::M_ERROR, "Put","Usage: [array] [d1 ...dn]  obj Put -> [array]");
    i->raiseerror(i->ArgumentTypeError);
    return;
  }


  TokenArray *pos    = dynamic_cast<TokenArray *>(i->pick(1).datum());

  if(pos == NULL)
  {
    i->message(SLIInterpreter::M_ERROR, "Put","Second argument must be an array indicating the position is a nested array.");
    i->message(SLIInterpreter::M_ERROR, "Put","Usage: [array] [d1 ...dn]  obj Put -> [array]");
    i->raiseerror(i->ArgumentTypeError);
    return;
  }

  for(Token *t=pos->begin(); t != pos->end(); ++t)
  {
    assert(t != NULL);
    IntegerDatum *idx=dynamic_cast<IntegerDatum *>(t->datum());
    if(idx == NULL)
    {
      i->message(SLIInterpreter::M_ERROR, "Put","Non integer index found.");
      i->message(SLIInterpreter::M_ERROR, "Put","Source array is unchanged.");
      i->raiseerror(i->ArgumentTypeError);
      return;
    }

    int j=idx->get();

    if (j <0)
    {
      i->message(SLIInterpreter::M_ERROR, "Put","Negative index found.");
      i->message(SLIInterpreter::M_ERROR, "Put","Source array is unchanged.");
      i->raiseerror(i->ArgumentTypeError);
      return;
    }

    if(j >= (int)source->size())
    {
      i->message(SLIInterpreter::M_ERROR, "Put","Index out of range.");
      i->message(SLIInterpreter::M_ERROR, "Put","Source array is unchanged.");
      i->raiseerror(i->ArgumentTypeError);
      return;
    }

    if(t < pos->end()-1)
    {
      source= dynamic_cast<TokenArray *>((*source)[j].datum());
      if(source==NULL)
      {
	i->message(SLIInterpreter::M_ERROR, "Put","Dimensions of index and array do not match.");
	i->message(SLIInterpreter::M_ERROR, "Put","Source array is unchanged.");
	i->raiseerror(i->ArgumentTypeError);
	return;
      }
    }
    else
    {
      // Now source points to the innermost target array and we can replace the object.
      (*source)[j].swap(i->top());
    }
  }

  i->EStack().pop();
  i->pop(2);

}

/* BeginDocumentation

Name: area - Return array of indices defining a 2d subarea of a 2d array.

Synopsis:
                source_width source_anchor_y source_anchor_x
    area_height   area_width   area_anchor_y   area_anchor_x
                                                        area -> [1d-indices]

Description:
  Given a -- hypothetical -- twodimensional array,
  "area" tells you, what indices you need to
  subscript a contiguous, twodimensional subarea.

  The subarea is defined by specifying it's size
  (width and height), as well as its location in the
  source array. The location is defined by specifying
  an anchor point in the source array as well as in
  the subarea. Anchor points are matched, see
  illustration, and examples below:

  source array: height=6, width=15, anchor=(2,5)
  subarea     : height=4, width= 5, anchor=(1,3)
  ...............
  ..ooooo........
  ..oooxo........
  ..ooooo........
  ..ooooo........
  ...............


  "area" returns an array of ONEDIMENSIONAL indices.
  There is a SLI function called "area2" returning
  twodimensional indices, as well as the conversion
  functions "cv1d" and "cv2d".
  (For information on the order of subscription in NEST
  arrays, see references below.)

Parameters:
   In: "area" takes seven integer arguments (one integer
       and three pairs). These arguments describe (1) the width of the
       (hypothetical) source array, (2) the height and width of the
       subarea, as well as (3&4) an anchor point in each of the two
       arrays (see illustration above):

         source_width   : width  of the (hypothetical) source
                          array to be subscribed into
         source_anchor_y,
         source_anchor_x: position of the anchor point relative
                          to ORIGIN OF THE SOURCE ARRAY

         area_heigh  t  : height of the subarea to be subscribed
         area_width     : width  of the subarea to be subscribed
         area_anchor_y,
         area_anchor_x  : position of the anchor point relative
                          to ORIGIN OF THE SUBAREA

  Out: "area" returns an array of ONEDIMENSIONAL indices:

         [1d-indices]   : flat integer array containing the indices
                          that can be used to subscript the
                          (hypothetical) source array in order to
                          access the desired subarea.

                          Indices are onedimensional, and are returned
                          in standard NEST (monotonic) counting order.
                          (For information on the order of
                          subscription in NEST arrays, see references
                          below.)

Examples:
  (Examples are illustrated):

  Ex. 1: source array: (height=5), width=10, anchor=(0,0)
         subarea     :  height=3, width= 3, anchor=(0,0)
         xoo.......
         ooo.......
         ooo.......
         ..........
         ..........

         10 0 0  3 3 0 0 area -> [0 1 2  10 11 12  20 21 22]

  Ex. 1b:source array: (height=5), width=10, anchor=(2,2)
         subarea     :  height=3, width= 3, anchor=(2,2)
         ooo.......
         ooo.......
         oox.......
         ..........
         ..........

         10 2 2  3 3 2 2 area -> [0 1 2  10 11 12  20 21 22]

  Ex. 1c:Note that anchor point may lie outside both
         arrays' bounds:
         source array: (height=5), width=10, anchor=(1,12)
         subarea     :  height=3, width= 3, anchor=(1,12)
         ooo.......
         ooo.......  x
         ooo.......
         ..........
         ..........

         10 1 12  3 3 1 12 area -> [0 1 2  10 11 12  20 21 22]

  Ex. 2: source array: (height=6), width=15, anchor=(2,5)
         subarea     :  height=4, width= 5, anchor=(1,3)
         ...............
         ..ooooo........
         ..oooxo........
         ..ooooo........
         ..ooooo........
         ...............

         15 2 5  4 5 1 3 area -> [17 18 19 20 21
                                  32 33 34 35 36
                                  47 48 49 50 51
                                  62 63 64 65 66]


Diagnostics:
  May raise the following SLI interpreter errors:
    StackUnderflowError
    ArgumentTypeError

  NO ARGUMENT RANGE CHECK IS PERFORMED.
  The result may be useless if subarea is not
  contained in the source array. Note that THIS
  RESTRICTION DOES NOT APPLY TO FUNCTION "area2".

  However, anchor points may lie outside the array
  bounds.

  Note that the height of the source array is not used in computation,
  and does not appear in the parameter list

Author: Ruediger Kupper

References: (TO BE DONE: NEST layer indexing conventions)

SeeAlso: area2
*/
void SLIArrayModule::AreaFunction::execute(SLIInterpreter *i) const
{
  if(i->load()<7 )
  {
    i->message(SLIInterpreter::M_ERROR, "area","Too few parameters supplied.");
    i->message(SLIInterpreter::M_ERROR, "area","Usage: sw say sax  ah aw aay aax  area");
    i->message(SLIInterpreter::M_ERROR, "area","where:  sw : source array width");
    i->message(SLIInterpreter::M_ERROR, "area","        say: source array anchor y position");
    i->message(SLIInterpreter::M_ERROR, "area","        sax: source array anchor x position");
    i->message(SLIInterpreter::M_ERROR, "area","        ah : subregion height");
    i->message(SLIInterpreter::M_ERROR, "area","        aw : subregion width");
    i->message(SLIInterpreter::M_ERROR, "area","        aay: subregion anchor y position");
    i->message(SLIInterpreter::M_ERROR, "area","        aax: subregion anchor x position");
    i->raiseerror(i->StackUnderflowError);
    return;
  }
  //  IntegerDatum* s_h_d    = dynamic_cast<IntegerDatum*>(i->pick(7).datum());
  IntegerDatum* s_w_d    = dynamic_cast<IntegerDatum*>(i->pick(6).datum());
  IntegerDatum* s_y_d    = dynamic_cast<IntegerDatum*>(i->pick(5).datum());
  IntegerDatum* s_x_d    = dynamic_cast<IntegerDatum*>(i->pick(4).datum());

  IntegerDatum* a_h_d    = dynamic_cast<IntegerDatum*>(i->pick(3).datum());
  IntegerDatum* a_w_d    = dynamic_cast<IntegerDatum*>(i->pick(2).datum());
  IntegerDatum* a_y_d    = dynamic_cast<IntegerDatum*>(i->pick(1).datum());
  IntegerDatum* a_x_d    = dynamic_cast<IntegerDatum*>(i->pick(0).datum());

//   if(s_h_d == NULL)
//   {
//     i->raiseerror(i->ArgumentTypeError);
//     return;
//   }
  if(s_w_d == NULL)
  {
    i->raiseerror(i->ArgumentTypeError);
    return;
  }
  if(s_y_d == NULL)
  {
    i->raiseerror(i->ArgumentTypeError);
    return;
  }
  if(s_x_d == NULL)
  {
    i->raiseerror(i->ArgumentTypeError);
    return;
  }
  if(a_h_d == NULL)
  {
    i->raiseerror(i->ArgumentTypeError);
    return;
  }
  if(a_w_d == NULL)
  {
    i->raiseerror(i->ArgumentTypeError);
    return;
  }
  if(a_y_d == NULL)
  {
    i->raiseerror(i->ArgumentTypeError);
    return;
  }
  if(a_x_d == NULL)
  {
    i->raiseerror(i->ArgumentTypeError);
    return;
  }

  // not needed for computation: long const s_h = s_h_d->get();
  long const s_w = s_w_d->get();
  long const s_y = s_y_d->get();
  long const s_x = s_x_d->get();

  long const a_h = a_h_d->get();
  long const a_w = a_w_d->get();
  long const a_y = a_y_d->get();
  long const a_x = a_x_d->get();

  TokenArray indices;
  indices.reserve(a_w * a_h);

  // compute upper left corner in source array:
  long const s_0_y = s_y-a_y;
  long const s_0_x = s_x-a_x;

  for(long y=0; y<a_h; ++y)
    {
      for(long x=0; x<a_w; ++x)
        {
          indices.push_back(s_0_x + s_0_y*s_w  +  x + y*s_w);
        }
    }

  i->pop(7);
  i->push_by_pointer(new TokenArray(indices));
  i->EStack().pop();
}

/* BeginDocumentation

Name: area2 - Return array of indices defining a 2d subarea of a 2d array.

Synopsis:
                             source_anchor_y source_anchor_x
    area_height   area_width   area_anchor_y   area_anchor_x
                                                       area2 -> [2d-indices]

Description:
  Given a -- hypothetical -- twodimensional array,
  "area" tells you, what indices you need to
  subscript a contiguous, twodimensional subarea.

  The subarea is defined by specifying it's size
  (width and height), as well as its location in the
  source array. The location is defined by specifying
  an anchor point in the source array as well as in
  the subarea. Anchor points are matched, see
  illustration, and examples below:

  source array: height=6, width=15, anchor=(2,5)
  subarea     : height=4, width= 5, anchor=(1,3)
  ...............
  ..ooooo........
  ..oooxo........
  ..ooooo........
  ..ooooo........
  ...............


  "area2" returns an array of TWODIMENSIONAL indices.
  There is a SLI function called "area" returning
  onedimensional indices, as well as the conversion
  functions "cv1d" and "cv2d".
  (For information on the order of subscription in NEST
  arrays, see references below.)

Parameters:
   In: "area2" takes six integer arguments (three pairs).
       These arguments describe (1) the height and width of the
       subarea to be indexed in the (hypothetical) source array, as
       well as (2&3) an anchor point in each of the two arrays (see
       illustration above):

         source_anchor_y,
         source_anchor_x: position of the anchor point relative
                          to ORIGIN OF THE SOURCE ARRAY

         area_heigh  t  : height of the subarea to be subscribed
         area_width     : width  of the subarea to be subscribed
         area_anchor_y,
         area_anchor_x  : position of the anchor point relative
                          to ORIGIN OF THE SUBAREA

  Out: "area" returns an array of ONEDIMENSIONAL indices:

         [2d-indices]   : flat integer array containing the indices
                          that can be used to subscript the
                          (hypothetical) source array in order to
                          access the desired subarea.

                          Indices are twodimensional. The returned
                          array is flat and has the following order:
                          [1y 1x  2y 2x  3y 3x  ...  ny nx]
                          That is, each pair of numbers indicates the
                          y- and the x-component of a respective
                          index.

                          The indices 1..n are returned in standard
                          NEST counting order. (For information on the
                          order of subscription in NEST arrays, see
                          references below.)

Examples:
  (Examples are illustrated):

  Ex. 1: source array: (height=5), (width=10), anchor=(0,0)
         subarea     :  height=3,   width= 3,  anchor=(0,0)
         xoo.......
         ooo.......
         ooo.......
         ..........
         ..........

         0 0  3 3 0 0 area2 -> [0 0  0 1  0 2
                                1 0  1 1  1 2
                                2 0  2 1  2 2]

  Ex. 1b:source array: (height=5), (width=10), anchor=(2,2)
         subarea     :  height=3,   width= 3,  anchor=(2,2)
         ooo.......
         ooo.......
         oox.......
         ..........
         ..........

         2 2  3 3 2 2 area2 -> [0 0  0 1  0 2
                                1 0  1 1  1 2
                                2 0  2 1  2 2]

  Ex. 1c:Note that anchor point may lie outside both
         arrays' bounds:
         source array: (height=5), (width=10), anchor=(1,12)
         subarea     :  height=3,   width= 3,  anchor=(1,12)
         ooo.......
         ooo.......  x
         ooo.......
         ..........
         ..........

         1 12  3 3 1 12 area2 -> [0 0  0 1  0 2
                                  1 0  1 1  1 2
                                  2 0  2 1  2 2]

  Ex. 2: source array: (height=6), (width=15), anchor=(2,5)
         subarea     :  height=4,   width= 5,  anchor=(1,3)
         ...............
         ..ooooo........
         ..oooxo........
         ..ooooo........
         ..ooooo........
         ...............

         2 5  4 5 1 3 area2 -> [1 2  1 3  1 4  1 5  1 6
                                2 2  2 3  2 4  2 5  2 6
                                3 2  3 3  3 4  3 5  3 6
                                4 2  4 3  4 4  4 5  4 6]

  Ex. 3: Note that subarea doesn't need to lie
         inside bounds of source array:
         source array: (height=4), (width= 8), anchor=(4,-1)
         subarea     :  height=2,   width= 3,  anchor=(1, 0)
         ........
         ........
         ........
        ooo......
        xoo


         4 -1  2 3 1 0 area2 -> [3 -1  3 0  3 1
                                 4 -1  4 0  4 1]

Diagnostics:
  "area2" may raise the following SLI interpreter errors:
    StackUnderflowError
    ArgumentTypeError

  No argument range check is performed. The returned
  indices may indicate regions outside the source
  array (see Example 3). This is not a bug, it's a
  feature :-). Note that restrictions apply to the
  related function "area".

  However, anchor points may lie outside the array
  bounds.

  Note that arguments source_width and source_height are not used in
  computation, and do not appear in the argument list.

Author: Ruediger Kupper

References: (TO BE DONE: NEST layer indexing conventions)

SeeAlso: area
*/
void SLIArrayModule::Area2Function::execute(SLIInterpreter *i) const
{
  if(i->load()<6 )
  {
    i->message(SLIInterpreter::M_ERROR, "area2","Too few parameters supplied.");
    i->message(SLIInterpreter::M_ERROR, "area2","Usage: say sax  ah aw aay aax  area2");
    i->message(SLIInterpreter::M_ERROR, "area2","where:  say: source array anchor y position");
    i->message(SLIInterpreter::M_ERROR, "area2","        sax: source array anchor x position");
    i->message(SLIInterpreter::M_ERROR, "area2","        ah : subregion height");
    i->message(SLIInterpreter::M_ERROR, "area2","        aw : subregion width");
    i->message(SLIInterpreter::M_ERROR, "area2","        aay: subregion anchor y position");
    i->message(SLIInterpreter::M_ERROR, "area2","        aax: subregion anchor x position");
    i->raiseerror(i->StackUnderflowError);
    return;
  }
//   IntegerDatum* s_h_d    = dynamic_cast<IntegerDatum*>(i->pick(7).datum());
//   IntegerDatum* s_w_d    = dynamic_cast<IntegerDatum*>(i->pick(6).datum());
  IntegerDatum* s_y_d    = dynamic_cast<IntegerDatum*>(i->pick(5).datum());
  IntegerDatum* s_x_d    = dynamic_cast<IntegerDatum*>(i->pick(4).datum());

  IntegerDatum* a_h_d    = dynamic_cast<IntegerDatum*>(i->pick(3).datum());
  IntegerDatum* a_w_d    = dynamic_cast<IntegerDatum*>(i->pick(2).datum());
  IntegerDatum* a_y_d    = dynamic_cast<IntegerDatum*>(i->pick(1).datum());
  IntegerDatum* a_x_d    = dynamic_cast<IntegerDatum*>(i->pick(0).datum());

//   if(s_h_d == NULL)
//   {
//     i->raiseerror(i->ArgumentTypeError);
//     return;
//   }
//   if(s_w_d == NULL)
//   {
//     i->raiseerror(i->ArgumentTypeError);
//     return;
//   }
  if(s_y_d == NULL)
  {
    i->raiseerror(i->ArgumentTypeError);
    return;
  }
  if(s_x_d == NULL)
  {
    i->raiseerror(i->ArgumentTypeError);
    return;
  }
  if(a_h_d == NULL)
  {
    i->raiseerror(i->ArgumentTypeError);
    return;
  }
  if(a_w_d == NULL)
  {
    i->raiseerror(i->ArgumentTypeError);
    return;
  }
  if(a_y_d == NULL)
  {
    i->raiseerror(i->ArgumentTypeError);
    return;
  }
  if(a_x_d == NULL)
  {
    i->raiseerror(i->ArgumentTypeError);
    return;
  }

  // not needed for computation: long const s_h = s_h_d->get();
  // not needed for computation: long const s_w = s_w_d->get();
  long const s_y = s_y_d->get();
  long const s_x = s_x_d->get();

  long const a_h = a_h_d->get();
  long const a_w = a_w_d->get();
  long const a_y = a_y_d->get();
  long const a_x = a_x_d->get();

  TokenArray indices;
  indices.reserve(a_w * a_h);

  // compute upper left corner in source array:
  long const s_0_y = s_y-a_y;
  long const s_0_x = s_x-a_x;

  for(long y=0; y<a_h; ++y)
    {
      for(long x=0; x<a_w; ++x)
        {
          indices.push_back(s_0_y + y);
          indices.push_back(s_0_x + x);
        }
    }

  i->pop(6);
  i->push_by_pointer(new TokenArray(indices));
  i->EStack().pop();
}

/* BeginDocumentation
Name: cv1d - convert 2-dimensional coordinates to 1-dim index
Synopsis: y   x   w  cv1d -> i

Description: This function converts a 2-dimensional matrix address to
the corresponding index of a linear array.  Useful if you have to handle
2-dimensional data (e.g. an image) which is stored in a linear array.
Note that by convention the origin is 0,0 and at the upper left corner.

Parameters: y : integer. the y-coordinate
x : integer. the x coordinate
w : integer. the border with of the 2-dimensional coordinate system

Example: 3 2 4 cv1d -> 14
3 is the y-coordinate (row)
2 is the x coordinate (column)
4 is the number of columns
14 is the index of the corresponding element in a 1-dimensional array

SeeAlso: cst, cva, cv2d, cvd, cvi, cvlit, cvn, cvs, cvt_a
*/
void SLIArrayModule::Cv1dFunction::execute(SLIInterpreter *i) const
{
  if(i->load()<3 )
    {
      i->message(SLIInterpreter::M_ERROR, "cv1d","Too few parameters supplied.");
      i->message(SLIInterpreter::M_ERROR, "cv1d","Usage: y x w cv1d");
      i->raiseerror(i->StackUnderflowError);
    return;
  }

  IntegerDatum *w= dynamic_cast<IntegerDatum*>(i->pick(0).datum());
  IntegerDatum *x= dynamic_cast<IntegerDatum*>(i->pick(1).datum());
  IntegerDatum *y= dynamic_cast<IntegerDatum*>(i->pick(2).datum());

  if(w == NULL)
  {
    i->message(SLIInterpreter::M_ERROR, "cv1d","integertype expected");
    i->message(SLIInterpreter::M_ERROR, "cv1d","Usage: y x w cv1d");
    i->raiseerror(i->ArgumentTypeError);
    return;
  }

  if(x == NULL)
  {
    i->message(SLIInterpreter::M_ERROR, "cv1d","integertype expected");
    i->message(SLIInterpreter::M_ERROR, "cv1d","Usage: y x w cv1d");
    i->raiseerror(i->ArgumentTypeError);
    return;
  }

  if(y == NULL)
  {
    i->message(SLIInterpreter::M_ERROR, "cv1d","integertype expected");
    i->message(SLIInterpreter::M_ERROR, "cv1d","Usage: y x w cv1d");
    i->raiseerror(i->ArgumentTypeError);
    return;
  }

  // y= y*w + x
  y->get()*=(w->get());
  y->get()+= (x->get());
  i->pop(2);
  i->EStack().pop();
  // no i->push(), because we change the objects directly
  // on the stack. Low overhead.
}

/* BeginDocumentation
Name: cv2d - convert 1-dimensional index to 2-dim coordinate
Synopsis: i  w  cv2d -> y   x
int int        int int

Description:This function transforms an array index to y,x
coordinate pair. Useful if you have intrinsically 2-dimensional
data stored in a linear array (e.g. images).

Parameters:i : integer. the index in the array
h : integer. the number of rows in the 2-dimensional space
w : integer. the number of columns in the 2-dimensional space


Example:14 4 cv2d -> 3 2
14 is the index of the corresponding element in a 1-dimensional array
4 is the number of columns
3 is the resulting y-coordinate (row)
2 is the resulting x-coordinate (column)

SeeAlso: cst, cva, cv1d, cvd, cvi, cvlit, cvn, cvs, cvt_a
*/
void SLIArrayModule::Cv2dFunction::execute(SLIInterpreter *i) const
{
  if(i->load()<2 )
    {
      i->message(SLIInterpreter::M_ERROR, "cv2d","Too few parameters supplied.");
      i->message(SLIInterpreter::M_ERROR, "cv2d","Usage: i w cv2d");
      i->raiseerror(i->StackUnderflowError);
      return;
    }

  IntegerDatum *w  = dynamic_cast<IntegerDatum*>(i->pick(0).datum());
  IntegerDatum *in = dynamic_cast<IntegerDatum*>(i->pick(1).datum());

  if(w == NULL)
  {
    i->message(SLIInterpreter::M_ERROR, "cv2d","integertype expected");
    i->message(SLIInterpreter::M_ERROR, "cv2d","Usage: i w cv2d");
    i->raiseerror(i->ArgumentTypeError);
    return;
  }

  if(in == NULL)
  {
    i->message(SLIInterpreter::M_ERROR, "cv2d","integertype expected");
    i->message(SLIInterpreter::M_ERROR, "cv2d","Usage: i w cv2d");
    i->raiseerror(i->ArgumentTypeError);
    return;
  }


  long tmp = in->get();
  //  y = i / w
  (in->get()) /= (w->get());
  // x = i % w
  *w = tmp % w->get();
  i->EStack().pop();
  // no i->push(), because we change the objects directly
  // on the stack. Low overhead.
}


/* BeginDocumentation
Name: GetMax - get maximal element
Synopsis: array GetMax -> int
Description: returns the maximum value in an array of ints.
SeeAlso: GetMin
Remarks: works only for integer arrays.
*/
void SLIArrayModule::GetMaxFunction::execute(SLIInterpreter *i) const
{
  if(i->load()<1 )
    {
      i->message(SLIInterpreter::M_ERROR, "GetMax","Too few parameters supplied.");
      i->message(SLIInterpreter::M_ERROR, "GetMax","Usage: <array> GetMax");
      i->raiseerror(i->StackUnderflowError);
      return;
    }

  TokenArray *a=dynamic_cast<TokenArray*>(i->top().datum());
  if (a==NULL)
    {
      i->message(SLIInterpreter::M_ERROR, "GetMax","argument must be an array");
      i->raiseerror(i->ArgumentTypeError);
      return;
    }

  IntegerDatum * tmp = dynamic_cast<IntegerDatum*>(a->begin()->datum());
  if (tmp==NULL)
    {
      i->message(SLIInterpreter::M_ERROR, "GetMax","argument array may only contain integers");
      i->raiseerror(i->ArgumentTypeError);
      return;
    }

  IntegerDatum * tmp2;
  unsigned int pos=0;
  while (pos < a->size())
    {
      tmp2 = dynamic_cast<IntegerDatum*>(a->get(pos).datum());
      if (tmp2==NULL)
	{
	  i->message(SLIInterpreter::M_ERROR, "GetMax","argument array may only contain integers");
	  i->raiseerror(i->ArgumentTypeError);
	  return;
	}
      if (tmp->get() < tmp2->get()) tmp=tmp2;
      ++pos;
    }
  Token result(*tmp);

  i->pop();
  i->push(result);
  i->EStack().pop();
}

/* BeginDocumentation
Name: GetMin - get minimal element
Synopsis: array GetMin -> int
Description: returns the minimum value in an array of ints.
Remarks: works only for integer arrays.
SeeAlso: GetMax
*/
void SLIArrayModule::GetMinFunction::execute(SLIInterpreter *i) const
{
  if(i->load()<1 )
    {
      i->message(SLIInterpreter::M_ERROR, "GetMin","Too few parameters supplied.");
      i->message(SLIInterpreter::M_ERROR, "GetMin","Usage: <array> GetMin");
      i->raiseerror(i->StackUnderflowError);
      return;
    }

  TokenArray *a=dynamic_cast<TokenArray*>(i->top().datum());
  if (a==NULL)
    {
      i->message(SLIInterpreter::M_ERROR, "GetMin","argument must be an array");
      i->raiseerror(i->ArgumentTypeError);
      return;
    }


  IntegerDatum * tmp = dynamic_cast<IntegerDatum*>(a->begin()->datum());
  if (tmp==NULL)
    {
      i->message(SLIInterpreter::M_ERROR, "GetMin","argument array may only contain integers");
      i->raiseerror(i->ArgumentTypeError);
      return;
    }

  IntegerDatum * tmp2;
  unsigned int pos=0;
  while (pos < a->size())
    {
      tmp2 = dynamic_cast<IntegerDatum*>(a->get(pos).datum());
      if (tmp2==NULL)
	{
	  i->message(SLIInterpreter::M_ERROR, "GetMin","argument array may only contain integers");
	  i->raiseerror(i->ArgumentTypeError);
	  return;
	}
      if (tmp->get() > tmp2->get()) tmp=tmp2;
      ++pos;
    }
  Token result(*tmp);

  i->pop();
  i->push(result);
  i->EStack().pop();
}

/* BeginDocumentation
Name: gabor_ - Return 2D array with Gabor patch.
Synopsis:
nr nc xmin xmax ymin ymax lambda orient phase sigma el
Description:
Returns an nr by nc matrix with a Gabor patch, computed over the
argument range of [xmin,xmax] by [ymin,ymax].
This function is the low level variant of the more user-friendly
GaborPatch.
SeeAlso: arr::GaborPatch
Author: Marc-Oliver Gewaltig
References: Petkov N and Kruizinga P: Biol. Cybern. 76, 83-96 (1997)
*/
void SLIArrayModule::GaborFunction::execute(SLIInterpreter *i) const
{
  if(i->load() < 11)
  {
    i->raiseerror("StackUnderflow");
    return;
  }

  long   nrow=0;
  long   ncol=0;
  double xmin=0.0;
  double xmax=0.0;
  double ymin=0.0;
  double ymax=0.0;
  double lambda=0.0;
  double phi=0.0;
  double phase=0.0;
  double sigma=0.0;
  double gamma=0.0;

  try
  {
    nrow=  getValue<long>(i->pick(10));
    ncol=  getValue<long>(i->pick(9));
    xmin=  getValue<double>(i->pick(8));
    xmax=  getValue<double>(i->pick(7));
    ymin=  getValue<double>(i->pick(6));
    ymax=  getValue<double>(i->pick(5));
    lambda=getValue<double>(i->pick(4));
    phi=   getValue<double>(i->pick(3));
    phase= getValue<double>(i->pick(2));
    sigma= getValue<double>(i->pick(1));
    gamma= getValue<double>(i->pick(0));
  }
  catch(...)
  {
    i->raiseerror("ArgumentType");
    return;
  }
  if(ymin>=ymax)
  {
    i->message(SLIInterpreter::M_ERROR, "Gabor_","y_max must be > y_min.");
    i->raiseerror("RangeCheck");
    return;
  }
  if(xmin>=xmax)
  {
    i->message(SLIInterpreter::M_ERROR, "Gabor_","x_max must be > x_min.");
    i->raiseerror("RangeCheck");
    return;
  }
  if((ncol<2) || (nrow<2))
  {
    i->message(SLIInterpreter::M_ERROR, "Gabor_","Matrix must have at least two rows and two columns.");
    i->raiseerror("RangeCheck");
    return;
  }

  assert(ymax>ymin);
  assert(xmax>xmin);
  assert(ncol>1);
  assert(nrow>1);

  const double sig_sq = 2.0*sigma*sigma;
  const double gam_sq = gamma*gamma;
  const double cos_phi = std::cos(phi);
  const double sin_phi = std::sin(phi);
  const double s_fact = 2.0*numerics::pi*std::sin(phi)/lambda;
  const double c_fact = 2.0*numerics::pi*std::cos(phi)/lambda;
  const double dx= (xmax-xmin)/(ncol-1.0);
  const double dy= (ymax-ymin)/(nrow-1.0);

  TokenArray result;
  result.reserve(nrow);

  std::vector<double> col(ncol);
  for(size_t r=0; r< (size_t)nrow; ++r)
  {
    const double y=ymin + r*dy;
    for(size_t c=0; c< (size_t)ncol; ++c)
    {
      const double x=xmin + c*dx;
      const double x1=x*cos_phi - y*sin_phi;
      const double y1=x*sin_phi + y*cos_phi;
      const double x2=x*c_fact - y*s_fact;

      col[c]= std::exp(-(x1*x1 + gam_sq*y1*y1)/sig_sq) * std::cos(x2 - phase);
    }
    result.push_back(new TokenArray(col));
  }
  i->pop(11);
  i->push(result);
  i->EStack().pop();
}

/* BeginDocumentation
Name: gauss2d_ - Return 2D array with Gauss patch.
Synopsis:
nr nc xmin xmax ymin ymax phi sigma gamma
Description:
Returns an nr by nc matrix with a Gauss patch, computed over the
argument range of [xmin,xmax] by [ymin,ymax].
This function is the low level variant of the more user-friendly
GaussPatch.
SeeAlso: arr::GaussPatch
Author: Marc-Oliver Gewaltig
*/
void SLIArrayModule::Gauss2dFunction::execute(SLIInterpreter *i) const
{
  if(i->load() < 9)
  {
    i->raiseerror("StackUnderflow");
    return;
  }

  long   nrow=0;
  long   ncol=0;
  double xmin=0.0;
  double xmax=0.0;
  double ymin=0.0;
  double ymax=0.0;
  double phi=0.0;
  double sigma=0.0;
  double gamma=0.0;

  try
  {
    nrow=  getValue<long>(i->pick(8));
    ncol=  getValue<long>(i->pick(7));
    xmin=  getValue<double>(i->pick(6));
    xmax=  getValue<double>(i->pick(5));
    ymin=  getValue<double>(i->pick(4));
    ymax=  getValue<double>(i->pick(3));
    phi=   getValue<double>(i->pick(2));
    sigma= getValue<double>(i->pick(1));
    gamma= getValue<double>(i->pick(0));
  }
  catch(...)
  {
    i->raiseerror("ArgumentType");
    return;
  }
  if(ymin>=ymax)
  {
    i->message(SLIInterpreter::M_ERROR, "gauss2d_","y_max must be > y_min.");
    i->raiseerror("RangeCheck");
    return;
  }
  if(xmin>=xmax)
  {
    i->message(SLIInterpreter::M_ERROR, "gauss2s_","x_max must be > x_min.");
    i->raiseerror("RangeCheck");
    return;
  }
  if((ncol<2) || (nrow<2))
  {
    i->message(SLIInterpreter::M_ERROR, "gauss2d_","Matrix must have at least two rows and two columns.");
    i->raiseerror("RangeCheck");
    return;
  }

  assert(ymax>ymin);
  assert(xmax>xmin);
  assert(ncol>1);
  assert(nrow>1);

  const double sig_sq = 2.0*sigma*sigma;
  const double gam_sq = gamma*gamma;
  const double dx= (xmax-xmin)/(ncol-1.0);
  const double dy= (ymax-ymin)/(nrow-1.0);
  const double cos_phi = std::cos(phi);
  const double sin_phi = std::sin(phi);

  TokenArray result;
  result.reserve(nrow);

  std::vector<double> col(ncol);
  for(size_t r=0; r< (size_t)nrow; ++r)
  {
    const double y=ymin + r*dy;
    col.assign(ncol,0.0); // clear contents
    for(size_t c=0; c< (size_t)ncol; ++c)
    {
      const double x=xmin + c*dx;
      const double x1=x*cos_phi - y*sin_phi;
      const double y1=x*sin_phi + y*cos_phi;

      col[c]= std::exp(-(x1*x1 + gam_sq*y1*y1)/sig_sq);
    }
    result.push_back(new TokenArray(col));
  }
  i->pop(9);
  i->push(result);
  i->EStack().pop();
}

void SLIArrayModule::Array2IntVectorFunction::execute(SLIInterpreter *i) const
{
  if(i->load()<1)
  {
    i->raiseerror(i->StackUnderflowError);
    return;
  }
  try
  {
    IntVectorDatum ivd( new std::vector<long>(getValue<std::vector<long> >(i->top())));
    i->pop();
    i->push(ivd);
  }
  catch(...)
  {
    i->raiseerror(i->ArgumentTypeError);
    return;
  }
  i->EStack().pop();
}

void SLIArrayModule::Array2DoubleVectorFunction::execute(SLIInterpreter *i) const
{
  if(i->load()<1)
  {
    i->raiseerror(i->StackUnderflowError);
    return;
  }
  try
  {
    DoubleVectorDatum ivd( new std::vector<double>(getValue<std::vector<double> >(i->top())));
    i->pop();
    i->push(ivd);
  }
  catch(...)
  {
    i->raiseerror(i->ArgumentTypeError);
    return;
  }
  i->EStack().pop();

}

void SLIArrayModule::IntVector2ArrayFunction::execute(SLIInterpreter *i) const
{
  if(i->load()<1)
  {
    i->raiseerror(i->StackUnderflowError);
    return;
  }
  IntVectorDatum *ivd= dynamic_cast<IntVectorDatum *>(i->top().datum());
  if(ivd == 0)
  {
    i->raiseerror(i->ArgumentTypeError);
    return;
  }
  TokenArray ad(**ivd);
  i->pop();
  i->push(ad);
  i->EStack().pop();
}

void SLIArrayModule::DoubleVector2ArrayFunction::execute(SLIInterpreter *i) const
{
  if(i->load()<1)
  {
    i->raiseerror(i->StackUnderflowError);
    return;
  }
  DoubleVectorDatum *ivd= dynamic_cast<DoubleVectorDatum *>(i->top().datum());
  if(ivd == 0)
  {
    i->raiseerror(i->ArgumentTypeError);
    return;
  }
  TokenArray ad(**ivd);
  i->pop();
  i->push(ad);
  i->EStack().pop();

}

void SLIArrayModule::FiniteQ_dFunction::execute(SLIInterpreter *i) const
{
  i->assert_stack_load(1);
  const double x = getValue<double>(i->pick(0));

  BoolDatum res(  -std::numeric_limits<double>::max() <= x
                && x <= std::numeric_limits<double>::max() );
  i->push(res);
  i->EStack().pop();
}

void SLIArrayModule::init(SLIInterpreter *i)
{
  i->createcommand("MapIndexed_a", &mapindexedfunction);
  i->createcommand("Map", &mapfunction);
  i->createcommand("MapThread_a", &mapthreadfunction);
  i->createcommand("Reverse", &reversefunction);
  i->createcommand("Rotate", &rotatefunction);
  i->createcommand("Flatten", &flattenfunction);
  i->createcommand("Sort", &sortfunction);
  i->createcommand("Transpose", &transposefunction);
  i->createcommand("Partition_a_i_i", &partitionfunction);
  i->createcommand(sli::imap, &imapfunction);
  i->createcommand(sli::imapindexed, &imapindexedfunction);
  i->createcommand("::MapThread", &imapthreadfunction);
  i->createcommand("Range", &rangefunction);
  i->createcommand("arrayload", &arrayloadfunction);
  i->createcommand("valid_a", &validfunction);
  i->createcommand("area", &areafunction);
  i->createcommand("area2", &area2function);
  i->createcommand("cv1d", &cv1dfunction);
  i->createcommand("cv2d", &cv2dfunction);
  i->createcommand("GetMax", &getmaxfunction);
  i->createcommand("GetMin", &getminfunction);
  i->createcommand("gabor_", &gaborfunction);
  i->createcommand("gauss2d_", &gauss2dfunction);
  i->createcommand("put_a_a_t", &put_a_a_tfunction);
  i->createcommand("array2intvector", &array2intvectorfunction);
  i->createcommand("array2doublevector", &array2doublevectorfunction);
  i->createcommand("doublevector2array", &doublevector2arrayfunction);
  i->createcommand("intvector2array", &intvector2arrayfunction);

  i->createcommand("finite_q_d", &finiteq_dfunction);
}
