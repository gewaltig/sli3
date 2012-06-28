############## SLI Module Specific Makefile V 0.5 #############
### Basic setup taken from 
### Orman, Andrew and Steve Talbott (1991): Managing Projects with make
###   Sebastopol, Ca: O'Reilly & Associates, Inc.
### adapted to SYNOD/SLI
###  Marc-Oliver Gewaltig March 1996, 
###  gewaltig@neuroinformatik.ruhr-uni-bochum.de
###
### base.mk, this file is used to 
### automatically generate the `makefile' including hidden
### dependencies. This process will only work, if the used c++
### compiler supports the -M option.
### The makefile is called by the main Makefile in the source directory
### and will not work properly if called by itself!
###
### mog. 3/1996

## C++ specific flags
#CC=gcc-mp-4.6
#CXX=g++-mp-4.6
CC=gcc
CXX=g++
GDB=-g
CCFLAGS= -O2 $(GDB) -Wall
CXXFLAGS= -O2 $(GDB) -Wall -pedantic

## Linker flags
LDLIBS= -lstdc++
LDFLAGS= $(GDB) $(LDLIBS) $(LIBREADLINE)

TARGET= sli3
OBJS=	sli_array.o\
	sli_token.o\
	sli_type.o\
	sli_arraytype.o\
	sli_stringtype.o\
	sli_integertype.o\
	sli_nametype.o\
	sli_dicttype.o\
	sli_functiontype.o\
	sli_allocator.o\
	sli_tokenstack.o\
	sli_name.o\
	sli_dictionary.o\
	sli_exceptions.o\
	sli_dictstack.o\
	sli_charcode.o\
	sli_scanner.o\
	sli_parser.o\
	sli_interpreter.o\
	sli_numerics.o\
	sli_builtins.o\
	sli_control.o\
	sli_iostreamtype.o

SOURCES = ${OBJS:.o=.cpp}

all: makefile sli test_token test_dictionary

sli: ${OBJS} sli_main.o
	${CC} -o $@ ${OBJS} sli_main.o ${LDFLAGS}

test_token: ${OBJS} test_token.o
	${CC} -o $@ ${OBJS} $@.o ${LDFLAGS}

test_dictionary: ${OBJS} test_dictionary.o
	${CC} -o $@ ${OBJS} $@.o ${LDFLAGS}

.cc.o:
	${CC} ${CFLAGS} -c $*.cc

makefile: base.mk
	@ echo 'Generating makefile...'
	@ rm -f $@
	@ cp base.mk $@
	@ echo '# Automatically-generated dependency list:' >> $@
	@ ${CC} ${CFLAGS} -M ${SOURCES} >> $@
	@ chmod -w $@

clean:
	@ echo 'Removing all binary files.'
	@ rm -f *.o sli test_token test_dictionary

distclean: clean
	@ rm -f makefile
# Automatically-generated dependency list:
sli_array.o: sli_array.cpp sli_array.h \
 /opt/local/include/gcc46/c++/typeinfo \
 /opt/local/include/gcc46/c++/exception \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++config.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/os_defines.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/cpu_defines.h \
 /opt/local/include/gcc46/c++/cstddef \
 /opt/local/lib/gcc46/gcc/x86_64-apple-darwin11/4.6.3/include/stddef.h \
 sli_token.h /opt/local/include/gcc46/c++/iostream \
 /opt/local/include/gcc46/c++/ostream /opt/local/include/gcc46/c++/ios \
 /opt/local/include/gcc46/c++/iosfwd \
 /opt/local/include/gcc46/c++/bits/stringfwd.h \
 /opt/local/include/gcc46/c++/bits/postypes.h \
 /opt/local/include/gcc46/c++/cwchar /usr/include/wchar.h \
 /usr/include/_types.h /usr/include/sys/_types.h /usr/include/sys/cdefs.h \
 /usr/include/sys/_symbol_aliasing.h \
 /usr/include/sys/_posix_availability.h /usr/include/machine/_types.h \
 /usr/include/i386/_types.h /usr/include/Availability.h \
 /usr/include/AvailabilityInternal.h \
 /opt/local/lib/gcc46/gcc/x86_64-apple-darwin11/4.6.3/include/stdarg.h \
 /usr/include/stdio.h /usr/include/time.h /usr/include/_structs.h \
 /usr/include/sys/_structs.h /usr/include/_wctype.h /usr/include/ctype.h \
 /usr/include/runetype.h /opt/local/include/gcc46/c++/bits/char_traits.h \
 /opt/local/include/gcc46/c++/bits/stl_algobase.h \
 /opt/local/include/gcc46/c++/bits/functexcept.h \
 /opt/local/include/gcc46/c++/bits/exception_defines.h \
 /opt/local/include/gcc46/c++/bits/cpp_type_traits.h \
 /opt/local/include/gcc46/c++/ext/type_traits.h \
 /opt/local/include/gcc46/c++/ext/numeric_traits.h \
 /opt/local/include/gcc46/c++/bits/stl_pair.h \
 /opt/local/include/gcc46/c++/bits/move.h \
 /opt/local/include/gcc46/c++/bits/concept_check.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator_base_types.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator_base_funcs.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator.h \
 /opt/local/include/gcc46/c++/debug/debug.h \
 /opt/local/include/gcc46/c++/bits/localefwd.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++locale.h \
 /opt/local/include/gcc46/c++/clocale /usr/include/locale.h \
 /usr/include/_locale.h /opt/local/include/gcc46/c++/cctype \
 /opt/local/include/gcc46/c++/bits/ios_base.h \
 /opt/local/include/gcc46/c++/ext/atomicity.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/gthr.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/gthr-default.h \
 /usr/include/pthread.h /usr/include/pthread_impl.h /usr/include/sched.h \
 /usr/include/unistd.h /usr/include/sys/unistd.h \
 /usr/include/sys/select.h /usr/include/sys/appleapiopts.h \
 /usr/include/sys/_select.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/atomic_word.h \
 /opt/local/include/gcc46/c++/bits/locale_classes.h \
 /opt/local/include/gcc46/c++/string \
 /opt/local/include/gcc46/c++/bits/allocator.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++allocator.h \
 /opt/local/include/gcc46/c++/ext/new_allocator.h \
 /opt/local/include/gcc46/c++/new \
 /opt/local/include/gcc46/c++/bits/ostream_insert.h \
 /opt/local/include/gcc46/c++/bits/cxxabi_forced.h \
 /opt/local/include/gcc46/c++/bits/stl_function.h \
 /opt/local/include/gcc46/c++/backward/binders.h \
 /opt/local/include/gcc46/c++/bits/range_access.h \
 /opt/local/include/gcc46/c++/bits/basic_string.h \
 /opt/local/include/gcc46/c++/initializer_list \
 /opt/local/include/gcc46/c++/bits/basic_string.tcc \
 /opt/local/include/gcc46/c++/bits/locale_classes.tcc \
 /opt/local/include/gcc46/c++/streambuf \
 /opt/local/include/gcc46/c++/bits/streambuf.tcc \
 /opt/local/include/gcc46/c++/bits/basic_ios.h \
 /opt/local/include/gcc46/c++/bits/locale_facets.h \
 /opt/local/include/gcc46/c++/cwctype /usr/include/wctype.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/ctype_base.h \
 /opt/local/include/gcc46/c++/bits/streambuf_iterator.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/ctype_inline.h \
 /opt/local/include/gcc46/c++/bits/locale_facets.tcc \
 /opt/local/include/gcc46/c++/bits/basic_ios.tcc \
 /opt/local/include/gcc46/c++/bits/ostream.tcc \
 /opt/local/include/gcc46/c++/istream \
 /opt/local/include/gcc46/c++/bits/istream.tcc sli_type.h sli_name.h \
 /opt/local/include/gcc46/c++/cassert /usr/include/assert.h \
 /usr/include/stdlib.h /usr/include/sys/wait.h /usr/include/sys/signal.h \
 /usr/include/machine/signal.h /usr/include/i386/signal.h \
 /usr/include/i386/_structs.h /usr/include/machine/_structs.h \
 /usr/include/mach/i386/_structs.h /usr/include/sys/resource.h \
 /usr/include/machine/endian.h /usr/include/i386/endian.h \
 /usr/include/sys/_endian.h /usr/include/libkern/_OSByteOrder.h \
 /usr/include/libkern/i386/_OSByteOrder.h /usr/include/alloca.h \
 /usr/include/machine/types.h /usr/include/i386/types.h \
 /opt/local/include/gcc46/c++/map \
 /opt/local/include/gcc46/c++/bits/stl_tree.h \
 /opt/local/include/gcc46/c++/bits/stl_map.h \
 /opt/local/include/gcc46/c++/bits/stl_multimap.h \
 /opt/local/include/gcc46/c++/deque \
 /opt/local/include/gcc46/c++/bits/stl_construct.h \
 /opt/local/include/gcc46/c++/bits/stl_uninitialized.h \
 /opt/local/include/gcc46/c++/bits/stl_deque.h \
 /opt/local/include/gcc46/c++/bits/deque.tcc sli_exceptions.h \
 /opt/local/include/gcc46/c++/vector \
 /opt/local/include/gcc46/c++/bits/stl_vector.h \
 /opt/local/include/gcc46/c++/bits/stl_bvector.h \
 /opt/local/include/gcc46/c++/bits/vector.tcc
sli_token.o: sli_token.cpp sli_token.h \
 /opt/local/include/gcc46/c++/iostream \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++config.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/os_defines.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/cpu_defines.h \
 /opt/local/include/gcc46/c++/ostream /opt/local/include/gcc46/c++/ios \
 /opt/local/include/gcc46/c++/iosfwd \
 /opt/local/include/gcc46/c++/bits/stringfwd.h \
 /opt/local/include/gcc46/c++/bits/postypes.h \
 /opt/local/include/gcc46/c++/cwchar /usr/include/wchar.h \
 /usr/include/_types.h /usr/include/sys/_types.h /usr/include/sys/cdefs.h \
 /usr/include/sys/_symbol_aliasing.h \
 /usr/include/sys/_posix_availability.h /usr/include/machine/_types.h \
 /usr/include/i386/_types.h /usr/include/Availability.h \
 /usr/include/AvailabilityInternal.h \
 /opt/local/lib/gcc46/gcc/x86_64-apple-darwin11/4.6.3/include/stdarg.h \
 /usr/include/stdio.h /usr/include/time.h /usr/include/_structs.h \
 /usr/include/sys/_structs.h /usr/include/_wctype.h /usr/include/ctype.h \
 /usr/include/runetype.h /opt/local/include/gcc46/c++/exception \
 /opt/local/include/gcc46/c++/bits/char_traits.h \
 /opt/local/include/gcc46/c++/bits/stl_algobase.h \
 /opt/local/include/gcc46/c++/bits/functexcept.h \
 /opt/local/include/gcc46/c++/bits/exception_defines.h \
 /opt/local/include/gcc46/c++/bits/cpp_type_traits.h \
 /opt/local/include/gcc46/c++/ext/type_traits.h \
 /opt/local/include/gcc46/c++/ext/numeric_traits.h \
 /opt/local/include/gcc46/c++/bits/stl_pair.h \
 /opt/local/include/gcc46/c++/bits/move.h \
 /opt/local/include/gcc46/c++/bits/concept_check.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator_base_types.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator_base_funcs.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator.h \
 /opt/local/include/gcc46/c++/debug/debug.h \
 /opt/local/include/gcc46/c++/bits/localefwd.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++locale.h \
 /opt/local/include/gcc46/c++/clocale /usr/include/locale.h \
 /usr/include/_locale.h /opt/local/include/gcc46/c++/cctype \
 /opt/local/include/gcc46/c++/bits/ios_base.h \
 /opt/local/include/gcc46/c++/ext/atomicity.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/gthr.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/gthr-default.h \
 /usr/include/pthread.h /usr/include/pthread_impl.h /usr/include/sched.h \
 /usr/include/unistd.h /usr/include/sys/unistd.h \
 /usr/include/sys/select.h /usr/include/sys/appleapiopts.h \
 /usr/include/sys/_select.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/atomic_word.h \
 /opt/local/include/gcc46/c++/bits/locale_classes.h \
 /opt/local/include/gcc46/c++/string \
 /opt/local/include/gcc46/c++/bits/allocator.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++allocator.h \
 /opt/local/include/gcc46/c++/ext/new_allocator.h \
 /opt/local/include/gcc46/c++/new \
 /opt/local/include/gcc46/c++/bits/ostream_insert.h \
 /opt/local/include/gcc46/c++/bits/cxxabi_forced.h \
 /opt/local/include/gcc46/c++/bits/stl_function.h \
 /opt/local/include/gcc46/c++/backward/binders.h \
 /opt/local/include/gcc46/c++/bits/range_access.h \
 /opt/local/include/gcc46/c++/bits/basic_string.h \
 /opt/local/include/gcc46/c++/initializer_list \
 /opt/local/include/gcc46/c++/bits/basic_string.tcc \
 /opt/local/include/gcc46/c++/bits/locale_classes.tcc \
 /opt/local/include/gcc46/c++/streambuf \
 /opt/local/include/gcc46/c++/bits/streambuf.tcc \
 /opt/local/include/gcc46/c++/bits/basic_ios.h \
 /opt/local/include/gcc46/c++/bits/locale_facets.h \
 /opt/local/include/gcc46/c++/cwctype /usr/include/wctype.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/ctype_base.h \
 /opt/local/include/gcc46/c++/bits/streambuf_iterator.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/ctype_inline.h \
 /opt/local/include/gcc46/c++/bits/locale_facets.tcc \
 /opt/local/include/gcc46/c++/bits/basic_ios.tcc \
 /opt/local/include/gcc46/c++/bits/ostream.tcc \
 /opt/local/include/gcc46/c++/istream \
 /opt/local/include/gcc46/c++/bits/istream.tcc sli_type.h sli_name.h \
 /opt/local/include/gcc46/c++/cassert /usr/include/assert.h \
 /usr/include/stdlib.h /usr/include/sys/wait.h /usr/include/sys/signal.h \
 /usr/include/machine/signal.h /usr/include/i386/signal.h \
 /usr/include/i386/_structs.h /usr/include/machine/_structs.h \
 /usr/include/mach/i386/_structs.h /usr/include/sys/resource.h \
 /usr/include/machine/endian.h /usr/include/i386/endian.h \
 /usr/include/sys/_endian.h /usr/include/libkern/_OSByteOrder.h \
 /usr/include/libkern/i386/_OSByteOrder.h /usr/include/alloca.h \
 /usr/include/machine/types.h /usr/include/i386/types.h \
 /opt/local/include/gcc46/c++/map \
 /opt/local/include/gcc46/c++/bits/stl_tree.h \
 /opt/local/include/gcc46/c++/bits/stl_map.h \
 /opt/local/include/gcc46/c++/bits/stl_multimap.h \
 /opt/local/include/gcc46/c++/deque \
 /opt/local/include/gcc46/c++/bits/stl_construct.h \
 /opt/local/include/gcc46/c++/bits/stl_uninitialized.h \
 /opt/local/include/gcc46/c++/bits/stl_deque.h \
 /opt/local/include/gcc46/c++/bits/deque.tcc sli_exceptions.h \
 /opt/local/include/gcc46/c++/vector \
 /opt/local/include/gcc46/c++/bits/stl_vector.h \
 /opt/local/include/gcc46/c++/bits/stl_bvector.h \
 /opt/local/include/gcc46/c++/bits/vector.tcc sli_string.h
