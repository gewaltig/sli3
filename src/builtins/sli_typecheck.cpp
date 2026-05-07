/*
 *  slitypecheck.cc
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


#include "sli_interpreter.h"
#include "sli_trietype.h"
#include "sli_typecheck.h"
#include "sli_control.h"  // TypeinfoFunction lives here
#include "sli_iostream.h"
#include <iostream>
#include <sstream>

namespace sli3
{

/*BeginDocumentation
Name: trie - Create a new type-trie object
Synopsis: /name -> /name typetrie
Description: Create a new typetrie with internal
name /name. This object is not bound to /name in the
current dictionary. This has to be done by an explicit def.
Examples: /square trie 
          [/doubletype] { dup mul } addtotrie def
          
Author: Marc-Oliver Gewaltig
SeeAlso: addtotrie
*/

void TrieFunction::execute(SLIInterpreter *i) const
{
    i->require_stack_load(1);
    i->require_stack_type(0,sli3::literaltype);

    TypeNode *trie= new TypeNode(i->top().data_.name_val);
    Token tmp(i->get_type(sli3::trietype));
    tmp.data_.trie_val=trie;
    i->push(tmp);
    i->EStack().pop();
}


/*BeginDocumentation
Name: addtotrie - Add a function variant to a trie-object
Synopsis: trie [type-list] obj addtotrie -> trie
Parameters: 
trie        - a trie object, obtained by a call to trie.
[type-list] - an array with type-names, corresponding to the
              types of the parameters, expected by the function.
Description:
addtotrie adds a new variant to the type-trie. Note, the type-list must
contain at least one type. (Functions without parameters cannot be
overloaded.

Author: Marc-Oliver Gewaltig
SeeAlso: trie
*/
    
void AddtotrieFunction::execute(SLIInterpreter *i) const
{
    i->require_stack_load(3);
    i->require_stack_type(2,sli3::trietype);
    i->require_stack_type(1,sli3::arraytype);

    TypeNode *trie= i->pick(2).data_.trie_val;
    TokenArray *ad=i->pick(1).data_.array_val;

    if(ad->size() == 0)
    {
	i->message(sli3::M_ERROR, "addtotrie","Can't store function without any parameters.");
	i->message(sli3::M_ERROR, "addtotrie","No change was made to the trie.");
	i->raiseerror(i->ArgumentTypeError);
	return;
    }

    // Stage 4.7: index-based reverse iteration (was a raw-pointer
    // `for (Token *t = ad->end()-1; t >= ad->begin(); --t)` which
    // decrements past begin() at loop exit -- UB on
    // std::vector::data() pointers).
    TypeArray a;
    a.reserve(ad->size());
    for (size_t pos = ad->size(); pos > 0; --pos)
    {
        Token const& t = (*ad)[pos - 1];
	if (not t.is_of_type(sli3::literaltype))
	{
	    // Stage 4.7: std::ostringstream's str() returned a
	    // temporary whose c_str() pointer dangled across the
	    // variadic message() call -- store the string in a
	    // local first.
	    std::ostringstream message;
	    message << "In trie " << trie->get_name() << ". "
	    	    << "Type name expected at position " << (pos - 1) << '.';
	    std::string const msg = message.str();
	    i->message(sli3::M_ERROR, "addtotrie", msg.c_str());
	    i->message(sli3::M_ERROR, "addtotrie","Array must contain typenames as literals.");
	    i->message(sli3::M_ERROR, "addtotrie","No change was made to the trie.");

	    i->raiseerror(i->ArgumentTypeError);
	    return;
	}
	long type_val=i->baselookup(t.data_.name_val);
	a.push_back(i->get_type(type_val));
    }

    trie->insert(a, i->top());
    // (Stage 4.7: dropped unconditional `trie->info(std::cerr)`;
    // it was leftover debug output that fired on every successful
    // addtotrie -- spamming stderr during bootstrap.)
    i->pop(2);
    i->EStack().pop();
}

