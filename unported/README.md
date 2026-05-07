# Unported / dead-on-disk sources

Files here are part of the original 2015 sli3 tree but are not built.
Two flavours:

1. **Out-of-scope but useful later** — `sli_processes.{h,cpp}`. Not in
   the build by design; revive when needed.
2. **Reference for what operators exist** — the rest. They use NEST
   2.x's `Datum*` / `LockPTR` / `*Datum` API and don't match sli3's
   modern `Token` / `SLIType` core, so they cannot be compiled
   against the current build. Kept as documentation for which
   operators existed in NEST 2.x; the equivalents are being rewritten
   in fresh files in the project root (`sli_array_module.cpp` for
   what was `sli_array_functions.h` etc.).

Decisions logged in `../implementation_spec.md`.

## File-by-file

### `sli_processes.{h,cpp}`

POSIX `fork`/`exec`/`waitpid` wrappers. Useful when SLI scripts spawn
subprocesses. Out of scope for the core-language phase per Q4
(2026-05-05).

To re-enable later: move both files back to the project root, add to
`CMakeLists.txt`, register the module from
`SLIInterpreter::init_internal_functions`. The file uses NEST 2.x-era
APIs (`StringDatum`, `IntegerDatum`, `LockPTR`-style `istreamdatum`)
that don't match the current sli3 type system — porting will be
substantial.

### `sli_io.{h,cpp}`, `sli_fdstream.{h,cpp}`

Legacy stream I/O. `sli_fdstream` is a custom `std::streambuf` over
POSIX fds — written before C++ had usable equivalents. Replaced
wholesale by `std::fstream` per Q4. The modern minimal I/O surface
lives in `sli_io_ops.{h,cpp}` (Slice 5b); a fuller rewrite is
deferred to Slice 6.

### `sli_module.cpp`

Defines `SLIModule::install` and `SLIModule::commandstring`. The
header (`sli_module.h`) is still in the build — `SLIInterpreter`
declares an `addmodule(SLIModule*)` overload and a
`std::vector<SLIModule*> modules_` field. But no concrete `SLIModule`
subclass exists in the modern build (modules are added via free
`init_*(SLIInterpreter*)` calls instead), so the .cpp's symbol
bodies aren't needed at link time. Also has a typo
(`const sli3::std::string` on line 28) that would prevent
compilation as-is.

### `sli_tokenutils.{h,cpp}`

Template helpers `getValue<T>(Token)` / `setValue<T>(Token, value)`
over the legacy `Datum*` abstraction. Not used in the modern code —
direct field access through `Token::data_.<field>_val` covers the
same ground.

### `sli_array_functions.h`

Header for the legacy ~2k-line `sli_array.cpp` (NEST 2.x array
stdlib). Replaced by `sli_array_module.{h,cpp}` (Slice 4).

### `sli_aggregatetoken.h`

Templated aggregate-token wrapper from the NEST 2.x design. Not
used in the modern build; the 16-byte `Token` plus `SLIType`
polymorphism covers the same job (see CLAUDE.md "Key architectural
choices").

### `sli_config.h`

Autoconf-style `@FOO@` placeholder header. Replaced by `config.h.in`
+ CMake `configure_file()` (Slice 5a).

### `test_dictionary.cpp`, `test_token.cpp`

Original 2015 test programs. They `#include "sli_tokenutils.h"` and
exercise the legacy `Datum*` API, so they don't compile against the
modern core. Replacements built incrementally as each container is
modernised — `test_array.cpp`, `test_array_module.cpp`,
`test_serialize.cpp`, `test_sli_eval.cpp`.
