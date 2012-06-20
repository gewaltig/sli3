#ifndef SLI_IOSTREAMTYPE_H
#define SLI_IOSTREAMTYPE_H
#include "sli_type.h"

namespace sli3
{
  class OstreamType: public SLIType
  {
  public:
  OstreamType(SLIInterpreter *sli, char const name[], sli_typeid type)
    :SLIType(sli, name, type)
      {executable_=false;}
    
    bool compare(const Token&t1, const Token&t2) const;
  };

  class IstreamType: public SLIType
  {
  public:
  IstreamType(SLIInterpreter *sli, char const name[], sli_typeid type)
    :SLIType(sli, name, type)
      {executable_=false;}
    
    bool compare(const Token&t1, const Token&t2) const;
  };

  class XIstreamType: public IstreamType
  {
  public:
  XIstreamType(SLIInterpreter *sli, char const name[], sli_typeid type)
    :IstreamType(sli, name, type)
      {executable_=true;}
    
    bool compare(const Token&t1, const Token&t2) const;
    void execute(Token &);
  };



}
#endif
