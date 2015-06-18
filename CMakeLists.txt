set(CMAKE_CXX_FLAGS "-g -W -Wall -pedantic -std=c++11")

add_executable(sli3 
  sli_main.cpp 
  sli_array.cpp 
  sli_token.cpp 
  sli_type.cpp 
  sli_arraytype.cpp
	sli_stringtype.cpp
	sli_integertype.cpp
	sli_nametype.cpp
	sli_dicttype.cpp
	sli_functiontype.cpp
	sli_allocator.cpp
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
	sli_iostreamtype.cpp
	sli_stack.cpp
	sli_type_trie.cpp
	sli_typecheck.cpp
	sli_math.cpp)
#	sli_io.cpp
#	sli_fdstream.cpp)
