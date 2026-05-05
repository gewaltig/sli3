#ifndef SLI_ARRAY_MODULE_H
#define SLI_ARRAY_MODULE_H

namespace sli3
{
class SLIInterpreter;

// Slice 4 rewrite of the legacy sliarray module. Implementations live in
// sli_array_module.cpp; this header exposes only the registration entry
// point. Operator function objects are private to the .cpp.
//
// Operators wired up here:
//   Range, Reverse, Rotate, Flatten, Sort, Transpose, Partition_a_i_i,
//   arrayload, arraystore, GetMin, GetMax, valid_a, finite_q_d,
//   Map / MapIndexed_a / MapThread_a (with internal ::Map / ::MapIndexed
//   / ::MapThread iterator functions that drive the dispatcher).
//
// Deferred (per implementation_spec.md): area, area2, gabor_, gauss2d_,
// cv1d, cv2d (NEST signal-processing helpers), and the int/double-vector
// conversions (require intvectortype / doublevectortype, not yet wired).
void init_sliarray(SLIInterpreter*);

}  // namespace sli3

#endif
