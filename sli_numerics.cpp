/*
 *  numerics.cpp
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


#include "config.h"
#include "sli_numerics.h"


#ifndef HAVE_M_E

#ifdef HAVE_CMATH_MAKROS_IGNORED
#define M_E_OK 
#undef __PURE_CNAME
#include <cmath>    
#define __PURE_CNAME
#else
#include <cmath>  
#endif

#else
#define M_E_OK
#include <cmath>  
#endif


#ifndef HAVE_M_PI

#ifdef HAVE_CMATH_MAKROS_IGNORED
#define M_PI_OK
#endif

#else
#define M_PI_OK
#endif


//
//   e
//
#ifdef HAVE_GSL_1_2 

#include <gsl/gsl_math.h>
const double numerics::e  = M_E;
const double numerics::pi = M_PI;

#else

#ifdef M_E_OK
const double numerics::e  = M_E;
#else
const double numerics::e  = 2.71828182845904523536028747135;
#endif

#ifdef M_PI_OK
const double numerics::pi = M_PI;
#else
const double numerics::pi = 3.14159265358979323846264338328;
#endif

#endif


// later also in namespace
long ld_round(double x)
{
    return (long)std::floor(x+0.5);
}

double dround(double x)
{
    return std::floor(x+0.5);
}

double dtruncate(double x)
{
    double ip; 

    std::modf(x,&ip);
    return ip;
}