/* BeginDocumentation

   Name: cva_t - Converts a type trie to an equivalent array

   Synopsis: trie cva_t -> /name array

   Description:
   cva_t maps the tree structure of the trie-object to an array.
   The first return value is the name of the trie object. 
   The second value is an array, representing the trie.

   The layout of a trie node is represented as:
   [/type [next] [alt]] for non-leaf nodes and
   [object]            for leaf nodes.

   /type is  a literal, representing the expected type.
   [next] is an array, representig the next parameter levels.
   [alt] is an array, representig parameter alternatives
         at the current level.

   This definitions recursively define the type-trie.

   Examples:
   /pop load cva_t -> /pop [/anytype [-pop-]]

   Diagnostics:
   This operation is rather low level and does not raise
   errors
   Bugs:

   Author:
   Marc-Oliver Gewaltig

   FirstVersion:
   May 20 1999

   Remarks:
   cva_t is the inverse function to cvt_a. 
   If cva_t is applied to the result of cvt_a, it yields
   the original argument:
   aTrie cva_t cvt_a -> aTrie
   
   SeeAlso: cvt_a, trie, addtotrie, type, cva
*/

void Cva_tFunction::execute(SLIInterpreter *i) const
{
    i->require_stack_load(1);
    i->require_stack_type(0,sli3::trietype);

    TypeNode *trie=i->top().data_.trie_val;
    TokenArray *array = new TokenArray();  
    trie->toTokenArray(*array);
    i->top()=i->new_token<sli3::literaltype>(trie->get_name());
    i->push(i->new_token<sli3::arraytype>(array));
    i->EStack().pop();
}

void TrieInfoFunction::execute(SLIInterpreter *i) const
{
    i->require_stack_load(2);
    i->require_stack_type(1,sli3::ostreamtype);
    i->require_stack_type(0,sli3::trietype);
    
    std::ostream *out = i->pick(1).data_.ostream_val->get();

    TypeNode *trie= i->top().data_.trie_val;
    trie->info(*out);
    i->pop(2);
    i->EStack().pop();
}

/* BeginDocumentation

   Name: cvt_a - Converts an array to the equivalent type trie.

   Synopsis:  /name array cvt_a -> trie

   Description:
   cvt_a tries to construct a type-trie object from a given array.
   The supplied literal is used as name for the trie-object.

   WARNING:
   Be very careful when using this function. If the supplied 
   array is not well formed, the interpreter will abort
   ungracefully! 

   Tries should not be constructed fron scratch using cvt_a.
   Use the operators trie and addtotrie for this purpose.
   Rather, cvt_a is provided to correct minor errors in tries
   with the help of cva_t.
 
   Parameters:

   The supplied array is the root trie node.
   The layout of each trie node must conform to the following
   pattern:
   [/type [next] [alt]] for non-leaf nodes and
   [object]            for leaf nodes.

   /type is  a literal, representing the expected type.
   object is any type of token. It is returned when this leaf of
          the trie is reached. 
   [next] is an array, representig the next parameter levels.
   [alt] is an array, representig parameter alternatives
         at the current level.

   This pattern recursively defines a type-trie. Note, however,
   that violations of this definition are handled ungracefully.

   Examples:
   /pop [/anytype [-pop-]]  cvt_a -> trie 

   Diagnostics:
   This operation is low level and does not raise
   errors. If the array is ill-formed, the interpreter will
   abort! 

   Bugs:
   Errors should be handled gracefully.
   
   Author:
   Marc-Oliver Gewaltig

   FirstVersion:
   May 20 1999

   Remarks:
   cvt_a is the inverse function to cva_t. 
   If cvt_a is applied to the result of cva_t, it yields
   the original argument:
   /name [array] cvt_a cva_t -> /name [array]
   
   SeeAlso: cva_t, trie, addtotrie, type, cst, cva, cv1d, cv2d, cvd, cvi, cvlit, cvn, cvs
*/
void Cvt_aFunction::execute(SLIInterpreter *i) const
{
    i->require_stack_load(2);
    i->require_stack_type(1,sli3::literaltype);
    i->require_stack_type(0,sli3::arraytype);

    Name name(i->pick(1).data_.name_val); 
    TypeNode *trie= new TypeNode(name);
    i->top().clear(); // Clear token before explicit assignment.
    i->top().type_=i->get_type(sli3::trietype);
    i->top().data_.trie_val=trie;
    i->EStack().pop();
}

