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
#include "sli_interpreter.h"
#include <iomanip>
namespace sli3
{

  void TypeNode::toTokenArray(TokenArray &a) const
  {
      if(type_ == NULL)
      {
          a.push_back(func_);
	  return;
      }

      static SLIInterpreter *sli=type_->get_interpreter();

      assert(a.size()==0);
      a.push_back(sli->new_token<sli3::literaltype>(type_->get_typename()));
      TokenArray *a_next_=new TokenArray();
      next_->toTokenArray(*a_next_);
      a.push_back(sli->new_token<sli3::arraytype>(a_next_));
      if(alt_ != NULL)
      {
          TokenArray *a_alt_= new TokenArray();
          alt_->toTokenArray(*a_alt_);
          a.push_back(sli->new_token<sli3::arraytype>(a_alt_));
      }
  }
    
  void TypeNode::info(std::ostream &out, std::deque<TypeNode const *> &tl) const
  {
      if(type_ == NULL)
	  return;
      
      if(next_ == NULL && alt_ == NULL) // Leaf node
      {
	  // print type list then function
	  for(int i=tl.size()-1; i>=0;--i)
	  {
	      out << std::setw(15) << std::left << tl[i]->type_->get_typename();
	  }
	  out  <<"calls " << func_ << std::endl;
      }
      else 
      { 
	  assert(next_ != NULL);
	  tl.push_back(this);
	  next_->info(out, tl);
	  tl.pop_back();
	  if(alt_ != NULL)
	  {
	      alt_->info(out, tl);
	  }
      }
  }
    
    
    TypeNode *TypeNode::get_alternative(TypeNode *pos, SLIType *type)
    {
	// Finds Node for the current type in the alt_ernative List,
	// starting at pos. If the type is not already present, a new
	// Node will be created.
	

        // Next we treat the special case sli3::anytype, since it must be at the end of the alternative list.
	if (type->get_typeid()==sli3::anytype)
	{
	    SLIType *any_type=type;
	    while(pos->alt_ !=0)
		pos = pos->alt_;
	    
	    // If the tail node is not anytype, we must create a new
            // tail node.
	    if(pos->type_ != any_type)
	    {
		pos->alt_=new TypeNode(any_type);
		pos= pos->alt_;
	    }
	    return pos;
	}
    
	// This is the general case, and we travese the alternative
        // list, until we hit the desired type or the end.
	while(type != pos->type_)
	{
	    if(pos->alt_ == NULL)
	    {
		pos->alt_ =new TypeNode(type);
                
		// If we are at the tail and it is "anytype"
                // we must insert the new node before the
                // tail, so anytype remains the tail.
		if(pos->type_->get_typeid() == sli3::anytype)
		{
		    TypeNode *new_tail= pos->alt_;
		
		    // Move the wildcard to the tail Node.
		    new_tail->type_=pos->type_;
		    pos->type_=type;
		    new_tail->func_.move(pos->func_);
		    new_tail->next_= pos->next_;
		    pos->next_=NULL; // The new node still has no next_ entries.
		    return pos;
		}
		return pos->alt_;
	    }
	
	    pos= pos->alt_; // pos->alt_ is always defined here.
	}
	
	return pos;
    }

/**
  Task:      Array 'a' adds a correct parameter list into the 
  'TypeNode'. Function 'f' will manage the handling 
  of a correct parameter list. If 'array' is empty, 
  function 'f' will handle a SLI procedure without
  input parameter.
  Insert will overwrite a function with identical parameter
  list which might be already in the trie.
  The type sli3::anytype will compare true to all other types.
  Since specific variants of a function must have precedence,
  nodes with type sli3::anytype must always be the tail of the
  alternative parameter list.
  
  Bugs:	   If a represents a parameter-list which is already 
  present, nothing happens, just a warning is
  issued. 
  
  Author:    Marc Oliver Gewaltig
  
  Date:      15.Apr.1998, 18.11.95
             completely rewritten, 16.Apr. 1998
	     rewritten for sli3, July 2012
  
  Parameter: a = array of datatypes
  f = interpreter function
  
*/

    void TypeNode::insert(const TypeArray& a, Token const &f)
    {
	TypeNode *pos=this;
        
        assert( ! a.empty());
        
        const unsigned int max_level=a.size();
        unsigned int level=0;
        while(level<max_level)
        {
            if(pos->type_==NULL)
            {
                if(pos->func_.type_ != NULL)
                    break;
                pos->type_=a[level];
            }
            else
                pos=get_alternative(pos,a[level]);

            if(pos->next_==NULL)
                pos->next_=new TypeNode(NULL);
            pos=pos->next_;
            ++level;
        }

        if(pos->func_.type_ != NULL)
        {
            std::string caller="addtotrie +"+name_.toString()+"+";
            SLIInterpreter *sli=f.type_->get_interpreter();
            sli->message(sli3::M_ERROR,caller.c_str(), "Ambiguous parameter list.");
            sli->message(sli3::M_ERROR,caller.c_str(), 
                         "A command with same or overlapping parameter list is already registered in the trie.");
            throw ArgumentType(level);
        }
        
        pos->func_=f;
    }
    
/*_____ end InsertFunction() _____________________________________*/

    void TypeNode::info(std::ostream &out) const
    {
	std::deque<TypeNode const * > tl;
	info(out, tl);
    }

}
