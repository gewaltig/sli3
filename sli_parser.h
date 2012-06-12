/*
 *  parser.h
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

#ifndef PARSER_H
#define PARSER_H
/* 
    SLI's parser.
*/

#include <typeinfo>
#include <iostream>
#include "token.h"
#include "tokenstack.h"


class Scanner;

class Parser 
{
    Scanner* s;
    
    Token arraytoken;
    Token proctoken;
    TokenStack ParseStack; 
    
    enum ParseResult   {
                     tokencontinue,
                     scancontinue,
                     tokencompleted,
                     noopenproc,
                     endprocexpected,
                     noopenarray,
                     endarrayexpected,
                     unexpectedeof
    };

    void init(std::istream &);
    
public:    
    Parser(void);
    Parser(std::istream &);
    
    bool operator()(Token&);
    bool readToken(std::istream &is, Token &t)
    {
        s->source(&is);
        return operator()(t);
    }
 
    bool readSymbol(std::istream &is, Token &t)
    {
        s->source(&is);
        return s->operator()(t);
    }

    Scanner const* scan(void) const
    {
        return s;
    }
    
  void clear_context()
  {
    if(s !=NULL)
    {
      s->clear_context();
    }
  }
};

bool operator==(Parser const &, Parser const &);

std::ostream& operator<<(std::ostream&, const Parser&);




#endif













