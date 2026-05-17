cmake_minimum_required(VERSION 3.20)
project(sli3 VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

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
# linenoise has a few unused-parameter / sign-comparison warnings in
# its vendored form. Don't let those fail our (eventual) -Werror.
set_source_files_properties(
  src/util/linenoise/linenoise.c
  PROPERTIES COMPILE_FLAGS "-w"
)
target_compile_options(sli3_core PRIVATE
  -Wall
  -Wextra
  -Wno-unused-parameter
)

# Tests link sli3_core (PRIVATE flags don't propagate). This INTERFACE
# helper carries the same flags, so tests inherit -Wall/-Wextra and
# can't accidentally compile warning-free for the wrong reason.
add_library(sli3_warnings INTERFACE)
target_compile_options(sli3_warnings INTERFACE
  -Wall
  -Wextra
  -Wno-unused-parameter
)

# Optional sanitizer build: -DSLI3_SANITIZE=ON enables ASAN+UBSan.
# Surfaces self-assign UAFs (Stage 1.1), null-payload UB (Stage 1.2),
# and the math-op UB (Stage 5: LONG_MIN, divide-by-zero, ...).
option(SLI3_SANITIZE "Enable AddressSanitizer + UndefinedBehaviorSanitizer" OFF)
if(SLI3_SANITIZE)
  target_compile_options(sli3_core PUBLIC
    -fsanitize=address,undefined
    -fno-omit-frame-pointer
    -g
  )
  target_link_options(sli3_core PUBLIC
    -fsanitize=address,undefined
  )
endif()

# --- Main binary ----------------------------------------------------------
add_executable(sli3 src/interpreter/sli_main.cpp)
target_link_libraries(sli3 PRIVATE sli3_core)

# --- Tests ----------------------------------------------------------------
enable_testing()

# Helper: register a test target consistently (link sli3_core + warnings,
# include tests/, register with CTest, set a hard timeout so a hung
# dispatcher does not hang CI).
function(sli3_add_test name source)
  add_executable(${name} ${source})
  target_link_libraries(${name} PRIVATE sli3_core sli3_warnings)
  target_include_directories(${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)
  add_test(NAME ${name} COMMAND ${name})
  set_tests_properties(${name} PROPERTIES TIMEOUT 60)
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
