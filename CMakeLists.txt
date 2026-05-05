cmake_minimum_required(VERSION 3.20)
project(sli3 CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Core library: everything except main(). Both the sli3 executable and
# test executables link against it.
add_library(sli3_core STATIC
  sli_array.cpp
  sli_token.cpp
  sli_type.cpp
  sli_arraytype.cpp
  sli_stringtype.cpp
  sli_integertype.cpp
  sli_nametype.cpp
  sli_dicttype.cpp
  sli_functiontype.cpp
  sli_tokenstack.cpp
  sli_name.cpp
  sli_dictionary.cpp
  sli_exceptions.cpp
  sli_dictstack.cpp
  sli_charcode.cpp
  sli_scanner.cpp
  sli_parser.cpp
  sli_interpreter.cpp
  sli_numerics.cpp
  sli_builtins.cpp
  sli_control.cpp
  sli_iostream.cpp
  sli_iostreamtype.cpp
  sli_stack.cpp
  sli_type_trie.cpp
  sli_typecheck.cpp
  sli_math.cpp
  sli_serialize.cpp
  sli_array_module.cpp
)
target_include_directories(sli3_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(sli3_core PRIVATE
  -Wall
  -Wextra
  -Wno-unused-parameter
)

add_executable(sli3 sli_main.cpp)
target_link_libraries(sli3 PRIVATE sli3_core)

# --- Tests ---------------------------------------------------------------
enable_testing()

add_executable(test_serialize test_serialize.cpp)
target_link_libraries(test_serialize PRIVATE sli3_core)
add_test(NAME test_serialize COMMAND test_serialize)

add_executable(test_array test_array.cpp)
target_link_libraries(test_array PRIVATE sli3_core)
add_test(NAME test_array COMMAND test_array)

add_executable(test_array_module test_array_module.cpp)
target_link_libraries(test_array_module PRIVATE sli3_core)
add_test(NAME test_array_module COMMAND test_array_module)
