/*
 *  tokenutils.cc
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

#include "sli_token.h"
#include "sli_tokenutils.h"
#include "sliexceptions.h"
#include <string>
#include <cmath> // for sqrt()

double getValue<double>(const Token&t)
{
  DoubleDatum *id= dynamic_cast<DoubleDatum *>(t.datum());
  if (id == NULL) 
    {// we have to create a Datum object to get the name...
      DoubleDatum const d;
      throw TypeMismatch(d.gettypename().toString(), 
			 t.datum()->gettypename().toString());
    }
  return id->get();
}
template<>
void setValue<double>(const Token&t, double const &value)
{
  DoubleDatum *id= dynamic_cast<DoubleDatum *>(t.datum());
  if (id == NULL) 
    {// we have to create a Datum object to get the name...
      DoubleDatum const d;
      throw TypeMismatch(d.gettypename().toString(), 
			 t.datum()->gettypename().toString());
    }
  (*id) = value;
}
template<>
float getValue<float>(const Token&t)
{
  DoubleDatum *id= dynamic_cast<DoubleDatum *>(t.datum());
  if (id == NULL) 
    {// we have to create a Datum object to get the name...
      DoubleDatum const d;
      throw TypeMismatch(d.gettypename().toString(), 
			 t.datum()->gettypename().toString());
    }
  return (float)id->get();
}

template<>
void setValue<float>(const Token&t, float const &value)
{
  DoubleDatum *id= dynamic_cast<DoubleDatum *>(t.datum());
  if (id == NULL) 
    {// we have to create a Datum object to get the name...
      DoubleDatum const d;
      throw TypeMismatch(d.gettypename().toString(), 
			 t.datum()->gettypename().toString());
    }
  (*id) = (double)value;
}



template<>
bool getValue<bool>(const Token&t)
{
  BoolDatum *bd = dynamic_cast<BoolDatum*>(t.datum());
  if (bd == NULL) 
    {// we have to create a Datum object to get the name...
      BoolDatum const d(false);
      throw TypeMismatch(d.gettypename().toString(), 
			 t.datum()->gettypename().toString());
    }
  return static_cast<bool>(*bd);
  // we should have used i->true_name, bit we don't know the interpreter here. 
}
template<>
void setValue<bool>(const Token&t, bool const &value)
{
  BoolDatum *bd= dynamic_cast<BoolDatum *>(t.datum());
  if (bd == NULL) 
    {// we have to create a Datum object to get the name...
      BoolDatum const d(false);
      throw TypeMismatch(d.gettypename().toString(), 
			 t.datum()->gettypename().toString());
    }
  *bd = BoolDatum( value );
  // we should have used i->true_name, bit we don't know the interpreter here. 
}



// These will handle StringDatum, NameDatum,
// LiteralDatum and SymbolDatum tokens:
template<>
std::string getValue<std::string>(const Token &t)
{
  // If it is a StringDatum, it can be casted to a string:
  std::string *s = dynamic_cast<std::string*>(t.datum());
  if (s != NULL)
    {
      return *s;
    }
  else
    {
      // If it is a NameDatum, LiteralDatum or SymbolDatum,
      // (or even a BoolDatum!) it can be casted to a Name:
      Name *n = dynamic_cast<Name*>(t.datum());
      if (n != NULL)
        {
          return n->toString();
        }
      else
        {
          // The given token can never yield a string!
          // we have to create Datum objects to get the expected names...
          StringDatum  const d1;
          NameDatum    const d2("dummy");
          LiteralDatum const d3("dummy");
          SymbolDatum  const d4("dummy");
          throw TypeMismatch(d1.gettypename().toString() + ", " +
			     d2.gettypename().toString() + ", " + 
			     d3.gettypename().toString() + ", or " +
			     d4.gettypename().toString(),
			     t.datum()->gettypename().toString());
        }
    }
}
template<>
void setValue<std::string>(const Token &t, std::string const &value)
{
  // If it is a StringDatum, it can be casted to a string:
  std::string *s = dynamic_cast<std::string*>(t.datum());
  if (s != NULL)
    {
      *s = value;
    }
  else
    {
      // If it is a BoolDatum, it -could- be set from a string, but
      // this operation shall not be allowed!
      BoolDatum *b = dynamic_cast<BoolDatum*>(t.datum());
      if (b != NULL)
        {
          // we have to create Datum objects to get the expected names...
          StringDatum  const d1;
          NameDatum    const d2("dummy");
          LiteralDatum const d3("dummy");
          SymbolDatum  const d4("dummy");
          throw TypeMismatch(d1.gettypename().toString() + ", " +
			     d2.gettypename().toString() + ", " + 
			     d3.gettypename().toString() + ", or " +
			     d4.gettypename().toString(),
			     t.datum()->gettypename().toString());
        }
      else
        {
          // If it is a NameDatum, LiteralDatum or SymbolDatum,
          // it can be casted to a Name:
          Name *n = dynamic_cast<Name*>(t.datum());
          if (n != NULL)
            {
              *n = Name(value);
            }
          else
            {
              // The given token can never hold a string!
              // we have to create Datum objects to get the expected names...
              StringDatum  const d1;
              NameDatum    const d2("dummy");
              LiteralDatum const d3("dummy");
              SymbolDatum  const d4("dummy");
              throw TypeMismatch(d1.gettypename().toString() + ", " +
				 d2.gettypename().toString() + ", " + 
				 d3.gettypename().toString() + ", or " +
				 d4.gettypename().toString(),
				 t.datum()->gettypename().toString());
            }
        }
    }
}




// These will convert homogeneous double arrays to vectors:
template<>
std::vector<double> getValue<std::vector<double> >(const Token &t)
{
  // try DoubleVectorDatum first
  DoubleVectorDatum* dvd = dynamic_cast<DoubleVectorDatum*>(t.datum());
  if ( dvd )
    return **dvd;

  // ok, try ArrayDatum
  ArrayDatum *ad= dynamic_cast<ArrayDatum *>(t.datum());
  if ( ad ) 
  {
    std::vector<double> data;
    ad->toVector(data);
    return data;
  }
  
  // out of options
  throw TypeMismatch(DoubleVectorDatum().gettypename().toString() 
                     + " or " + ArrayDatum().gettypename().toString(),  
                   	 t.datum()->gettypename().toString());
}

template<>
void setValue<std::vector<double> >(const Token&t, std::vector<double> const &value)
{
  ArrayDatum *ad= dynamic_cast<ArrayDatum *>(t.datum());
  if (ad == NULL) 
    {// we have to create a Datum object to get the name...
      ArrayDatum const d;
      throw TypeMismatch(d.gettypename().toString(), 
			 t.datum()->gettypename().toString());
    }
  // ArrayDatum is an AggregateDatum, which means, it is derived from
  // TokenArray. Hence, we can use ad just like a TokenArray:
  if (ad->size() != value.size())
    {// arrays have incompatible size
      throw RangeCheck(value.size());
    }
  for (size_t i=0; i<ad->size(); ++i)
    {
      setValue<double>((*ad)[i], value[i]);
    }
}


// These will convert homogeneous int arrays to vectors:
template<>
std::vector<long> getValue<std::vector<long> >(const Token &t)
{
  // try IntVectorDatum first
  IntVectorDatum* ivd = dynamic_cast<IntVectorDatum*>(t.datum());
  if ( ivd )
    return **ivd;

  // ok, try ArrayDatum
  ArrayDatum *ad= dynamic_cast<ArrayDatum *>(t.datum());
  if ( ad ) 
  {
    std::vector<long> data;
    ad->toVector(data);
    return data;
  }
  
  // out of options
  throw TypeMismatch(IntVectorDatum().gettypename().toString() 
                     + " or " + ArrayDatum().gettypename().toString(),  
                   	 t.datum()->gettypename().toString());
}

template<>
void setValue<std::vector<long> >(const Token&t, std::vector<long> const &value)
{
  ArrayDatum *ad= dynamic_cast<ArrayDatum *>(t.datum());
  if (ad == NULL) 
    {// we have to create a Datum object to get the name...
      ArrayDatum const d;
      throw TypeMismatch(d.gettypename().toString(), 
			 t.datum()->gettypename().toString());
    }
  // ArrayDatum is an AggregateDatum, which means, it is derived from
  // TokenArray. Hence, we can use ad just like a TokenArray:
  if (ad->size() != value.size())
    {// arrays have incompatible size
      throw RangeCheck(value.size());
    }
  for (size_t i=0; i<ad->size(); ++i)
    {
      setValue<long>((*ad)[i], value[i]);
    }
}



// These will convert homogeneous double arrays to valarrays:
template<>
std::valarray<double> getValue<std::valarray<double> >(const Token &t)
{
  // first get data to vector, then copy to valarray
  const std::vector<double> tmp = getValue<std::vector<double> >(t);

  std::valarray<double> data(tmp.size());
  for (size_t i = 0; i < tmp.size(); ++i )
    data[i] = tmp[i];
  return data;
}

template<>
void setValue<std::valarray<double> >(const Token&t, std::valarray<double> const &value)
{
  ArrayDatum *ad= dynamic_cast<ArrayDatum *>(t.datum());
  if (ad == NULL) 
    {// we have to create a Datum object to get the name...
      ArrayDatum const d;
      throw TypeMismatch(d.gettypename().toString(), 
			 t.datum()->gettypename().toString());
    }
  // ArrayDatum is an AggregateDatum, which means, it is derived from
  // TokenArray. Hence, we can use ad just like a TokenArray:
  if (ad->size() != value.size())
    {// arrays have incompatible size
      throw RangeCheck(value.size());
    }
  for (size_t i=0; i<ad->size(); ++i)
    {
      setValue<double>((*ad)[i], value[i]);
    }
}



// These will convert homogeneous int arrays to valarrays:
template<>
std::valarray<long> getValue<std::valarray<long> >(const Token &t)
{
  // first get data to vector, then copy to valarray
  const std::vector<long> tmp = getValue<std::vector<long> >(t);

  std::valarray<long> data(tmp.size());
  for (size_t i = 0; i < tmp.size(); ++i )
    data[i] = tmp[i];
  return data;
}

template<>
void setValue<std::valarray<long> >(const Token&t, std::valarray<long> const &value)
{
  ArrayDatum *ad= dynamic_cast<ArrayDatum *>(t.datum());
  if (ad == NULL) 
    {// we have to create a Datum object to get the name...
      ArrayDatum const d;
      throw TypeMismatch(d.gettypename().toString(), 
			 t.datum()->gettypename().toString());
    }
  // ArrayDatum is an AggregateDatum, which means, it is derived from
  // TokenArray. Hence, we can use ad just like a TokenArray:
  if (ad->size() != value.size())
    {// arrays have incompatible size
      throw RangeCheck(value.size());
    }
  for (size_t i=0; i<ad->size(); ++i)
    {
      setValue<long>((*ad)[i], value[i]);
    }
}
