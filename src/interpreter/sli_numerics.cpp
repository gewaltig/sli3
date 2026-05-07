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


#include "sli_numerics.h"
#include <cmath>

const double numerics::e  = 2.71828182845904523536028747135;
const double numerics::pi = 3.14159265358979323846264338328;


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
