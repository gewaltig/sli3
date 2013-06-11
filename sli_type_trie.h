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
    class SLIInterpreter;
    
    typedef std::vector<SLIType *> TypeArray;
    
    class TypeNode
    {
	
    public:
	TypeNode(Name const &n);
	
	~TypeNode();
	
	void  insert(const TypeArray& , Token const &);
	Token& lookup(TokenStack &st);
	
	bool operator == (const TypeNode &) const;
	bool equals(unsigned int , unsigned int) const;
	
	void toTokenArray(TokenArray &) const;
	void info(std::ostream &) const;
	
	
	refcount_t add_reference(void);
	refcount_t remove_reference(void);
	refcount_t references() const;
	
	Name const & get_name() const;

    private:
	TypeNode(SLIType *);
	TypeNode(const TypeNode &);
	
	//    TypeNode operator=(const TypeNode &){}; // disable this operator
	TypeNode * get_alternative(TypeNode *, SLIType *);
	void info( std::ostream &, std::deque<TypeNode const *> &) const;
	
	refcount_t refs_;         //!< number of references to this Node
	Name       name_;        //!< Name of the trie (only for root)

	SLIType    *type_;        //!< expected type at this stack level
	Token       func_;        //!< Object stored (only for leaf).
	
	TypeNode    *alt_;         //!< alternative parameter at this level
	TypeNode    *next_;         //!< next stack level
	
    };
    
    inline 
    TypeNode::TypeNode(Name const &name)
	: refs_(1),
	  name_(name),
	  type_(NULL),
	  func_(),
	  alt_(NULL),
	  next_(NULL){}
    
    inline 
    TypeNode::TypeNode(SLIType* t)
      : refs_(1), name_(), type_(t), func_(), alt_(NULL),next_(NULL) 
    {}
    
    inline 
    TypeNode::TypeNode(const TypeNode &node)
	:refs_(1),
	 name_(node.name_),
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

    inline  
    TypeNode::~TypeNode()
    {
	if (next_ != NULL)
	    next_->remove_reference();
	if (alt_ != NULL)
	    alt_->remove_reference();
    }    
    
    inline 
    refcount_t TypeNode::add_reference(void)
    { 
	return ++refs_; 
    }
    
    inline 
    refcount_t TypeNode::remove_reference(void)
    {
	if(--refs_ > 0)
	    return refs_;
	
	delete this;
	return 0;
    }
    
    inline 
    refcount_t TypeNode::references() const
    {
	return refs_;
    }
    
    inline
    Name const & TypeNode::get_name() const
    {
	return name_;
    }
    
    /**
       Task:      Token on stack 'st' will be compared with the TypeNode.
       Each stack element must have an equivalent type on the 
       current tree level. By reaching a leaf the interpreter 
       function will be returned.           
       
       Author:    Marc Oliver Gewaltig
       
       Date:      18.11.95, 
       rewritten on Apr. 16 1998
       rewritten for sli3, June 2012
       
       Parameter: st = stack
       
    */
    inline
    Token& TypeNode::lookup(TokenStack &st)
    {
      
      const size_t load =st.load();
      size_t level=0;
      TypeNode *pos=this;
	
      while(pos->next_ != NULL)
        {
          assert(pos->type_!=0);
          
          if(level>=load)
            throw StackUnderflow(level+1, load);
          
          const SLIType *find_type=st.pick(level).type_;
          while(pos->type_ != find_type)
            {
              if ((pos->alt_ == NULL) and (pos->type_->get_typeid() != sli3::anytype))
                throw ArgumentType(level);
              pos=pos->alt_;
              assert(pos->type_ != NULL);
            }
          ++level;
          pos=pos->next_;
        }
      // We have reached a leaf, we can return the function.
      //assert(pos->func_.type_ != NULL);
      return pos->func_;
    }


 inline
 bool TypeNode::operator == (const TypeNode &tt) const
 {
     return (this == &tt);
 }

}    
#endif


