/*
 *  slimath.cc
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
    slimath.cc
*/

//#include "config.h"
#include "sli_math.h"
#include "sli_type.h"
#include "sli_interpreter.h"

#include <cmath>

namespace sli3
{

    template< class T1>
    T1 max(T1 const &a, T1 const &b)
    {
	return (a>b) ? a : b;
    }
    
    template< class T1>
    T1 min(T1 const &a, T1 const &b)
    {
	return (a<b) ? a : b;
    }

/*
  All functions in this file do not check their arguments and need to be protected by a type trie.
 */

void IntegerFunction::execute(SLIInterpreter *i) const
{
    static SLIType *int_tid=i->get_type(sli3::integertype);

    // i->require_stack_load(1);
    // i->require_stack_type(0, sli3::doubletype);
    i->EStack().pop();

    i->top().type_=int_tid;
    i->top().data_.long_val=static_cast<long>(i->top().data_.double_val);
}

void DoubleFunction::execute(SLIInterpreter *i) const
{
    static SLIType *double_tid=i->get_type(sli3::doubletype);

    // i->require_stack_load(1);
    // i->require_stack_type(0, sli3::integertype);
    i->EStack().pop();

    i->top().type_=double_tid;
    i->top().data_.double_val=static_cast<double>(i->top().data_.long_val);
}

void Add_iiFunction::execute(SLIInterpreter *i) const
{
//    // i->require_stack_load(2);
    i->EStack().pop();

    i->pick(1).data_.long_val+= i->pick(0).data_.long_val;
    i->pop();
}

void Add_ddFunction::execute(SLIInterpreter *i) const
{
    // i->require_stack_load(2);
    i->EStack().pop();

    i->pick(1).data_.double_val+= i->pick(0).data_.double_val;
    i->pop();
}

void Add_diFunction::execute(SLIInterpreter *i) const
{
    // i->require_stack_load(2);
    i->EStack().pop();

    i->pick(1).data_.double_val+= i->pick(0).data_.long_val;
    i->pop();
}

void Add_idFunction::execute(SLIInterpreter *i) const
{
    // i->require_stack_load(2);
    i->EStack().pop();

    i->pick(1).data_.double_val = i->pick(1).data_.long_val + i->pick(0).data_.double_val;
    i->pick(1).type_=i->pick(0).type_; // change type id to double
    i->pop();
}

//-----------------------------------------------------
void Sub_iiFunction::execute(SLIInterpreter *i) const
{
    // i->require_stack_load(2);
    i->EStack().pop();

    i->pick(1).data_.long_val-= i->pick(0).data_.long_val;
    i->pop();
}

void Sub_ddFunction::execute(SLIInterpreter *i) const
{
    // i->require_stack_load(2);
    i->EStack().pop();

    i->pick(1).data_.double_val-= i->pick(0).data_.double_val;
    i->pop();
}

void Sub_diFunction::execute(SLIInterpreter *i) const
{
    // i->require_stack_load(2);
    i->EStack().pop();

    i->pick(1).data_.double_val-= i->pick(0).data_.long_val;
    i->pop();
}

void Sub_idFunction::execute(SLIInterpreter *i) const
{
    // i->require_stack_load(2);
    i->EStack().pop();

    i->pick(1).data_.double_val = i->pick(1).data_.long_val - i->pick(0).data_.double_val;
    i->pick(1).type_=i->pick(0).type_; // change type id to double
    i->pop();
}
//-----------------------------------------------------
void Mul_iiFunction::execute(SLIInterpreter *i) const
{
    // i->require_stack_load(2);
    i->EStack().pop();

    i->pick(1).data_.long_val*= i->pick(0).data_.long_val;
    i->pop();
}

void Mul_ddFunction::execute(SLIInterpreter *i) const
{
    // i->require_stack_load(2);
    i->EStack().pop();

    i->pick(1).data_.double_val*= i->pick(0).data_.double_val;
    i->pop();
}

void Mul_diFunction::execute(SLIInterpreter *i) const
{
    // i->require_stack_load(2);
    i->EStack().pop();

    i->pick(1).data_.double_val*= i->pick(0).data_.long_val;
    i->pop();
}

void Mul_idFunction::execute(SLIInterpreter *i) const
{
    // i->require_stack_load(2);
    i->EStack().pop();

    i->pick(1).data_.double_val = i->pick(1).data_.long_val * i->pick(0).data_.double_val;
    i->pick(1).type_=i->pick(0).type_; // change type id to double
    i->pop();
}
//-----------------------------------------------------
void Div_iiFunction::execute(SLIInterpreter *i) const
{
    // i->require_stack_load(2);

    if(i->pick(0).data_.long_val !=0)
    {
	i->pick(1).data_.long_val /= i->pick(0).data_.long_val;
	i->EStack().pop();
	i->pop();
    }
    else
	i->raiseerror(i->DivisionByZeroError);
}

void Div_ddFunction::execute(SLIInterpreter *i) const
{
    // i->require_stack_load(2);

    if(i->pick(0).data_.double_val !=0.0)
    {
	i->pick(1).data_.double_val /= i->pick(0).data_.double_val;
	i->EStack().pop();
	i->pop();
    }
    else
	i->raiseerror(i->DivisionByZeroError);
}

void Div_diFunction::execute(SLIInterpreter *i) const
{
    // i->require_stack_load(2);

    if(i->pick(0).data_.long_val !=0)
    {
	i->pick(1).data_.double_val /= i->pick(0).data_.long_val;
	i->EStack().pop();
	i->pop();
    }
    else
	i->raiseerror(i->DivisionByZeroError);
}

void Div_idFunction::execute(SLIInterpreter *i) const
{
    // i->require_stack_load(2);

    if(i->pick(0).data_.double_val !=0.0)
    {
	i->pick(1).data_.double_val =i->pick(1).data_.long_val / i->pick(0).data_.double_val;
	i->pick(1).type_ =  i->pick(0).type_; 
	i->EStack().pop();
	i->pop();
    }
    else
	i->raiseerror(i->DivisionByZeroError);
}

//-----------------------------------------------------
/* BeginDocumentation
Name: mod - compute the modulo of two integer numbers.
Synopsis: int int mod -> int
Examples: 7 4 mod -> 3
SeeAlso: E, sin, cos, exp, log
*/
void Mod_iiFunction::execute(SLIInterpreter *i) const
{
    // i->require_stack_load(2);

    if(i->pick(0).data_.long_val !=0)
    {
	i->pick(1).data_.long_val %= i->pick(0).data_.long_val;
	i->EStack().pop();
	i->pop();
    }
    else
	i->raiseerror(i->DivisionByZeroError);
}


/* BeginDocumentation
 Name: sin - Calculate the sine of double number.
 Synopsis:  double sin -> double

 Description: Alternatives: Function sin_d (undocumented) 
 -> behaviour and synopsis are the same.

 Examples:  1.0 sin -> 0.841471

 Author: Hehl
 FirstVersion: 8.6.1999
 SeeAlso: cos
*/ 

void Sin_dFunction::execute(SLIInterpreter *i) const
{
    // i->require_stack_load(1);
    
    i->top().data_.double_val= std::sin(i->top().data_.double_val);
    i->EStack().pop();
}

/* BeginDocumentation
 Name: asin - Calculate the arc sine of double number.
 Synopsis:  double asin -> double

 Description: Alternatives: Function asin_d (undocumented) 
 -> behaviour and synopsis are the same.

 Examples:  1.0 asin -> 1.570796

 Author: Diesmann
 FirstVersion: 090225
 SeeAlso: sin, acos
*/ 

void Asin_dFunction::execute(SLIInterpreter *i) const
{
    // i->require_stack_load(1);
    
    i->top().data_.double_val= std::asin(i->top().data_.double_val);
    i->EStack().pop();
}


/* BeginDocumentation
 Name: cos - Calculate the cosine of double number.
 Synopsis:  double cos -> double

 Description: Alternatives: Function cos_d (undocumented) 
 -> behaviour and synopsis are the same.

 Examples: 1.0 cos -> 0.540302

 Author: Hehl
 FirstVersion: 8.6.1999
 SeeAlso: sin
*/ 

void Cos_dFunction::execute(SLIInterpreter *i) const
{
    // i->require_stack_load(1);
    
    i->top().data_.double_val= std::cos(i->top().data_.double_val);
    i->EStack().pop();
}

/* BeginDocumentation
 Name: acos - Calculate the arc cosine of double number.
 Synopsis:  double acos -> double

 Description: Alternatives: Function acos_d (undocumented) 
 -> behaviour and synopsis are the same.

 Examples: 1.0 acos -> 0.0

 Author: Diesmann
 FirstVersion: 090225
 SeeAlso: cos, asin
*/ 

void Acos_dFunction::execute(SLIInterpreter *i) const
{
    // i->require_stack_load(1);
    
    i->top().data_.double_val= std::acos(i->top().data_.double_val);
    i->EStack().pop();
}


/* BeginDocumentation
 Name: exp - Calculate the exponential of double number
 Synopsis:  double exp -> double
 Examples: 1.0 exp -> 2.71828  
 Author: Hehl
 FirstVersion: 10.6.1999
 SeeAlso: E, sin, cos
*/ 


void Exp_dFunction::execute(SLIInterpreter *i) const
{
    // i->require_stack_load(1);
    
    i->top().data_.double_val= std::exp(i->top().data_.double_val);
    i->EStack().pop();
}

/* BeginDocumentation
 Name: log - Calculate decadic logarithm of double number.
 Synopsis:  double exp -> double
 Examples: 10.0 log -> 1.0
 Author: Gewaltig
 FirstVersion: 7.6.2000
 SeeAlso: E, sin, cos, exp
*/ 

void Log_dFunction::execute(SLIInterpreter *i) const
{
    // i->require_stack_load(1);
    double &val=i->top().data_.double_val;
    
    if(val>0.0)
    {
	val= std::log10(val);
	i->EStack().pop();
    }
    else
	i->raiseerror(i->RangeCheckError);
}

/* BeginDocumentation
 Name: ln - Calculate natural logarithm of double number.
 Synopsis:  double ln -> double
 Examples: E ln -> 1.0
 Author: Gewaltig
 FirstVersion: 7.6.2000
 SeeAlso: E, sin, cos, exp, log
*/ 

void Ln_dFunction::execute(SLIInterpreter *i) const
{
    // i->require_stack_load(1);
    double &val=i->top().data_.double_val;
    
    if(val>0.0)
    {
	val= std::log(val);
	i->EStack().pop();
    }
    else
	i->raiseerror(i->RangeCheckError);
}

/*BeginDocumentation
Name: sqr - Compute the square of a number.
Examples: 2.0 sqr -> 4.0
Synopsis: number sqr -> double
SeeAlso: sqrt
*/
void Sqr_dFunction::execute(SLIInterpreter *i) const
{
    // i->require_stack_load(1);
    double &val=i->top().data_.double_val;

    val *=val;
    i->EStack().pop();
}

/*BeginDocumentation
Name: sqrt - compute the square root of a non-negative number
Synopsis: number sqrt -> double
Description: sqrt computes the the square root of a number.
If the value is negative, a RangeCheck error is raised.
Examples: 4 sqrt -> 2.0
SeeAlso: sqr
*/
void Sqrt_dFunction::execute(SLIInterpreter *i) const
{
    // i->require_stack_load(1);
    double &val=i->top().data_.double_val;
    
    if(val>=0.0)
    {
	val= std::sqrt(val);
	i->EStack().pop();
    }
    else
	i->raiseerror(i->RangeCheckError);
}

/*BeginDocumentation
Name: pow - raise a number to a power
Synopsis: x y pow -> number
Description: pow computes x raised to the y-th power (x^y).
Remarks: Raises a RangeCheck error if x is zero and y is negative.
Author: Plesser
FirstVersion: 17.05.2004
SeeAlso: exp, log
*/
void Pow_ddFunction::execute(SLIInterpreter *i) const
{
    // i->require_stack_load(2);
    double &op1=i->pick(1).data_.double_val;
    double &op2=i->top().data_.double_val;
    
    if(not (op1==0.0 and op2<0.0))
    {
	op1= std::pow(op1,op2);
	i->EStack().pop();
	i->pop();
    }
    else
	i->raiseerror(i->RangeCheckError);
}

void Pow_diFunction::execute(SLIInterpreter *i) const
{
    // i->require_stack_load(2);
    double &op1=i->pick(1).data_.double_val;
    long &op2=i->top().data_.long_val;
    
    if(not (op1==0.0 and op2<0))
    {
	op1= std::pow(op1,static_cast<double>(op2));
	i->EStack().pop();
	i->pop();
    }
    else
	i->raiseerror(i->RangeCheckError);
}


/* BeginDocumentation
 Name: modf - Decomposes its argument into fractional and integral part
 Synopsis: double modf -> double double
 Description:
 This is an interface to the C++ function 
 double std::modf(double, double*)
 Examples: 2.5 modf -> 0.5 2
 Author: Diesmann
 FirstVersion: 17.5.2005
 References: Stroustrup 3rd ed p 661 
 SeeAlso: frexp
*/ 
void Modf_dFunction::execute(SLIInterpreter *i) const
{
    // i->require_stack_load(1);
    double &op1=i->top().data_.double_val;
    i->push(0.0);

    op1= std::modf(op1, &(i->top().data_.double_val));

    i->EStack().pop();
}


/* BeginDocumentation
 Name: frexp - Decomposes its argument into an exponent of 2 and a factor
 Synopsis: double frexp -> double integer
 Description:
 This is an interface to the C++ function 
 double std::frexp(double,int*)
 In accordance with the normalized representation of the mantissa 
 in IEEE doubles, the factor is in the interval [0.5,1). 
 Examples: -5 dexp frexp -> 0.5 -4
 Author: Diesmann
 FirstVersion: 17.5.2005
 References: Stroustrup 3rd ed p 661 
 SeeAlso: ldexp, dexp, modf
*/ 
void Frexp_dFunction::execute(SLIInterpreter *i) const
{
    // i->require_stack_load(1);
    double &op1=i->top().data_.double_val;
    i->push(0);

    int val;

    op1= std::frexp(op1, &val);
    i->push(val);

    i->EStack().pop();
}


/* BeginDocumentation
 Name: ldexp - computes the product of integer power of 2 and a factor
 Synopsis: double integer ldexp -> double 
 Description:
 This is an interface to the C++ function 
 double std::ldexp(double,int)
 Examples: 
  1.5 3 ldexp -> 12
  12.0 frexp -> 0.75 4
  12.0 frexp ldexp -> 12.0
  1.0 -5 ldexp frexp -> 0.5 -4
 Author: Diesmann
 FirstVersion: 17.5.2005
 References: Stroustrup 3rd ed p 661 
 SeeAlso: frexp, dexp, modf
*/ 
void Ldexp_diFunction::execute(SLIInterpreter *i) const
{
    // i->require_stack_load(2);
    
    i->pick(1).data_.double_val= std::ldexp( i->pick(1).data_.double_val,i->pick(0).data_.long_val);

    i->pop();
    i->EStack().pop();
}

/* BeginDocumentation
 Name: dexp - computes an integer power of 2 and returns the result as double
 Synopsis: integer dexp -> double 
 Description:
 This is an interface to the C++ expression 
 double std::ldexp(1.0,int)
 Examples: -5 dexp frexp -> 0.5 -4
 Author: Diesmann
 FirstVersion: 17.5.2005
 References: Stroustrup 3rd ed p 661 
 SeeAlso: frexp, ldexp, modf
*/ 
void Dexp_iFunction::execute(SLIInterpreter *i) const
{
    // i->require_stack_load(1);
    static SLIType *double_tid=i->get_type(sli3::doubletype);

    i->top().data_.double_val= std::ldexp(1.0, i->top().data_.long_val );
    i->top().type_=double_tid;
 
    i->EStack().pop();
}


//----------------------------------

/* BeginDocumentation
 Name: abs_i - absolute value of integer
 Synopsis:  integer abs -> integer
           
 Description:
    implemented by C/C++ 
      long   labs(long) and

 Examples: -3 abs_i -> 3
 
 Remarks: If you are not sure, if the value is of type double or integer, use abs. 
If e.g. abs_d gets an integer as argument, NEST will exit throwing an assertion.     
 Author: Diesmann 
 FirstVersion: 27.4.1999
 References: Stroustrup 3rd ed p 661 
 SeeAlso: abs
 */ 
void Abs_iFunction::execute(SLIInterpreter *i) const
{
    // i->require_stack_load(1);
    i->EStack().pop();

    i->top().data_.long_val=std::labs(i->top().data_.long_val);
}

/* BeginDocumentation
 Name: abs_d - absolute value of double
 Synopsis:  double abs -> double
           
 Description:
    implemented by C/C++ 
      double fabs(double)
     
 Examples: -3.456 abs_d -> 3.456
 
 Remarks: If you are not sure, if the value is of type double or integer, use abs. 
 If e.g. abs_d gets an integer as argument, NEST will exit throwing an assertion.    
 
 Author: Diesmann
 FirstVersion: 27.4.1999
 References: Stroustrup 3rd ed p 660 
 SeeAlso: abs
*/ 
void Abs_dFunction::execute(SLIInterpreter *i) const
{ 
    // i->require_stack_load(1);
    i->EStack().pop();

    i->top().data_.double_val=std::fabs(i->top().data_.double_val);
}


/* BeginDocumentation
 Name: neg_i - reverse sign of integer value
 Synopsis:  integer neg -> integer
 Author: Diesmann
 FirstVersion: 29.7.1999
 Remarks:
    implemented by C/C++ 
      - operator 
  This function is called CHS in HP48S and
  related dialects.

 SeeAlso:neg
*/ 
void Neg_iFunction::execute(SLIInterpreter *i) const
{
    // i->require_stack_load(1);
    i->EStack().pop();

    i->top().data_.long_val= -i->top().data_.long_val;
}

/* BeginDocumentation
 Name: neg_d - reverse sign of double value
 Synopsis:   double neg -> double
 Author: Diesmann
 FirstVersion: 29.7.1999
 Remarks:
    implemented by C/C++ 
      - operator 
  This function is called CHS in HP48S and
  related dialects.

 SeeAlso: neg
*/
void Neg_dFunction::execute(SLIInterpreter *i) const
{
    // i->require_stack_load(1);
    i->EStack().pop();

    i->top().data_.double_val= - i->top().data_.double_val;
}

/* BeginDocumentation
   Name: inv - compute 1/x
   Synopsis:   double inv -> double
   Examples: 2.0 inv -> 0.5
   Author: Gewaltig
*/
void Inv_dFunction::execute(SLIInterpreter *i) const
{
    // i->require_stack_load(1);

    double &val=i->top().data_.double_val;
    if(val !=0.0)
    {
	val =1.0/val;
	i->EStack().pop();
    }
    else
	i->raiseerror(i->DivisionByZeroError);
}


/*BeginDocumentation:
Name: eq - Test two objects for equality
Synopsis: any1 any2 eq -> bool

Description: eq returns true if the two arguments are equal and false
otherwise. 

eq can also be applied to container objecs like arrays, procedures, strings,
and dictionaries:
* two arrays or strings are equal, if all their components are equal.
* two dictionaries are equal, if they represent the same object.
SeeAlso: neq, gt, lt, leq, geq
*/

void EqFunction::execute(SLIInterpreter *i) const
{
    static SLIType *bool_tid=i->get_type(sli3::booltype);

    // i->require_stack_load(2);
    i->EStack().pop();
    
    bool result = i->pick(1)== i->pick(0);
    i->pop();
    i->top().data_.bool_val=result;
    i->top().type_=bool_tid;
}

/*BeginDocumentation:
Name: neq - Test two objects for inequality
Synopsis: any1 any2 neq -> bool

Description: neq returns true if the two arguments are not equal and false
otherwise. 

neq can also be applied to container objecs like arrays, procedures, strings,
and dictionaries:
* two arrays or strings are equal, if all their components are equal.
* two dictionaries are equal, if they represent the same object.
SeeAlso: eq, gt, lt, leq, geq
*/
void NeqFunction::execute(SLIInterpreter *i) const
{
    static SLIType *bool_tid=i->get_type(sli3::booltype);

    // i->require_stack_load(2);
    i->EStack().pop();
    
    bool result = not (i->pick(1) == i->pick(0));
    i->pop();
    i->top().data_.bool_val=result;
    i->top().type_=bool_tid;
   }

/*BeginDocumentation:
Name: geq - Test if one object is greater or equal than another object
Synopsis: any1 any2 geq -> bool

Description: geq returns true if any1 >= any2 and false
otherwise. 

geq can only be applied to numbers and strings.
SeeAlso: eq, neq, gt, lt, leq
*/

void Geq_iiFunction::execute(SLIInterpreter *i) const
{
    static SLIType *bool_tid=i->get_type(sli3::booltype);

    // i->require_stack_load(2);
    i->EStack().pop();
    
    bool result = i->pick(1).data_.long_val >= i->pick(0).data_.long_val;
    i->pop();
    i->top().data_.bool_val=result;
    i->top().type_=bool_tid;
}

void Geq_idFunction::execute(SLIInterpreter *i) const
{
    static SLIType *bool_tid=i->get_type(sli3::booltype);

    // i->require_stack_load(2);
    i->EStack().pop();
    
    bool result = i->pick(1).data_.long_val >= i->pick(0).data_.double_val;
    i->pop();
    i->top().data_.bool_val=result;
    i->top().type_=bool_tid;
 
}

void Geq_diFunction::execute(SLIInterpreter *i) const
{
    static SLIType *bool_tid=i->get_type(sli3::booltype);

    // i->require_stack_load(2);
    i->EStack().pop();
    
    bool result = i->pick(1).data_.double_val >= i->pick(0).data_.long_val;
    i->pop();
    i->top().data_.bool_val=result;
    i->top().type_=bool_tid;
}

void Geq_ddFunction::execute(SLIInterpreter *i) const
{
    static SLIType *bool_tid=i->get_type(sli3::booltype);

    // i->require_stack_load(2);
    i->EStack().pop();
    
    bool result = i->pick(1).data_.double_val >= i->pick(0).data_.double_val;
    i->pop();
    i->top().data_.bool_val=result;
    i->top().type_=bool_tid;
}

/*BeginDocumentation:
Name: leq - Test if one object is less or equal than another object
Synopsis: any1 any2 leq -> bool

Description: geq returns true if any1 <= any2 and false
otherwise. 

geq can only be applied to numbers and strings.
SeeAlso: eq, neq, gt, lt, geq
*/

void Leq_iiFunction::execute(SLIInterpreter *i) const
{
    static SLIType *bool_tid=i->get_type(sli3::booltype);

    // i->require_stack_load(2);
    i->EStack().pop();
    
    bool result = i->pick(1).data_.long_val <= i->pick(0).data_.long_val;
    i->pop();
    i->top().data_.bool_val=result;
    i->top().type_=bool_tid;
}

void Leq_idFunction::execute(SLIInterpreter *i) const
{
    static SLIType *bool_tid=i->get_type(sli3::booltype);

    // i->require_stack_load(2);
    i->EStack().pop();
    
    bool result = i->pick(1).data_.long_val <= i->pick(0).data_.double_val;
    i->pop();
    i->top().data_.bool_val=result;
    i->top().type_=bool_tid;
}

void Leq_diFunction::execute(SLIInterpreter *i) const
{
    static SLIType *bool_tid=i->get_type(sli3::booltype);

    // i->require_stack_load(2);
    i->EStack().pop();
    
    bool result = i->pick(1).data_.double_val <= i->pick(0).data_.long_val;
    i->pop();
    i->top().data_.bool_val=result;
    i->top().type_=bool_tid;
}

void Leq_ddFunction::execute(SLIInterpreter *i) const
{
    static SLIType *bool_tid=i->get_type(sli3::booltype);

    // i->require_stack_load(2);
    i->EStack().pop();
    
    bool result = i->pick(1).data_.double_val <= i->pick(0).data_.double_val;
    i->pop();
    i->top().data_.bool_val=result;
    i->top().type_=bool_tid;
}

/*BeginDocumentation
Name: not - logical not operator.
Synopsis: bool not -> bool
          int  not -> int
Description: For booleans, not turns true info false and vice versa.
             For integer arguments, not performs a bit-wise not where 
             1 is replaced by 0 and vice versa.
Examples: 1 2 eq not -> true
               0 not -> -1
SeeAlso: and, or, xor
*/

void Not_bFunction::execute(SLIInterpreter *i) const
{
    // i->require_stack_load(1);
    i->EStack().pop();

    i->top().data_.bool_val = not i->top().data_.bool_val;
}


void Not_iFunction::execute(SLIInterpreter *i) const
{
    // i->require_stack_load(1);
    i->EStack().pop();

    i->top().data_.long_val = ~ i->top().data_.long_val;
}

/*BeginDocumentation
Name: or - logical or operator.
Synopsis: bool1 bool2 or -> bool
          int1  int2  or -> int

Description: For booleans, or returns true, if either,
             the arguments are true and false
	     otherwise.
             For integers, or performs a bitwise or between the
             two arguments.

Examples: true  false or -> true
          true  true  or -> true
          false false or -> false
          2     8     or -> 10
          1     0     or -> 1
SeeAlso: and, or, not
*/
void Or_bbFunction::execute(SLIInterpreter *i) const
{
    // i->require_stack_load(2);
    i->EStack().pop();
    
    i->pick(1).data_.bool_val = i->pick(1).data_.bool_val or i->pick(0).data_.bool_val ;

    i->pop();
    
}

void Or_iiFunction::execute(SLIInterpreter *i) const
{
    // i->require_stack_load(2);
    i->EStack().pop();
    
    i->pick(1).data_.long_val |= i->pick(0).data_.long_val ;

    i->pop();
    
}

/*BeginDocumentation
Name: xor - logical xor operator.
Synopsis: bool1 bool2 xor -> bool

Description: For booleans, xor returns true, if either,
             but not both of the arguments are true and false
	     otherwise.

Examples: true  false xor -> true
          true  true  xor -> false
          false false xor -> false
SeeAlso: and, or, not
*/

void Xor_bbFunction::execute(SLIInterpreter *i) const
{
    // i->require_stack_load(2);
    i->EStack().pop();
    
    i->pick(1).data_.bool_val = i->pick(1).data_.bool_val xor i->pick(0).data_.bool_val ;

    i->pop();
}

void Xor_iiFunction::execute(SLIInterpreter *i) const
{
    // i->require_stack_load(2);
    i->EStack().pop();
    
    i->pick(1).data_.long_val ^= i->pick(0).data_.long_val ;

    i->pop();
}

/*BeginDocumentation
Name: and - logical and operator.
Synopsis: bool1 bool2 and -> bool
          int1  int2  and -> int
Description: For booleans, and returns true if both arguments are true
             For integer arguments, and performs a bit-wise and between 
             the two integers.

Examples: true true and -> true
          10   24   and -> 8
SeeAlso: or, xor, not
*/


void AndFunction::execute(SLIInterpreter *i) const
{
    // i->require_stack_load(2);
    i->EStack().pop();
    
    i->pick(1).data_.bool_val = i->pick(1).data_.bool_val and i->pick(0).data_.bool_val ;

    i->pop();
}

void And_iiFunction::execute(SLIInterpreter *i) const
{
     // i->require_stack_load(2);
    i->EStack().pop();
    
    i->pick(1).data_.long_val &= i->pick(0).data_.long_val ;

    i->pop();
}


//---------------------------------------------------
/*BeginDocumentation:
Name: gt - Test if one object is greater than another object
Synopsis: any1 any2 gt -> bool

Description: gt returns true if any1 > any2 and false
otherwise. 

gt can only be applied to numbers and strings.
SeeAlso: eq, neq, gt, lt, leq
*/
void Gt_idFunction::execute(SLIInterpreter *i) const
{
    static SLIType *bool_tid=i->get_type(sli3::booltype);

    // i->require_stack_load(2);
    i->EStack().pop();
    
    bool result = i->pick(1).data_.long_val > i->pick(0).data_.double_val;
    i->pop();
    i->top().data_.bool_val=result;
    i->top().type_=bool_tid;
}

void Gt_diFunction::execute(SLIInterpreter *i) const
{
// call: double integer gt bool
    static SLIType *bool_tid=i->get_type(sli3::booltype);

    // i->require_stack_load(2);
    i->EStack().pop();
    
    bool result = i->pick(1).data_.double_val > i->pick(0).data_.long_val;
    i->pop();
    i->top().data_.bool_val=result;
    i->top().type_=bool_tid;
}


void Gt_iiFunction::execute(SLIInterpreter *i) const
{
// call: integer integer gt bool
    static SLIType *bool_tid=i->get_type(sli3::booltype);

    // i->require_stack_load(2);
    i->EStack().pop();
    
    bool result = i->pick(1).data_.long_val > i->pick(0).data_.long_val;
    i->pop();
    i->top().data_.bool_val=result;
    i->top().type_=bool_tid;
}

void Gt_ddFunction::execute(SLIInterpreter *i) const
{
// call: double double gt bool
    static SLIType *bool_tid=i->get_type(sli3::booltype);

    // i->require_stack_load(2);
    i->EStack().pop();
    
    bool result = i->pick(1).data_.double_val > i->pick(0).data_.double_val;
    i->pop();
    i->top().data_.bool_val=result;
    i->top().type_=bool_tid;
}

/*BeginDocumentation:
Name: lt - Test if one object is less than another object
Synopsis: any1 any2 lt -> bool

Description: lt returns true if any1 < any2 and false
otherwise. 

lt can only be applied to numbers and strings.
SeeAlso: eq, neq, gt, lt, leq
*/

void Lt_idFunction::execute(SLIInterpreter *i) const
{
    static SLIType *bool_tid=i->get_type(sli3::booltype);

    // i->require_stack_load(2);
    i->EStack().pop();
    
    bool result = i->pick(1).data_.long_val < i->pick(0).data_.double_val;
    i->pop();
    i->top().data_.bool_val=result;
    i->top().type_=bool_tid;
}

void Lt_diFunction::execute(SLIInterpreter *i) const
{
// call: double integer lt bool
    static SLIType *bool_tid=i->get_type(sli3::booltype);

    // i->require_stack_load(2);
    i->EStack().pop();
    
    bool result = i->pick(1).data_.double_val < i->pick(0).data_.long_val;
    i->pop();
    i->top().data_.bool_val=result;
    i->top().type_=bool_tid;
}


void Lt_iiFunction::execute(SLIInterpreter *i) const
{
// call: integer integer lt bool
    static SLIType *bool_tid=i->get_type(sli3::booltype);

    // i->require_stack_load(2);
    i->EStack().pop();
    
    bool result = i->pick(1).data_.long_val < i->pick(0).data_.long_val;
    i->pop();
    i->top().data_.bool_val=result;
    i->top().type_=bool_tid;
}

void Lt_ddFunction::execute(SLIInterpreter *i) const
{
// call: double double gt bool
    static SLIType *bool_tid=i->get_type(sli3::booltype);

    // i->require_stack_load(2);
    i->EStack().pop();
    
    bool result = i->pick(1).data_.double_val < i->pick(0).data_.double_val;
    i->pop();
    i->top().data_.bool_val=result;
    i->top().type_=bool_tid;
}

/* move to string functions 
void Gt_ssFunction::execute(SLIInterpreter *i) const
{
// call: string string gt bool
    assert(i->OStack.load() >1);
    i->EStack().pop();

    StringDatum *op1 = static_cast<StringDatum *>(i->OStack.pick(1).datum());
    StringDatum *op2 = static_cast<StringDatum *>(i->OStack.pick(0).datum());
    assert(op1 !=NULL && op2 != NULL);

    bool result = *op1 > *op2;

    i->OStack.pop(2);    
    i->OStack.push_by_pointer(new BoolDatum(result));
}



void Lt_ssFunction::execute(SLIInterpreter *i) const
{
// call: string string gt bool
    assert(i->OStack.load() >1);
    i->EStack().pop();

    StringDatum *op1 = static_cast<StringDatum *>(i->OStack.pick(1).datum());
    StringDatum *op2 = static_cast<StringDatum *>(i->OStack.pick(0).datum());
    assert(op1 !=NULL && op2 != NULL);

    bool result = *op1 < *op2;

    i->OStack.pop(2);    
    i->OStack.push_by_pointer(new BoolDatum(result));
}

*/


// Documentation can be found in file synod2/lib/sli/mathematica.sli
void UnitStep_dFunction::execute(SLIInterpreter *i) const
{
    // i->require_stack_load(1);

    if(i->top().data_.double_val>=0)
	i->top().data_.double_val=1.0;
    i->EStack().pop();
}

// Documentation can be found in file synod2/lib/sli/mathematica.sli
void UnitStep_iFunction::execute(SLIInterpreter *i) const
{
    // i->require_stack_load(1);

    i->top().data_.long_val= i->top().data_.double_val >=0;
    i->EStack().pop();
}


// Documentation can be found in file synod2/lib/sli/mathematica.sli
void UnitStep_daFunction::execute(SLIInterpreter *i) const
{
    // i->require_stack_load(1);

    TokenArray *a = i->pick(0).data_.array_val;

    double result = 1.0;

    for (size_t j=0; j < a->size(); ++j)
    {
	(*a)[j].require_type(sli3::doubletype);

        if ((*a)[j].data_.double_val < 0.0)
        {
	    result=0.0;
	    break;
        }
    }

    i->top()=i->new_token<sli3::doubletype>(result);
    i->EStack().pop();
}

// Documentation can be found in file synod2/lib/sli/mathematica.sli
void UnitStep_iaFunction::execute(SLIInterpreter *i) const
{
    // i->require_stack_load(1);

    TokenArray *a = i->pick(0).data_.array_val;

    long result = 1;

    for (size_t j=0; j < a->size(); ++j)
    {
	(*a)[j].require_type(sli3::integertype);

        if ((*a)[j].data_.long_val < 0.0)
        {
	    result=0;
	    break;
        }
    }

    i->top()=i->new_token<sli3::integertype>(result);
    i->EStack().pop();
}

// round to the nearest integer
void Round_dFunction::execute(SLIInterpreter *i) const
{
    // i->require_stack_load(1);
    i->top().data_.double_val= std::floor(i->top().data_.double_val+0.5);
    i->EStack().pop();
}

// Documentation can be found in file synod2/lib/sli/mathematica.sli
void Floor_dFunction::execute(SLIInterpreter *i) const
{
    // i->require_stack_load(1);
    i->top().data_.double_val= std::floor(i->top().data_.double_val);
    i->EStack().pop();
}

// Documentation can be found in file synod2/lib/sli/mathematica.sli
void Ceil_dFunction::execute(SLIInterpreter *i) const
{
    // i->require_stack_load(1);
    i->top().data_.double_val= std::ceil(i->top().data_.double_val);
    i->EStack().pop();
}



// Documentation can be found in file synod2/lib/sli/typeinit.sli
void Max_i_iFunction::execute(SLIInterpreter *i) const
{
    // i->require_stack_load(2);

    i->pick(1).data_.long_val = max(i->pick(0).data_.long_val,i->pick(1).data_.long_val);
    i->pop();
    i->EStack().pop();
}

void Max_i_dFunction::execute(SLIInterpreter *i) const
{
    // i->require_stack_load(2);

    i->pick(1).data_.double_val = max(static_cast<double>(i->pick(1).data_.long_val),i->pick(0).data_.double_val);
    i->pick(1).type_=i->top().type_;
    i->pop();
    i->EStack().pop();
}

void Max_d_iFunction::execute(SLIInterpreter *i) const
{
    // i->require_stack_load(2);

    i->pick(1).data_.double_val = max(static_cast<double>(i->pick(0).data_.long_val),i->pick(1).data_.double_val);
    i->pop();
    i->EStack().pop();

}
void Max_d_dFunction::execute(SLIInterpreter *i) const
{
    // i->require_stack_load(2);

    i->pick(1).data_.double_val = max(i->pick(0).data_.double_val,i->pick(1).data_.double_val);
    i->pop();
    i->EStack().pop();
}

// Documentation can be found in file synod2/lib/sli/typeinit.sli
void Min_i_iFunction::execute(SLIInterpreter *i) const
{
    // i->require_stack_load(2);

    i->pick(1).data_.long_val = min(i->pick(0).data_.long_val,i->pick(1).data_.long_val);
    i->pop();
    i->EStack().pop();
}

void Min_i_dFunction::execute(SLIInterpreter *i) const
{
    // i->require_stack_load(2);

    i->pick(1).data_.double_val = min(static_cast<double>(i->pick(1).data_.long_val),i->pick(0).data_.double_val);
    i->pick(1).type_=i->top().type_;
    i->pop();
    i->EStack().pop();
}

void Min_d_iFunction::execute(SLIInterpreter *i) const
{
    // i->require_stack_load(2);

    i->pick(1).data_.double_val = min(static_cast<double>(i->pick(0).data_.long_val),i->pick(1).data_.double_val);
    i->pop();
    i->EStack().pop();

}
void Min_d_dFunction::execute(SLIInterpreter *i) const
{
    // i->require_stack_load(2);

    i->pick(1).data_.double_val = min(i->pick(0).data_.double_val,i->pick(1).data_.double_val);
    i->pop();
    i->EStack().pop();
}




IntegerFunction integerfunction;
DoubleFunction doublefunction;
Add_ddFunction add_ddfunction;
Add_diFunction add_difunction;
Add_idFunction add_idfunction;
Add_iiFunction add_iifunction;
Sub_ddFunction sub_ddfunction;
Sub_diFunction sub_difunction;
Sub_idFunction sub_idfunction;
Sub_iiFunction sub_iifunction;

Mul_ddFunction mul_ddfunction;
Mul_diFunction mul_difunction;
Mul_idFunction mul_idfunction;
Mul_iiFunction mul_iifunction;
Div_ddFunction div_ddfunction;
Div_diFunction div_difunction;
Div_idFunction div_idfunction;
Div_iiFunction div_iifunction;
Sin_dFunction sin_dfunction;
Asin_dFunction asin_dfunction;
Cos_dFunction cos_dfunction;
Acos_dFunction acos_dfunction;
Exp_dFunction exp_dfunction;
Ln_dFunction ln_dfunction;
Log_dFunction log_dfunction;
Sqr_dFunction sqr_dfunction;
Sqrt_dFunction sqrt_dfunction;
Pow_ddFunction pow_ddfunction;
Pow_diFunction pow_difunction;

Modf_dFunction modf_dfunction;
Frexp_dFunction frexp_dfunction;

Ldexp_diFunction ldexp_difunction;
Dexp_iFunction   dexp_ifunction;

Mod_iiFunction mod_iifunction;

Abs_iFunction abs_ifunction;
Abs_dFunction abs_dfunction;

Neg_iFunction neg_ifunction;
Neg_dFunction neg_dfunction;
Inv_dFunction inv_dfunction;

EqFunction  eqfunction;
Or_bbFunction  or_bbfunction;
Or_iiFunction  or_iifunction;
Xor_bbFunction  xor_bbfunction;
Xor_iiFunction  xor_iifunction;
AndFunction  andfunction;
And_iiFunction  and_iifunction;

Geq_iiFunction geq_iifunction;
Geq_idFunction geq_idfunction;
Geq_diFunction geq_difunction;
Geq_ddFunction geq_ddfunction;

Leq_iiFunction leq_iifunction;
Leq_idFunction leq_idfunction;
Leq_diFunction leq_difunction;
Leq_ddFunction leq_ddfunction;


NeqFunction neqfunction;
Not_bFunction not_bfunction;
Not_iFunction not_ifunction;

Gt_iiFunction gt_iifunction;
Gt_ddFunction gt_ddfunction;
Gt_idFunction gt_idfunction;
Gt_diFunction gt_difunction;
//Gt_ssFunction gt_ssfunction;

Lt_iiFunction lt_iifunction;
Lt_ddFunction lt_ddfunction;
Lt_idFunction lt_idfunction;
Lt_diFunction lt_difunction;
//Lt_ssFunction lt_ssfunction;

UnitStep_iFunction unitstep_ifunction;
UnitStep_dFunction unitstep_dfunction;
UnitStep_iaFunction unitstep_iafunction;
UnitStep_daFunction unitstep_dafunction;

Round_dFunction round_dfunction;
Floor_dFunction floor_dfunction;
Ceil_dFunction ceil_dfunction;

Max_d_dFunction max_d_dfunction;
Max_d_iFunction max_d_ifunction;
Max_i_dFunction max_i_dfunction;
Max_i_iFunction max_i_ifunction;

Min_d_dFunction min_d_dfunction;
Min_d_iFunction min_d_ifunction;
Min_i_dFunction min_i_dfunction;
Min_i_iFunction min_i_ifunction;


void init_slimath(SLIInterpreter *i)
{
    i->createcommand("int_d", &integerfunction);
    i->createcommand("double_i", &doublefunction);
    i->createcommand("add_dd", &add_ddfunction);
    i->createcommand("add_di", &add_difunction);
    i->createcommand("add_id", &add_idfunction);
    i->createcommand("add_ii", &add_iifunction);
    //
    i->createcommand("sub_dd", &sub_ddfunction);
    i->createcommand("sub_di", &sub_difunction);
    i->createcommand("sub_id", &sub_idfunction);
    i->createcommand("sub_ii", &sub_iifunction);
    //
    i->createcommand("mul_dd", &mul_ddfunction);
    i->createcommand("mul_di", &mul_difunction);
    i->createcommand("mul_id", &mul_idfunction);
    i->createcommand("mul_ii", &mul_iifunction);
    //
    i->createcommand("div_dd", &div_ddfunction);
    i->createcommand("div_di", &div_difunction);
    i->createcommand("div_id", &div_idfunction);
    i->createcommand("div_ii", &div_iifunction);
    i->createcommand("mod",    &mod_iifunction);
    //
    i->createcommand("sin_d", &sin_dfunction);
    i->createcommand("asin_d", &asin_dfunction);
    i->createcommand("cos_d", &cos_dfunction);
    i->createcommand("acos_d", &acos_dfunction);
    i->createcommand("exp_d", &exp_dfunction);
    i->createcommand("log_d", &log_dfunction);
    i->createcommand("ln_d", &ln_dfunction);
    i->createcommand("sqr_d", &sqr_dfunction);
    i->createcommand("sqrt_d", &sqrt_dfunction);
    i->createcommand("pow_dd", &pow_ddfunction);
    i->createcommand("pow_di", &pow_difunction);
    //

    i->createcommand("modf_d", &modf_dfunction);
    i->createcommand("frexp_d", &frexp_dfunction);
    //
    i->createcommand("ldexp_di", &ldexp_difunction);
    i->createcommand("dexp_i",   &dexp_ifunction);
    //
    i->createcommand("abs_i", &abs_ifunction);
    i->createcommand("abs_d", &abs_dfunction);
    //
    i->createcommand("neg_i", &neg_ifunction);
    i->createcommand("neg_d", &neg_dfunction);
    i->createcommand("inv", &inv_dfunction);
    //
    i->createcommand("eq", &eqfunction);
    i->createcommand("and", &andfunction);
    i->createcommand("and_ii", &and_iifunction);
    i->createcommand("or_ii", &or_iifunction);
    i->createcommand("or_bb", &or_bbfunction);
    i->createcommand("xor_bb", &xor_bbfunction);
    i->createcommand("xor_ii", &xor_iifunction);

    i->createcommand("leq_ii", &leq_iifunction);
    i->createcommand("leq_id", &leq_idfunction);
    i->createcommand("leq_di", &leq_difunction);
    i->createcommand("leq_dd", &leq_ddfunction);

    i->createcommand("geq_ii", &geq_iifunction);
    i->createcommand("geq_id", &geq_idfunction);
    i->createcommand("geq_di", &geq_difunction);
    i->createcommand("geq_dd", &geq_ddfunction);

    i->createcommand("neq", &neqfunction);
    i->createcommand("not_b", &not_bfunction);
    i->createcommand("not_i", &not_ifunction); 
    //
    i->createcommand("gt_ii", &gt_iifunction);
    i->createcommand("gt_dd", &gt_ddfunction);
    i->createcommand("gt_id", &gt_idfunction);
    i->createcommand("gt_di", &gt_difunction);
//    i->createcommand("gt_ss", &gt_ssfunction);
    //
    i->createcommand("lt_ii", &lt_iifunction);
    i->createcommand("lt_dd", &lt_ddfunction);
    i->createcommand("lt_id", &lt_idfunction);
    i->createcommand("lt_di", &lt_difunction);
//    i->createcommand("lt_ss", &lt_ssfunction);
    //
    i->createcommand("UnitStep_i", &unitstep_ifunction);
    i->createcommand("UnitStep_d", &unitstep_dfunction);
    i->createcommand("UnitStep_ia", &unitstep_iafunction);
    i->createcommand("UnitStep_da", &unitstep_dafunction);
    //
    i->createcommand("round_d", &round_dfunction);
    i->createcommand("floor_d", &floor_dfunction);
    i->createcommand("ceil_d",  &ceil_dfunction);
    //
    i->createcommand("max_d_d", &max_d_dfunction);
    i->createcommand("max_d_i", &max_d_ifunction);
    i->createcommand("max_i_d", &max_i_dfunction);
    i->createcommand("max_i_i", &max_i_ifunction);
    //
    i->createcommand("min_d_d", &min_d_dfunction);
    i->createcommand("min_d_i", &min_d_ifunction);
    i->createcommand("min_i_d", &min_i_dfunction);
    i->createcommand("min_i_i", &min_i_ifunction);
}

}