/*BeginDocumentation
Name: type - Return the type of an object
Synopsis: obj type -> /typename
Examples: 1 type -> /integertype
*/
// `arr cvx_a -> proc` — convert array to executable procedure. Same
// underlying TokenArray; only the SLIType discriminator changes.
// Mirrors NEST 2.20.2 sli/slidata.cc Cvx_aFunction.
class CvxAFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        i->require_stack_type(0, sli3::arraytype);
        i->top().type_ = i->get_type(sli3::proceduretype);
        i->EStack().pop();
    }
};

CvxAFunction cvx_a_fn;

void TypeFunction::execute(SLIInterpreter *i) const
{
    static SLIType *literal_t=i->get_type(sli3::literaltype);

    i->require_stack_load(1);
    Token &top=i->top();
    if (top.type_ == nullptr)
    {
        // A null-typed Token has slipped onto the operand stack from
        // some upstream bug. Surface it as a proper error rather than
        // crashing on a vtable read in tmp.type_->get_typename().
        i->raiseerror(i->ArgumentTypeError);
        return;
    }
    Token tmp;
    tmp.move(top);
    top.type_= literal_t;
    // Stage 4.8: Name -> size_t conversion needs .toIndex(); the
    // implicit Name->int->size_t path was relying on Name's
    // operator int() returning the handle, which is a long that
    // truncates on platforms where size_t is wider. Use the
    // explicit handle accessor.
    top.data_.name_val = tmp.type_->get_typename().toIndex();
    i->EStack().pop();
}


// Convert nametype <-> literaltype (and the inert symbol form). Both
// types share the same Name-handle payload — only the SLIType pointer
// changes. typeinit.sli uses these to implement /cvlit (the trie).
class Cvlit_nFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        i->require_stack_type(0, sli3::nametype);
        i->top().type_ = i->get_type(sli3::literaltype);
        i->EStack().pop();
    }
};

class Cvn_lFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        i->require_stack_type(0, sli3::literaltype);
        i->top().type_ = i->get_type(sli3::nametype);
        i->EStack().pop();
    }
};

// proceduretype <-> literalproceduretype share TokenArray payload.
class Cvlit_pFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        i->require_stack_type(0, sli3::proceduretype);
        i->top().type_ = i->get_type(sli3::litproceduretype);
        i->EStack().pop();
    }
};

class Cvx_lpFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        i->require_stack_type(0, sli3::litproceduretype);
        i->top().type_ = i->get_type(sli3::proceduretype);
        i->EStack().pop();
    }
};

TrieFunction      triefunction;
TrieInfoFunction  trieinfofunction;
AddtotrieFunction addtotriefunction;
Cva_tFunction     cva_tfunction;
Cvt_aFunction     cvt_afunction;
TypeFunction      typefunction;
Cvlit_nFunction   cvlit_nfunction;
Cvn_lFunction     cvn_lfunction;
Cvlit_pFunction   cvlit_pfunction;
Cvx_lpFunction    cvx_lpfunction;

void init_slitypecheck(SLIInterpreter *i)
{
    i->createcommand("trie",      &triefunction);
    i->createcommand("addtotrie", &addtotriefunction);
    i->createcommand("trieinfo_os_t", &trieinfofunction);
    i->createcommand("cva_t", &cva_tfunction);
    i->createcommand("cvt_a", &cvt_afunction);
    i->createcommand("type",      &typefunction);
    i->createcommand("cvx_a",     &cvx_a_fn);
    // typeinfo is registered by init_slicontrol, see sli_control.cpp.
    i->createcommand("cvlit_n",   &cvlit_nfunction);
    i->createcommand("cvn_l",     &cvn_lfunction);
    i->createcommand("cvlit_p",   &cvlit_pfunction);
    i->createcommand("cvx_lp",    &cvx_lpfunction);
}

}
