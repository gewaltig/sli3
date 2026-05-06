#ifndef SLI_IO_OPS_H
#define SLI_IO_OPS_H

namespace sli3
{
class SLIInterpreter;

// Minimal stream I/O surface: ifstream, ofstream, file, close, eof,
// getline_is, cvx_f. Just enough for the bootstrap to load typeinit.sli
// and friends. The full modern I/O layer is slice 6.
void init_io_ops(SLIInterpreter*);

}  // namespace sli3

#endif