sli_type.o: sli_type.cpp sli_type.h /opt/local/include/gcc46/c++/string \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++config.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/os_defines.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/cpu_defines.h \
 /opt/local/include/gcc46/c++/bits/stringfwd.h \
 /opt/local/include/gcc46/c++/bits/char_traits.h \
 /opt/local/include/gcc46/c++/bits/stl_algobase.h \
 /opt/local/include/gcc46/c++/bits/functexcept.h \
 /opt/local/include/gcc46/c++/bits/exception_defines.h \
 /opt/local/include/gcc46/c++/bits/cpp_type_traits.h \
 /opt/local/include/gcc46/c++/ext/type_traits.h \
 /opt/local/include/gcc46/c++/ext/numeric_traits.h \
 /opt/local/include/gcc46/c++/bits/stl_pair.h \
 /opt/local/include/gcc46/c++/bits/move.h \
 /opt/local/include/gcc46/c++/bits/concept_check.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator_base_types.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator_base_funcs.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator.h \
 /opt/local/include/gcc46/c++/debug/debug.h \
 /opt/local/include/gcc46/c++/bits/postypes.h \
 /opt/local/include/gcc46/c++/cwchar /usr/include/wchar.h \
 /usr/include/_types.h /usr/include/sys/_types.h /usr/include/sys/cdefs.h \
 /usr/include/sys/_symbol_aliasing.h \
 /usr/include/sys/_posix_availability.h /usr/include/machine/_types.h \
 /usr/include/i386/_types.h /usr/include/Availability.h \
 /usr/include/AvailabilityInternal.h \
 /opt/local/lib/gcc46/gcc/x86_64-apple-darwin11/4.6.3/include/stdarg.h \
 /usr/include/stdio.h /usr/include/time.h /usr/include/_structs.h \
 /usr/include/sys/_structs.h /usr/include/_wctype.h /usr/include/ctype.h \
 /usr/include/runetype.h /opt/local/include/gcc46/c++/bits/allocator.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++allocator.h \
 /opt/local/include/gcc46/c++/ext/new_allocator.h \
 /opt/local/include/gcc46/c++/new /opt/local/include/gcc46/c++/exception \
 /opt/local/include/gcc46/c++/bits/localefwd.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++locale.h \
 /opt/local/include/gcc46/c++/clocale /usr/include/locale.h \
 /usr/include/_locale.h /opt/local/include/gcc46/c++/iosfwd \
 /opt/local/include/gcc46/c++/cctype \
 /opt/local/include/gcc46/c++/bits/ostream_insert.h \
 /opt/local/include/gcc46/c++/bits/cxxabi_forced.h \
 /opt/local/include/gcc46/c++/bits/stl_function.h \
 /opt/local/include/gcc46/c++/backward/binders.h \
 /opt/local/include/gcc46/c++/bits/range_access.h \
 /opt/local/include/gcc46/c++/bits/basic_string.h \
 /opt/local/include/gcc46/c++/ext/atomicity.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/gthr.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/gthr-default.h \
 /usr/include/pthread.h /usr/include/pthread_impl.h /usr/include/sched.h \
 /usr/include/unistd.h /usr/include/sys/unistd.h \
 /usr/include/sys/select.h /usr/include/sys/appleapiopts.h \
 /usr/include/sys/_select.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/atomic_word.h \
 /opt/local/include/gcc46/c++/initializer_list \
 /opt/local/include/gcc46/c++/bits/basic_string.tcc sli_name.h \
 /opt/local/include/gcc46/c++/cassert /usr/include/assert.h \
 /usr/include/stdlib.h /usr/include/sys/wait.h /usr/include/sys/signal.h \
 /usr/include/machine/signal.h /usr/include/i386/signal.h \
 /usr/include/i386/_structs.h /usr/include/machine/_structs.h \
 /usr/include/mach/i386/_structs.h /usr/include/sys/resource.h \
 /usr/include/machine/endian.h /usr/include/i386/endian.h \
 /usr/include/sys/_endian.h /usr/include/libkern/_OSByteOrder.h \
 /usr/include/libkern/i386/_OSByteOrder.h /usr/include/alloca.h \
 /usr/include/machine/types.h /usr/include/i386/types.h \
 /opt/local/include/gcc46/c++/map \
 /opt/local/include/gcc46/c++/bits/stl_tree.h \
 /opt/local/include/gcc46/c++/bits/stl_map.h \
 /opt/local/include/gcc46/c++/bits/stl_multimap.h \
 /opt/local/include/gcc46/c++/deque \
 /opt/local/include/gcc46/c++/bits/stl_construct.h \
 /opt/local/include/gcc46/c++/bits/stl_uninitialized.h \
 /opt/local/include/gcc46/c++/bits/stl_deque.h \
 /opt/local/include/gcc46/c++/bits/deque.tcc \
 /opt/local/include/gcc46/c++/iostream \
 /opt/local/include/gcc46/c++/ostream /opt/local/include/gcc46/c++/ios \
 /opt/local/include/gcc46/c++/bits/ios_base.h \
 /opt/local/include/gcc46/c++/bits/locale_classes.h \
 /opt/local/include/gcc46/c++/bits/locale_classes.tcc \
 /opt/local/include/gcc46/c++/streambuf \
 /opt/local/include/gcc46/c++/bits/streambuf.tcc \
 /opt/local/include/gcc46/c++/bits/basic_ios.h \
 /opt/local/include/gcc46/c++/bits/locale_facets.h \
 /opt/local/include/gcc46/c++/cwctype /usr/include/wctype.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/ctype_base.h \
 /opt/local/include/gcc46/c++/bits/streambuf_iterator.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/ctype_inline.h \
 /opt/local/include/gcc46/c++/bits/locale_facets.tcc \
 /opt/local/include/gcc46/c++/bits/basic_ios.tcc \
 /opt/local/include/gcc46/c++/bits/ostream.tcc \
 /opt/local/include/gcc46/c++/istream \
 /opt/local/include/gcc46/c++/bits/istream.tcc sli_token.h \
 sli_exceptions.h /opt/local/include/gcc46/c++/vector \
 /opt/local/include/gcc46/c++/bits/stl_vector.h \
 /opt/local/include/gcc46/c++/bits/stl_bvector.h \
 /opt/local/include/gcc46/c++/bits/vector.tcc sli_interpreter.h \
 sli_allocator.h /opt/local/include/gcc46/c++/cstdlib sli_arraytype.h \
 sli_array.h /opt/local/include/gcc46/c++/typeinfo \
 /opt/local/include/gcc46/c++/cstddef \
 /opt/local/lib/gcc46/gcc/x86_64-apple-darwin11/4.6.3/include/stddef.h \
 sli_integertype.h SLI_token.h sli_tokenstack.h sli_string.h \
 sli_dictionary.h sli_dictstack.h /opt/local/include/gcc46/c++/list \
 /opt/local/include/gcc46/c++/bits/stl_list.h \
 /opt/local/include/gcc46/c++/bits/list.tcc sli_builtins.h sli_function.h
sli_arraytype.o: sli_arraytype.cpp sli_arraytype.h sli_type.h \
 /opt/local/include/gcc46/c++/string \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++config.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/os_defines.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/cpu_defines.h \
 /opt/local/include/gcc46/c++/bits/stringfwd.h \
 /opt/local/include/gcc46/c++/bits/char_traits.h \
 /opt/local/include/gcc46/c++/bits/stl_algobase.h \
 /opt/local/include/gcc46/c++/bits/functexcept.h \
 /opt/local/include/gcc46/c++/bits/exception_defines.h \
 /opt/local/include/gcc46/c++/bits/cpp_type_traits.h \
 /opt/local/include/gcc46/c++/ext/type_traits.h \
 /opt/local/include/gcc46/c++/ext/numeric_traits.h \
 /opt/local/include/gcc46/c++/bits/stl_pair.h \
 /opt/local/include/gcc46/c++/bits/move.h \
 /opt/local/include/gcc46/c++/bits/concept_check.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator_base_types.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator_base_funcs.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator.h \
 /opt/local/include/gcc46/c++/debug/debug.h \
 /opt/local/include/gcc46/c++/bits/postypes.h \
 /opt/local/include/gcc46/c++/cwchar /usr/include/wchar.h \
 /usr/include/_types.h /usr/include/sys/_types.h /usr/include/sys/cdefs.h \
 /usr/include/sys/_symbol_aliasing.h \
 /usr/include/sys/_posix_availability.h /usr/include/machine/_types.h \
 /usr/include/i386/_types.h /usr/include/Availability.h \
 /usr/include/AvailabilityInternal.h \
 /opt/local/lib/gcc46/gcc/x86_64-apple-darwin11/4.6.3/include/stdarg.h \
 /usr/include/stdio.h /usr/include/time.h /usr/include/_structs.h \
 /usr/include/sys/_structs.h /usr/include/_wctype.h /usr/include/ctype.h \
 /usr/include/runetype.h /opt/local/include/gcc46/c++/bits/allocator.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++allocator.h \
 /opt/local/include/gcc46/c++/ext/new_allocator.h \
 /opt/local/include/gcc46/c++/new /opt/local/include/gcc46/c++/exception \
 /opt/local/include/gcc46/c++/bits/localefwd.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++locale.h \
 /opt/local/include/gcc46/c++/clocale /usr/include/locale.h \
 /usr/include/_locale.h /opt/local/include/gcc46/c++/iosfwd \
 /opt/local/include/gcc46/c++/cctype \
 /opt/local/include/gcc46/c++/bits/ostream_insert.h \
 /opt/local/include/gcc46/c++/bits/cxxabi_forced.h \
 /opt/local/include/gcc46/c++/bits/stl_function.h \
 /opt/local/include/gcc46/c++/backward/binders.h \
 /opt/local/include/gcc46/c++/bits/range_access.h \
 /opt/local/include/gcc46/c++/bits/basic_string.h \
 /opt/local/include/gcc46/c++/ext/atomicity.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/gthr.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/gthr-default.h \
 /usr/include/pthread.h /usr/include/pthread_impl.h /usr/include/sched.h \
 /usr/include/unistd.h /usr/include/sys/unistd.h \
 /usr/include/sys/select.h /usr/include/sys/appleapiopts.h \
 /usr/include/sys/_select.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/atomic_word.h \
 /opt/local/include/gcc46/c++/initializer_list \
 /opt/local/include/gcc46/c++/bits/basic_string.tcc sli_name.h \
 /opt/local/include/gcc46/c++/cassert /usr/include/assert.h \
 /usr/include/stdlib.h /usr/include/sys/wait.h /usr/include/sys/signal.h \
 /usr/include/machine/signal.h /usr/include/i386/signal.h \
 /usr/include/i386/_structs.h /usr/include/machine/_structs.h \
 /usr/include/mach/i386/_structs.h /usr/include/sys/resource.h \
 /usr/include/machine/endian.h /usr/include/i386/endian.h \
 /usr/include/sys/_endian.h /usr/include/libkern/_OSByteOrder.h \
 /usr/include/libkern/i386/_OSByteOrder.h /usr/include/alloca.h \
 /usr/include/machine/types.h /usr/include/i386/types.h \
 /opt/local/include/gcc46/c++/map \
 /opt/local/include/gcc46/c++/bits/stl_tree.h \
 /opt/local/include/gcc46/c++/bits/stl_map.h \
 /opt/local/include/gcc46/c++/bits/stl_multimap.h \
 /opt/local/include/gcc46/c++/deque \
 /opt/local/include/gcc46/c++/bits/stl_construct.h \
 /opt/local/include/gcc46/c++/bits/stl_uninitialized.h \
 /opt/local/include/gcc46/c++/bits/stl_deque.h \
 /opt/local/include/gcc46/c++/bits/deque.tcc \
 /opt/local/include/gcc46/c++/iostream \
 /opt/local/include/gcc46/c++/ostream /opt/local/include/gcc46/c++/ios \
 /opt/local/include/gcc46/c++/bits/ios_base.h \
 /opt/local/include/gcc46/c++/bits/locale_classes.h \
 /opt/local/include/gcc46/c++/bits/locale_classes.tcc \
 /opt/local/include/gcc46/c++/streambuf \
 /opt/local/include/gcc46/c++/bits/streambuf.tcc \
 /opt/local/include/gcc46/c++/bits/basic_ios.h \
 /opt/local/include/gcc46/c++/bits/locale_facets.h \
 /opt/local/include/gcc46/c++/cwctype /usr/include/wctype.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/ctype_base.h \
 /opt/local/include/gcc46/c++/bits/streambuf_iterator.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/ctype_inline.h \
 /opt/local/include/gcc46/c++/bits/locale_facets.tcc \
 /opt/local/include/gcc46/c++/bits/basic_ios.tcc \
 /opt/local/include/gcc46/c++/bits/ostream.tcc \
 /opt/local/include/gcc46/c++/istream \
 /opt/local/include/gcc46/c++/bits/istream.tcc sli_token.h \
 sli_exceptions.h /opt/local/include/gcc46/c++/vector \
 /opt/local/include/gcc46/c++/bits/stl_vector.h \
 /opt/local/include/gcc46/c++/bits/stl_bvector.h \
 /opt/local/include/gcc46/c++/bits/vector.tcc sli_array.h \
 /opt/local/include/gcc46/c++/typeinfo \
 /opt/local/include/gcc46/c++/cstddef \
 /opt/local/lib/gcc46/gcc/x86_64-apple-darwin11/4.6.3/include/stddef.h \
 sli_interpreter.h sli_allocator.h /opt/local/include/gcc46/c++/cstdlib \
 sli_integertype.h SLI_token.h sli_tokenstack.h sli_string.h \
 sli_dictionary.h sli_dictstack.h /opt/local/include/gcc46/c++/list \
 /opt/local/include/gcc46/c++/bits/stl_list.h \
 /opt/local/include/gcc46/c++/bits/list.tcc sli_builtins.h sli_function.h
sli_stringtype.o: sli_stringtype.cpp sli_stringtype.h sli_type.h \
 /opt/local/include/gcc46/c++/string \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++config.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/os_defines.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/cpu_defines.h \
 /opt/local/include/gcc46/c++/bits/stringfwd.h \
 /opt/local/include/gcc46/c++/bits/char_traits.h \
 /opt/local/include/gcc46/c++/bits/stl_algobase.h \
 /opt/local/include/gcc46/c++/bits/functexcept.h \
 /opt/local/include/gcc46/c++/bits/exception_defines.h \
 /opt/local/include/gcc46/c++/bits/cpp_type_traits.h \
 /opt/local/include/gcc46/c++/ext/type_traits.h \
 /opt/local/include/gcc46/c++/ext/numeric_traits.h \
 /opt/local/include/gcc46/c++/bits/stl_pair.h \
 /opt/local/include/gcc46/c++/bits/move.h \
 /opt/local/include/gcc46/c++/bits/concept_check.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator_base_types.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator_base_funcs.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator.h \
 /opt/local/include/gcc46/c++/debug/debug.h \
 /opt/local/include/gcc46/c++/bits/postypes.h \
 /opt/local/include/gcc46/c++/cwchar /usr/include/wchar.h \
 /usr/include/_types.h /usr/include/sys/_types.h /usr/include/sys/cdefs.h \
 /usr/include/sys/_symbol_aliasing.h \
 /usr/include/sys/_posix_availability.h /usr/include/machine/_types.h \
 /usr/include/i386/_types.h /usr/include/Availability.h \
 /usr/include/AvailabilityInternal.h \
 /opt/local/lib/gcc46/gcc/x86_64-apple-darwin11/4.6.3/include/stdarg.h \
 /usr/include/stdio.h /usr/include/time.h /usr/include/_structs.h \
 /usr/include/sys/_structs.h /usr/include/_wctype.h /usr/include/ctype.h \
 /usr/include/runetype.h /opt/local/include/gcc46/c++/bits/allocator.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++allocator.h \
 /opt/local/include/gcc46/c++/ext/new_allocator.h \
 /opt/local/include/gcc46/c++/new /opt/local/include/gcc46/c++/exception \
 /opt/local/include/gcc46/c++/bits/localefwd.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++locale.h \
 /opt/local/include/gcc46/c++/clocale /usr/include/locale.h \
 /usr/include/_locale.h /opt/local/include/gcc46/c++/iosfwd \
 /opt/local/include/gcc46/c++/cctype \
 /opt/local/include/gcc46/c++/bits/ostream_insert.h \
 /opt/local/include/gcc46/c++/bits/cxxabi_forced.h \
 /opt/local/include/gcc46/c++/bits/stl_function.h \
 /opt/local/include/gcc46/c++/backward/binders.h \
 /opt/local/include/gcc46/c++/bits/range_access.h \
 /opt/local/include/gcc46/c++/bits/basic_string.h \
 /opt/local/include/gcc46/c++/ext/atomicity.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/gthr.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/gthr-default.h \
 /usr/include/pthread.h /usr/include/pthread_impl.h /usr/include/sched.h \
 /usr/include/unistd.h /usr/include/sys/unistd.h \
 /usr/include/sys/select.h /usr/include/sys/appleapiopts.h \
 /usr/include/sys/_select.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/atomic_word.h \
 /opt/local/include/gcc46/c++/initializer_list \
 /opt/local/include/gcc46/c++/bits/basic_string.tcc sli_name.h \
 /opt/local/include/gcc46/c++/cassert /usr/include/assert.h \
 /usr/include/stdlib.h /usr/include/sys/wait.h /usr/include/sys/signal.h \
 /usr/include/machine/signal.h /usr/include/i386/signal.h \
 /usr/include/i386/_structs.h /usr/include/machine/_structs.h \
 /usr/include/mach/i386/_structs.h /usr/include/sys/resource.h \
 /usr/include/machine/endian.h /usr/include/i386/endian.h \
 /usr/include/sys/_endian.h /usr/include/libkern/_OSByteOrder.h \
 /usr/include/libkern/i386/_OSByteOrder.h /usr/include/alloca.h \
 /usr/include/machine/types.h /usr/include/i386/types.h \
 /opt/local/include/gcc46/c++/map \
 /opt/local/include/gcc46/c++/bits/stl_tree.h \
 /opt/local/include/gcc46/c++/bits/stl_map.h \
 /opt/local/include/gcc46/c++/bits/stl_multimap.h \
 /opt/local/include/gcc46/c++/deque \
 /opt/local/include/gcc46/c++/bits/stl_construct.h \
 /opt/local/include/gcc46/c++/bits/stl_uninitialized.h \
 /opt/local/include/gcc46/c++/bits/stl_deque.h \
 /opt/local/include/gcc46/c++/bits/deque.tcc \
 /opt/local/include/gcc46/c++/iostream \
 /opt/local/include/gcc46/c++/ostream /opt/local/include/gcc46/c++/ios \
 /opt/local/include/gcc46/c++/bits/ios_base.h \
 /opt/local/include/gcc46/c++/bits/locale_classes.h \
 /opt/local/include/gcc46/c++/bits/locale_classes.tcc \
 /opt/local/include/gcc46/c++/streambuf \
 /opt/local/include/gcc46/c++/bits/streambuf.tcc \
 /opt/local/include/gcc46/c++/bits/basic_ios.h \
 /opt/local/include/gcc46/c++/bits/locale_facets.h \
 /opt/local/include/gcc46/c++/cwctype /usr/include/wctype.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/ctype_base.h \
 /opt/local/include/gcc46/c++/bits/streambuf_iterator.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/ctype_inline.h \
 /opt/local/include/gcc46/c++/bits/locale_facets.tcc \
 /opt/local/include/gcc46/c++/bits/basic_ios.tcc \
 /opt/local/include/gcc46/c++/bits/ostream.tcc \
 /opt/local/include/gcc46/c++/istream \
 /opt/local/include/gcc46/c++/bits/istream.tcc sli_token.h \
 sli_exceptions.h /opt/local/include/gcc46/c++/vector \
 /opt/local/include/gcc46/c++/bits/stl_vector.h \
 /opt/local/include/gcc46/c++/bits/stl_bvector.h \
 /opt/local/include/gcc46/c++/bits/vector.tcc sli_string.h \
 sli_interpreter.h sli_allocator.h /opt/local/include/gcc46/c++/cstdlib \
 sli_arraytype.h sli_array.h /opt/local/include/gcc46/c++/typeinfo \
 /opt/local/include/gcc46/c++/cstddef \
 /opt/local/lib/gcc46/gcc/x86_64-apple-darwin11/4.6.3/include/stddef.h \
 sli_integertype.h SLI_token.h sli_tokenstack.h sli_dictionary.h \
 sli_dictstack.h /opt/local/include/gcc46/c++/list \
 /opt/local/include/gcc46/c++/bits/stl_list.h \
 /opt/local/include/gcc46/c++/bits/list.tcc sli_builtins.h sli_function.h
