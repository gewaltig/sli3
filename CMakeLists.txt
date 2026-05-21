cmake_minimum_required(VERSION 3.20)

# In-source builds pollute the tree with CMakeFiles/, Makefile, generated
# config.h, and all binaries; gitignore is structured around build/ being
# elsewhere. Checked before project() so compiler detection doesn't write
# CMakeFiles/ into the source tree before the guard fires.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  message(FATAL_ERROR
    "In-source builds are not allowed. "
    "Run: cmake -S . -B build && cmake --build build -j")
endif()

project(sli3 VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Link-time optimization for Release builds. Enabled by default so that
# function placement is decided across all TUs rather than per-file —
# this neutralizes the icache-layout sensitivity that otherwise makes
# trimming any single function body show up as a 10–20% hot-path swing
# in the dispatcher benches. Toggle off with -DSLI3_LTO=OFF if your
# compiler can't keep up.
option(SLI3_LTO "Enable link-time optimization for Release builds" OFF)
if(SLI3_LTO AND CMAKE_BUILD_TYPE STREQUAL "Release")
  include(CheckIPOSupported)
  check_ipo_supported(RESULT _sli3_ipo_ok OUTPUT _sli3_ipo_msg)
  if(_sli3_ipo_ok)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
  else()
    message(WARNING "LTO requested but not supported: ${_sli3_ipo_msg}")
  endif()
endif()

# Uniform artifact layout on Linux and macOS: everything lands under the
# build directory, never beside the sources.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# config.h carries the few compile-time constants the rewritten startup
# reads (version, default sli-init.sli location). Generated into the
# build dir so a regenerate doesn't dirty the source tree.
set(SLI3_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(SLI3_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(SLI3_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(SLI3_VERSION       "${PROJECT_VERSION}")
set(SLI3_DATADIR       "${CMAKE_CURRENT_SOURCE_DIR}/lib")
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/config.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/config.h
  @ONLY
)

# --- Core library ---------------------------------------------------------
# Everything except main(). Both the sli3 executable and test executables
# link against it. Sources are grouped by concern under src/<area>/; every
# subdir is a PUBLIC include path so existing bare-name #includes work.
add_library(sli3_core STATIC
  # types
  src/types/sli_token.cpp
  src/types/sli_type.cpp
  src/types/sli_integertype.cpp
  src/types/sli_stringtype.cpp
  src/types/sli_arraytype.cpp
  src/types/sli_nametype.cpp
  src/types/sli_dicttype.cpp
  src/types/sli_functiontype.cpp
  src/types/sli_iostreamtype.cpp
  src/types/sli_regextype.cpp

  # containers
  src/containers/sli_array.cpp
  src/containers/sli_dictionary.cpp
  src/containers/sli_dictstack.cpp
  src/containers/sli_tokenstack.cpp
  src/containers/sli_iostream.cpp
  src/containers/sli_name.cpp

  # parser
  src/parser/sli_scanner.cpp
  src/parser/sli_parser.cpp
  src/parser/sli_charcode.cpp

  # interpreter
  src/interpreter/sli_interpreter.cpp
  src/interpreter/sli_numerics.cpp
  src/interpreter/sli_signal.cpp

  # builtins / stdlib
  src/builtins/sli_builtins.cpp
  src/builtins/sli_control.cpp
  src/builtins/sli_math.cpp
  src/builtins/sli_stack.cpp
  src/builtins/sli_typecheck.cpp
  src/builtins/sli_array_module.cpp
  src/builtins/sli_container_ops.cpp
  src/builtins/sli_io_ops.cpp
  src/builtins/sli_startup.cpp
  src/builtins/sli_readline.cpp
  src/builtins/sli_state_ops.cpp
  src/builtins/sli_access_ops.cpp
  src/builtins/sli_regex_module.cpp
  src/builtins/sli_trace.cpp

  # trie / serialize / util
  src/trie/sli_type_trie.cpp
  src/serialize/sli_serialize.cpp
  src/util/sli_exceptions.cpp

  # vendored: linenoise (BSD-2, see src/util/linenoise/LICENSE)
  src/util/linenoise/linenoise.c
)
target_include_directories(sli3_core PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/src/types
  ${CMAKE_CURRENT_SOURCE_DIR}/src/containers
  ${CMAKE_CURRENT_SOURCE_DIR}/src/parser
  ${CMAKE_CURRENT_SOURCE_DIR}/src/interpreter
  ${CMAKE_CURRENT_SOURCE_DIR}/src/builtins
  ${CMAKE_CURRENT_SOURCE_DIR}/src/trie
  ${CMAKE_CURRENT_SOURCE_DIR}/src/serialize
  ${CMAKE_CURRENT_SOURCE_DIR}/src/util
  ${CMAKE_CURRENT_BINARY_DIR}
)

# Pointer tagging: the typeid lives in the top byte of every SLIType*.
# Direct deref of those tagged pointers only works on CPUs that
# transparently mask the top byte — ARM64 with Top-Byte-Ignore (TBI),
# enabled by default on macOS Apple Silicon and Linux aarch64. On
# x86_64 the same scheme yields a non-canonical address and segfaults
# unless Intel LAM is active (Linux 6.4+, Sapphire Rapids+). To keep
# the build "uniform" — same source, same configure command, working
# binary on either OS — enable the SLI3_NO_PTR_TAG fallback whenever
# the target is not AArch64. That trades one extra byte-load per
# dispatch (id_ field on SLIType) for portable correctness.
string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" SLI3_CPU)
if(NOT SLI3_CPU MATCHES "^(aarch64|arm64)$")
  target_compile_definitions(sli3_core PUBLIC SLI3_NO_PTR_TAG=1)
  message(STATUS
    "sli3: SLI3_NO_PTR_TAG enabled (CPU=${CMAKE_SYSTEM_PROCESSOR} "
    "lacks TBI; using id_-field fallback for typeid).")
endif()
# linenoise has a few unused-parameter / sign-comparison warnings in
# its vendored form. Don't let those fail our (eventual) -Werror.
# GCC (Linux), Clang (Linux), and AppleClang (macOS) all accept -w.
set_source_files_properties(
  src/util/linenoise/linenoise.c
  PROPERTIES COMPILE_FLAGS
    "$<$<C_COMPILER_ID:GNU,Clang,AppleClang>:-w>"
)

# Warning flags are GCC/Clang/AppleClang dialect — identical across Linux
# and macOS, silently skipped for any future non-GCC-compatible compiler.
# -falign-functions=64 aligns every function to a 64-byte boundary so a
# body-size change in any one TU can't shift later functions to a worse
# icache offset; without it, removing a single `std::cerr` call from a
# cold deserialize path costs ~15 % on the dispatcher benches because
# execute_dispatch_ ends up straddling a cache line. Cost: a few NOPs
# per function (avg ~32 bytes), well under 1 % binary growth.
set(SLI3_WARNING_FLAGS
  $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall>
  $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wextra>
  $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wno-unused-parameter>
  $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-falign-functions=64>
)
target_compile_options(sli3_core PRIVATE ${SLI3_WARNING_FLAGS})

# Tests link sli3_core (PRIVATE flags don't propagate). This INTERFACE
# helper carries the same flags, so tests inherit -Wall/-Wextra and
# can't accidentally compile warning-free for the wrong reason.
add_library(sli3_warnings INTERFACE)
target_compile_options(sli3_warnings INTERFACE ${SLI3_WARNING_FLAGS})

# Optional sanitizer build: -DSLI3_SANITIZE=ON enables ASAN+UBSan.
# Surfaces self-assign UAFs (Stage 1.1), null-payload UB (Stage 1.2),
# and the math-op UB (Stage 5: LONG_MIN, divide-by-zero, ...).
# Sanitizer flags are GCC/Clang/AppleClang dialect; they work the same on
# Linux and macOS but are gated by compiler id so the option is a no-op
# under unsupported toolchains rather than a hard failure.
option(SLI3_SANITIZE "Enable AddressSanitizer + UndefinedBehaviorSanitizer" OFF)
if(SLI3_SANITIZE)
  target_compile_options(sli3_core PUBLIC
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-fsanitize=address,undefined>
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-fno-omit-frame-pointer>
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-g>
  )
  target_link_options(sli3_core PUBLIC
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-fsanitize=address,undefined>
  )
endif()

# --- Main binary ----------------------------------------------------------
add_executable(sli3 src/interpreter/sli_main.cpp)
target_link_libraries(sli3 PRIVATE sli3_core)

# --- Tests ----------------------------------------------------------------
enable_testing()

# Under SLI3_SANITIZE the Linux ASan runtime auto-enables LeakSanitizer,
# which flags every parser/scanner/stack heap object still live at
# process exit (the test fixtures construct SLIInterpreter but don't
# tear down its containers — same on macOS, but Apple's ASan ships
# without LSan, so it never reported them). The mode is documented as
# surfacing UAFs / UB, not exit-time leaks. Disable LSan in the test
# environment so the sanitizer build behaves the same on both OSes.
if(SLI3_SANITIZE)
  set(SLI3_SANITIZE_TEST_ENV "ASAN_OPTIONS=detect_leaks=0")
endif()

# Helper: register a test target consistently (link sli3_core + warnings,
# include tests/, register with CTest, set a hard timeout so a hung
# dispatcher does not hang CI).
function(sli3_add_test name source)
  add_executable(${name} ${source})
  target_link_libraries(${name} PRIVATE sli3_core sli3_warnings)
  target_include_directories(${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)
  add_test(NAME ${name} COMMAND ${name})
  set_tests_properties(${name} PROPERTIES TIMEOUT 60)
  if(SLI3_SANITIZE_TEST_ENV)
    set_tests_properties(${name} PROPERTIES
      ENVIRONMENT "${SLI3_SANITIZE_TEST_ENV}")
  endif()
endfunction()

sli3_add_test(test_serialize         tests/test_serialize.cpp)
sli3_add_test(test_array             tests/test_array.cpp)
sli3_add_test(test_array_module      tests/test_array_module.cpp)
sli3_add_test(test_sli_eval          tests/test_sli_eval.cpp)
sli3_add_test(test_token             tests/test_token.cpp)
sli3_add_test(test_round_trip        tests/test_round_trip.cpp)
sli3_add_test(test_two_interpreters  tests/test_two_interpreters.cpp)
sli3_add_test(test_tokenstack        tests/test_tokenstack.cpp)
sli3_add_test(test_name              tests/test_name.cpp)
sli3_add_test(test_dictstack         tests/test_dictstack.cpp)
sli3_add_test(test_dictionary        tests/test_dictionary.cpp)
sli3_add_test(test_errors            tests/test_errors.cpp)
sli3_add_test(test_errors_dispatch   tests/test_errors_dispatch.cpp)
sli3_add_test(test_math_overflow     tests/test_math_overflow.cpp)
sli3_add_test(test_stack_ops_negatives tests/test_stack_ops_negatives.cpp)
sli3_add_test(test_scanner_edge_cases  tests/test_scanner_edge_cases.cpp)
sli3_add_test(test_features            tests/test_features.cpp)
sli3_add_test(test_state_ops           tests/test_state_ops.cpp)
sli3_add_test(test_bind                tests/test_bind.cpp)
sli3_add_test(test_dispatch_abi        tests/test_dispatch_abi.cpp)
sli3_add_test(test_access              tests/test_access.cpp)
sli3_add_test(test_regex_module        tests/test_regex_module.cpp)
sli3_add_test(test_signal_trap         tests/test_signal_trap.cpp)
