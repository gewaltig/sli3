/*
 *  sliexceptions.cc
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
#include "sli_exceptions.h"
#include "sli_interpreter.h"
#include <sstream>

namespace sli3
{
std::string DivisionByZero::message()
{
    return "You cannot divide by zero.";
}

std::string TypeMismatch::message()
{
  if (!provided_.empty() && !expected_.empty())
    return "Expected datatype: " + expected_ + "\nProvided datatype: " + provided_;
  else if (!expected_.empty())
    return "Expected datatype: " + expected_;
  else
    return "The expected datatype is unknown in the current context.";
}

std::string RangeCheck::message()
{
  if(size_ > 0)
    {
      std::ostringstream out;
      out << "Array with length " << size_ << " expected.";
      return out.str();
    }
  else
    {
      // Empty message.
      // Added due to incorrect use of RangeCheck 
      // in nestmodule.cpp
      return std::string();
    }
}

std::string ArgumentType::message()
{
  std::ostringstream out;

  out << "The type of";
  if(where)
    {
      out <<" the ";
      if(where==1)
	out << "first";
      else if(where==2)
	out << "second";
      else if(where==3)
	out <<"third";
      else
	out << where << "th";
      out << " parameter";
    }
  else
    out << " one or more parameters";
  out <<" did not match the argument(s) of this function.";
    
  return out.str();
}

std::string UndefinedName::message()
{
  return "Key '/"+name_ + "' does not exist in dictionary.";
}

std::string EntryTypeMismatch::message()
{
  return "Expected datatype: " + expected_ + "\nProvided datatype: " 
    + provided_;
}

std::string StackUnderflow::message()
{
  std::ostringstream out;
  if(needed)
    {
      out << "Command needs (at least) "<< needed << " argument(s)";
      if(given)
	out << ", but the stack has only " << given;
      out <<".";
    }
  else
    {
      out << "Command needs more arguments";
      if(given)
	out << "than "<< given;
      out <<".";
    }

  return out.str();
}

std::string IOError::message()
{
  return std::string();
}

std::string SystemSignal::message()
{
  std::ostringstream out;
  out << "The operation was interrupted by the system signal " << signal_ <<".";
  return out.str();
}

std::string UnaccessedDictionaryEntry::message()
{
  return "Unused dictionary items: " + msg_;
}

std::string DynamicModuleManagementError::message()
{
  if(msg_.empty())
    {
      return "Unloading of dynamic modules is not implemented yet.";
    }
  else
    {
      return msg_;
    }
}

std::string NamingConflict::message()
{
  return msg_;

}
}