sli_integertype.o: sli_integertype.cpp sli_integertype.h sli_type.h \
 /opt/local/include/gcc46/c++/string \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++config.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/os_defines.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/cpu_defines.h \
 /opt/local/include/gcc46/c++/bits/stringfwd.h \
 /opt/local/include/gcc46/c++/bits/char_traits.h \
 /opt/local/include/gcc46/c++/bits/stl_algobase.h \
 /opt/local/include/gcc46/c++/bits/functexcept.h \
 /opt/local/include/gcc46/c++/bits/exception_defines.h \
 /opt/local/include/gcc46/c++/bits/cpp_type_traits.h \
 /opt/local/include/gcc46/c++/ext/type_traits.h \
 /opt/local/include/gcc46/c++/ext/numeric_traits.h \
 /opt/local/include/gcc46/c++/bits/stl_pair.h \
 /opt/local/include/gcc46/c++/bits/move.h \
 /opt/local/include/gcc46/c++/bits/concept_check.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator_base_types.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator_base_funcs.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator.h \
 /opt/local/include/gcc46/c++/debug/debug.h \
 /opt/local/include/gcc46/c++/bits/postypes.h \
 /opt/local/include/gcc46/c++/cwchar /usr/include/wchar.h \
 /usr/include/_types.h /usr/include/sys/_types.h /usr/include/sys/cdefs.h \
 /usr/include/sys/_symbol_aliasing.h \
 /usr/include/sys/_posix_availability.h /usr/include/machine/_types.h \
 /usr/include/i386/_types.h /usr/include/Availability.h \
 /usr/include/AvailabilityInternal.h \
 /opt/local/lib/gcc46/gcc/x86_64-apple-darwin11/4.6.3/include/stdarg.h \
 /usr/include/stdio.h /usr/include/time.h /usr/include/_structs.h \
 /usr/include/sys/_structs.h /usr/include/_wctype.h /usr/include/ctype.h \
 /usr/include/runetype.h /opt/local/include/gcc46/c++/bits/allocator.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++allocator.h \
 /opt/local/include/gcc46/c++/ext/new_allocator.h \
 /opt/local/include/gcc46/c++/new /opt/local/include/gcc46/c++/exception \
 /opt/local/include/gcc46/c++/bits/localefwd.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++locale.h \
 /opt/local/include/gcc46/c++/clocale /usr/include/locale.h \
 /usr/include/_locale.h /opt/local/include/gcc46/c++/iosfwd \
 /opt/local/include/gcc46/c++/cctype \
 /opt/local/include/gcc46/c++/bits/ostream_insert.h \
 /opt/local/include/gcc46/c++/bits/cxxabi_forced.h \
 /opt/local/include/gcc46/c++/bits/stl_function.h \
 /opt/local/include/gcc46/c++/backward/binders.h \
 /opt/local/include/gcc46/c++/bits/range_access.h \
 /opt/local/include/gcc46/c++/bits/basic_string.h \
 /opt/local/include/gcc46/c++/ext/atomicity.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/gthr.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/gthr-default.h \
 /usr/include/pthread.h /usr/include/pthread_impl.h /usr/include/sched.h \
 /usr/include/unistd.h /usr/include/sys/unistd.h \
 /usr/include/sys/select.h /usr/include/sys/appleapiopts.h \
 /usr/include/sys/_select.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/atomic_word.h \
 /opt/local/include/gcc46/c++/initializer_list \
 /opt/local/include/gcc46/c++/bits/basic_string.tcc sli_name.h \
 /opt/local/include/gcc46/c++/cassert /usr/include/assert.h \
 /usr/include/stdlib.h /usr/include/sys/wait.h /usr/include/sys/signal.h \
 /usr/include/machine/signal.h /usr/include/i386/signal.h \
 /usr/include/i386/_structs.h /usr/include/machine/_structs.h \
 /usr/include/mach/i386/_structs.h /usr/include/sys/resource.h \
 /usr/include/machine/endian.h /usr/include/i386/endian.h \
 /usr/include/sys/_endian.h /usr/include/libkern/_OSByteOrder.h \
 /usr/include/libkern/i386/_OSByteOrder.h /usr/include/alloca.h \
 /usr/include/machine/types.h /usr/include/i386/types.h \
 /opt/local/include/gcc46/c++/map \
 /opt/local/include/gcc46/c++/bits/stl_tree.h \
 /opt/local/include/gcc46/c++/bits/stl_map.h \
 /opt/local/include/gcc46/c++/bits/stl_multimap.h \
 /opt/local/include/gcc46/c++/deque \
 /opt/local/include/gcc46/c++/bits/stl_construct.h \
 /opt/local/include/gcc46/c++/bits/stl_uninitialized.h \
 /opt/local/include/gcc46/c++/bits/stl_deque.h \
 /opt/local/include/gcc46/c++/bits/deque.tcc \
 /opt/local/include/gcc46/c++/iostream \
 /opt/local/include/gcc46/c++/ostream /opt/local/include/gcc46/c++/ios \
 /opt/local/include/gcc46/c++/bits/ios_base.h \
 /opt/local/include/gcc46/c++/bits/locale_classes.h \
 /opt/local/include/gcc46/c++/bits/locale_classes.tcc \
 /opt/local/include/gcc46/c++/streambuf \
 /opt/local/include/gcc46/c++/bits/streambuf.tcc \
 /opt/local/include/gcc46/c++/bits/basic_ios.h \
 /opt/local/include/gcc46/c++/bits/locale_facets.h \
 /opt/local/include/gcc46/c++/cwctype /usr/include/wctype.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/ctype_base.h \
 /opt/local/include/gcc46/c++/bits/streambuf_iterator.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/ctype_inline.h \
 /opt/local/include/gcc46/c++/bits/locale_facets.tcc \
 /opt/local/include/gcc46/c++/bits/basic_ios.tcc \
 /opt/local/include/gcc46/c++/bits/ostream.tcc \
 /opt/local/include/gcc46/c++/istream \
 /opt/local/include/gcc46/c++/bits/istream.tcc SLI_token.h \
 sli_exceptions.h /opt/local/include/gcc46/c++/vector \
 /opt/local/include/gcc46/c++/bits/stl_vector.h \
 /opt/local/include/gcc46/c++/bits/stl_bvector.h \
 /opt/local/include/gcc46/c++/bits/vector.tcc sli_array.h \
 /opt/local/include/gcc46/c++/typeinfo \
 /opt/local/include/gcc46/c++/cstddef \
 /opt/local/lib/gcc46/gcc/x86_64-apple-darwin11/4.6.3/include/stddef.h \
 sli_token.h
sli_nametype.o: sli_nametype.cpp sli_nametype.h sli_type.h \
 /opt/local/include/gcc46/c++/string \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++config.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/os_defines.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/cpu_defines.h \
 /opt/local/include/gcc46/c++/bits/stringfwd.h \
 /opt/local/include/gcc46/c++/bits/char_traits.h \
 /opt/local/include/gcc46/c++/bits/stl_algobase.h \
 /opt/local/include/gcc46/c++/bits/functexcept.h \
 /opt/local/include/gcc46/c++/bits/exception_defines.h \
 /opt/local/include/gcc46/c++/bits/cpp_type_traits.h \
 /opt/local/include/gcc46/c++/ext/type_traits.h \
 /opt/local/include/gcc46/c++/ext/numeric_traits.h \
 /opt/local/include/gcc46/c++/bits/stl_pair.h \
 /opt/local/include/gcc46/c++/bits/move.h \
 /opt/local/include/gcc46/c++/bits/concept_check.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator_base_types.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator_base_funcs.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator.h \
 /opt/local/include/gcc46/c++/debug/debug.h \
 /opt/local/include/gcc46/c++/bits/postypes.h \
 /opt/local/include/gcc46/c++/cwchar /usr/include/wchar.h \
 /usr/include/_types.h /usr/include/sys/_types.h /usr/include/sys/cdefs.h \
 /usr/include/sys/_symbol_aliasing.h \
 /usr/include/sys/_posix_availability.h /usr/include/machine/_types.h \
 /usr/include/i386/_types.h /usr/include/Availability.h \
 /usr/include/AvailabilityInternal.h \
 /opt/local/lib/gcc46/gcc/x86_64-apple-darwin11/4.6.3/include/stdarg.h \
 /usr/include/stdio.h /usr/include/time.h /usr/include/_structs.h \
 /usr/include/sys/_structs.h /usr/include/_wctype.h /usr/include/ctype.h \
 /usr/include/runetype.h /opt/local/include/gcc46/c++/bits/allocator.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++allocator.h \
 /opt/local/include/gcc46/c++/ext/new_allocator.h \
 /opt/local/include/gcc46/c++/new /opt/local/include/gcc46/c++/exception \
 /opt/local/include/gcc46/c++/bits/localefwd.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++locale.h \
 /opt/local/include/gcc46/c++/clocale /usr/include/locale.h \
 /usr/include/_locale.h /opt/local/include/gcc46/c++/iosfwd \
 /opt/local/include/gcc46/c++/cctype \
 /opt/local/include/gcc46/c++/bits/ostream_insert.h \
 /opt/local/include/gcc46/c++/bits/cxxabi_forced.h \
 /opt/local/include/gcc46/c++/bits/stl_function.h \
 /opt/local/include/gcc46/c++/backward/binders.h \
 /opt/local/include/gcc46/c++/bits/range_access.h \
 /opt/local/include/gcc46/c++/bits/basic_string.h \
 /opt/local/include/gcc46/c++/ext/atomicity.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/gthr.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/gthr-default.h \
 /usr/include/pthread.h /usr/include/pthread_impl.h /usr/include/sched.h \
 /usr/include/unistd.h /usr/include/sys/unistd.h \
 /usr/include/sys/select.h /usr/include/sys/appleapiopts.h \
 /usr/include/sys/_select.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/atomic_word.h \
 /opt/local/include/gcc46/c++/initializer_list \
 /opt/local/include/gcc46/c++/bits/basic_string.tcc sli_name.h \
 /opt/local/include/gcc46/c++/cassert /usr/include/assert.h \
 /usr/include/stdlib.h /usr/include/sys/wait.h /usr/include/sys/signal.h \
 /usr/include/machine/signal.h /usr/include/i386/signal.h \
 /usr/include/i386/_structs.h /usr/include/machine/_structs.h \
 /usr/include/mach/i386/_structs.h /usr/include/sys/resource.h \
 /usr/include/machine/endian.h /usr/include/i386/endian.h \
 /usr/include/sys/_endian.h /usr/include/libkern/_OSByteOrder.h \
 /usr/include/libkern/i386/_OSByteOrder.h /usr/include/alloca.h \
 /usr/include/machine/types.h /usr/include/i386/types.h \
 /opt/local/include/gcc46/c++/map \
 /opt/local/include/gcc46/c++/bits/stl_tree.h \
 /opt/local/include/gcc46/c++/bits/stl_map.h \
 /opt/local/include/gcc46/c++/bits/stl_multimap.h \
 /opt/local/include/gcc46/c++/deque \
 /opt/local/include/gcc46/c++/bits/stl_construct.h \
 /opt/local/include/gcc46/c++/bits/stl_uninitialized.h \
 /opt/local/include/gcc46/c++/bits/stl_deque.h \
 /opt/local/include/gcc46/c++/bits/deque.tcc \
 /opt/local/include/gcc46/c++/iostream \
 /opt/local/include/gcc46/c++/ostream /opt/local/include/gcc46/c++/ios \
 /opt/local/include/gcc46/c++/bits/ios_base.h \
 /opt/local/include/gcc46/c++/bits/locale_classes.h \
 /opt/local/include/gcc46/c++/bits/locale_classes.tcc \
 /opt/local/include/gcc46/c++/streambuf \
 /opt/local/include/gcc46/c++/bits/streambuf.tcc \
 /opt/local/include/gcc46/c++/bits/basic_ios.h \
 /opt/local/include/gcc46/c++/bits/locale_facets.h \
 /opt/local/include/gcc46/c++/cwctype /usr/include/wctype.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/ctype_base.h \
 /opt/local/include/gcc46/c++/bits/streambuf_iterator.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/ctype_inline.h \
 /opt/local/include/gcc46/c++/bits/locale_facets.tcc \
 /opt/local/include/gcc46/c++/bits/basic_ios.tcc \
 /opt/local/include/gcc46/c++/bits/ostream.tcc \
 /opt/local/include/gcc46/c++/istream \
 /opt/local/include/gcc46/c++/bits/istream.tcc sli_token.h \
 sli_exceptions.h /opt/local/include/gcc46/c++/vector \
 /opt/local/include/gcc46/c++/bits/stl_vector.h \
 /opt/local/include/gcc46/c++/bits/stl_bvector.h \
 /opt/local/include/gcc46/c++/bits/vector.tcc sli_interpreter.h \
 sli_allocator.h /opt/local/include/gcc46/c++/cstdlib sli_arraytype.h \
 sli_array.h /opt/local/include/gcc46/c++/typeinfo \
 /opt/local/include/gcc46/c++/cstddef \
 /opt/local/lib/gcc46/gcc/x86_64-apple-darwin11/4.6.3/include/stddef.h \
 sli_integertype.h SLI_token.h sli_tokenstack.h sli_string.h \
 sli_dictionary.h sli_dictstack.h /opt/local/include/gcc46/c++/list \
 /opt/local/include/gcc46/c++/bits/stl_list.h \
 /opt/local/include/gcc46/c++/bits/list.tcc sli_builtins.h sli_function.h
sli_dicttype.o: sli_dicttype.cpp sli_token.h \
 /opt/local/include/gcc46/c++/iostream \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++config.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/os_defines.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/cpu_defines.h \
 /opt/local/include/gcc46/c++/ostream /opt/local/include/gcc46/c++/ios \
 /opt/local/include/gcc46/c++/iosfwd \
 /opt/local/include/gcc46/c++/bits/stringfwd.h \
 /opt/local/include/gcc46/c++/bits/postypes.h \
 /opt/local/include/gcc46/c++/cwchar /usr/include/wchar.h \
 /usr/include/_types.h /usr/include/sys/_types.h /usr/include/sys/cdefs.h \
 /usr/include/sys/_symbol_aliasing.h \
 /usr/include/sys/_posix_availability.h /usr/include/machine/_types.h \
 /usr/include/i386/_types.h /usr/include/Availability.h \
 /usr/include/AvailabilityInternal.h \
 /opt/local/lib/gcc46/gcc/x86_64-apple-darwin11/4.6.3/include/stdarg.h \
 /usr/include/stdio.h /usr/include/time.h /usr/include/_structs.h \
 /usr/include/sys/_structs.h /usr/include/_wctype.h /usr/include/ctype.h \
 /usr/include/runetype.h /opt/local/include/gcc46/c++/exception \
 /opt/local/include/gcc46/c++/bits/char_traits.h \
 /opt/local/include/gcc46/c++/bits/stl_algobase.h \
 /opt/local/include/gcc46/c++/bits/functexcept.h \
 /opt/local/include/gcc46/c++/bits/exception_defines.h \
 /opt/local/include/gcc46/c++/bits/cpp_type_traits.h \
 /opt/local/include/gcc46/c++/ext/type_traits.h \
 /opt/local/include/gcc46/c++/ext/numeric_traits.h \
 /opt/local/include/gcc46/c++/bits/stl_pair.h \
 /opt/local/include/gcc46/c++/bits/move.h \
 /opt/local/include/gcc46/c++/bits/concept_check.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator_base_types.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator_base_funcs.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator.h \
 /opt/local/include/gcc46/c++/debug/debug.h \
 /opt/local/include/gcc46/c++/bits/localefwd.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++locale.h \
 /opt/local/include/gcc46/c++/clocale /usr/include/locale.h \
 /usr/include/_locale.h /opt/local/include/gcc46/c++/cctype \
 /opt/local/include/gcc46/c++/bits/ios_base.h \
 /opt/local/include/gcc46/c++/ext/atomicity.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/gthr.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/gthr-default.h \
 /usr/include/pthread.h /usr/include/pthread_impl.h /usr/include/sched.h \
 /usr/include/unistd.h /usr/include/sys/unistd.h \
 /usr/include/sys/select.h /usr/include/sys/appleapiopts.h \
 /usr/include/sys/_select.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/atomic_word.h \
 /opt/local/include/gcc46/c++/bits/locale_classes.h \
 /opt/local/include/gcc46/c++/string \
 /opt/local/include/gcc46/c++/bits/allocator.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++allocator.h \
 /opt/local/include/gcc46/c++/ext/new_allocator.h \
 /opt/local/include/gcc46/c++/new \
 /opt/local/include/gcc46/c++/bits/ostream_insert.h \
 /opt/local/include/gcc46/c++/bits/cxxabi_forced.h \
 /opt/local/include/gcc46/c++/bits/stl_function.h \
 /opt/local/include/gcc46/c++/backward/binders.h \
 /opt/local/include/gcc46/c++/bits/range_access.h \
 /opt/local/include/gcc46/c++/bits/basic_string.h \
 /opt/local/include/gcc46/c++/initializer_list \
 /opt/local/include/gcc46/c++/bits/basic_string.tcc \
 /opt/local/include/gcc46/c++/bits/locale_classes.tcc \
 /opt/local/include/gcc46/c++/streambuf \
 /opt/local/include/gcc46/c++/bits/streambuf.tcc \
 /opt/local/include/gcc46/c++/bits/basic_ios.h \
 /opt/local/include/gcc46/c++/bits/locale_facets.h \
 /opt/local/include/gcc46/c++/cwctype /usr/include/wctype.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/ctype_base.h \
 /opt/local/include/gcc46/c++/bits/streambuf_iterator.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/ctype_inline.h \
 /opt/local/include/gcc46/c++/bits/locale_facets.tcc \
 /opt/local/include/gcc46/c++/bits/basic_ios.tcc \
 /opt/local/include/gcc46/c++/bits/ostream.tcc \
 /opt/local/include/gcc46/c++/istream \
 /opt/local/include/gcc46/c++/bits/istream.tcc sli_type.h sli_name.h \
 /opt/local/include/gcc46/c++/cassert /usr/include/assert.h \
 /usr/include/stdlib.h /usr/include/sys/wait.h /usr/include/sys/signal.h \
 /usr/include/machine/signal.h /usr/include/i386/signal.h \
 /usr/include/i386/_structs.h /usr/include/machine/_structs.h \
 /usr/include/mach/i386/_structs.h /usr/include/sys/resource.h \
 /usr/include/machine/endian.h /usr/include/i386/endian.h \
 /usr/include/sys/_endian.h /usr/include/libkern/_OSByteOrder.h \
 /usr/include/libkern/i386/_OSByteOrder.h /usr/include/alloca.h \
 /usr/include/machine/types.h /usr/include/i386/types.h \
 /opt/local/include/gcc46/c++/map \
 /opt/local/include/gcc46/c++/bits/stl_tree.h \
 /opt/local/include/gcc46/c++/bits/stl_map.h \
 /opt/local/include/gcc46/c++/bits/stl_multimap.h \
 /opt/local/include/gcc46/c++/deque \
 /opt/local/include/gcc46/c++/bits/stl_construct.h \
 /opt/local/include/gcc46/c++/bits/stl_uninitialized.h \
 /opt/local/include/gcc46/c++/bits/stl_deque.h \
 /opt/local/include/gcc46/c++/bits/deque.tcc sli_exceptions.h \
 /opt/local/include/gcc46/c++/vector \
 /opt/local/include/gcc46/c++/bits/stl_vector.h \
 /opt/local/include/gcc46/c++/bits/stl_bvector.h \
 /opt/local/include/gcc46/c++/bits/vector.tcc sli_dicttype.h \
 sli_dictionary.h
