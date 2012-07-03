/*
 *  typechk.h
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

#ifndef SLI_TYPE_TRIE_H
#define SLI_TYPE_TRIE_H
/* 
    classes for dynamic type checking in SLI
*/

/******************************************************************
Project:   SLI 3.0

Task:      With a TypeTrie it will be possible to perfrom a type 
           check of (SLI) function input parameters. A TypeNode 
           represents the position and the datatype of a single 
           input parameter. The leaves of the tree will contain the 
           interpreter function of correct input parameters.

           A simple add type tree:
           -----------------------

           root
            |
           long -----------------> double -|
            |                        |
           long -> double -|       long -> double -|
           (add)   (add)           (add)   (add)

Baseclass: None

Inherit :  

History:  This is a revised version of the type checking mechanism based on
          tries.
Author:    Marc-Oliver Gewaltig, Thomas Matyak

Date:      18.11.95
*******************************************************************/


#include "sli_function.h"
#include "sli_array.h"
#include "sli_tokenstack.h"
#include "sli_name.h"
#include <iostream>
#include <vector>

namespace sli3
{

  typedef std::vector<unsigned int> TypeArray;

  class TypeNode
  {
  private:
    unsigned int refs_;         // number of references to this Node
    
  public:
    unsigned int type_;        // expected type at this stack level    
    Token        func_;         // points to the operator or an error func.
    
    TypeNode    *alt_;         // points to the next parameter alternative
    TypeNode    *next;         // points to the next stack level for this path
    
    void add_reference(void)
    { ++refs_; }
    
    unsigned int remove_reference(void)
    {
      if(--refs_ > 0)
	return refs_;
      
      delete this;
      return 0;
    }
      
    unsigned int references() const
    {
      return refs_;
    }

  TypeNode(const SLIType *t, Token f=Token())
    : refs_(1), type_(t), func_(f),alt_(NULL),next_(NULL) {}
    
    TypeNode(const TypeNode &node)
      :refs_(1),
      type_(node.type_),
      func_(node.func_),
      alt_(node.alt_),
      next_(node.next_)
	{
	  if(next_ != NULL)
	    next_->add_reference();
	  if(alt_ != NULL)
	    alt_->add_reference();
	}

    ~TypeNode()
      {
	if (next_ != NULL)
	  next_->remove_reference();
	if (alt_ != NULL)
	  alt->remove_reference();
      }
    
    void toTokenArray(TokenArray &) const;
    
    //    TypeTrie operator=(const TypeTrie &){}; // disable this operator
    TypeNode * get_alternative(TypeNode *, SLIType *);
    
    void  insert(const TypeArray& , Token const &);
    
    const Token& lookup(const TokenStack &st) const;
    
    bool operator == (const TypeTrie &) const;
    
    inline bool equals(const Name &, const Name &) const;
    void toTokenArray(TokenArray &) const;
    void info(std::ostream &) const;
  private:
    void info(SLIInterpreter *, std::ostream &, std::vector<TypeNode const *> &) const;

};

// Typename comparison including /anytype which compares
// positively for all other typenames

 inline
   bool TypeNode::equals(unsigned int t1, unsigned int t2) const
   {
     return(t1==t2 || t1==sli3::anytype || t2==sli3::anytype);
   }
 
 inline
   Token& TypeNode::lookup(const TokenStack &st) const
 {
   /*
     Task:      Token on stack 'st' will be compared with the TypeTrie.
     Each stack element must have an equivalent type on the 
     current tree level. By reaching a leaf the interpreter 
     function will be returned.           
     
     Author:    Marc Oliver Gewaltig
     
     Date:      18.11.95, 
     rewritten on Apr. 16 1998
     rewritten for sli3, June 2012

     Parameter: st = stack
     
   */
   const size_t load =st.load();
   size_t level=0;
   
   TypeNode *pos=this;
   
   while(level<load)
     {
       unsigned int find_type=st.pick(level)->type_->get_type_id();
       
       // Step 1: find the type at the current stack level in the
       // list of alternatives. Unfortunately, this search is O(n).
       
       while (! equals(find_type, pos->type))
	 if (pos->alt != NULL)
	   pos = pos->alt;
	 else
	   throw ArgumentType(level);
    
    // Now go to the next argument.
       pos = pos->next;     
    
       // Now check if we have reached a leaf.
       if(pos->next == NULL)
	 return pos->func;

       ++level;
     }

   throw StackUnderflow(level+1, load) ;
 }


 inline
   bool TypeNode::operator == (const TypeNode &tt) const
   {
     return (this == &tt);
   }

#endif

