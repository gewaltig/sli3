# Unported / deferred sources

Files here are part of the original 2015 sli3 tree but are not built yet.
Decisions logged in `../implementation_spec.md`.

## `sli_processes.{h,cpp}`

POSIX `fork`/`exec`/`waitpid` wrappers. Useful when SLI scripts spawn
subprocesses. Out of scope for the core-language phase per Q4 (2026-05-05).

To re-enable later: move both files back to the project root, add to
`CMakeLists.txt`, register the module from
`SLIInterpreter::init_internal_functions`. The file uses NEST 2.x-era APIs
(`StringDatum`, `IntegerDatum`, `LockPTR`-style `istreamdatum`) that don't
match the current sli3 type system — porting will be substantial.