sli_functiontype.o: sli_functiontype.cpp sli_functiontype.h sli_type.h \
 /opt/local/include/gcc46/c++/string \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++config.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/os_defines.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/cpu_defines.h \
 /opt/local/include/gcc46/c++/bits/stringfwd.h \
 /opt/local/include/gcc46/c++/bits/char_traits.h \
 /opt/local/include/gcc46/c++/bits/stl_algobase.h \
 /opt/local/include/gcc46/c++/bits/functexcept.h \
 /opt/local/include/gcc46/c++/bits/exception_defines.h \
 /opt/local/include/gcc46/c++/bits/cpp_type_traits.h \
 /opt/local/include/gcc46/c++/ext/type_traits.h \
 /opt/local/include/gcc46/c++/ext/numeric_traits.h \
 /opt/local/include/gcc46/c++/bits/stl_pair.h \
 /opt/local/include/gcc46/c++/bits/move.h \
 /opt/local/include/gcc46/c++/bits/concept_check.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator_base_types.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator_base_funcs.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator.h \
 /opt/local/include/gcc46/c++/debug/debug.h \
 /opt/local/include/gcc46/c++/bits/postypes.h \
 /opt/local/include/gcc46/c++/cwchar /usr/include/wchar.h \
 /usr/include/_types.h /usr/include/sys/_types.h /usr/include/sys/cdefs.h \
 /usr/include/sys/_symbol_aliasing.h \
 /usr/include/sys/_posix_availability.h /usr/include/machine/_types.h \
 /usr/include/i386/_types.h /usr/include/Availability.h \
 /usr/include/AvailabilityInternal.h \
 /opt/local/lib/gcc46/gcc/x86_64-apple-darwin11/4.6.3/include/stdarg.h \
 /usr/include/stdio.h /usr/include/time.h /usr/include/_structs.h \
 /usr/include/sys/_structs.h /usr/include/_wctype.h /usr/include/ctype.h \
 /usr/include/runetype.h /opt/local/include/gcc46/c++/bits/allocator.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++allocator.h \
 /opt/local/include/gcc46/c++/ext/new_allocator.h \
 /opt/local/include/gcc46/c++/new /opt/local/include/gcc46/c++/exception \
 /opt/local/include/gcc46/c++/bits/localefwd.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++locale.h \
 /opt/local/include/gcc46/c++/clocale /usr/include/locale.h \
 /usr/include/_locale.h /opt/local/include/gcc46/c++/iosfwd \
 /opt/local/include/gcc46/c++/cctype \
 /opt/local/include/gcc46/c++/bits/ostream_insert.h \
 /opt/local/include/gcc46/c++/bits/cxxabi_forced.h \
 /opt/local/include/gcc46/c++/bits/stl_function.h \
 /opt/local/include/gcc46/c++/backward/binders.h \
 /opt/local/include/gcc46/c++/bits/range_access.h \
 /opt/local/include/gcc46/c++/bits/basic_string.h \
 /opt/local/include/gcc46/c++/ext/atomicity.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/gthr.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/gthr-default.h \
 /usr/include/pthread.h /usr/include/pthread_impl.h /usr/include/sched.h \
 /usr/include/unistd.h /usr/include/sys/unistd.h \
 /usr/include/sys/select.h /usr/include/sys/appleapiopts.h \
 /usr/include/sys/_select.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/atomic_word.h \
 /opt/local/include/gcc46/c++/initializer_list \
 /opt/local/include/gcc46/c++/bits/basic_string.tcc sli_name.h \
 /opt/local/include/gcc46/c++/cassert /usr/include/assert.h \
 /usr/include/stdlib.h /usr/include/sys/wait.h /usr/include/sys/signal.h \
 /usr/include/machine/signal.h /usr/include/i386/signal.h \
 /usr/include/i386/_structs.h /usr/include/machine/_structs.h \
 /usr/include/mach/i386/_structs.h /usr/include/sys/resource.h \
 /usr/include/machine/endian.h /usr/include/i386/endian.h \
 /usr/include/sys/_endian.h /usr/include/libkern/_OSByteOrder.h \
 /usr/include/libkern/i386/_OSByteOrder.h /usr/include/alloca.h \
 /usr/include/machine/types.h /usr/include/i386/types.h \
 /opt/local/include/gcc46/c++/map \
 /opt/local/include/gcc46/c++/bits/stl_tree.h \
 /opt/local/include/gcc46/c++/bits/stl_map.h \
 /opt/local/include/gcc46/c++/bits/stl_multimap.h \
 /opt/local/include/gcc46/c++/deque \
 /opt/local/include/gcc46/c++/bits/stl_construct.h \
 /opt/local/include/gcc46/c++/bits/stl_uninitialized.h \
 /opt/local/include/gcc46/c++/bits/stl_deque.h \
 /opt/local/include/gcc46/c++/bits/deque.tcc \
 /opt/local/include/gcc46/c++/iostream \
 /opt/local/include/gcc46/c++/ostream /opt/local/include/gcc46/c++/ios \
 /opt/local/include/gcc46/c++/bits/ios_base.h \
 /opt/local/include/gcc46/c++/bits/locale_classes.h \
 /opt/local/include/gcc46/c++/bits/locale_classes.tcc \
 /opt/local/include/gcc46/c++/streambuf \
 /opt/local/include/gcc46/c++/bits/streambuf.tcc \
 /opt/local/include/gcc46/c++/bits/basic_ios.h \
 /opt/local/include/gcc46/c++/bits/locale_facets.h \
 /opt/local/include/gcc46/c++/cwctype /usr/include/wctype.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/ctype_base.h \
 /opt/local/include/gcc46/c++/bits/streambuf_iterator.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/ctype_inline.h \
 /opt/local/include/gcc46/c++/bits/locale_facets.tcc \
 /opt/local/include/gcc46/c++/bits/basic_ios.tcc \
 /opt/local/include/gcc46/c++/bits/ostream.tcc \
 /opt/local/include/gcc46/c++/istream \
 /opt/local/include/gcc46/c++/bits/istream.tcc sli_interpreter.h \
 sli_token.h sli_exceptions.h /opt/local/include/gcc46/c++/vector \
 /opt/local/include/gcc46/c++/bits/stl_vector.h \
 /opt/local/include/gcc46/c++/bits/stl_bvector.h \
 /opt/local/include/gcc46/c++/bits/vector.tcc sli_allocator.h \
 /opt/local/include/gcc46/c++/cstdlib sli_arraytype.h sli_array.h \
 /opt/local/include/gcc46/c++/typeinfo \
 /opt/local/include/gcc46/c++/cstddef \
 /opt/local/lib/gcc46/gcc/x86_64-apple-darwin11/4.6.3/include/stddef.h \
 sli_integertype.h SLI_token.h sli_tokenstack.h sli_string.h \
 sli_dictionary.h sli_dictstack.h /opt/local/include/gcc46/c++/list \
 /opt/local/include/gcc46/c++/bits/stl_list.h \
 /opt/local/include/gcc46/c++/bits/list.tcc sli_builtins.h sli_function.h
sli_allocator.o: sli_allocator.cpp sli_allocator.h \
 /opt/local/include/gcc46/c++/cassert /usr/include/assert.h \
 /usr/include/sys/cdefs.h /usr/include/sys/_symbol_aliasing.h \
 /usr/include/sys/_posix_availability.h /usr/include/stdlib.h \
 /usr/include/Availability.h /usr/include/AvailabilityInternal.h \
 /usr/include/_types.h /usr/include/sys/_types.h \
 /usr/include/machine/_types.h /usr/include/i386/_types.h \
 /usr/include/sys/wait.h /usr/include/sys/signal.h \
 /usr/include/sys/appleapiopts.h /usr/include/machine/signal.h \
 /usr/include/i386/signal.h /usr/include/i386/_structs.h \
 /usr/include/sys/_structs.h /usr/include/machine/_structs.h \
 /usr/include/mach/i386/_structs.h /usr/include/sys/resource.h \
 /usr/include/machine/endian.h /usr/include/i386/endian.h \
 /usr/include/sys/_endian.h /usr/include/libkern/_OSByteOrder.h \
 /usr/include/libkern/i386/_OSByteOrder.h /usr/include/alloca.h \
 /usr/include/machine/types.h /usr/include/i386/types.h \
 /opt/local/include/gcc46/c++/cstdlib \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++config.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/os_defines.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/cpu_defines.h \
 /opt/local/include/gcc46/c++/string \
 /opt/local/include/gcc46/c++/bits/stringfwd.h \
 /opt/local/include/gcc46/c++/bits/char_traits.h \
 /opt/local/include/gcc46/c++/bits/stl_algobase.h \
 /opt/local/include/gcc46/c++/bits/functexcept.h \
 /opt/local/include/gcc46/c++/bits/exception_defines.h \
 /opt/local/include/gcc46/c++/bits/cpp_type_traits.h \
 /opt/local/include/gcc46/c++/ext/type_traits.h \
 /opt/local/include/gcc46/c++/ext/numeric_traits.h \
 /opt/local/include/gcc46/c++/bits/stl_pair.h \
 /opt/local/include/gcc46/c++/bits/move.h \
 /opt/local/include/gcc46/c++/bits/concept_check.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator_base_types.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator_base_funcs.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator.h \
 /opt/local/include/gcc46/c++/debug/debug.h \
 /opt/local/include/gcc46/c++/bits/postypes.h \
 /opt/local/include/gcc46/c++/cwchar /usr/include/wchar.h \
 /opt/local/lib/gcc46/gcc/x86_64-apple-darwin11/4.6.3/include/stdarg.h \
 /usr/include/stdio.h /usr/include/time.h /usr/include/_structs.h \
 /usr/include/_wctype.h /usr/include/ctype.h /usr/include/runetype.h \
 /opt/local/include/gcc46/c++/bits/allocator.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++allocator.h \
 /opt/local/include/gcc46/c++/ext/new_allocator.h \
 /opt/local/include/gcc46/c++/new /opt/local/include/gcc46/c++/exception \
 /opt/local/include/gcc46/c++/bits/localefwd.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++locale.h \
 /opt/local/include/gcc46/c++/clocale /usr/include/locale.h \
 /usr/include/_locale.h /opt/local/include/gcc46/c++/iosfwd \
 /opt/local/include/gcc46/c++/cctype \
 /opt/local/include/gcc46/c++/bits/ostream_insert.h \
 /opt/local/include/gcc46/c++/bits/cxxabi_forced.h \
 /opt/local/include/gcc46/c++/bits/stl_function.h \
 /opt/local/include/gcc46/c++/backward/binders.h \
 /opt/local/include/gcc46/c++/bits/range_access.h \
 /opt/local/include/gcc46/c++/bits/basic_string.h \
 /opt/local/include/gcc46/c++/ext/atomicity.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/gthr.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/gthr-default.h \
 /usr/include/pthread.h /usr/include/pthread_impl.h /usr/include/sched.h \
 /usr/include/unistd.h /usr/include/sys/unistd.h \
 /usr/include/sys/select.h /usr/include/sys/_select.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/atomic_word.h \
 /opt/local/include/gcc46/c++/initializer_list \
 /opt/local/include/gcc46/c++/bits/basic_string.tcc
sli_tokenstack.o: sli_tokenstack.cpp sli_tokenstack.h sli_token.h \
 /opt/local/include/gcc46/c++/iostream \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++config.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/os_defines.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/cpu_defines.h \
 /opt/local/include/gcc46/c++/ostream /opt/local/include/gcc46/c++/ios \
 /opt/local/include/gcc46/c++/iosfwd \
 /opt/local/include/gcc46/c++/bits/stringfwd.h \
 /opt/local/include/gcc46/c++/bits/postypes.h \
 /opt/local/include/gcc46/c++/cwchar /usr/include/wchar.h \
 /usr/include/_types.h /usr/include/sys/_types.h /usr/include/sys/cdefs.h \
 /usr/include/sys/_symbol_aliasing.h \
 /usr/include/sys/_posix_availability.h /usr/include/machine/_types.h \
 /usr/include/i386/_types.h /usr/include/Availability.h \
 /usr/include/AvailabilityInternal.h \
 /opt/local/lib/gcc46/gcc/x86_64-apple-darwin11/4.6.3/include/stdarg.h \
 /usr/include/stdio.h /usr/include/time.h /usr/include/_structs.h \
 /usr/include/sys/_structs.h /usr/include/_wctype.h /usr/include/ctype.h \
 /usr/include/runetype.h /opt/local/include/gcc46/c++/exception \
 /opt/local/include/gcc46/c++/bits/char_traits.h \
 /opt/local/include/gcc46/c++/bits/stl_algobase.h \
 /opt/local/include/gcc46/c++/bits/functexcept.h \
 /opt/local/include/gcc46/c++/bits/exception_defines.h \
 /opt/local/include/gcc46/c++/bits/cpp_type_traits.h \
 /opt/local/include/gcc46/c++/ext/type_traits.h \
 /opt/local/include/gcc46/c++/ext/numeric_traits.h \
 /opt/local/include/gcc46/c++/bits/stl_pair.h \
 /opt/local/include/gcc46/c++/bits/move.h \
 /opt/local/include/gcc46/c++/bits/concept_check.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator_base_types.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator_base_funcs.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator.h \
 /opt/local/include/gcc46/c++/debug/debug.h \
 /opt/local/include/gcc46/c++/bits/localefwd.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++locale.h \
 /opt/local/include/gcc46/c++/clocale /usr/include/locale.h \
 /usr/include/_locale.h /opt/local/include/gcc46/c++/cctype \
 /opt/local/include/gcc46/c++/bits/ios_base.h \
 /opt/local/include/gcc46/c++/ext/atomicity.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/gthr.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/gthr-default.h \
 /usr/include/pthread.h /usr/include/pthread_impl.h /usr/include/sched.h \
 /usr/include/unistd.h /usr/include/sys/unistd.h \
 /usr/include/sys/select.h /usr/include/sys/appleapiopts.h \
 /usr/include/sys/_select.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/atomic_word.h \
 /opt/local/include/gcc46/c++/bits/locale_classes.h \
 /opt/local/include/gcc46/c++/string \
 /opt/local/include/gcc46/c++/bits/allocator.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++allocator.h \
 /opt/local/include/gcc46/c++/ext/new_allocator.h \
 /opt/local/include/gcc46/c++/new \
 /opt/local/include/gcc46/c++/bits/ostream_insert.h \
 /opt/local/include/gcc46/c++/bits/cxxabi_forced.h \
 /opt/local/include/gcc46/c++/bits/stl_function.h \
 /opt/local/include/gcc46/c++/backward/binders.h \
 /opt/local/include/gcc46/c++/bits/range_access.h \
 /opt/local/include/gcc46/c++/bits/basic_string.h \
 /opt/local/include/gcc46/c++/initializer_list \
 /opt/local/include/gcc46/c++/bits/basic_string.tcc \
 /opt/local/include/gcc46/c++/bits/locale_classes.tcc \
 /opt/local/include/gcc46/c++/streambuf \
 /opt/local/include/gcc46/c++/bits/streambuf.tcc \
 /opt/local/include/gcc46/c++/bits/basic_ios.h \
 /opt/local/include/gcc46/c++/bits/locale_facets.h \
 /opt/local/include/gcc46/c++/cwctype /usr/include/wctype.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/ctype_base.h \
 /opt/local/include/gcc46/c++/bits/streambuf_iterator.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/ctype_inline.h \
 /opt/local/include/gcc46/c++/bits/locale_facets.tcc \
 /opt/local/include/gcc46/c++/bits/basic_ios.tcc \
 /opt/local/include/gcc46/c++/bits/ostream.tcc \
 /opt/local/include/gcc46/c++/istream \
 /opt/local/include/gcc46/c++/bits/istream.tcc sli_type.h sli_name.h \
 /opt/local/include/gcc46/c++/cassert /usr/include/assert.h \
 /usr/include/stdlib.h /usr/include/sys/wait.h /usr/include/sys/signal.h \
 /usr/include/machine/signal.h /usr/include/i386/signal.h \
 /usr/include/i386/_structs.h /usr/include/machine/_structs.h \
 /usr/include/mach/i386/_structs.h /usr/include/sys/resource.h \
 /usr/include/machine/endian.h /usr/include/i386/endian.h \
 /usr/include/sys/_endian.h /usr/include/libkern/_OSByteOrder.h \
 /usr/include/libkern/i386/_OSByteOrder.h /usr/include/alloca.h \
 /usr/include/machine/types.h /usr/include/i386/types.h \
 /opt/local/include/gcc46/c++/map \
 /opt/local/include/gcc46/c++/bits/stl_tree.h \
 /opt/local/include/gcc46/c++/bits/stl_map.h \
 /opt/local/include/gcc46/c++/bits/stl_multimap.h \
 /opt/local/include/gcc46/c++/deque \
 /opt/local/include/gcc46/c++/bits/stl_construct.h \
 /opt/local/include/gcc46/c++/bits/stl_uninitialized.h \
 /opt/local/include/gcc46/c++/bits/stl_deque.h \
 /opt/local/include/gcc46/c++/bits/deque.tcc sli_exceptions.h \
 /opt/local/include/gcc46/c++/vector \
 /opt/local/include/gcc46/c++/bits/stl_vector.h \
 /opt/local/include/gcc46/c++/bits/stl_bvector.h \
 /opt/local/include/gcc46/c++/bits/vector.tcc sli_array.h \
 /opt/local/include/gcc46/c++/typeinfo \
 /opt/local/include/gcc46/c++/cstddef \
 /opt/local/lib/gcc46/gcc/x86_64-apple-darwin11/4.6.3/include/stddef.h
