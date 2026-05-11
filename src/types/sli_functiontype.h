#ifndef SLI_FUNCTIONTYPE_H
#define SLI_FUNCTIONTYPE_H
#include "sli_type.h"

namespace sli3
{
  class FunctionType: public SLIType
  {
  public:
  FunctionType(SLIInterpreter *sli, char const name[], sli_typeid type)
    :SLIType(sli, name, type){}

    bool compare(const Token&t1, const Token&t2) const override;
    std::ostream & print(std::ostream&, const Token &) const override;
    std::ostream & pprint(std::ostream&, const Token &) const override;

    // Serialize by operator name; deserialize re-resolves via the
    // dictionary stack. Requires the same operator to be registered
    // under the same name in the loading interpreter. This holds for
    // every built-in (createcommand binds in systemdict at init).
    void serialize(Token const&, Writer&) const override;
    void deserialize(Reader&, Token&) const override;

    void execute(Token &) override;
  };


}
#endif
