cmake_minimum_required(VERSION 3.20)
project(sli3 VERSION 0.1.0 LANGUAGES CXX)

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

  # trie / serialize / util
  src/trie/sli_type_trie.cpp
  src/serialize/sli_serialize.cpp
  src/util/sli_exceptions.cpp
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
target_compile_options(sli3_core PRIVATE
  -Wall
  -Wextra
  -Wno-unused-parameter
)

# --- Main binary ----------------------------------------------------------
add_executable(sli3 src/interpreter/sli_main.cpp)
target_link_libraries(sli3 PRIVATE sli3_core)

# --- Tests ----------------------------------------------------------------
enable_testing()

# tests/ is a PUBLIC include path on each test target so they can pick up
# test_harness.h without a relative ../tests/ prefix.
add_executable(test_serialize tests/test_serialize.cpp)
target_link_libraries(test_serialize PRIVATE sli3_core)
target_include_directories(test_serialize PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)
add_test(NAME test_serialize COMMAND test_serialize)

add_executable(test_array tests/test_array.cpp)
target_link_libraries(test_array PRIVATE sli3_core)
target_include_directories(test_array PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)
add_test(NAME test_array COMMAND test_array)

add_executable(test_array_module tests/test_array_module.cpp)
target_link_libraries(test_array_module PRIVATE sli3_core)
target_include_directories(test_array_module PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)
add_test(NAME test_array_module COMMAND test_array_module)

add_executable(test_sli_eval tests/test_sli_eval.cpp)
target_link_libraries(test_sli_eval PRIVATE sli3_core)
target_include_directories(test_sli_eval PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)
add_test(NAME test_sli_eval COMMAND test_sli_eval)