sli_name.o: sli_name.cpp sli_name.h /opt/local/include/gcc46/c++/cassert \
 /usr/include/assert.h /usr/include/sys/cdefs.h \
 /usr/include/sys/_symbol_aliasing.h \
 /usr/include/sys/_posix_availability.h /usr/include/stdlib.h \
 /usr/include/Availability.h /usr/include/AvailabilityInternal.h \
 /usr/include/_types.h /usr/include/sys/_types.h \
 /usr/include/machine/_types.h /usr/include/i386/_types.h \
 /usr/include/sys/wait.h /usr/include/sys/signal.h \
 /usr/include/sys/appleapiopts.h /usr/include/machine/signal.h \
 /usr/include/i386/signal.h /usr/include/i386/_structs.h \
 /usr/include/sys/_structs.h /usr/include/machine/_structs.h \
 /usr/include/mach/i386/_structs.h /usr/include/sys/resource.h \
 /usr/include/machine/endian.h /usr/include/i386/endian.h \
 /usr/include/sys/_endian.h /usr/include/libkern/_OSByteOrder.h \
 /usr/include/libkern/i386/_OSByteOrder.h /usr/include/alloca.h \
 /usr/include/machine/types.h /usr/include/i386/types.h \
 /opt/local/include/gcc46/c++/map \
 /opt/local/include/gcc46/c++/bits/stl_tree.h \
 /opt/local/include/gcc46/c++/bits/stl_algobase.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++config.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/os_defines.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/cpu_defines.h \
 /opt/local/include/gcc46/c++/bits/functexcept.h \
 /opt/local/include/gcc46/c++/bits/exception_defines.h \
 /opt/local/include/gcc46/c++/bits/cpp_type_traits.h \
 /opt/local/include/gcc46/c++/ext/type_traits.h \
 /opt/local/include/gcc46/c++/ext/numeric_traits.h \
 /opt/local/include/gcc46/c++/bits/stl_pair.h \
 /opt/local/include/gcc46/c++/bits/move.h \
 /opt/local/include/gcc46/c++/bits/concept_check.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator_base_types.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator_base_funcs.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator.h \
 /opt/local/include/gcc46/c++/debug/debug.h \
 /opt/local/include/gcc46/c++/bits/allocator.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++allocator.h \
 /opt/local/include/gcc46/c++/ext/new_allocator.h \
 /opt/local/include/gcc46/c++/new /opt/local/include/gcc46/c++/exception \
 /opt/local/include/gcc46/c++/bits/stl_function.h \
 /opt/local/include/gcc46/c++/backward/binders.h \
 /opt/local/include/gcc46/c++/bits/stl_map.h \
 /opt/local/include/gcc46/c++/initializer_list \
 /opt/local/include/gcc46/c++/bits/stl_multimap.h \
 /opt/local/include/gcc46/c++/bits/range_access.h \
 /opt/local/include/gcc46/c++/string \
 /opt/local/include/gcc46/c++/bits/stringfwd.h \
 /opt/local/include/gcc46/c++/bits/char_traits.h \
 /opt/local/include/gcc46/c++/bits/postypes.h \
 /opt/local/include/gcc46/c++/cwchar /usr/include/wchar.h \
 /opt/local/lib/gcc46/gcc/x86_64-apple-darwin11/4.6.3/include/stdarg.h \
 /usr/include/stdio.h /usr/include/time.h /usr/include/_structs.h \
 /usr/include/_wctype.h /usr/include/ctype.h /usr/include/runetype.h \
 /opt/local/include/gcc46/c++/bits/localefwd.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++locale.h \
 /opt/local/include/gcc46/c++/clocale /usr/include/locale.h \
 /usr/include/_locale.h /opt/local/include/gcc46/c++/iosfwd \
 /opt/local/include/gcc46/c++/cctype \
 /opt/local/include/gcc46/c++/bits/ostream_insert.h \
 /opt/local/include/gcc46/c++/bits/cxxabi_forced.h \
 /opt/local/include/gcc46/c++/bits/basic_string.h \
 /opt/local/include/gcc46/c++/ext/atomicity.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/gthr.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/gthr-default.h \
 /usr/include/pthread.h /usr/include/pthread_impl.h /usr/include/sched.h \
 /usr/include/unistd.h /usr/include/sys/unistd.h \
 /usr/include/sys/select.h /usr/include/sys/_select.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/atomic_word.h \
 /opt/local/include/gcc46/c++/bits/basic_string.tcc \
 /opt/local/include/gcc46/c++/deque \
 /opt/local/include/gcc46/c++/bits/stl_construct.h \
 /opt/local/include/gcc46/c++/bits/stl_uninitialized.h \
 /opt/local/include/gcc46/c++/bits/stl_deque.h \
 /opt/local/include/gcc46/c++/bits/deque.tcc \
 /opt/local/include/gcc46/c++/iostream \
 /opt/local/include/gcc46/c++/ostream /opt/local/include/gcc46/c++/ios \
 /opt/local/include/gcc46/c++/bits/ios_base.h \
 /opt/local/include/gcc46/c++/bits/locale_classes.h \
 /opt/local/include/gcc46/c++/bits/locale_classes.tcc \
 /opt/local/include/gcc46/c++/streambuf \
 /opt/local/include/gcc46/c++/bits/streambuf.tcc \
 /opt/local/include/gcc46/c++/bits/basic_ios.h \
 /opt/local/include/gcc46/c++/bits/locale_facets.h \
 /opt/local/include/gcc46/c++/cwctype /usr/include/wctype.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/ctype_base.h \
 /opt/local/include/gcc46/c++/bits/streambuf_iterator.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/ctype_inline.h \
 /opt/local/include/gcc46/c++/bits/locale_facets.tcc \
 /opt/local/include/gcc46/c++/bits/basic_ios.tcc \
 /opt/local/include/gcc46/c++/bits/ostream.tcc \
 /opt/local/include/gcc46/c++/istream \
 /opt/local/include/gcc46/c++/bits/istream.tcc \
 /opt/local/include/gcc46/c++/iomanip
sli_dictionary.o: sli_dictionary.cpp sli_dictionary.h sli_name.h \
 /opt/local/include/gcc46/c++/cassert /usr/include/assert.h \
 /usr/include/sys/cdefs.h /usr/include/sys/_symbol_aliasing.h \
 /usr/include/sys/_posix_availability.h /usr/include/stdlib.h \
 /usr/include/Availability.h /usr/include/AvailabilityInternal.h \
 /usr/include/_types.h /usr/include/sys/_types.h \
 /usr/include/machine/_types.h /usr/include/i386/_types.h \
 /usr/include/sys/wait.h /usr/include/sys/signal.h \
 /usr/include/sys/appleapiopts.h /usr/include/machine/signal.h \
 /usr/include/i386/signal.h /usr/include/i386/_structs.h \
 /usr/include/sys/_structs.h /usr/include/machine/_structs.h \
 /usr/include/mach/i386/_structs.h /usr/include/sys/resource.h \
 /usr/include/machine/endian.h /usr/include/i386/endian.h \
 /usr/include/sys/_endian.h /usr/include/libkern/_OSByteOrder.h \
 /usr/include/libkern/i386/_OSByteOrder.h /usr/include/alloca.h \
 /usr/include/machine/types.h /usr/include/i386/types.h \
 /opt/local/include/gcc46/c++/map \
 /opt/local/include/gcc46/c++/bits/stl_tree.h \
 /opt/local/include/gcc46/c++/bits/stl_algobase.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++config.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/os_defines.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/cpu_defines.h \
 /opt/local/include/gcc46/c++/bits/functexcept.h \
 /opt/local/include/gcc46/c++/bits/exception_defines.h \
 /opt/local/include/gcc46/c++/bits/cpp_type_traits.h \
 /opt/local/include/gcc46/c++/ext/type_traits.h \
 /opt/local/include/gcc46/c++/ext/numeric_traits.h \
 /opt/local/include/gcc46/c++/bits/stl_pair.h \
 /opt/local/include/gcc46/c++/bits/move.h \
 /opt/local/include/gcc46/c++/bits/concept_check.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator_base_types.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator_base_funcs.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator.h \
 /opt/local/include/gcc46/c++/debug/debug.h \
 /opt/local/include/gcc46/c++/bits/allocator.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++allocator.h \
 /opt/local/include/gcc46/c++/ext/new_allocator.h \
 /opt/local/include/gcc46/c++/new /opt/local/include/gcc46/c++/exception \
 /opt/local/include/gcc46/c++/bits/stl_function.h \
 /opt/local/include/gcc46/c++/backward/binders.h \
 /opt/local/include/gcc46/c++/bits/stl_map.h \
 /opt/local/include/gcc46/c++/initializer_list \
 /opt/local/include/gcc46/c++/bits/stl_multimap.h \
 /opt/local/include/gcc46/c++/bits/range_access.h \
 /opt/local/include/gcc46/c++/string \
 /opt/local/include/gcc46/c++/bits/stringfwd.h \
 /opt/local/include/gcc46/c++/bits/char_traits.h \
 /opt/local/include/gcc46/c++/bits/postypes.h \
 /opt/local/include/gcc46/c++/cwchar /usr/include/wchar.h \
 /opt/local/lib/gcc46/gcc/x86_64-apple-darwin11/4.6.3/include/stdarg.h \
 /usr/include/stdio.h /usr/include/time.h /usr/include/_structs.h \
 /usr/include/_wctype.h /usr/include/ctype.h /usr/include/runetype.h \
 /opt/local/include/gcc46/c++/bits/localefwd.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++locale.h \
 /opt/local/include/gcc46/c++/clocale /usr/include/locale.h \
 /usr/include/_locale.h /opt/local/include/gcc46/c++/iosfwd \
 /opt/local/include/gcc46/c++/cctype \
 /opt/local/include/gcc46/c++/bits/ostream_insert.h \
 /opt/local/include/gcc46/c++/bits/cxxabi_forced.h \
 /opt/local/include/gcc46/c++/bits/basic_string.h \
 /opt/local/include/gcc46/c++/ext/atomicity.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/gthr.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/gthr-default.h \
 /usr/include/pthread.h /usr/include/pthread_impl.h /usr/include/sched.h \
 /usr/include/unistd.h /usr/include/sys/unistd.h \
 /usr/include/sys/select.h /usr/include/sys/_select.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/atomic_word.h \
 /opt/local/include/gcc46/c++/bits/basic_string.tcc \
 /opt/local/include/gcc46/c++/deque \
 /opt/local/include/gcc46/c++/bits/stl_construct.h \
 /opt/local/include/gcc46/c++/bits/stl_uninitialized.h \
 /opt/local/include/gcc46/c++/bits/stl_deque.h \
 /opt/local/include/gcc46/c++/bits/deque.tcc \
 /opt/local/include/gcc46/c++/iostream \
 /opt/local/include/gcc46/c++/ostream /opt/local/include/gcc46/c++/ios \
 /opt/local/include/gcc46/c++/bits/ios_base.h \
 /opt/local/include/gcc46/c++/bits/locale_classes.h \
 /opt/local/include/gcc46/c++/bits/locale_classes.tcc \
 /opt/local/include/gcc46/c++/streambuf \
 /opt/local/include/gcc46/c++/bits/streambuf.tcc \
 /opt/local/include/gcc46/c++/bits/basic_ios.h \
 /opt/local/include/gcc46/c++/bits/locale_facets.h \
 /opt/local/include/gcc46/c++/cwctype /usr/include/wctype.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/ctype_base.h \
 /opt/local/include/gcc46/c++/bits/streambuf_iterator.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/ctype_inline.h \
 /opt/local/include/gcc46/c++/bits/locale_facets.tcc \
 /opt/local/include/gcc46/c++/bits/basic_ios.tcc \
 /opt/local/include/gcc46/c++/bits/ostream.tcc \
 /opt/local/include/gcc46/c++/istream \
 /opt/local/include/gcc46/c++/bits/istream.tcc sli_token.h sli_type.h \
 sli_exceptions.h /opt/local/include/gcc46/c++/vector \
 /opt/local/include/gcc46/c++/bits/stl_vector.h \
 /opt/local/include/gcc46/c++/bits/stl_bvector.h \
 /opt/local/include/gcc46/c++/bits/vector.tcc \
 /opt/local/include/gcc46/c++/iomanip \
 /opt/local/include/gcc46/c++/algorithm \
 /opt/local/include/gcc46/c++/utility \
 /opt/local/include/gcc46/c++/bits/stl_relops.h \
 /opt/local/include/gcc46/c++/bits/stl_algo.h \
 /opt/local/include/gcc46/c++/cstdlib \
 /opt/local/include/gcc46/c++/bits/algorithmfwd.h \
 /opt/local/include/gcc46/c++/bits/stl_heap.h \
 /opt/local/include/gcc46/c++/bits/stl_tempbuf.h
sli_exceptions.o: sli_exceptions.cpp config.h sli_exceptions.h sli_name.h \
 /opt/local/include/gcc46/c++/cassert /usr/include/assert.h \
 /usr/include/sys/cdefs.h /usr/include/sys/_symbol_aliasing.h \
 /usr/include/sys/_posix_availability.h /usr/include/stdlib.h \
 /usr/include/Availability.h /usr/include/AvailabilityInternal.h \
 /usr/include/_types.h /usr/include/sys/_types.h \
 /usr/include/machine/_types.h /usr/include/i386/_types.h \
 /usr/include/sys/wait.h /usr/include/sys/signal.h \
 /usr/include/sys/appleapiopts.h /usr/include/machine/signal.h \
 /usr/include/i386/signal.h /usr/include/i386/_structs.h \
 /usr/include/sys/_structs.h /usr/include/machine/_structs.h \
 /usr/include/mach/i386/_structs.h /usr/include/sys/resource.h \
 /usr/include/machine/endian.h /usr/include/i386/endian.h \
 /usr/include/sys/_endian.h /usr/include/libkern/_OSByteOrder.h \
 /usr/include/libkern/i386/_OSByteOrder.h /usr/include/alloca.h \
 /usr/include/machine/types.h /usr/include/i386/types.h \
 /opt/local/include/gcc46/c++/map \
 /opt/local/include/gcc46/c++/bits/stl_tree.h \
 /opt/local/include/gcc46/c++/bits/stl_algobase.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++config.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/os_defines.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/cpu_defines.h \
 /opt/local/include/gcc46/c++/bits/functexcept.h \
 /opt/local/include/gcc46/c++/bits/exception_defines.h \
 /opt/local/include/gcc46/c++/bits/cpp_type_traits.h \
 /opt/local/include/gcc46/c++/ext/type_traits.h \
 /opt/local/include/gcc46/c++/ext/numeric_traits.h \
 /opt/local/include/gcc46/c++/bits/stl_pair.h \
 /opt/local/include/gcc46/c++/bits/move.h \
 /opt/local/include/gcc46/c++/bits/concept_check.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator_base_types.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator_base_funcs.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator.h \
 /opt/local/include/gcc46/c++/debug/debug.h \
 /opt/local/include/gcc46/c++/bits/allocator.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++allocator.h \
 /opt/local/include/gcc46/c++/ext/new_allocator.h \
 /opt/local/include/gcc46/c++/new /opt/local/include/gcc46/c++/exception \
 /opt/local/include/gcc46/c++/bits/stl_function.h \
 /opt/local/include/gcc46/c++/backward/binders.h \
 /opt/local/include/gcc46/c++/bits/stl_map.h \
 /opt/local/include/gcc46/c++/initializer_list \
 /opt/local/include/gcc46/c++/bits/stl_multimap.h \
 /opt/local/include/gcc46/c++/bits/range_access.h \
 /opt/local/include/gcc46/c++/string \
 /opt/local/include/gcc46/c++/bits/stringfwd.h \
 /opt/local/include/gcc46/c++/bits/char_traits.h \
 /opt/local/include/gcc46/c++/bits/postypes.h \
 /opt/local/include/gcc46/c++/cwchar /usr/include/wchar.h \
 /opt/local/lib/gcc46/gcc/x86_64-apple-darwin11/4.6.3/include/stdarg.h \
 /usr/include/stdio.h /usr/include/time.h /usr/include/_structs.h \
 /usr/include/_wctype.h /usr/include/ctype.h /usr/include/runetype.h \
 /opt/local/include/gcc46/c++/bits/localefwd.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++locale.h \
 /opt/local/include/gcc46/c++/clocale /usr/include/locale.h \
 /usr/include/_locale.h /opt/local/include/gcc46/c++/iosfwd \
 /opt/local/include/gcc46/c++/cctype \
 /opt/local/include/gcc46/c++/bits/ostream_insert.h \
 /opt/local/include/gcc46/c++/bits/cxxabi_forced.h \
 /opt/local/include/gcc46/c++/bits/basic_string.h \
 /opt/local/include/gcc46/c++/ext/atomicity.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/gthr.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/gthr-default.h \
 /usr/include/pthread.h /usr/include/pthread_impl.h /usr/include/sched.h \
 /usr/include/unistd.h /usr/include/sys/unistd.h \
 /usr/include/sys/select.h /usr/include/sys/_select.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/atomic_word.h \
 /opt/local/include/gcc46/c++/bits/basic_string.tcc \
 /opt/local/include/gcc46/c++/deque \
 /opt/local/include/gcc46/c++/bits/stl_construct.h \
 /opt/local/include/gcc46/c++/bits/stl_uninitialized.h \
 /opt/local/include/gcc46/c++/bits/stl_deque.h \
 /opt/local/include/gcc46/c++/bits/deque.tcc \
 /opt/local/include/gcc46/c++/iostream \
 /opt/local/include/gcc46/c++/ostream /opt/local/include/gcc46/c++/ios \
 /opt/local/include/gcc46/c++/bits/ios_base.h \
 /opt/local/include/gcc46/c++/bits/locale_classes.h \
 /opt/local/include/gcc46/c++/bits/locale_classes.tcc \
 /opt/local/include/gcc46/c++/streambuf \
 /opt/local/include/gcc46/c++/bits/streambuf.tcc \
 /opt/local/include/gcc46/c++/bits/basic_ios.h \
 /opt/local/include/gcc46/c++/bits/locale_facets.h \
 /opt/local/include/gcc46/c++/cwctype /usr/include/wctype.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/ctype_base.h \
 /opt/local/include/gcc46/c++/bits/streambuf_iterator.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/ctype_inline.h \
 /opt/local/include/gcc46/c++/bits/locale_facets.tcc \
 /opt/local/include/gcc46/c++/bits/basic_ios.tcc \
 /opt/local/include/gcc46/c++/bits/ostream.tcc \
 /opt/local/include/gcc46/c++/istream \
 /opt/local/include/gcc46/c++/bits/istream.tcc \
 /opt/local/include/gcc46/c++/vector \
 /opt/local/include/gcc46/c++/bits/stl_vector.h \
 /opt/local/include/gcc46/c++/bits/stl_bvector.h \
 /opt/local/include/gcc46/c++/bits/vector.tcc sli_interpreter.h \
 sli_type.h sli_token.h sli_allocator.h \
 /opt/local/include/gcc46/c++/cstdlib sli_arraytype.h sli_array.h \
 /opt/local/include/gcc46/c++/typeinfo \
 /opt/local/include/gcc46/c++/cstddef \
 /opt/local/lib/gcc46/gcc/x86_64-apple-darwin11/4.6.3/include/stddef.h \
 sli_integertype.h SLI_token.h sli_tokenstack.h sli_string.h \
 sli_dictionary.h sli_dictstack.h /opt/local/include/gcc46/c++/list \
 /opt/local/include/gcc46/c++/bits/stl_list.h \
 /opt/local/include/gcc46/c++/bits/list.tcc sli_builtins.h sli_function.h \
 /opt/local/include/gcc46/c++/sstream \
 /opt/local/include/gcc46/c++/bits/sstream.tcc
