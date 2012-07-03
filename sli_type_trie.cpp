/*
 *  typechk.cc
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
    typechk.cc
*/

/******************************************************************
Project:   SLI-2.0, taken from: SLIDE - GUI for SLI

Task:      With a TypeTrie it will be possible to perfrom a type 
           check of (SLI) function input parameters. A TypeNode 
           represents the position and the datatype of a single 
           input parameter. The leaves of the tree will contain the 
           interpreter function of correct input parameters.

           A simple add type tree:
           -----------------------

           *root
            |
           long -----------------> double ->0
            |                        |
           long ->  double->0      long  ->  double -> 0
            |         |             |          |
           (add)->0 (add)->0      (add)->0  (add)->0
            |        |             |         |
	    0        0             0         0

Baseclass: None  

Inherit :  

Author:    Marc-Oliver Gewaltig, Thomas Matyak

Date:      18.11.95

*******************************************************************/


#include "sli_type_trie.h"
#include "sli_array.h"
#include "sli_exceptions.h"

namespace sli3
{

  void TypeNode::toTokenArray(SLIInterpreter *sli, TokenArray &a) const
  {
    assert(a.size()==0);
    if(next == NULL && alt == NULL) // Leaf node
      {
	a.push_back(func);
      }
    else 
      { 
	assert(next != NULL);
	a.push_back(sli->new_token<sli3::literaltype>(type_->get_typename()));
	TokenArray *a_next=new TokenArray();
	next->toTokenArray(*a_next);
	a.push_back(sli->new_token<sli3::arraytype>(a_next));
	if(alt != NULL)
	  {
	    TokenArray *a_alt= new TokenArray();
	    alt->toTokenArray(*a_alt);
	    a.push_back(sli->new_token<sli3::arraytype>(ArrayDatum(a_alt)));
	  }
      }
  }
    
  void TypeNode::info(SLIInterpreter *sli, std::ostream &out, std::deque<TypeNode const *> &tl) const
  {
    if(next == NULL && alt == NULL) // Leaf node
      {
	// print type list then function
	for(int i=tl.size()-1; i>=0;--i)
	  {
	    out << std::setw(15) << std::left << sli->get_type(tl[i]->type_)->get_typename();
	  }
	out  <<"calls " << func << std::endl;
      }
    else 
      { 
	assert(next != NULL);
	tl.push_back(this);
	next->info(sli, out, tl);
	tl.pop_back();
	if(alt != NULL)
	  {
	    alt->info(sli, out, tl);
	  }
      }
  }
    
    
TypeNode *TypeTrie::get_alternative(TypeTrie::TypeNode *pos, unsigned int type)
{
  // Finds Node for the current type in the alternative List,
  // starting at pos. If the type is not already present, a new
  // Node will be created.
  
  while(type != pos->type)
    {
      if(pos->alt == NULL)
	pos->alt =new TypeNode(type);
      
      if(pos->type == sli3::anytype)
	{
	  // any must have been the tail and the previous 
	  // if must have added an extra Node, thus the following
	  // assertion must hold:
	  //assert(pos->alt->alt == NULL);
      
	  TypeNode *new_tail= pos->alt;
      
	  // Move the wildcard to the tail Node.
      
	  pos->type=type;
	  new_tail->type=sli3::anytype;
	  new_tail->func.swap(pos->func);
	  new_tail->next= pos->next;
	  pos->next=NULL;
	  break;
	  // this  while() cycle will terminate, as 
	  // pos->type==type by asignment.
	}
      else
	pos= pos->alt; // pos->alt is always defined here.
    }
  
  return pos;
}

void TypeTrie::insert(const TypeArray& a, Token &f)
{
/*
Task:      Array 'a' adds a correct parameter list into the 
           'TypeTrie'. Function 'f' will manage the handling 
           of a correct parameter list. If 'array' is empty, 
           function 'f' will handle a SLI procedure without
           input parameter.
	   Insert will overwrite a function with identical parameter
	   list which might be already in the trie.

Bugs:	   If a represents a parameter-list which is already 
	   present, nothing happens, just a warning is
	   issued. 

Author:    Marc Oliver Gewaltig

Date:      15.Apr.1998, 18.11.95
            completely rewritten, 16.Apr. 1998

Parameter: a = array of datatypes
           f = interpreter function

*/
    TypeNode *pos=this;
     
    for(unsigned int level=0; level < a.size(); ++level)
      {
      
	pos= getalternative(pos,a[level]);
      
	if(pos->next == NULL)
	  pos->next = new TypeNode();
	pos=pos->next;
      }
  
    /* Error conditions:
       1. If pos->next!=NULL, the parameter list overlaps with 
       an existing function definition.
       2. If pos->alt != NULL, something undefined must have happened.
       This should be impossible.
    */
    if (pos->next == NULL)
      {
	pos->type=sli3::anytype;
	pos->func=f;
      }
    else
      std::cout << "Method 'TypeTrie::InsertFunction'"<< std::endl
		<< "Warning! Ambigous Function Definition ." << std::endl
		<< "A function with longer, but identical initial parameter "
		<< "list is already present!" << std::endl
		<< "Nothing was changed." << std::endl;
}

/*_____ end InsertFunction() _____________________________________*/

  void TypeNode::info(SLIInterpreter *sli, std::ostream &out) const
{
  std::deque<TypeNode const * > tl;
  info(sli, out, tl);
}

