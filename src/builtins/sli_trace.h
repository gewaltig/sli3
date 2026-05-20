#ifndef SLI_TRACE_H
#define SLI_TRACE_H

#include "sli_function.h"

namespace sli3
{
class SLIInterpreter;

/**
 * /trace — pretty-print errordict /estack one *frame* per line.
 * Decodes the nine continuation marker layouts directly from the
 * snapshot, hides REPL/executive baseline frames, and annotates the
 * failing op (body[pos-1]) on the innermost frame.
 */
class TraceFunction : public SLIFunction
{
public:
    TraceFunction() {}
    void execute(SLIInterpreter*) const override;
};

void init_trace(SLIInterpreter*);

}  // namespace sli3

#endif