sli_dictstack.o: sli_dictstack.cpp sli_interpreter.h sli_type.h \
 /opt/local/include/gcc46/c++/string \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++config.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/os_defines.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/cpu_defines.h \
 /opt/local/include/gcc46/c++/bits/stringfwd.h \
 /opt/local/include/gcc46/c++/bits/char_traits.h \
 /opt/local/include/gcc46/c++/bits/stl_algobase.h \
 /opt/local/include/gcc46/c++/bits/functexcept.h \
 /opt/local/include/gcc46/c++/bits/exception_defines.h \
 /opt/local/include/gcc46/c++/bits/cpp_type_traits.h \
 /opt/local/include/gcc46/c++/ext/type_traits.h \
 /opt/local/include/gcc46/c++/ext/numeric_traits.h \
 /opt/local/include/gcc46/c++/bits/stl_pair.h \
 /opt/local/include/gcc46/c++/bits/move.h \
 /opt/local/include/gcc46/c++/bits/concept_check.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator_base_types.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator_base_funcs.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator.h \
 /opt/local/include/gcc46/c++/debug/debug.h \
 /opt/local/include/gcc46/c++/bits/postypes.h \
 /opt/local/include/gcc46/c++/cwchar /usr/include/wchar.h \
 /usr/include/_types.h /usr/include/sys/_types.h /usr/include/sys/cdefs.h \
 /usr/include/sys/_symbol_aliasing.h \
 /usr/include/sys/_posix_availability.h /usr/include/machine/_types.h \
 /usr/include/i386/_types.h /usr/include/Availability.h \
 /usr/include/AvailabilityInternal.h \
 /opt/local/lib/gcc46/gcc/x86_64-apple-darwin11/4.6.3/include/stdarg.h \
 /usr/include/stdio.h /usr/include/time.h /usr/include/_structs.h \
 /usr/include/sys/_structs.h /usr/include/_wctype.h /usr/include/ctype.h \
 /usr/include/runetype.h /opt/local/include/gcc46/c++/bits/allocator.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++allocator.h \
 /opt/local/include/gcc46/c++/ext/new_allocator.h \
 /opt/local/include/gcc46/c++/new /opt/local/include/gcc46/c++/exception \
 /opt/local/include/gcc46/c++/bits/localefwd.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++locale.h \
 /opt/local/include/gcc46/c++/clocale /usr/include/locale.h \
 /usr/include/_locale.h /opt/local/include/gcc46/c++/iosfwd \
 /opt/local/include/gcc46/c++/cctype \
 /opt/local/include/gcc46/c++/bits/ostream_insert.h \
 /opt/local/include/gcc46/c++/bits/cxxabi_forced.h \
 /opt/local/include/gcc46/c++/bits/stl_function.h \
 /opt/local/include/gcc46/c++/backward/binders.h \
 /opt/local/include/gcc46/c++/bits/range_access.h \
 /opt/local/include/gcc46/c++/bits/basic_string.h \
 /opt/local/include/gcc46/c++/ext/atomicity.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/gthr.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/gthr-default.h \
 /usr/include/pthread.h /usr/include/pthread_impl.h /usr/include/sched.h \
 /usr/include/unistd.h /usr/include/sys/unistd.h \
 /usr/include/sys/select.h /usr/include/sys/appleapiopts.h \
 /usr/include/sys/_select.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/atomic_word.h \
 /opt/local/include/gcc46/c++/initializer_list \
 /opt/local/include/gcc46/c++/bits/basic_string.tcc sli_name.h \
 /opt/local/include/gcc46/c++/cassert /usr/include/assert.h \
 /usr/include/stdlib.h /usr/include/sys/wait.h /usr/include/sys/signal.h \
 /usr/include/machine/signal.h /usr/include/i386/signal.h \
 /usr/include/i386/_structs.h /usr/include/machine/_structs.h \
 /usr/include/mach/i386/_structs.h /usr/include/sys/resource.h \
 /usr/include/machine/endian.h /usr/include/i386/endian.h \
 /usr/include/sys/_endian.h /usr/include/libkern/_OSByteOrder.h \
 /usr/include/libkern/i386/_OSByteOrder.h /usr/include/alloca.h \
 /usr/include/machine/types.h /usr/include/i386/types.h \
 /opt/local/include/gcc46/c++/map \
 /opt/local/include/gcc46/c++/bits/stl_tree.h \
 /opt/local/include/gcc46/c++/bits/stl_map.h \
 /opt/local/include/gcc46/c++/bits/stl_multimap.h \
 /opt/local/include/gcc46/c++/deque \
 /opt/local/include/gcc46/c++/bits/stl_construct.h \
 /opt/local/include/gcc46/c++/bits/stl_uninitialized.h \
 /opt/local/include/gcc46/c++/bits/stl_deque.h \
 /opt/local/include/gcc46/c++/bits/deque.tcc \
 /opt/local/include/gcc46/c++/iostream \
 /opt/local/include/gcc46/c++/ostream /opt/local/include/gcc46/c++/ios \
 /opt/local/include/gcc46/c++/bits/ios_base.h \
 /opt/local/include/gcc46/c++/bits/locale_classes.h \
 /opt/local/include/gcc46/c++/bits/locale_classes.tcc \
 /opt/local/include/gcc46/c++/streambuf \
 /opt/local/include/gcc46/c++/bits/streambuf.tcc \
 /opt/local/include/gcc46/c++/bits/basic_ios.h \
 /opt/local/include/gcc46/c++/bits/locale_facets.h \
 /opt/local/include/gcc46/c++/cwctype /usr/include/wctype.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/ctype_base.h \
 /opt/local/include/gcc46/c++/bits/streambuf_iterator.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/ctype_inline.h \
 /opt/local/include/gcc46/c++/bits/locale_facets.tcc \
 /opt/local/include/gcc46/c++/bits/basic_ios.tcc \
 /opt/local/include/gcc46/c++/bits/ostream.tcc \
 /opt/local/include/gcc46/c++/istream \
 /opt/local/include/gcc46/c++/bits/istream.tcc sli_token.h \
 sli_exceptions.h /opt/local/include/gcc46/c++/vector \
 /opt/local/include/gcc46/c++/bits/stl_vector.h \
 /opt/local/include/gcc46/c++/bits/stl_bvector.h \
 /opt/local/include/gcc46/c++/bits/vector.tcc sli_allocator.h \
 /opt/local/include/gcc46/c++/cstdlib sli_arraytype.h sli_array.h \
 /opt/local/include/gcc46/c++/typeinfo \
 /opt/local/include/gcc46/c++/cstddef \
 /opt/local/lib/gcc46/gcc/x86_64-apple-darwin11/4.6.3/include/stddef.h \
 sli_integertype.h SLI_token.h sli_tokenstack.h sli_string.h \
 sli_dictionary.h sli_dictstack.h /opt/local/include/gcc46/c++/list \
 /opt/local/include/gcc46/c++/bits/stl_list.h \
 /opt/local/include/gcc46/c++/bits/list.tcc sli_builtins.h sli_function.h
sli_charcode.o: sli_charcode.cpp sli_charcode.h \
 /opt/local/include/gcc46/c++/vector \
 /opt/local/include/gcc46/c++/bits/stl_algobase.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++config.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/os_defines.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/cpu_defines.h \
 /opt/local/include/gcc46/c++/bits/functexcept.h \
 /opt/local/include/gcc46/c++/bits/exception_defines.h \
 /opt/local/include/gcc46/c++/bits/cpp_type_traits.h \
 /opt/local/include/gcc46/c++/ext/type_traits.h \
 /opt/local/include/gcc46/c++/ext/numeric_traits.h \
 /opt/local/include/gcc46/c++/bits/stl_pair.h \
 /opt/local/include/gcc46/c++/bits/move.h \
 /opt/local/include/gcc46/c++/bits/concept_check.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator_base_types.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator_base_funcs.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator.h \
 /opt/local/include/gcc46/c++/debug/debug.h \
 /opt/local/include/gcc46/c++/bits/allocator.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++allocator.h \
 /opt/local/include/gcc46/c++/ext/new_allocator.h \
 /opt/local/include/gcc46/c++/new /opt/local/include/gcc46/c++/exception \
 /opt/local/include/gcc46/c++/bits/stl_construct.h \
 /opt/local/include/gcc46/c++/bits/stl_uninitialized.h \
 /opt/local/include/gcc46/c++/bits/stl_vector.h \
 /opt/local/include/gcc46/c++/initializer_list \
 /opt/local/include/gcc46/c++/bits/stl_bvector.h \
 /opt/local/include/gcc46/c++/bits/range_access.h \
 /opt/local/include/gcc46/c++/bits/vector.tcc \
 /opt/local/include/gcc46/c++/cassert /usr/include/assert.h \
 /usr/include/sys/cdefs.h /usr/include/sys/_symbol_aliasing.h \
 /usr/include/sys/_posix_availability.h /usr/include/stdlib.h \
 /usr/include/Availability.h /usr/include/AvailabilityInternal.h \
 /usr/include/_types.h /usr/include/sys/_types.h \
 /usr/include/machine/_types.h /usr/include/i386/_types.h \
 /usr/include/sys/wait.h /usr/include/sys/signal.h \
 /usr/include/sys/appleapiopts.h /usr/include/machine/signal.h \
 /usr/include/i386/signal.h /usr/include/i386/_structs.h \
 /usr/include/sys/_structs.h /usr/include/machine/_structs.h \
 /usr/include/mach/i386/_structs.h /usr/include/sys/resource.h \
 /usr/include/machine/endian.h /usr/include/i386/endian.h \
 /usr/include/sys/_endian.h /usr/include/libkern/_OSByteOrder.h \
 /usr/include/libkern/i386/_OSByteOrder.h /usr/include/alloca.h \
 /usr/include/machine/types.h /usr/include/i386/types.h
sli_scanner.o: sli_scanner.cpp sli_scanner.h sli_token.h \
 /opt/local/include/gcc46/c++/iostream \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++config.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/os_defines.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/cpu_defines.h \
 /opt/local/include/gcc46/c++/ostream /opt/local/include/gcc46/c++/ios \
 /opt/local/include/gcc46/c++/iosfwd \
 /opt/local/include/gcc46/c++/bits/stringfwd.h \
 /opt/local/include/gcc46/c++/bits/postypes.h \
 /opt/local/include/gcc46/c++/cwchar /usr/include/wchar.h \
 /usr/include/_types.h /usr/include/sys/_types.h /usr/include/sys/cdefs.h \
 /usr/include/sys/_symbol_aliasing.h \
 /usr/include/sys/_posix_availability.h /usr/include/machine/_types.h \
 /usr/include/i386/_types.h /usr/include/Availability.h \
 /usr/include/AvailabilityInternal.h \
 /opt/local/lib/gcc46/gcc/x86_64-apple-darwin11/4.6.3/include/stdarg.h \
 /usr/include/stdio.h /usr/include/time.h /usr/include/_structs.h \
 /usr/include/sys/_structs.h /usr/include/_wctype.h /usr/include/ctype.h \
 /usr/include/runetype.h /opt/local/include/gcc46/c++/exception \
 /opt/local/include/gcc46/c++/bits/char_traits.h \
 /opt/local/include/gcc46/c++/bits/stl_algobase.h \
 /opt/local/include/gcc46/c++/bits/functexcept.h \
 /opt/local/include/gcc46/c++/bits/exception_defines.h \
 /opt/local/include/gcc46/c++/bits/cpp_type_traits.h \
 /opt/local/include/gcc46/c++/ext/type_traits.h \
 /opt/local/include/gcc46/c++/ext/numeric_traits.h \
 /opt/local/include/gcc46/c++/bits/stl_pair.h \
 /opt/local/include/gcc46/c++/bits/move.h \
 /opt/local/include/gcc46/c++/bits/concept_check.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator_base_types.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator_base_funcs.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator.h \
 /opt/local/include/gcc46/c++/debug/debug.h \
 /opt/local/include/gcc46/c++/bits/localefwd.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++locale.h \
 /opt/local/include/gcc46/c++/clocale /usr/include/locale.h \
 /usr/include/_locale.h /opt/local/include/gcc46/c++/cctype \
 /opt/local/include/gcc46/c++/bits/ios_base.h \
 /opt/local/include/gcc46/c++/ext/atomicity.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/gthr.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/gthr-default.h \
 /usr/include/pthread.h /usr/include/pthread_impl.h /usr/include/sched.h \
 /usr/include/unistd.h /usr/include/sys/unistd.h \
 /usr/include/sys/select.h /usr/include/sys/appleapiopts.h \
 /usr/include/sys/_select.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/atomic_word.h \
 /opt/local/include/gcc46/c++/bits/locale_classes.h \
 /opt/local/include/gcc46/c++/string \
 /opt/local/include/gcc46/c++/bits/allocator.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++allocator.h \
 /opt/local/include/gcc46/c++/ext/new_allocator.h \
 /opt/local/include/gcc46/c++/new \
 /opt/local/include/gcc46/c++/bits/ostream_insert.h \
 /opt/local/include/gcc46/c++/bits/cxxabi_forced.h \
 /opt/local/include/gcc46/c++/bits/stl_function.h \
 /opt/local/include/gcc46/c++/backward/binders.h \
 /opt/local/include/gcc46/c++/bits/range_access.h \
 /opt/local/include/gcc46/c++/bits/basic_string.h \
 /opt/local/include/gcc46/c++/initializer_list \
 /opt/local/include/gcc46/c++/bits/basic_string.tcc \
 /opt/local/include/gcc46/c++/bits/locale_classes.tcc \
 /opt/local/include/gcc46/c++/streambuf \
 /opt/local/include/gcc46/c++/bits/streambuf.tcc \
 /opt/local/include/gcc46/c++/bits/basic_ios.h \
 /opt/local/include/gcc46/c++/bits/locale_facets.h \
 /opt/local/include/gcc46/c++/cwctype /usr/include/wctype.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/ctype_base.h \
 /opt/local/include/gcc46/c++/bits/streambuf_iterator.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/ctype_inline.h \
 /opt/local/include/gcc46/c++/bits/locale_facets.tcc \
 /opt/local/include/gcc46/c++/bits/basic_ios.tcc \
 /opt/local/include/gcc46/c++/bits/ostream.tcc \
 /opt/local/include/gcc46/c++/istream \
 /opt/local/include/gcc46/c++/bits/istream.tcc sli_type.h sli_name.h \
 /opt/local/include/gcc46/c++/cassert /usr/include/assert.h \
 /usr/include/stdlib.h /usr/include/sys/wait.h /usr/include/sys/signal.h \
 /usr/include/machine/signal.h /usr/include/i386/signal.h \
 /usr/include/i386/_structs.h /usr/include/machine/_structs.h \
 /usr/include/mach/i386/_structs.h /usr/include/sys/resource.h \
 /usr/include/machine/endian.h /usr/include/i386/endian.h \
 /usr/include/sys/_endian.h /usr/include/libkern/_OSByteOrder.h \
 /usr/include/libkern/i386/_OSByteOrder.h /usr/include/alloca.h \
 /usr/include/machine/types.h /usr/include/i386/types.h \
 /opt/local/include/gcc46/c++/map \
 /opt/local/include/gcc46/c++/bits/stl_tree.h \
 /opt/local/include/gcc46/c++/bits/stl_map.h \
 /opt/local/include/gcc46/c++/bits/stl_multimap.h \
 /opt/local/include/gcc46/c++/deque \
 /opt/local/include/gcc46/c++/bits/stl_construct.h \
 /opt/local/include/gcc46/c++/bits/stl_uninitialized.h \
 /opt/local/include/gcc46/c++/bits/stl_deque.h \
 /opt/local/include/gcc46/c++/bits/deque.tcc sli_exceptions.h \
 /opt/local/include/gcc46/c++/vector \
 /opt/local/include/gcc46/c++/bits/stl_vector.h \
 /opt/local/include/gcc46/c++/bits/stl_bvector.h \
 /opt/local/include/gcc46/c++/bits/vector.tcc sli_charcode.h \
 sli_interpreter.h sli_allocator.h /opt/local/include/gcc46/c++/cstdlib \
 sli_arraytype.h sli_array.h /opt/local/include/gcc46/c++/typeinfo \
 /opt/local/include/gcc46/c++/cstddef \
 /opt/local/lib/gcc46/gcc/x86_64-apple-darwin11/4.6.3/include/stddef.h \
 sli_integertype.h SLI_token.h sli_tokenstack.h sli_string.h \
 sli_dictionary.h sli_dictstack.h /opt/local/include/gcc46/c++/list \
 /opt/local/include/gcc46/c++/bits/stl_list.h \
 /opt/local/include/gcc46/c++/bits/list.tcc sli_builtins.h sli_function.h \
 /opt/local/include/gcc46/c++/cmath /usr/include/math.h \
 /usr/include/architecture/i386/math.h \
 /opt/local/include/gcc46/c++/sstream \
 /opt/local/include/gcc46/c++/bits/sstream.tcc \
 /opt/local/include/gcc46/c++/limits
sli_parser.o: sli_parser.cpp sli_interpreter.h sli_type.h \
 /opt/local/include/gcc46/c++/string \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++config.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/os_defines.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/cpu_defines.h \
 /opt/local/include/gcc46/c++/bits/stringfwd.h \
 /opt/local/include/gcc46/c++/bits/char_traits.h \
 /opt/local/include/gcc46/c++/bits/stl_algobase.h \
 /opt/local/include/gcc46/c++/bits/functexcept.h \
 /opt/local/include/gcc46/c++/bits/exception_defines.h \
 /opt/local/include/gcc46/c++/bits/cpp_type_traits.h \
 /opt/local/include/gcc46/c++/ext/type_traits.h \
 /opt/local/include/gcc46/c++/ext/numeric_traits.h \
 /opt/local/include/gcc46/c++/bits/stl_pair.h \
 /opt/local/include/gcc46/c++/bits/move.h \
 /opt/local/include/gcc46/c++/bits/concept_check.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator_base_types.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator_base_funcs.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator.h \
 /opt/local/include/gcc46/c++/debug/debug.h \
 /opt/local/include/gcc46/c++/bits/postypes.h \
 /opt/local/include/gcc46/c++/cwchar /usr/include/wchar.h \
 /usr/include/_types.h /usr/include/sys/_types.h /usr/include/sys/cdefs.h \
 /usr/include/sys/_symbol_aliasing.h \
 /usr/include/sys/_posix_availability.h /usr/include/machine/_types.h \
 /usr/include/i386/_types.h /usr/include/Availability.h \
 /usr/include/AvailabilityInternal.h \
 /opt/local/lib/gcc46/gcc/x86_64-apple-darwin11/4.6.3/include/stdarg.h \
 /usr/include/stdio.h /usr/include/time.h /usr/include/_structs.h \
 /usr/include/sys/_structs.h /usr/include/_wctype.h /usr/include/ctype.h \
 /usr/include/runetype.h /opt/local/include/gcc46/c++/bits/allocator.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++allocator.h \
 /opt/local/include/gcc46/c++/ext/new_allocator.h \
 /opt/local/include/gcc46/c++/new /opt/local/include/gcc46/c++/exception \
 /opt/local/include/gcc46/c++/bits/localefwd.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++locale.h \
 /opt/local/include/gcc46/c++/clocale /usr/include/locale.h \
 /usr/include/_locale.h /opt/local/include/gcc46/c++/iosfwd \
 /opt/local/include/gcc46/c++/cctype \
 /opt/local/include/gcc46/c++/bits/ostream_insert.h \
 /opt/local/include/gcc46/c++/bits/cxxabi_forced.h \
 /opt/local/include/gcc46/c++/bits/stl_function.h \
 /opt/local/include/gcc46/c++/backward/binders.h \
 /opt/local/include/gcc46/c++/bits/range_access.h \
 /opt/local/include/gcc46/c++/bits/basic_string.h \
 /opt/local/include/gcc46/c++/ext/atomicity.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/gthr.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/gthr-default.h \
 /usr/include/pthread.h /usr/include/pthread_impl.h /usr/include/sched.h \
 /usr/include/unistd.h /usr/include/sys/unistd.h \
 /usr/include/sys/select.h /usr/include/sys/appleapiopts.h \
 /usr/include/sys/_select.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/atomic_word.h \
 /opt/local/include/gcc46/c++/initializer_list \
 /opt/local/include/gcc46/c++/bits/basic_string.tcc sli_name.h \
 /opt/local/include/gcc46/c++/cassert /usr/include/assert.h \
 /usr/include/stdlib.h /usr/include/sys/wait.h /usr/include/sys/signal.h \
 /usr/include/machine/signal.h /usr/include/i386/signal.h \
 /usr/include/i386/_structs.h /usr/include/machine/_structs.h \
 /usr/include/mach/i386/_structs.h /usr/include/sys/resource.h \
 /usr/include/machine/endian.h /usr/include/i386/endian.h \
 /usr/include/sys/_endian.h /usr/include/libkern/_OSByteOrder.h \
 /usr/include/libkern/i386/_OSByteOrder.h /usr/include/alloca.h \
 /usr/include/machine/types.h /usr/include/i386/types.h \
 /opt/local/include/gcc46/c++/map \
 /opt/local/include/gcc46/c++/bits/stl_tree.h \
 /opt/local/include/gcc46/c++/bits/stl_map.h \
 /opt/local/include/gcc46/c++/bits/stl_multimap.h \
 /opt/local/include/gcc46/c++/deque \
 /opt/local/include/gcc46/c++/bits/stl_construct.h \
 /opt/local/include/gcc46/c++/bits/stl_uninitialized.h \
 /opt/local/include/gcc46/c++/bits/stl_deque.h \
 /opt/local/include/gcc46/c++/bits/deque.tcc \
 /opt/local/include/gcc46/c++/iostream \
 /opt/local/include/gcc46/c++/ostream /opt/local/include/gcc46/c++/ios \
 /opt/local/include/gcc46/c++/bits/ios_base.h \
 /opt/local/include/gcc46/c++/bits/locale_classes.h \
 /opt/local/include/gcc46/c++/bits/locale_classes.tcc \
 /opt/local/include/gcc46/c++/streambuf \
 /opt/local/include/gcc46/c++/bits/streambuf.tcc \
 /opt/local/include/gcc46/c++/bits/basic_ios.h \
 /opt/local/include/gcc46/c++/bits/locale_facets.h \
 /opt/local/include/gcc46/c++/cwctype /usr/include/wctype.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/ctype_base.h \
 /opt/local/include/gcc46/c++/bits/streambuf_iterator.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/ctype_inline.h \
 /opt/local/include/gcc46/c++/bits/locale_facets.tcc \
 /opt/local/include/gcc46/c++/bits/basic_ios.tcc \
 /opt/local/include/gcc46/c++/bits/ostream.tcc \
 /opt/local/include/gcc46/c++/istream \
 /opt/local/include/gcc46/c++/bits/istream.tcc sli_token.h \
 sli_exceptions.h /opt/local/include/gcc46/c++/vector \
 /opt/local/include/gcc46/c++/bits/stl_vector.h \
 /opt/local/include/gcc46/c++/bits/stl_bvector.h \
 /opt/local/include/gcc46/c++/bits/vector.tcc sli_allocator.h \
 /opt/local/include/gcc46/c++/cstdlib sli_arraytype.h sli_array.h \
 /opt/local/include/gcc46/c++/typeinfo \
 /opt/local/include/gcc46/c++/cstddef \
 /opt/local/lib/gcc46/gcc/x86_64-apple-darwin11/4.6.3/include/stddef.h \
 sli_integertype.h SLI_token.h sli_tokenstack.h sli_string.h \
 sli_dictionary.h sli_dictstack.h /opt/local/include/gcc46/c++/list \
 /opt/local/include/gcc46/c++/bits/stl_list.h \
 /opt/local/include/gcc46/c++/bits/list.tcc sli_builtins.h sli_function.h \
 sli_scanner.h sli_charcode.h sli_parser.h sli_nametype.h
sli_interpreter.o: sli_interpreter.cpp sli_interpreter.h sli_type.h \
 /opt/local/include/gcc46/c++/string \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++config.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/os_defines.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/cpu_defines.h \
 /opt/local/include/gcc46/c++/bits/stringfwd.h \
 /opt/local/include/gcc46/c++/bits/char_traits.h \
 /opt/local/include/gcc46/c++/bits/stl_algobase.h \
 /opt/local/include/gcc46/c++/bits/functexcept.h \
 /opt/local/include/gcc46/c++/bits/exception_defines.h \
 /opt/local/include/gcc46/c++/bits/cpp_type_traits.h \
 /opt/local/include/gcc46/c++/ext/type_traits.h \
 /opt/local/include/gcc46/c++/ext/numeric_traits.h \
 /opt/local/include/gcc46/c++/bits/stl_pair.h \
 /opt/local/include/gcc46/c++/bits/move.h \
 /opt/local/include/gcc46/c++/bits/concept_check.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator_base_types.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator_base_funcs.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator.h \
 /opt/local/include/gcc46/c++/debug/debug.h \
 /opt/local/include/gcc46/c++/bits/postypes.h \
 /opt/local/include/gcc46/c++/cwchar /usr/include/wchar.h \
 /usr/include/_types.h /usr/include/sys/_types.h /usr/include/sys/cdefs.h \
 /usr/include/sys/_symbol_aliasing.h \
 /usr/include/sys/_posix_availability.h /usr/include/machine/_types.h \
 /usr/include/i386/_types.h /usr/include/Availability.h \
 /usr/include/AvailabilityInternal.h \
 /opt/local/lib/gcc46/gcc/x86_64-apple-darwin11/4.6.3/include/stdarg.h \
 /usr/include/stdio.h /usr/include/time.h /usr/include/_structs.h \
 /usr/include/sys/_structs.h /usr/include/_wctype.h /usr/include/ctype.h \
 /usr/include/runetype.h /opt/local/include/gcc46/c++/bits/allocator.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++allocator.h \
 /opt/local/include/gcc46/c++/ext/new_allocator.h \
 /opt/local/include/gcc46/c++/new /opt/local/include/gcc46/c++/exception \
 /opt/local/include/gcc46/c++/bits/localefwd.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++locale.h \
 /opt/local/include/gcc46/c++/clocale /usr/include/locale.h \
 /usr/include/_locale.h /opt/local/include/gcc46/c++/iosfwd \
 /opt/local/include/gcc46/c++/cctype \
 /opt/local/include/gcc46/c++/bits/ostream_insert.h \
 /opt/local/include/gcc46/c++/bits/cxxabi_forced.h \
 /opt/local/include/gcc46/c++/bits/stl_function.h \
 /opt/local/include/gcc46/c++/backward/binders.h \
 /opt/local/include/gcc46/c++/bits/range_access.h \
 /opt/local/include/gcc46/c++/bits/basic_string.h \
 /opt/local/include/gcc46/c++/ext/atomicity.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/gthr.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/gthr-default.h \
 /usr/include/pthread.h /usr/include/pthread_impl.h /usr/include/sched.h \
 /usr/include/unistd.h /usr/include/sys/unistd.h \
 /usr/include/sys/select.h /usr/include/sys/appleapiopts.h \
 /usr/include/sys/_select.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/atomic_word.h \
 /opt/local/include/gcc46/c++/initializer_list \
 /opt/local/include/gcc46/c++/bits/basic_string.tcc sli_name.h \
 /opt/local/include/gcc46/c++/cassert /usr/include/assert.h \
 /usr/include/stdlib.h /usr/include/sys/wait.h /usr/include/sys/signal.h \
 /usr/include/machine/signal.h /usr/include/i386/signal.h \
 /usr/include/i386/_structs.h /usr/include/machine/_structs.h \
 /usr/include/mach/i386/_structs.h /usr/include/sys/resource.h \
 /usr/include/machine/endian.h /usr/include/i386/endian.h \
 /usr/include/sys/_endian.h /usr/include/libkern/_OSByteOrder.h \
 /usr/include/libkern/i386/_OSByteOrder.h /usr/include/alloca.h \
 /usr/include/machine/types.h /usr/include/i386/types.h \
 /opt/local/include/gcc46/c++/map \
 /opt/local/include/gcc46/c++/bits/stl_tree.h \
 /opt/local/include/gcc46/c++/bits/stl_map.h \
 /opt/local/include/gcc46/c++/bits/stl_multimap.h \
 /opt/local/include/gcc46/c++/deque \
 /opt/local/include/gcc46/c++/bits/stl_construct.h \
 /opt/local/include/gcc46/c++/bits/stl_uninitialized.h \
 /opt/local/include/gcc46/c++/bits/stl_deque.h \
 /opt/local/include/gcc46/c++/bits/deque.tcc \
 /opt/local/include/gcc46/c++/iostream \
 /opt/local/include/gcc46/c++/ostream /opt/local/include/gcc46/c++/ios \
 /opt/local/include/gcc46/c++/bits/ios_base.h \
 /opt/local/include/gcc46/c++/bits/locale_classes.h \
 /opt/local/include/gcc46/c++/bits/locale_classes.tcc \
 /opt/local/include/gcc46/c++/streambuf \
 /opt/local/include/gcc46/c++/bits/streambuf.tcc \
 /opt/local/include/gcc46/c++/bits/basic_ios.h \
 /opt/local/include/gcc46/c++/bits/locale_facets.h \
 /opt/local/include/gcc46/c++/cwctype /usr/include/wctype.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/ctype_base.h \
 /opt/local/include/gcc46/c++/bits/streambuf_iterator.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/ctype_inline.h \
 /opt/local/include/gcc46/c++/bits/locale_facets.tcc \
 /opt/local/include/gcc46/c++/bits/basic_ios.tcc \
 /opt/local/include/gcc46/c++/bits/ostream.tcc \
 /opt/local/include/gcc46/c++/istream \
 /opt/local/include/gcc46/c++/bits/istream.tcc sli_token.h \
 sli_exceptions.h /opt/local/include/gcc46/c++/vector \
 /opt/local/include/gcc46/c++/bits/stl_vector.h \
 /opt/local/include/gcc46/c++/bits/stl_bvector.h \
 /opt/local/include/gcc46/c++/bits/vector.tcc sli_allocator.h \
 /opt/local/include/gcc46/c++/cstdlib sli_arraytype.h sli_array.h \
 /opt/local/include/gcc46/c++/typeinfo \
 /opt/local/include/gcc46/c++/cstddef \
 /opt/local/lib/gcc46/gcc/x86_64-apple-darwin11/4.6.3/include/stddef.h \
 sli_integertype.h SLI_token.h sli_tokenstack.h sli_string.h \
 sli_dictionary.h sli_dictstack.h /opt/local/include/gcc46/c++/list \
 /opt/local/include/gcc46/c++/bits/stl_list.h \
 /opt/local/include/gcc46/c++/bits/list.tcc sli_builtins.h sli_function.h \
 sli_nametype.h sli_dicttype.h sli_functiontype.h sli_stringtype.h \
 compose.hpp /opt/local/include/gcc46/c++/sstream \
 /opt/local/include/gcc46/c++/bits/sstream.tcc sli_numerics.h \
 /opt/local/include/gcc46/c++/limits /opt/local/include/gcc46/c++/cmath \
 /usr/include/math.h /usr/include/architecture/i386/math.h sli_parser.h \
 sli_scanner.h sli_charcode.h sli_control.h
sli_numerics.o: sli_numerics.cpp config.h sli_numerics.h \
 /opt/local/include/gcc46/c++/limits \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++config.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/os_defines.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/cpu_defines.h \
 /opt/local/include/gcc46/c++/cmath \
 /opt/local/include/gcc46/c++/bits/cpp_type_traits.h \
 /opt/local/include/gcc46/c++/ext/type_traits.h /usr/include/math.h \
 /usr/include/architecture/i386/math.h /usr/include/sys/cdefs.h \
 /usr/include/sys/_symbol_aliasing.h \
 /usr/include/sys/_posix_availability.h
sli_builtins.o: sli_builtins.cpp sli_builtins.h sli_function.h sli_name.h \
 /opt/local/include/gcc46/c++/cassert /usr/include/assert.h \
 /usr/include/sys/cdefs.h /usr/include/sys/_symbol_aliasing.h \
 /usr/include/sys/_posix_availability.h /usr/include/stdlib.h \
 /usr/include/Availability.h /usr/include/AvailabilityInternal.h \
 /usr/include/_types.h /usr/include/sys/_types.h \
 /usr/include/machine/_types.h /usr/include/i386/_types.h \
 /usr/include/sys/wait.h /usr/include/sys/signal.h \
 /usr/include/sys/appleapiopts.h /usr/include/machine/signal.h \
 /usr/include/i386/signal.h /usr/include/i386/_structs.h \
 /usr/include/sys/_structs.h /usr/include/machine/_structs.h \
 /usr/include/mach/i386/_structs.h /usr/include/sys/resource.h \
 /usr/include/machine/endian.h /usr/include/i386/endian.h \
 /usr/include/sys/_endian.h /usr/include/libkern/_OSByteOrder.h \
 /usr/include/libkern/i386/_OSByteOrder.h /usr/include/alloca.h \
 /usr/include/machine/types.h /usr/include/i386/types.h \
 /opt/local/include/gcc46/c++/map \
 /opt/local/include/gcc46/c++/bits/stl_tree.h \
 /opt/local/include/gcc46/c++/bits/stl_algobase.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++config.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/os_defines.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/cpu_defines.h \
 /opt/local/include/gcc46/c++/bits/functexcept.h \
 /opt/local/include/gcc46/c++/bits/exception_defines.h \
 /opt/local/include/gcc46/c++/bits/cpp_type_traits.h \
 /opt/local/include/gcc46/c++/ext/type_traits.h \
 /opt/local/include/gcc46/c++/ext/numeric_traits.h \
 /opt/local/include/gcc46/c++/bits/stl_pair.h \
 /opt/local/include/gcc46/c++/bits/move.h \
 /opt/local/include/gcc46/c++/bits/concept_check.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator_base_types.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator_base_funcs.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator.h \
 /opt/local/include/gcc46/c++/debug/debug.h \
 /opt/local/include/gcc46/c++/bits/allocator.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++allocator.h \
 /opt/local/include/gcc46/c++/ext/new_allocator.h \
 /opt/local/include/gcc46/c++/new /opt/local/include/gcc46/c++/exception \
 /opt/local/include/gcc46/c++/bits/stl_function.h \
 /opt/local/include/gcc46/c++/backward/binders.h \
 /opt/local/include/gcc46/c++/bits/stl_map.h \
 /opt/local/include/gcc46/c++/initializer_list \
 /opt/local/include/gcc46/c++/bits/stl_multimap.h \
 /opt/local/include/gcc46/c++/bits/range_access.h \
 /opt/local/include/gcc46/c++/string \
 /opt/local/include/gcc46/c++/bits/stringfwd.h \
 /opt/local/include/gcc46/c++/bits/char_traits.h \
 /opt/local/include/gcc46/c++/bits/postypes.h \
 /opt/local/include/gcc46/c++/cwchar /usr/include/wchar.h \
 /opt/local/lib/gcc46/gcc/x86_64-apple-darwin11/4.6.3/include/stdarg.h \
 /usr/include/stdio.h /usr/include/time.h /usr/include/_structs.h \
 /usr/include/_wctype.h /usr/include/ctype.h /usr/include/runetype.h \
 /opt/local/include/gcc46/c++/bits/localefwd.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++locale.h \
 /opt/local/include/gcc46/c++/clocale /usr/include/locale.h \
 /usr/include/_locale.h /opt/local/include/gcc46/c++/iosfwd \
 /opt/local/include/gcc46/c++/cctype \
 /opt/local/include/gcc46/c++/bits/ostream_insert.h \
 /opt/local/include/gcc46/c++/bits/cxxabi_forced.h \
 /opt/local/include/gcc46/c++/bits/basic_string.h \
 /opt/local/include/gcc46/c++/ext/atomicity.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/gthr.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/gthr-default.h \
 /usr/include/pthread.h /usr/include/pthread_impl.h /usr/include/sched.h \
 /usr/include/unistd.h /usr/include/sys/unistd.h \
 /usr/include/sys/select.h /usr/include/sys/_select.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/atomic_word.h \
 /opt/local/include/gcc46/c++/bits/basic_string.tcc \
 /opt/local/include/gcc46/c++/deque \
 /opt/local/include/gcc46/c++/bits/stl_construct.h \
 /opt/local/include/gcc46/c++/bits/stl_uninitialized.h \
 /opt/local/include/gcc46/c++/bits/stl_deque.h \
 /opt/local/include/gcc46/c++/bits/deque.tcc \
 /opt/local/include/gcc46/c++/iostream \
 /opt/local/include/gcc46/c++/ostream /opt/local/include/gcc46/c++/ios \
 /opt/local/include/gcc46/c++/bits/ios_base.h \
 /opt/local/include/gcc46/c++/bits/locale_classes.h \
 /opt/local/include/gcc46/c++/bits/locale_classes.tcc \
 /opt/local/include/gcc46/c++/streambuf \
 /opt/local/include/gcc46/c++/bits/streambuf.tcc \
 /opt/local/include/gcc46/c++/bits/basic_ios.h \
 /opt/local/include/gcc46/c++/bits/locale_facets.h \
 /opt/local/include/gcc46/c++/cwctype /usr/include/wctype.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/ctype_base.h \
 /opt/local/include/gcc46/c++/bits/streambuf_iterator.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/ctype_inline.h \
 /opt/local/include/gcc46/c++/bits/locale_facets.tcc \
 /opt/local/include/gcc46/c++/bits/basic_ios.tcc \
 /opt/local/include/gcc46/c++/bits/ostream.tcc \
 /opt/local/include/gcc46/c++/istream \
 /opt/local/include/gcc46/c++/bits/istream.tcc sli_interpreter.h \
 sli_type.h sli_token.h sli_exceptions.h \
 /opt/local/include/gcc46/c++/vector \
 /opt/local/include/gcc46/c++/bits/stl_vector.h \
 /opt/local/include/gcc46/c++/bits/stl_bvector.h \
 /opt/local/include/gcc46/c++/bits/vector.tcc sli_allocator.h \
 /opt/local/include/gcc46/c++/cstdlib sli_arraytype.h sli_array.h \
 /opt/local/include/gcc46/c++/typeinfo \
 /opt/local/include/gcc46/c++/cstddef \
 /opt/local/lib/gcc46/gcc/x86_64-apple-darwin11/4.6.3/include/stddef.h \
 sli_integertype.h SLI_token.h sli_tokenstack.h sli_string.h \
 sli_dictionary.h sli_dictstack.h /opt/local/include/gcc46/c++/list \
 /opt/local/include/gcc46/c++/bits/stl_list.h \
 /opt/local/include/gcc46/c++/bits/list.tcc sli_iostream.h sli_lockptr.h
sli_control.o: sli_control.cpp sli_control.h sli_interpreter.h sli_type.h \
 /opt/local/include/gcc46/c++/string \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++config.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/os_defines.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/cpu_defines.h \
 /opt/local/include/gcc46/c++/bits/stringfwd.h \
 /opt/local/include/gcc46/c++/bits/char_traits.h \
 /opt/local/include/gcc46/c++/bits/stl_algobase.h \
 /opt/local/include/gcc46/c++/bits/functexcept.h \
 /opt/local/include/gcc46/c++/bits/exception_defines.h \
 /opt/local/include/gcc46/c++/bits/cpp_type_traits.h \
 /opt/local/include/gcc46/c++/ext/type_traits.h \
 /opt/local/include/gcc46/c++/ext/numeric_traits.h \
 /opt/local/include/gcc46/c++/bits/stl_pair.h \
 /opt/local/include/gcc46/c++/bits/move.h \
 /opt/local/include/gcc46/c++/bits/concept_check.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator_base_types.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator_base_funcs.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator.h \
 /opt/local/include/gcc46/c++/debug/debug.h \
 /opt/local/include/gcc46/c++/bits/postypes.h \
 /opt/local/include/gcc46/c++/cwchar /usr/include/wchar.h \
 /usr/include/_types.h /usr/include/sys/_types.h /usr/include/sys/cdefs.h \
 /usr/include/sys/_symbol_aliasing.h \
 /usr/include/sys/_posix_availability.h /usr/include/machine/_types.h \
 /usr/include/i386/_types.h /usr/include/Availability.h \
 /usr/include/AvailabilityInternal.h \
 /opt/local/lib/gcc46/gcc/x86_64-apple-darwin11/4.6.3/include/stdarg.h \
 /usr/include/stdio.h /usr/include/time.h /usr/include/_structs.h \
 /usr/include/sys/_structs.h /usr/include/_wctype.h /usr/include/ctype.h \
 /usr/include/runetype.h /opt/local/include/gcc46/c++/bits/allocator.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++allocator.h \
 /opt/local/include/gcc46/c++/ext/new_allocator.h \
 /opt/local/include/gcc46/c++/new /opt/local/include/gcc46/c++/exception \
 /opt/local/include/gcc46/c++/bits/localefwd.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++locale.h \
 /opt/local/include/gcc46/c++/clocale /usr/include/locale.h \
 /usr/include/_locale.h /opt/local/include/gcc46/c++/iosfwd \
 /opt/local/include/gcc46/c++/cctype \
 /opt/local/include/gcc46/c++/bits/ostream_insert.h \
 /opt/local/include/gcc46/c++/bits/cxxabi_forced.h \
 /opt/local/include/gcc46/c++/bits/stl_function.h \
 /opt/local/include/gcc46/c++/backward/binders.h \
 /opt/local/include/gcc46/c++/bits/range_access.h \
 /opt/local/include/gcc46/c++/bits/basic_string.h \
 /opt/local/include/gcc46/c++/ext/atomicity.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/gthr.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/gthr-default.h \
 /usr/include/pthread.h /usr/include/pthread_impl.h /usr/include/sched.h \
 /usr/include/unistd.h /usr/include/sys/unistd.h \
 /usr/include/sys/select.h /usr/include/sys/appleapiopts.h \
 /usr/include/sys/_select.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/atomic_word.h \
 /opt/local/include/gcc46/c++/initializer_list \
 /opt/local/include/gcc46/c++/bits/basic_string.tcc sli_name.h \
 /opt/local/include/gcc46/c++/cassert /usr/include/assert.h \
 /usr/include/stdlib.h /usr/include/sys/wait.h /usr/include/sys/signal.h \
 /usr/include/machine/signal.h /usr/include/i386/signal.h \
 /usr/include/i386/_structs.h /usr/include/machine/_structs.h \
 /usr/include/mach/i386/_structs.h /usr/include/sys/resource.h \
 /usr/include/machine/endian.h /usr/include/i386/endian.h \
 /usr/include/sys/_endian.h /usr/include/libkern/_OSByteOrder.h \
 /usr/include/libkern/i386/_OSByteOrder.h /usr/include/alloca.h \
 /usr/include/machine/types.h /usr/include/i386/types.h \
 /opt/local/include/gcc46/c++/map \
 /opt/local/include/gcc46/c++/bits/stl_tree.h \
 /opt/local/include/gcc46/c++/bits/stl_map.h \
 /opt/local/include/gcc46/c++/bits/stl_multimap.h \
 /opt/local/include/gcc46/c++/deque \
 /opt/local/include/gcc46/c++/bits/stl_construct.h \
 /opt/local/include/gcc46/c++/bits/stl_uninitialized.h \
 /opt/local/include/gcc46/c++/bits/stl_deque.h \
 /opt/local/include/gcc46/c++/bits/deque.tcc \
 /opt/local/include/gcc46/c++/iostream \
 /opt/local/include/gcc46/c++/ostream /opt/local/include/gcc46/c++/ios \
 /opt/local/include/gcc46/c++/bits/ios_base.h \
 /opt/local/include/gcc46/c++/bits/locale_classes.h \
 /opt/local/include/gcc46/c++/bits/locale_classes.tcc \
 /opt/local/include/gcc46/c++/streambuf \
 /opt/local/include/gcc46/c++/bits/streambuf.tcc \
 /opt/local/include/gcc46/c++/bits/basic_ios.h \
 /opt/local/include/gcc46/c++/bits/locale_facets.h \
 /opt/local/include/gcc46/c++/cwctype /usr/include/wctype.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/ctype_base.h \
 /opt/local/include/gcc46/c++/bits/streambuf_iterator.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/ctype_inline.h \
 /opt/local/include/gcc46/c++/bits/locale_facets.tcc \
 /opt/local/include/gcc46/c++/bits/basic_ios.tcc \
 /opt/local/include/gcc46/c++/bits/ostream.tcc \
 /opt/local/include/gcc46/c++/istream \
 /opt/local/include/gcc46/c++/bits/istream.tcc sli_token.h \
 sli_exceptions.h /opt/local/include/gcc46/c++/vector \
 /opt/local/include/gcc46/c++/bits/stl_vector.h \
 /opt/local/include/gcc46/c++/bits/stl_bvector.h \
 /opt/local/include/gcc46/c++/bits/vector.tcc sli_allocator.h \
 /opt/local/include/gcc46/c++/cstdlib sli_arraytype.h sli_array.h \
 /opt/local/include/gcc46/c++/typeinfo \
 /opt/local/include/gcc46/c++/cstddef \
 /opt/local/lib/gcc46/gcc/x86_64-apple-darwin11/4.6.3/include/stddef.h \
 sli_integertype.h SLI_token.h sli_tokenstack.h sli_string.h \
 sli_dictionary.h sli_dictstack.h /opt/local/include/gcc46/c++/list \
 /opt/local/include/gcc46/c++/bits/stl_list.h \
 /opt/local/include/gcc46/c++/bits/list.tcc sli_builtins.h sli_function.h \
 sli_scanner.h sli_charcode.h sli_parser.h sli_iostreamtype.h \
 sli_iostream.h sli_lockptr.h sli_stringtype.h sli_functiontype.h \
 /usr/include/sys/time.h /usr/include/sys/times.h \
 /opt/local/include/gcc46/c++/sstream \
 /opt/local/include/gcc46/c++/bits/sstream.tcc
sli_iostreamtype.o: sli_iostreamtype.cpp sli_functiontype.h sli_type.h \
 /opt/local/include/gcc46/c++/string \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++config.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/os_defines.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/cpu_defines.h \
 /opt/local/include/gcc46/c++/bits/stringfwd.h \
 /opt/local/include/gcc46/c++/bits/char_traits.h \
 /opt/local/include/gcc46/c++/bits/stl_algobase.h \
 /opt/local/include/gcc46/c++/bits/functexcept.h \
 /opt/local/include/gcc46/c++/bits/exception_defines.h \
 /opt/local/include/gcc46/c++/bits/cpp_type_traits.h \
 /opt/local/include/gcc46/c++/ext/type_traits.h \
 /opt/local/include/gcc46/c++/ext/numeric_traits.h \
 /opt/local/include/gcc46/c++/bits/stl_pair.h \
 /opt/local/include/gcc46/c++/bits/move.h \
 /opt/local/include/gcc46/c++/bits/concept_check.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator_base_types.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator_base_funcs.h \
 /opt/local/include/gcc46/c++/bits/stl_iterator.h \
 /opt/local/include/gcc46/c++/debug/debug.h \
 /opt/local/include/gcc46/c++/bits/postypes.h \
 /opt/local/include/gcc46/c++/cwchar /usr/include/wchar.h \
 /usr/include/_types.h /usr/include/sys/_types.h /usr/include/sys/cdefs.h \
 /usr/include/sys/_symbol_aliasing.h \
 /usr/include/sys/_posix_availability.h /usr/include/machine/_types.h \
 /usr/include/i386/_types.h /usr/include/Availability.h \
 /usr/include/AvailabilityInternal.h \
 /opt/local/lib/gcc46/gcc/x86_64-apple-darwin11/4.6.3/include/stdarg.h \
 /usr/include/stdio.h /usr/include/time.h /usr/include/_structs.h \
 /usr/include/sys/_structs.h /usr/include/_wctype.h /usr/include/ctype.h \
 /usr/include/runetype.h /opt/local/include/gcc46/c++/bits/allocator.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++allocator.h \
 /opt/local/include/gcc46/c++/ext/new_allocator.h \
 /opt/local/include/gcc46/c++/new /opt/local/include/gcc46/c++/exception \
 /opt/local/include/gcc46/c++/bits/localefwd.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/c++locale.h \
 /opt/local/include/gcc46/c++/clocale /usr/include/locale.h \
 /usr/include/_locale.h /opt/local/include/gcc46/c++/iosfwd \
 /opt/local/include/gcc46/c++/cctype \
 /opt/local/include/gcc46/c++/bits/ostream_insert.h \
 /opt/local/include/gcc46/c++/bits/cxxabi_forced.h \
 /opt/local/include/gcc46/c++/bits/stl_function.h \
 /opt/local/include/gcc46/c++/backward/binders.h \
 /opt/local/include/gcc46/c++/bits/range_access.h \
 /opt/local/include/gcc46/c++/bits/basic_string.h \
 /opt/local/include/gcc46/c++/ext/atomicity.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/gthr.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/gthr-default.h \
 /usr/include/pthread.h /usr/include/pthread_impl.h /usr/include/sched.h \
 /usr/include/unistd.h /usr/include/sys/unistd.h \
 /usr/include/sys/select.h /usr/include/sys/appleapiopts.h \
 /usr/include/sys/_select.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/atomic_word.h \
 /opt/local/include/gcc46/c++/initializer_list \
 /opt/local/include/gcc46/c++/bits/basic_string.tcc sli_name.h \
 /opt/local/include/gcc46/c++/cassert /usr/include/assert.h \
 /usr/include/stdlib.h /usr/include/sys/wait.h /usr/include/sys/signal.h \
 /usr/include/machine/signal.h /usr/include/i386/signal.h \
 /usr/include/i386/_structs.h /usr/include/machine/_structs.h \
 /usr/include/mach/i386/_structs.h /usr/include/sys/resource.h \
 /usr/include/machine/endian.h /usr/include/i386/endian.h \
 /usr/include/sys/_endian.h /usr/include/libkern/_OSByteOrder.h \
 /usr/include/libkern/i386/_OSByteOrder.h /usr/include/alloca.h \
 /usr/include/machine/types.h /usr/include/i386/types.h \
 /opt/local/include/gcc46/c++/map \
 /opt/local/include/gcc46/c++/bits/stl_tree.h \
 /opt/local/include/gcc46/c++/bits/stl_map.h \
 /opt/local/include/gcc46/c++/bits/stl_multimap.h \
 /opt/local/include/gcc46/c++/deque \
 /opt/local/include/gcc46/c++/bits/stl_construct.h \
 /opt/local/include/gcc46/c++/bits/stl_uninitialized.h \
 /opt/local/include/gcc46/c++/bits/stl_deque.h \
 /opt/local/include/gcc46/c++/bits/deque.tcc \
 /opt/local/include/gcc46/c++/iostream \
 /opt/local/include/gcc46/c++/ostream /opt/local/include/gcc46/c++/ios \
 /opt/local/include/gcc46/c++/bits/ios_base.h \
 /opt/local/include/gcc46/c++/bits/locale_classes.h \
 /opt/local/include/gcc46/c++/bits/locale_classes.tcc \
 /opt/local/include/gcc46/c++/streambuf \
 /opt/local/include/gcc46/c++/bits/streambuf.tcc \
 /opt/local/include/gcc46/c++/bits/basic_ios.h \
 /opt/local/include/gcc46/c++/bits/locale_facets.h \
 /opt/local/include/gcc46/c++/cwctype /usr/include/wctype.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/ctype_base.h \
 /opt/local/include/gcc46/c++/bits/streambuf_iterator.h \
 /opt/local/include/gcc46/c++//x86_64-apple-darwin11/bits/ctype_inline.h \
 /opt/local/include/gcc46/c++/bits/locale_facets.tcc \
 /opt/local/include/gcc46/c++/bits/basic_ios.tcc \
 /opt/local/include/gcc46/c++/bits/ostream.tcc \
 /opt/local/include/gcc46/c++/istream \
 /opt/local/include/gcc46/c++/bits/istream.tcc sli_interpreter.h \
 sli_token.h sli_exceptions.h /opt/local/include/gcc46/c++/vector \
 /opt/local/include/gcc46/c++/bits/stl_vector.h \
 /opt/local/include/gcc46/c++/bits/stl_bvector.h \
 /opt/local/include/gcc46/c++/bits/vector.tcc sli_allocator.h \
 /opt/local/include/gcc46/c++/cstdlib sli_arraytype.h sli_array.h \
 /opt/local/include/gcc46/c++/typeinfo \
 /opt/local/include/gcc46/c++/cstddef \
 /opt/local/lib/gcc46/gcc/x86_64-apple-darwin11/4.6.3/include/stddef.h \
 sli_integertype.h SLI_token.h sli_tokenstack.h sli_string.h \
 sli_dictionary.h sli_dictstack.h /opt/local/include/gcc46/c++/list \
 /opt/local/include/gcc46/c++/bits/stl_list.h \
 /opt/local/include/gcc46/c++/bits/list.tcc sli_builtins.h sli_function.h \
 sli_iostreamtype.h
