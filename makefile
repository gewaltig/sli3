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
CC=clang
CXX=clang
GDB=-g
CCFLAGS= -O2 $(GDB) -Wall
#CXXFLAGS= -O3 $(GDB) -I/opt/local/include -Wall -pedantic

CXXFLAGS= -O3 -Wall -pedantic

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
	sli_iostreamtype.o\
	sli_stack.o\
	sli_type_trie.o\
	sli_typecheck.o\
	sli_math.o

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
	@ g++ ${CXXFLAGS} -M ${SOURCES} >> $@
	@ chmod -w $@

clean:
	@ echo 'Removing all binary files.'
	@ rm -f *.o sli test_token test_dictionary

distclean: clean
	@ rm -f makefile
# Automatically-generated dependency list:
sli_array.o: sli_array.cpp sli_array.h /usr/include/c++/4.2.1/typeinfo \
  /usr/include/c++/4.2.1/exception \
  /usr/include/c++/4.2.1/bits/c++config.h \
  /usr/include/c++/4.2.1/bits/os_defines.h /usr/include/unistd.h \
  /usr/include/_types.h /usr/include/sys/_types.h \
  /usr/include/sys/cdefs.h /usr/include/sys/_symbol_aliasing.h \
  /usr/include/sys/_posix_availability.h /usr/include/machine/_types.h \
  /usr/include/i386/_types.h /usr/include/sys/unistd.h \
  /usr/include/Availability.h /usr/include/AvailabilityInternal.h \
  /usr/include/sys/select.h /usr/include/sys/appleapiopts.h \
  /usr/include/sys/_structs.h /usr/include/sys/_select.h \
  /usr/include/c++/4.2.1/bits/cpu_defines.h \
  /usr/include/c++/4.2.1/cstddef \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/stddef.h \
  sli_token.h /usr/include/c++/4.2.1/iostream \
  /usr/include/c++/4.2.1/ostream /usr/include/c++/4.2.1/ios \
  /usr/include/c++/4.2.1/iosfwd /usr/include/c++/4.2.1/bits/c++locale.h \
  /usr/include/c++/4.2.1/clocale /usr/include/locale.h \
  /usr/include/_locale.h /usr/include/c++/4.2.1/cstring \
  /usr/include/string.h /usr/include/strings.h \
  /usr/include/c++/4.2.1/cstdio /usr/include/stdio.h \
  /usr/include/c++/4.2.1/cstdarg \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/stdarg.h \
  /usr/include/c++/4.2.1/bits/c++io.h /usr/include/c++/4.2.1/bits/gthr.h \
  /usr/include/c++/4.2.1/bits/gthr-default.h /usr/include/pthread.h \
  /usr/include/pthread_impl.h /usr/include/sched.h /usr/include/time.h \
  /usr/include/_structs.h /usr/include/c++/4.2.1/cctype \
  /usr/include/ctype.h /usr/include/runetype.h \
  /usr/include/c++/4.2.1/bits/stringfwd.h \
  /usr/include/c++/4.2.1/bits/postypes.h /usr/include/c++/4.2.1/cwchar \
  /usr/include/c++/4.2.1/ctime /usr/include/wchar.h \
  /usr/include/_wctype.h /usr/include/stdint.h \
  /usr/include/c++/4.2.1/bits/functexcept.h \
  /usr/include/c++/4.2.1/exception_defines.h \
  /usr/include/c++/4.2.1/bits/char_traits.h \
  /usr/include/c++/4.2.1/bits/stl_algobase.h \
  /usr/include/c++/4.2.1/climits \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/limits.h \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/syslimits.h \
  /usr/include/limits.h /usr/include/machine/limits.h \
  /usr/include/i386/limits.h /usr/include/i386/_limits.h \
  /usr/include/sys/syslimits.h /usr/include/c++/4.2.1/cstdlib \
  /usr/include/stdlib.h /usr/include/sys/wait.h /usr/include/sys/signal.h \
  /usr/include/machine/signal.h /usr/include/i386/signal.h \
  /usr/include/i386/_structs.h /usr/include/machine/_structs.h \
  /usr/include/mach/i386/_structs.h /usr/include/sys/resource.h \
  /usr/include/machine/endian.h /usr/include/i386/endian.h \
  /usr/include/sys/_endian.h /usr/include/libkern/_OSByteOrder.h \
  /usr/include/libkern/i386/_OSByteOrder.h /usr/include/alloca.h \
  /usr/include/machine/types.h /usr/include/i386/types.h \
  /usr/include/i386/_types.h /usr/include/c++/4.2.1/bits/stl_pair.h \
  /usr/include/c++/4.2.1/bits/cpp_type_traits.h \
  /usr/include/c++/4.2.1/ext/type_traits.h /usr/include/c++/4.2.1/utility \
  /usr/include/c++/4.2.1/bits/stl_relops.h \
  /usr/include/c++/4.2.1/bits/stl_iterator_base_types.h \
  /usr/include/c++/4.2.1/bits/stl_iterator_base_funcs.h \
  /usr/include/c++/4.2.1/bits/concept_check.h \
  /usr/include/c++/4.2.1/bits/stl_iterator.h \
  /usr/include/c++/4.2.1/debug/debug.h \
  /usr/include/c++/4.2.1/bits/localefwd.h \
  /usr/include/c++/4.2.1/bits/ios_base.h \
  /usr/include/c++/4.2.1/ext/atomicity.h \
  /usr/include/c++/4.2.1/bits/atomic_word.h \
  /usr/include/c++/4.2.1/bits/locale_classes.h \
  /usr/include/c++/4.2.1/string /usr/include/c++/4.2.1/memory \
  /usr/include/c++/4.2.1/bits/allocator.h \
  /usr/include/c++/4.2.1/bits/c++allocator.h \
  /usr/include/c++/4.2.1/ext/new_allocator.h /usr/include/c++/4.2.1/new \
  /usr/include/c++/4.2.1/bits/stl_construct.h \
  /usr/include/c++/4.2.1/bits/stl_uninitialized.h \
  /usr/include/c++/4.2.1/bits/stl_raw_storage_iter.h \
  /usr/include/c++/4.2.1/limits \
  /usr/include/c++/4.2.1/bits/ostream_insert.h \
  /usr/include/c++/4.2.1/bits/stl_function.h \
  /usr/include/c++/4.2.1/bits/basic_string.h \
  /usr/include/c++/4.2.1/algorithm /usr/include/c++/4.2.1/bits/stl_algo.h \
  /usr/include/c++/4.2.1/bits/stl_heap.h \
  /usr/include/c++/4.2.1/bits/stl_tempbuf.h \
  /usr/include/c++/4.2.1/bits/basic_string.tcc \
  /usr/include/c++/4.2.1/streambuf \
  /usr/include/c++/4.2.1/bits/streambuf.tcc \
  /usr/include/c++/4.2.1/bits/basic_ios.h \
  /usr/include/c++/4.2.1/bits/streambuf_iterator.h \
  /usr/include/c++/4.2.1/bits/locale_facets.h \
  /usr/include/c++/4.2.1/cwctype /usr/include/wctype.h \
  /usr/include/c++/4.2.1/bits/ctype_base.h \
  /usr/include/c++/4.2.1/bits/ctype_inline.h \
  /usr/include/c++/4.2.1/bits/codecvt.h \
  /usr/include/c++/4.2.1/bits/time_members.h \
  /usr/include/c++/4.2.1/bits/messages_members.h \
  /usr/include/c++/4.2.1/bits/basic_ios.tcc \
  /usr/include/c++/4.2.1/bits/ostream.tcc /usr/include/c++/4.2.1/locale \
  /usr/include/c++/4.2.1/bits/locale_facets.tcc \
  /usr/include/c++/4.2.1/istream /usr/include/c++/4.2.1/bits/istream.tcc \
  sli_type.h sli_name.h /usr/include/c++/4.2.1/cassert \
  /usr/include/assert.h /usr/include/c++/4.2.1/map \
  /usr/include/c++/4.2.1/bits/stl_tree.h \
  /usr/include/c++/4.2.1/bits/stl_map.h \
  /usr/include/c++/4.2.1/bits/stl_multimap.h /usr/include/c++/4.2.1/deque \
  /usr/include/c++/4.2.1/bits/stl_deque.h \
  /usr/include/c++/4.2.1/bits/deque.tcc sli_exceptions.h \
  /usr/include/c++/4.2.1/vector /usr/include/c++/4.2.1/bits/stl_vector.h \
  /usr/include/c++/4.2.1/bits/stl_bvector.h \
  /usr/include/c++/4.2.1/bits/vector.tcc sli_allocator.h
sli_token.o: sli_token.cpp sli_token.h /usr/include/c++/4.2.1/iostream \
  /usr/include/c++/4.2.1/bits/c++config.h \
  /usr/include/c++/4.2.1/bits/os_defines.h /usr/include/unistd.h \
  /usr/include/_types.h /usr/include/sys/_types.h \
  /usr/include/sys/cdefs.h /usr/include/sys/_symbol_aliasing.h \
  /usr/include/sys/_posix_availability.h /usr/include/machine/_types.h \
  /usr/include/i386/_types.h /usr/include/sys/unistd.h \
  /usr/include/Availability.h /usr/include/AvailabilityInternal.h \
  /usr/include/sys/select.h /usr/include/sys/appleapiopts.h \
  /usr/include/sys/_structs.h /usr/include/sys/_select.h \
  /usr/include/c++/4.2.1/bits/cpu_defines.h \
  /usr/include/c++/4.2.1/ostream /usr/include/c++/4.2.1/ios \
  /usr/include/c++/4.2.1/iosfwd /usr/include/c++/4.2.1/bits/c++locale.h \
  /usr/include/c++/4.2.1/clocale /usr/include/locale.h \
  /usr/include/_locale.h /usr/include/c++/4.2.1/cstring \
  /usr/include/c++/4.2.1/cstddef \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/stddef.h \
  /usr/include/string.h /usr/include/strings.h \
  /usr/include/c++/4.2.1/cstdio /usr/include/stdio.h \
  /usr/include/c++/4.2.1/cstdarg \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/stdarg.h \
  /usr/include/c++/4.2.1/bits/c++io.h /usr/include/c++/4.2.1/bits/gthr.h \
  /usr/include/c++/4.2.1/bits/gthr-default.h /usr/include/pthread.h \
  /usr/include/pthread_impl.h /usr/include/sched.h /usr/include/time.h \
  /usr/include/_structs.h /usr/include/c++/4.2.1/cctype \
  /usr/include/ctype.h /usr/include/runetype.h \
  /usr/include/c++/4.2.1/bits/stringfwd.h \
  /usr/include/c++/4.2.1/bits/postypes.h /usr/include/c++/4.2.1/cwchar \
  /usr/include/c++/4.2.1/ctime /usr/include/wchar.h \
  /usr/include/_wctype.h /usr/include/stdint.h \
  /usr/include/c++/4.2.1/bits/functexcept.h \
  /usr/include/c++/4.2.1/exception_defines.h \
  /usr/include/c++/4.2.1/exception \
  /usr/include/c++/4.2.1/bits/char_traits.h \
  /usr/include/c++/4.2.1/bits/stl_algobase.h \
  /usr/include/c++/4.2.1/climits \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/limits.h \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/syslimits.h \
  /usr/include/limits.h /usr/include/machine/limits.h \
  /usr/include/i386/limits.h /usr/include/i386/_limits.h \
  /usr/include/sys/syslimits.h /usr/include/c++/4.2.1/cstdlib \
  /usr/include/stdlib.h /usr/include/sys/wait.h /usr/include/sys/signal.h \
  /usr/include/machine/signal.h /usr/include/i386/signal.h \
  /usr/include/i386/_structs.h /usr/include/machine/_structs.h \
  /usr/include/mach/i386/_structs.h /usr/include/sys/resource.h \
  /usr/include/machine/endian.h /usr/include/i386/endian.h \
  /usr/include/sys/_endian.h /usr/include/libkern/_OSByteOrder.h \
  /usr/include/libkern/i386/_OSByteOrder.h /usr/include/alloca.h \
  /usr/include/machine/types.h /usr/include/i386/types.h \
  /usr/include/i386/_types.h /usr/include/c++/4.2.1/bits/stl_pair.h \
  /usr/include/c++/4.2.1/bits/cpp_type_traits.h \
  /usr/include/c++/4.2.1/ext/type_traits.h /usr/include/c++/4.2.1/utility \
  /usr/include/c++/4.2.1/bits/stl_relops.h \
  /usr/include/c++/4.2.1/bits/stl_iterator_base_types.h \
  /usr/include/c++/4.2.1/bits/stl_iterator_base_funcs.h \
  /usr/include/c++/4.2.1/bits/concept_check.h \
  /usr/include/c++/4.2.1/bits/stl_iterator.h \
  /usr/include/c++/4.2.1/debug/debug.h \
  /usr/include/c++/4.2.1/bits/localefwd.h \
  /usr/include/c++/4.2.1/bits/ios_base.h \
  /usr/include/c++/4.2.1/ext/atomicity.h \
  /usr/include/c++/4.2.1/bits/atomic_word.h \
  /usr/include/c++/4.2.1/bits/locale_classes.h \
  /usr/include/c++/4.2.1/string /usr/include/c++/4.2.1/memory \
  /usr/include/c++/4.2.1/bits/allocator.h \
  /usr/include/c++/4.2.1/bits/c++allocator.h \
  /usr/include/c++/4.2.1/ext/new_allocator.h /usr/include/c++/4.2.1/new \
  /usr/include/c++/4.2.1/bits/stl_construct.h \
  /usr/include/c++/4.2.1/bits/stl_uninitialized.h \
  /usr/include/c++/4.2.1/bits/stl_raw_storage_iter.h \
  /usr/include/c++/4.2.1/limits \
  /usr/include/c++/4.2.1/bits/ostream_insert.h \
  /usr/include/c++/4.2.1/bits/stl_function.h \
  /usr/include/c++/4.2.1/bits/basic_string.h \
  /usr/include/c++/4.2.1/algorithm /usr/include/c++/4.2.1/bits/stl_algo.h \
  /usr/include/c++/4.2.1/bits/stl_heap.h \
  /usr/include/c++/4.2.1/bits/stl_tempbuf.h \
  /usr/include/c++/4.2.1/bits/basic_string.tcc \
  /usr/include/c++/4.2.1/streambuf \
  /usr/include/c++/4.2.1/bits/streambuf.tcc \
  /usr/include/c++/4.2.1/bits/basic_ios.h \
  /usr/include/c++/4.2.1/bits/streambuf_iterator.h \
  /usr/include/c++/4.2.1/bits/locale_facets.h \
  /usr/include/c++/4.2.1/cwctype /usr/include/wctype.h \
  /usr/include/c++/4.2.1/bits/ctype_base.h \
  /usr/include/c++/4.2.1/bits/ctype_inline.h \
  /usr/include/c++/4.2.1/bits/codecvt.h \
  /usr/include/c++/4.2.1/bits/time_members.h \
  /usr/include/c++/4.2.1/bits/messages_members.h \
  /usr/include/c++/4.2.1/bits/basic_ios.tcc \
  /usr/include/c++/4.2.1/bits/ostream.tcc /usr/include/c++/4.2.1/locale \
  /usr/include/c++/4.2.1/bits/locale_facets.tcc \
  /usr/include/c++/4.2.1/typeinfo /usr/include/c++/4.2.1/istream \
  /usr/include/c++/4.2.1/bits/istream.tcc sli_type.h sli_name.h \
  /usr/include/c++/4.2.1/cassert /usr/include/assert.h \
  /usr/include/c++/4.2.1/map /usr/include/c++/4.2.1/bits/stl_tree.h \
  /usr/include/c++/4.2.1/bits/stl_map.h \
  /usr/include/c++/4.2.1/bits/stl_multimap.h /usr/include/c++/4.2.1/deque \
  /usr/include/c++/4.2.1/bits/stl_deque.h \
  /usr/include/c++/4.2.1/bits/deque.tcc sli_exceptions.h \
  /usr/include/c++/4.2.1/vector /usr/include/c++/4.2.1/bits/stl_vector.h \
  /usr/include/c++/4.2.1/bits/stl_bvector.h \
  /usr/include/c++/4.2.1/bits/vector.tcc sli_string.h
sli_type.o: sli_type.cpp sli_type.h /usr/include/c++/4.2.1/string \
  /usr/include/c++/4.2.1/bits/c++config.h \
  /usr/include/c++/4.2.1/bits/os_defines.h /usr/include/unistd.h \
  /usr/include/_types.h /usr/include/sys/_types.h \
  /usr/include/sys/cdefs.h /usr/include/sys/_symbol_aliasing.h \
  /usr/include/sys/_posix_availability.h /usr/include/machine/_types.h \
  /usr/include/i386/_types.h /usr/include/sys/unistd.h \
  /usr/include/Availability.h /usr/include/AvailabilityInternal.h \
  /usr/include/sys/select.h /usr/include/sys/appleapiopts.h \
  /usr/include/sys/_structs.h /usr/include/sys/_select.h \
  /usr/include/c++/4.2.1/bits/cpu_defines.h \
  /usr/include/c++/4.2.1/bits/stringfwd.h \
  /usr/include/c++/4.2.1/bits/char_traits.h \
  /usr/include/c++/4.2.1/cstring /usr/include/c++/4.2.1/cstddef \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/stddef.h \
  /usr/include/string.h /usr/include/strings.h \
  /usr/include/c++/4.2.1/bits/stl_algobase.h \
  /usr/include/c++/4.2.1/climits \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/limits.h \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/syslimits.h \
  /usr/include/limits.h /usr/include/machine/limits.h \
  /usr/include/i386/limits.h /usr/include/i386/_limits.h \
  /usr/include/sys/syslimits.h /usr/include/c++/4.2.1/cstdlib \
  /usr/include/stdlib.h /usr/include/sys/wait.h /usr/include/sys/signal.h \
  /usr/include/machine/signal.h /usr/include/i386/signal.h \
  /usr/include/i386/_structs.h /usr/include/machine/_structs.h \
  /usr/include/mach/i386/_structs.h /usr/include/sys/resource.h \
  /usr/include/machine/endian.h /usr/include/i386/endian.h \
  /usr/include/sys/_endian.h /usr/include/libkern/_OSByteOrder.h \
  /usr/include/libkern/i386/_OSByteOrder.h /usr/include/alloca.h \
  /usr/include/machine/types.h /usr/include/i386/types.h \
  /usr/include/i386/_types.h /usr/include/c++/4.2.1/iosfwd \
  /usr/include/c++/4.2.1/bits/c++locale.h /usr/include/c++/4.2.1/clocale \
  /usr/include/locale.h /usr/include/_locale.h \
  /usr/include/c++/4.2.1/cstdio /usr/include/stdio.h \
  /usr/include/c++/4.2.1/cstdarg \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/stdarg.h \
  /usr/include/c++/4.2.1/bits/c++io.h /usr/include/c++/4.2.1/bits/gthr.h \
  /usr/include/c++/4.2.1/bits/gthr-default.h /usr/include/pthread.h \
  /usr/include/pthread_impl.h /usr/include/sched.h /usr/include/time.h \
  /usr/include/_structs.h /usr/include/c++/4.2.1/cctype \
  /usr/include/ctype.h /usr/include/runetype.h \
  /usr/include/c++/4.2.1/bits/postypes.h /usr/include/c++/4.2.1/cwchar \
  /usr/include/c++/4.2.1/ctime /usr/include/wchar.h \
  /usr/include/_wctype.h /usr/include/stdint.h \
  /usr/include/c++/4.2.1/bits/functexcept.h \
  /usr/include/c++/4.2.1/exception_defines.h \
  /usr/include/c++/4.2.1/bits/stl_pair.h \
  /usr/include/c++/4.2.1/bits/cpp_type_traits.h \
  /usr/include/c++/4.2.1/ext/type_traits.h /usr/include/c++/4.2.1/utility \
  /usr/include/c++/4.2.1/bits/stl_relops.h \
  /usr/include/c++/4.2.1/bits/stl_iterator_base_types.h \
  /usr/include/c++/4.2.1/bits/stl_iterator_base_funcs.h \
  /usr/include/c++/4.2.1/bits/concept_check.h \
  /usr/include/c++/4.2.1/bits/stl_iterator.h \
  /usr/include/c++/4.2.1/debug/debug.h /usr/include/c++/4.2.1/memory \
  /usr/include/c++/4.2.1/bits/allocator.h \
  /usr/include/c++/4.2.1/bits/c++allocator.h \
  /usr/include/c++/4.2.1/ext/new_allocator.h /usr/include/c++/4.2.1/new \
  /usr/include/c++/4.2.1/exception \
  /usr/include/c++/4.2.1/bits/stl_construct.h \
  /usr/include/c++/4.2.1/bits/stl_uninitialized.h \
  /usr/include/c++/4.2.1/bits/stl_raw_storage_iter.h \
  /usr/include/c++/4.2.1/limits \
  /usr/include/c++/4.2.1/bits/ostream_insert.h \
  /usr/include/c++/4.2.1/bits/stl_function.h \
  /usr/include/c++/4.2.1/bits/basic_string.h \
  /usr/include/c++/4.2.1/ext/atomicity.h \
  /usr/include/c++/4.2.1/bits/atomic_word.h \
  /usr/include/c++/4.2.1/algorithm /usr/include/c++/4.2.1/bits/stl_algo.h \
  /usr/include/c++/4.2.1/bits/stl_heap.h \
  /usr/include/c++/4.2.1/bits/stl_tempbuf.h \
  /usr/include/c++/4.2.1/bits/basic_string.tcc sli_name.h \
  /usr/include/c++/4.2.1/cassert /usr/include/assert.h \
  /usr/include/c++/4.2.1/map /usr/include/c++/4.2.1/bits/stl_tree.h \
  /usr/include/c++/4.2.1/bits/stl_map.h \
  /usr/include/c++/4.2.1/bits/stl_multimap.h /usr/include/c++/4.2.1/deque \
  /usr/include/c++/4.2.1/bits/stl_deque.h \
  /usr/include/c++/4.2.1/bits/deque.tcc /usr/include/c++/4.2.1/iostream \
  /usr/include/c++/4.2.1/ostream /usr/include/c++/4.2.1/ios \
  /usr/include/c++/4.2.1/bits/localefwd.h \
  /usr/include/c++/4.2.1/bits/ios_base.h \
  /usr/include/c++/4.2.1/bits/locale_classes.h \
  /usr/include/c++/4.2.1/streambuf \
  /usr/include/c++/4.2.1/bits/streambuf.tcc \
  /usr/include/c++/4.2.1/bits/basic_ios.h \
  /usr/include/c++/4.2.1/bits/streambuf_iterator.h \
  /usr/include/c++/4.2.1/bits/locale_facets.h \
  /usr/include/c++/4.2.1/cwctype /usr/include/wctype.h \
  /usr/include/c++/4.2.1/bits/ctype_base.h \
  /usr/include/c++/4.2.1/bits/ctype_inline.h \
  /usr/include/c++/4.2.1/bits/codecvt.h \
  /usr/include/c++/4.2.1/bits/time_members.h \
  /usr/include/c++/4.2.1/bits/messages_members.h \
  /usr/include/c++/4.2.1/bits/basic_ios.tcc \
  /usr/include/c++/4.2.1/bits/ostream.tcc /usr/include/c++/4.2.1/locale \
  /usr/include/c++/4.2.1/bits/locale_facets.tcc \
  /usr/include/c++/4.2.1/typeinfo /usr/include/c++/4.2.1/istream \
  /usr/include/c++/4.2.1/bits/istream.tcc sli_token.h sli_exceptions.h \
  /usr/include/c++/4.2.1/vector /usr/include/c++/4.2.1/bits/stl_vector.h \
  /usr/include/c++/4.2.1/bits/stl_bvector.h \
  /usr/include/c++/4.2.1/bits/vector.tcc sli_interpreter.h \
  sli_allocator.h sli_arraytype.h sli_array.h sli_integertype.h \
  SLI_token.h sli_tokenstack.h sli_string.h sli_dictionary.h \
  sli_dictstack.h /usr/include/c++/4.2.1/list \
  /usr/include/c++/4.2.1/bits/stl_list.h \
  /usr/include/c++/4.2.1/bits/list.tcc sli_builtins.h sli_function.h \
  sli_module.h
sli_arraytype.o: sli_arraytype.cpp sli_arraytype.h sli_type.h \
  /usr/include/c++/4.2.1/string /usr/include/c++/4.2.1/bits/c++config.h \
  /usr/include/c++/4.2.1/bits/os_defines.h /usr/include/unistd.h \
  /usr/include/_types.h /usr/include/sys/_types.h \
  /usr/include/sys/cdefs.h /usr/include/sys/_symbol_aliasing.h \
  /usr/include/sys/_posix_availability.h /usr/include/machine/_types.h \
  /usr/include/i386/_types.h /usr/include/sys/unistd.h \
  /usr/include/Availability.h /usr/include/AvailabilityInternal.h \
  /usr/include/sys/select.h /usr/include/sys/appleapiopts.h \
  /usr/include/sys/_structs.h /usr/include/sys/_select.h \
  /usr/include/c++/4.2.1/bits/cpu_defines.h \
  /usr/include/c++/4.2.1/bits/stringfwd.h \
  /usr/include/c++/4.2.1/bits/char_traits.h \
  /usr/include/c++/4.2.1/cstring /usr/include/c++/4.2.1/cstddef \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/stddef.h \
  /usr/include/string.h /usr/include/strings.h \
  /usr/include/c++/4.2.1/bits/stl_algobase.h \
  /usr/include/c++/4.2.1/climits \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/limits.h \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/syslimits.h \
  /usr/include/limits.h /usr/include/machine/limits.h \
  /usr/include/i386/limits.h /usr/include/i386/_limits.h \
  /usr/include/sys/syslimits.h /usr/include/c++/4.2.1/cstdlib \
  /usr/include/stdlib.h /usr/include/sys/wait.h /usr/include/sys/signal.h \
  /usr/include/machine/signal.h /usr/include/i386/signal.h \
  /usr/include/i386/_structs.h /usr/include/machine/_structs.h \
  /usr/include/mach/i386/_structs.h /usr/include/sys/resource.h \
  /usr/include/machine/endian.h /usr/include/i386/endian.h \
  /usr/include/sys/_endian.h /usr/include/libkern/_OSByteOrder.h \
  /usr/include/libkern/i386/_OSByteOrder.h /usr/include/alloca.h \
  /usr/include/machine/types.h /usr/include/i386/types.h \
  /usr/include/i386/_types.h /usr/include/c++/4.2.1/iosfwd \
  /usr/include/c++/4.2.1/bits/c++locale.h /usr/include/c++/4.2.1/clocale \
  /usr/include/locale.h /usr/include/_locale.h \
  /usr/include/c++/4.2.1/cstdio /usr/include/stdio.h \
  /usr/include/c++/4.2.1/cstdarg \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/stdarg.h \
  /usr/include/c++/4.2.1/bits/c++io.h /usr/include/c++/4.2.1/bits/gthr.h \
  /usr/include/c++/4.2.1/bits/gthr-default.h /usr/include/pthread.h \
  /usr/include/pthread_impl.h /usr/include/sched.h /usr/include/time.h \
  /usr/include/_structs.h /usr/include/c++/4.2.1/cctype \
  /usr/include/ctype.h /usr/include/runetype.h \
  /usr/include/c++/4.2.1/bits/postypes.h /usr/include/c++/4.2.1/cwchar \
  /usr/include/c++/4.2.1/ctime /usr/include/wchar.h \
  /usr/include/_wctype.h /usr/include/stdint.h \
  /usr/include/c++/4.2.1/bits/functexcept.h \
  /usr/include/c++/4.2.1/exception_defines.h \
  /usr/include/c++/4.2.1/bits/stl_pair.h \
  /usr/include/c++/4.2.1/bits/cpp_type_traits.h \
  /usr/include/c++/4.2.1/ext/type_traits.h /usr/include/c++/4.2.1/utility \
  /usr/include/c++/4.2.1/bits/stl_relops.h \
  /usr/include/c++/4.2.1/bits/stl_iterator_base_types.h \
  /usr/include/c++/4.2.1/bits/stl_iterator_base_funcs.h \
  /usr/include/c++/4.2.1/bits/concept_check.h \
  /usr/include/c++/4.2.1/bits/stl_iterator.h \
  /usr/include/c++/4.2.1/debug/debug.h /usr/include/c++/4.2.1/memory \
  /usr/include/c++/4.2.1/bits/allocator.h \
  /usr/include/c++/4.2.1/bits/c++allocator.h \
  /usr/include/c++/4.2.1/ext/new_allocator.h /usr/include/c++/4.2.1/new \
  /usr/include/c++/4.2.1/exception \
  /usr/include/c++/4.2.1/bits/stl_construct.h \
  /usr/include/c++/4.2.1/bits/stl_uninitialized.h \
  /usr/include/c++/4.2.1/bits/stl_raw_storage_iter.h \
  /usr/include/c++/4.2.1/limits \
  /usr/include/c++/4.2.1/bits/ostream_insert.h \
  /usr/include/c++/4.2.1/bits/stl_function.h \
  /usr/include/c++/4.2.1/bits/basic_string.h \
  /usr/include/c++/4.2.1/ext/atomicity.h \
  /usr/include/c++/4.2.1/bits/atomic_word.h \
  /usr/include/c++/4.2.1/algorithm /usr/include/c++/4.2.1/bits/stl_algo.h \
  /usr/include/c++/4.2.1/bits/stl_heap.h \
  /usr/include/c++/4.2.1/bits/stl_tempbuf.h \
  /usr/include/c++/4.2.1/bits/basic_string.tcc sli_name.h \
  /usr/include/c++/4.2.1/cassert /usr/include/assert.h \
  /usr/include/c++/4.2.1/map /usr/include/c++/4.2.1/bits/stl_tree.h \
  /usr/include/c++/4.2.1/bits/stl_map.h \
  /usr/include/c++/4.2.1/bits/stl_multimap.h /usr/include/c++/4.2.1/deque \
  /usr/include/c++/4.2.1/bits/stl_deque.h \
  /usr/include/c++/4.2.1/bits/deque.tcc /usr/include/c++/4.2.1/iostream \
  /usr/include/c++/4.2.1/ostream /usr/include/c++/4.2.1/ios \
  /usr/include/c++/4.2.1/bits/localefwd.h \
  /usr/include/c++/4.2.1/bits/ios_base.h \
  /usr/include/c++/4.2.1/bits/locale_classes.h \
  /usr/include/c++/4.2.1/streambuf \
  /usr/include/c++/4.2.1/bits/streambuf.tcc \
  /usr/include/c++/4.2.1/bits/basic_ios.h \
  /usr/include/c++/4.2.1/bits/streambuf_iterator.h \
  /usr/include/c++/4.2.1/bits/locale_facets.h \
  /usr/include/c++/4.2.1/cwctype /usr/include/wctype.h \
  /usr/include/c++/4.2.1/bits/ctype_base.h \
  /usr/include/c++/4.2.1/bits/ctype_inline.h \
  /usr/include/c++/4.2.1/bits/codecvt.h \
  /usr/include/c++/4.2.1/bits/time_members.h \
  /usr/include/c++/4.2.1/bits/messages_members.h \
  /usr/include/c++/4.2.1/bits/basic_ios.tcc \
  /usr/include/c++/4.2.1/bits/ostream.tcc /usr/include/c++/4.2.1/locale \
  /usr/include/c++/4.2.1/bits/locale_facets.tcc \
  /usr/include/c++/4.2.1/typeinfo /usr/include/c++/4.2.1/istream \
  /usr/include/c++/4.2.1/bits/istream.tcc sli_token.h sli_exceptions.h \
  /usr/include/c++/4.2.1/vector /usr/include/c++/4.2.1/bits/stl_vector.h \
  /usr/include/c++/4.2.1/bits/stl_bvector.h \
  /usr/include/c++/4.2.1/bits/vector.tcc sli_array.h sli_allocator.h \
  sli_interpreter.h sli_integertype.h SLI_token.h sli_tokenstack.h \
  sli_string.h sli_dictionary.h sli_dictstack.h \
  /usr/include/c++/4.2.1/list /usr/include/c++/4.2.1/bits/stl_list.h \
  /usr/include/c++/4.2.1/bits/list.tcc sli_builtins.h sli_function.h \
  sli_module.h
sli_stringtype.o: sli_stringtype.cpp sli_stringtype.h sli_type.h \
  /usr/include/c++/4.2.1/string /usr/include/c++/4.2.1/bits/c++config.h \
  /usr/include/c++/4.2.1/bits/os_defines.h /usr/include/unistd.h \
  /usr/include/_types.h /usr/include/sys/_types.h \
  /usr/include/sys/cdefs.h /usr/include/sys/_symbol_aliasing.h \
  /usr/include/sys/_posix_availability.h /usr/include/machine/_types.h \
  /usr/include/i386/_types.h /usr/include/sys/unistd.h \
  /usr/include/Availability.h /usr/include/AvailabilityInternal.h \
  /usr/include/sys/select.h /usr/include/sys/appleapiopts.h \
  /usr/include/sys/_structs.h /usr/include/sys/_select.h \
  /usr/include/c++/4.2.1/bits/cpu_defines.h \
  /usr/include/c++/4.2.1/bits/stringfwd.h \
  /usr/include/c++/4.2.1/bits/char_traits.h \
  /usr/include/c++/4.2.1/cstring /usr/include/c++/4.2.1/cstddef \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/stddef.h \
  /usr/include/string.h /usr/include/strings.h \
  /usr/include/c++/4.2.1/bits/stl_algobase.h \
  /usr/include/c++/4.2.1/climits \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/limits.h \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/syslimits.h \
  /usr/include/limits.h /usr/include/machine/limits.h \
  /usr/include/i386/limits.h /usr/include/i386/_limits.h \
  /usr/include/sys/syslimits.h /usr/include/c++/4.2.1/cstdlib \
  /usr/include/stdlib.h /usr/include/sys/wait.h /usr/include/sys/signal.h \
  /usr/include/machine/signal.h /usr/include/i386/signal.h \
  /usr/include/i386/_structs.h /usr/include/machine/_structs.h \
  /usr/include/mach/i386/_structs.h /usr/include/sys/resource.h \
  /usr/include/machine/endian.h /usr/include/i386/endian.h \
  /usr/include/sys/_endian.h /usr/include/libkern/_OSByteOrder.h \
  /usr/include/libkern/i386/_OSByteOrder.h /usr/include/alloca.h \
  /usr/include/machine/types.h /usr/include/i386/types.h \
  /usr/include/i386/_types.h /usr/include/c++/4.2.1/iosfwd \
  /usr/include/c++/4.2.1/bits/c++locale.h /usr/include/c++/4.2.1/clocale \
  /usr/include/locale.h /usr/include/_locale.h \
  /usr/include/c++/4.2.1/cstdio /usr/include/stdio.h \
  /usr/include/c++/4.2.1/cstdarg \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/stdarg.h \
  /usr/include/c++/4.2.1/bits/c++io.h /usr/include/c++/4.2.1/bits/gthr.h \
  /usr/include/c++/4.2.1/bits/gthr-default.h /usr/include/pthread.h \
  /usr/include/pthread_impl.h /usr/include/sched.h /usr/include/time.h \
  /usr/include/_structs.h /usr/include/c++/4.2.1/cctype \
  /usr/include/ctype.h /usr/include/runetype.h \
  /usr/include/c++/4.2.1/bits/postypes.h /usr/include/c++/4.2.1/cwchar \
  /usr/include/c++/4.2.1/ctime /usr/include/wchar.h \
  /usr/include/_wctype.h /usr/include/stdint.h \
  /usr/include/c++/4.2.1/bits/functexcept.h \
  /usr/include/c++/4.2.1/exception_defines.h \
  /usr/include/c++/4.2.1/bits/stl_pair.h \
  /usr/include/c++/4.2.1/bits/cpp_type_traits.h \
  /usr/include/c++/4.2.1/ext/type_traits.h /usr/include/c++/4.2.1/utility \
  /usr/include/c++/4.2.1/bits/stl_relops.h \
  /usr/include/c++/4.2.1/bits/stl_iterator_base_types.h \
  /usr/include/c++/4.2.1/bits/stl_iterator_base_funcs.h \
  /usr/include/c++/4.2.1/bits/concept_check.h \
  /usr/include/c++/4.2.1/bits/stl_iterator.h \
  /usr/include/c++/4.2.1/debug/debug.h /usr/include/c++/4.2.1/memory \
  /usr/include/c++/4.2.1/bits/allocator.h \
  /usr/include/c++/4.2.1/bits/c++allocator.h \
  /usr/include/c++/4.2.1/ext/new_allocator.h /usr/include/c++/4.2.1/new \
  /usr/include/c++/4.2.1/exception \
  /usr/include/c++/4.2.1/bits/stl_construct.h \
  /usr/include/c++/4.2.1/bits/stl_uninitialized.h \
  /usr/include/c++/4.2.1/bits/stl_raw_storage_iter.h \
  /usr/include/c++/4.2.1/limits \
  /usr/include/c++/4.2.1/bits/ostream_insert.h \
  /usr/include/c++/4.2.1/bits/stl_function.h \
  /usr/include/c++/4.2.1/bits/basic_string.h \
  /usr/include/c++/4.2.1/ext/atomicity.h \
  /usr/include/c++/4.2.1/bits/atomic_word.h \
  /usr/include/c++/4.2.1/algorithm /usr/include/c++/4.2.1/bits/stl_algo.h \
  /usr/include/c++/4.2.1/bits/stl_heap.h \
  /usr/include/c++/4.2.1/bits/stl_tempbuf.h \
  /usr/include/c++/4.2.1/bits/basic_string.tcc sli_name.h \
  /usr/include/c++/4.2.1/cassert /usr/include/assert.h \
  /usr/include/c++/4.2.1/map /usr/include/c++/4.2.1/bits/stl_tree.h \
  /usr/include/c++/4.2.1/bits/stl_map.h \
  /usr/include/c++/4.2.1/bits/stl_multimap.h /usr/include/c++/4.2.1/deque \
  /usr/include/c++/4.2.1/bits/stl_deque.h \
  /usr/include/c++/4.2.1/bits/deque.tcc /usr/include/c++/4.2.1/iostream \
  /usr/include/c++/4.2.1/ostream /usr/include/c++/4.2.1/ios \
  /usr/include/c++/4.2.1/bits/localefwd.h \
  /usr/include/c++/4.2.1/bits/ios_base.h \
  /usr/include/c++/4.2.1/bits/locale_classes.h \
  /usr/include/c++/4.2.1/streambuf \
  /usr/include/c++/4.2.1/bits/streambuf.tcc \
  /usr/include/c++/4.2.1/bits/basic_ios.h \
  /usr/include/c++/4.2.1/bits/streambuf_iterator.h \
  /usr/include/c++/4.2.1/bits/locale_facets.h \
  /usr/include/c++/4.2.1/cwctype /usr/include/wctype.h \
  /usr/include/c++/4.2.1/bits/ctype_base.h \
  /usr/include/c++/4.2.1/bits/ctype_inline.h \
  /usr/include/c++/4.2.1/bits/codecvt.h \
  /usr/include/c++/4.2.1/bits/time_members.h \
  /usr/include/c++/4.2.1/bits/messages_members.h \
  /usr/include/c++/4.2.1/bits/basic_ios.tcc \
  /usr/include/c++/4.2.1/bits/ostream.tcc /usr/include/c++/4.2.1/locale \
  /usr/include/c++/4.2.1/bits/locale_facets.tcc \
  /usr/include/c++/4.2.1/typeinfo /usr/include/c++/4.2.1/istream \
  /usr/include/c++/4.2.1/bits/istream.tcc sli_token.h sli_exceptions.h \
  /usr/include/c++/4.2.1/vector /usr/include/c++/4.2.1/bits/stl_vector.h \
  /usr/include/c++/4.2.1/bits/stl_bvector.h \
  /usr/include/c++/4.2.1/bits/vector.tcc sli_string.h sli_interpreter.h \
  sli_allocator.h sli_arraytype.h sli_array.h sli_integertype.h \
  SLI_token.h sli_tokenstack.h sli_dictionary.h sli_dictstack.h \
  /usr/include/c++/4.2.1/list /usr/include/c++/4.2.1/bits/stl_list.h \
  /usr/include/c++/4.2.1/bits/list.tcc sli_builtins.h sli_function.h \
  sli_module.h
sli_integertype.o: sli_integertype.cpp sli_integertype.h sli_type.h \
  /usr/include/c++/4.2.1/string /usr/include/c++/4.2.1/bits/c++config.h \
  /usr/include/c++/4.2.1/bits/os_defines.h /usr/include/unistd.h \
  /usr/include/_types.h /usr/include/sys/_types.h \
  /usr/include/sys/cdefs.h /usr/include/sys/_symbol_aliasing.h \
  /usr/include/sys/_posix_availability.h /usr/include/machine/_types.h \
  /usr/include/i386/_types.h /usr/include/sys/unistd.h \
  /usr/include/Availability.h /usr/include/AvailabilityInternal.h \
  /usr/include/sys/select.h /usr/include/sys/appleapiopts.h \
  /usr/include/sys/_structs.h /usr/include/sys/_select.h \
  /usr/include/c++/4.2.1/bits/cpu_defines.h \
  /usr/include/c++/4.2.1/bits/stringfwd.h \
  /usr/include/c++/4.2.1/bits/char_traits.h \
  /usr/include/c++/4.2.1/cstring /usr/include/c++/4.2.1/cstddef \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/stddef.h \
  /usr/include/string.h /usr/include/strings.h \
  /usr/include/c++/4.2.1/bits/stl_algobase.h \
  /usr/include/c++/4.2.1/climits \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/limits.h \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/syslimits.h \
  /usr/include/limits.h /usr/include/machine/limits.h \
  /usr/include/i386/limits.h /usr/include/i386/_limits.h \
  /usr/include/sys/syslimits.h /usr/include/c++/4.2.1/cstdlib \
  /usr/include/stdlib.h /usr/include/sys/wait.h /usr/include/sys/signal.h \
  /usr/include/machine/signal.h /usr/include/i386/signal.h \
  /usr/include/i386/_structs.h /usr/include/machine/_structs.h \
  /usr/include/mach/i386/_structs.h /usr/include/sys/resource.h \
  /usr/include/machine/endian.h /usr/include/i386/endian.h \
  /usr/include/sys/_endian.h /usr/include/libkern/_OSByteOrder.h \
  /usr/include/libkern/i386/_OSByteOrder.h /usr/include/alloca.h \
  /usr/include/machine/types.h /usr/include/i386/types.h \
  /usr/include/i386/_types.h /usr/include/c++/4.2.1/iosfwd \
  /usr/include/c++/4.2.1/bits/c++locale.h /usr/include/c++/4.2.1/clocale \
  /usr/include/locale.h /usr/include/_locale.h \
  /usr/include/c++/4.2.1/cstdio /usr/include/stdio.h \
  /usr/include/c++/4.2.1/cstdarg \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/stdarg.h \
  /usr/include/c++/4.2.1/bits/c++io.h /usr/include/c++/4.2.1/bits/gthr.h \
  /usr/include/c++/4.2.1/bits/gthr-default.h /usr/include/pthread.h \
  /usr/include/pthread_impl.h /usr/include/sched.h /usr/include/time.h \
  /usr/include/_structs.h /usr/include/c++/4.2.1/cctype \
  /usr/include/ctype.h /usr/include/runetype.h \
  /usr/include/c++/4.2.1/bits/postypes.h /usr/include/c++/4.2.1/cwchar \
  /usr/include/c++/4.2.1/ctime /usr/include/wchar.h \
  /usr/include/_wctype.h /usr/include/stdint.h \
  /usr/include/c++/4.2.1/bits/functexcept.h \
  /usr/include/c++/4.2.1/exception_defines.h \
  /usr/include/c++/4.2.1/bits/stl_pair.h \
  /usr/include/c++/4.2.1/bits/cpp_type_traits.h \
  /usr/include/c++/4.2.1/ext/type_traits.h /usr/include/c++/4.2.1/utility \
  /usr/include/c++/4.2.1/bits/stl_relops.h \
  /usr/include/c++/4.2.1/bits/stl_iterator_base_types.h \
  /usr/include/c++/4.2.1/bits/stl_iterator_base_funcs.h \
  /usr/include/c++/4.2.1/bits/concept_check.h \
  /usr/include/c++/4.2.1/bits/stl_iterator.h \
  /usr/include/c++/4.2.1/debug/debug.h /usr/include/c++/4.2.1/memory \
  /usr/include/c++/4.2.1/bits/allocator.h \
  /usr/include/c++/4.2.1/bits/c++allocator.h \
  /usr/include/c++/4.2.1/ext/new_allocator.h /usr/include/c++/4.2.1/new \
  /usr/include/c++/4.2.1/exception \
  /usr/include/c++/4.2.1/bits/stl_construct.h \
  /usr/include/c++/4.2.1/bits/stl_uninitialized.h \
  /usr/include/c++/4.2.1/bits/stl_raw_storage_iter.h \
  /usr/include/c++/4.2.1/limits \
  /usr/include/c++/4.2.1/bits/ostream_insert.h \
  /usr/include/c++/4.2.1/bits/stl_function.h \
  /usr/include/c++/4.2.1/bits/basic_string.h \
  /usr/include/c++/4.2.1/ext/atomicity.h \
  /usr/include/c++/4.2.1/bits/atomic_word.h \
  /usr/include/c++/4.2.1/algorithm /usr/include/c++/4.2.1/bits/stl_algo.h \
  /usr/include/c++/4.2.1/bits/stl_heap.h \
  /usr/include/c++/4.2.1/bits/stl_tempbuf.h \
  /usr/include/c++/4.2.1/bits/basic_string.tcc sli_name.h \
  /usr/include/c++/4.2.1/cassert /usr/include/assert.h \
  /usr/include/c++/4.2.1/map /usr/include/c++/4.2.1/bits/stl_tree.h \
  /usr/include/c++/4.2.1/bits/stl_map.h \
  /usr/include/c++/4.2.1/bits/stl_multimap.h /usr/include/c++/4.2.1/deque \
  /usr/include/c++/4.2.1/bits/stl_deque.h \
  /usr/include/c++/4.2.1/bits/deque.tcc /usr/include/c++/4.2.1/iostream \
  /usr/include/c++/4.2.1/ostream /usr/include/c++/4.2.1/ios \
  /usr/include/c++/4.2.1/bits/localefwd.h \
  /usr/include/c++/4.2.1/bits/ios_base.h \
  /usr/include/c++/4.2.1/bits/locale_classes.h \
  /usr/include/c++/4.2.1/streambuf \
  /usr/include/c++/4.2.1/bits/streambuf.tcc \
  /usr/include/c++/4.2.1/bits/basic_ios.h \
  /usr/include/c++/4.2.1/bits/streambuf_iterator.h \
  /usr/include/c++/4.2.1/bits/locale_facets.h \
  /usr/include/c++/4.2.1/cwctype /usr/include/wctype.h \
  /usr/include/c++/4.2.1/bits/ctype_base.h \
  /usr/include/c++/4.2.1/bits/ctype_inline.h \
  /usr/include/c++/4.2.1/bits/codecvt.h \
  /usr/include/c++/4.2.1/bits/time_members.h \
  /usr/include/c++/4.2.1/bits/messages_members.h \
  /usr/include/c++/4.2.1/bits/basic_ios.tcc \
  /usr/include/c++/4.2.1/bits/ostream.tcc /usr/include/c++/4.2.1/locale \
  /usr/include/c++/4.2.1/bits/locale_facets.tcc \
  /usr/include/c++/4.2.1/typeinfo /usr/include/c++/4.2.1/istream \
  /usr/include/c++/4.2.1/bits/istream.tcc SLI_token.h sli_exceptions.h \
  /usr/include/c++/4.2.1/vector /usr/include/c++/4.2.1/bits/stl_vector.h \
  /usr/include/c++/4.2.1/bits/stl_bvector.h \
  /usr/include/c++/4.2.1/bits/vector.tcc sli_array.h sli_token.h \
  sli_allocator.h
sli_nametype.o: sli_nametype.cpp sli_nametype.h sli_type.h \
  /usr/include/c++/4.2.1/string /usr/include/c++/4.2.1/bits/c++config.h \
  /usr/include/c++/4.2.1/bits/os_defines.h /usr/include/unistd.h \
  /usr/include/_types.h /usr/include/sys/_types.h \
  /usr/include/sys/cdefs.h /usr/include/sys/_symbol_aliasing.h \
  /usr/include/sys/_posix_availability.h /usr/include/machine/_types.h \
  /usr/include/i386/_types.h /usr/include/sys/unistd.h \
  /usr/include/Availability.h /usr/include/AvailabilityInternal.h \
  /usr/include/sys/select.h /usr/include/sys/appleapiopts.h \
  /usr/include/sys/_structs.h /usr/include/sys/_select.h \
  /usr/include/c++/4.2.1/bits/cpu_defines.h \
  /usr/include/c++/4.2.1/bits/stringfwd.h \
  /usr/include/c++/4.2.1/bits/char_traits.h \
  /usr/include/c++/4.2.1/cstring /usr/include/c++/4.2.1/cstddef \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/stddef.h \
  /usr/include/string.h /usr/include/strings.h \
  /usr/include/c++/4.2.1/bits/stl_algobase.h \
  /usr/include/c++/4.2.1/climits \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/limits.h \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/syslimits.h \
  /usr/include/limits.h /usr/include/machine/limits.h \
  /usr/include/i386/limits.h /usr/include/i386/_limits.h \
  /usr/include/sys/syslimits.h /usr/include/c++/4.2.1/cstdlib \
  /usr/include/stdlib.h /usr/include/sys/wait.h /usr/include/sys/signal.h \
  /usr/include/machine/signal.h /usr/include/i386/signal.h \
  /usr/include/i386/_structs.h /usr/include/machine/_structs.h \
  /usr/include/mach/i386/_structs.h /usr/include/sys/resource.h \
  /usr/include/machine/endian.h /usr/include/i386/endian.h \
  /usr/include/sys/_endian.h /usr/include/libkern/_OSByteOrder.h \
  /usr/include/libkern/i386/_OSByteOrder.h /usr/include/alloca.h \
  /usr/include/machine/types.h /usr/include/i386/types.h \
  /usr/include/i386/_types.h /usr/include/c++/4.2.1/iosfwd \
  /usr/include/c++/4.2.1/bits/c++locale.h /usr/include/c++/4.2.1/clocale \
  /usr/include/locale.h /usr/include/_locale.h \
  /usr/include/c++/4.2.1/cstdio /usr/include/stdio.h \
  /usr/include/c++/4.2.1/cstdarg \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/stdarg.h \
  /usr/include/c++/4.2.1/bits/c++io.h /usr/include/c++/4.2.1/bits/gthr.h \
  /usr/include/c++/4.2.1/bits/gthr-default.h /usr/include/pthread.h \
  /usr/include/pthread_impl.h /usr/include/sched.h /usr/include/time.h \
  /usr/include/_structs.h /usr/include/c++/4.2.1/cctype \
  /usr/include/ctype.h /usr/include/runetype.h \
  /usr/include/c++/4.2.1/bits/postypes.h /usr/include/c++/4.2.1/cwchar \
  /usr/include/c++/4.2.1/ctime /usr/include/wchar.h \
  /usr/include/_wctype.h /usr/include/stdint.h \
  /usr/include/c++/4.2.1/bits/functexcept.h \
  /usr/include/c++/4.2.1/exception_defines.h \
  /usr/include/c++/4.2.1/bits/stl_pair.h \
  /usr/include/c++/4.2.1/bits/cpp_type_traits.h \
  /usr/include/c++/4.2.1/ext/type_traits.h /usr/include/c++/4.2.1/utility \
  /usr/include/c++/4.2.1/bits/stl_relops.h \
  /usr/include/c++/4.2.1/bits/stl_iterator_base_types.h \
  /usr/include/c++/4.2.1/bits/stl_iterator_base_funcs.h \
  /usr/include/c++/4.2.1/bits/concept_check.h \
  /usr/include/c++/4.2.1/bits/stl_iterator.h \
  /usr/include/c++/4.2.1/debug/debug.h /usr/include/c++/4.2.1/memory \
  /usr/include/c++/4.2.1/bits/allocator.h \
  /usr/include/c++/4.2.1/bits/c++allocator.h \
  /usr/include/c++/4.2.1/ext/new_allocator.h /usr/include/c++/4.2.1/new \
  /usr/include/c++/4.2.1/exception \
  /usr/include/c++/4.2.1/bits/stl_construct.h \
  /usr/include/c++/4.2.1/bits/stl_uninitialized.h \
  /usr/include/c++/4.2.1/bits/stl_raw_storage_iter.h \
  /usr/include/c++/4.2.1/limits \
  /usr/include/c++/4.2.1/bits/ostream_insert.h \
  /usr/include/c++/4.2.1/bits/stl_function.h \
  /usr/include/c++/4.2.1/bits/basic_string.h \
  /usr/include/c++/4.2.1/ext/atomicity.h \
  /usr/include/c++/4.2.1/bits/atomic_word.h \
  /usr/include/c++/4.2.1/algorithm /usr/include/c++/4.2.1/bits/stl_algo.h \
  /usr/include/c++/4.2.1/bits/stl_heap.h \
  /usr/include/c++/4.2.1/bits/stl_tempbuf.h \
  /usr/include/c++/4.2.1/bits/basic_string.tcc sli_name.h \
  /usr/include/c++/4.2.1/cassert /usr/include/assert.h \
  /usr/include/c++/4.2.1/map /usr/include/c++/4.2.1/bits/stl_tree.h \
  /usr/include/c++/4.2.1/bits/stl_map.h \
  /usr/include/c++/4.2.1/bits/stl_multimap.h /usr/include/c++/4.2.1/deque \
  /usr/include/c++/4.2.1/bits/stl_deque.h \
  /usr/include/c++/4.2.1/bits/deque.tcc /usr/include/c++/4.2.1/iostream \
  /usr/include/c++/4.2.1/ostream /usr/include/c++/4.2.1/ios \
  /usr/include/c++/4.2.1/bits/localefwd.h \
  /usr/include/c++/4.2.1/bits/ios_base.h \
  /usr/include/c++/4.2.1/bits/locale_classes.h \
  /usr/include/c++/4.2.1/streambuf \
  /usr/include/c++/4.2.1/bits/streambuf.tcc \
  /usr/include/c++/4.2.1/bits/basic_ios.h \
  /usr/include/c++/4.2.1/bits/streambuf_iterator.h \
  /usr/include/c++/4.2.1/bits/locale_facets.h \
  /usr/include/c++/4.2.1/cwctype /usr/include/wctype.h \
  /usr/include/c++/4.2.1/bits/ctype_base.h \
  /usr/include/c++/4.2.1/bits/ctype_inline.h \
  /usr/include/c++/4.2.1/bits/codecvt.h \
  /usr/include/c++/4.2.1/bits/time_members.h \
  /usr/include/c++/4.2.1/bits/messages_members.h \
  /usr/include/c++/4.2.1/bits/basic_ios.tcc \
  /usr/include/c++/4.2.1/bits/ostream.tcc /usr/include/c++/4.2.1/locale \
  /usr/include/c++/4.2.1/bits/locale_facets.tcc \
  /usr/include/c++/4.2.1/typeinfo /usr/include/c++/4.2.1/istream \
  /usr/include/c++/4.2.1/bits/istream.tcc sli_token.h sli_exceptions.h \
  /usr/include/c++/4.2.1/vector /usr/include/c++/4.2.1/bits/stl_vector.h \
  /usr/include/c++/4.2.1/bits/stl_bvector.h \
  /usr/include/c++/4.2.1/bits/vector.tcc sli_interpreter.h \
  sli_allocator.h sli_arraytype.h sli_array.h sli_integertype.h \
  SLI_token.h sli_tokenstack.h sli_string.h sli_dictionary.h \
  sli_dictstack.h /usr/include/c++/4.2.1/list \
  /usr/include/c++/4.2.1/bits/stl_list.h \
  /usr/include/c++/4.2.1/bits/list.tcc sli_builtins.h sli_function.h \
  sli_module.h
sli_dicttype.o: sli_dicttype.cpp sli_token.h \
  /usr/include/c++/4.2.1/iostream /usr/include/c++/4.2.1/bits/c++config.h \
  /usr/include/c++/4.2.1/bits/os_defines.h /usr/include/unistd.h \
  /usr/include/_types.h /usr/include/sys/_types.h \
  /usr/include/sys/cdefs.h /usr/include/sys/_symbol_aliasing.h \
  /usr/include/sys/_posix_availability.h /usr/include/machine/_types.h \
  /usr/include/i386/_types.h /usr/include/sys/unistd.h \
  /usr/include/Availability.h /usr/include/AvailabilityInternal.h \
  /usr/include/sys/select.h /usr/include/sys/appleapiopts.h \
  /usr/include/sys/_structs.h /usr/include/sys/_select.h \
  /usr/include/c++/4.2.1/bits/cpu_defines.h \
  /usr/include/c++/4.2.1/ostream /usr/include/c++/4.2.1/ios \
  /usr/include/c++/4.2.1/iosfwd /usr/include/c++/4.2.1/bits/c++locale.h \
  /usr/include/c++/4.2.1/clocale /usr/include/locale.h \
  /usr/include/_locale.h /usr/include/c++/4.2.1/cstring \
  /usr/include/c++/4.2.1/cstddef \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/stddef.h \
  /usr/include/string.h /usr/include/strings.h \
  /usr/include/c++/4.2.1/cstdio /usr/include/stdio.h \
  /usr/include/c++/4.2.1/cstdarg \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/stdarg.h \
  /usr/include/c++/4.2.1/bits/c++io.h /usr/include/c++/4.2.1/bits/gthr.h \
  /usr/include/c++/4.2.1/bits/gthr-default.h /usr/include/pthread.h \
  /usr/include/pthread_impl.h /usr/include/sched.h /usr/include/time.h \
  /usr/include/_structs.h /usr/include/c++/4.2.1/cctype \
  /usr/include/ctype.h /usr/include/runetype.h \
  /usr/include/c++/4.2.1/bits/stringfwd.h \
  /usr/include/c++/4.2.1/bits/postypes.h /usr/include/c++/4.2.1/cwchar \
  /usr/include/c++/4.2.1/ctime /usr/include/wchar.h \
  /usr/include/_wctype.h /usr/include/stdint.h \
  /usr/include/c++/4.2.1/bits/functexcept.h \
  /usr/include/c++/4.2.1/exception_defines.h \
  /usr/include/c++/4.2.1/exception \
  /usr/include/c++/4.2.1/bits/char_traits.h \
  /usr/include/c++/4.2.1/bits/stl_algobase.h \
  /usr/include/c++/4.2.1/climits \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/limits.h \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/syslimits.h \
  /usr/include/limits.h /usr/include/machine/limits.h \
  /usr/include/i386/limits.h /usr/include/i386/_limits.h \
  /usr/include/sys/syslimits.h /usr/include/c++/4.2.1/cstdlib \
  /usr/include/stdlib.h /usr/include/sys/wait.h /usr/include/sys/signal.h \
  /usr/include/machine/signal.h /usr/include/i386/signal.h \
  /usr/include/i386/_structs.h /usr/include/machine/_structs.h \
  /usr/include/mach/i386/_structs.h /usr/include/sys/resource.h \
  /usr/include/machine/endian.h /usr/include/i386/endian.h \
  /usr/include/sys/_endian.h /usr/include/libkern/_OSByteOrder.h \
  /usr/include/libkern/i386/_OSByteOrder.h /usr/include/alloca.h \
  /usr/include/machine/types.h /usr/include/i386/types.h \
  /usr/include/i386/_types.h /usr/include/c++/4.2.1/bits/stl_pair.h \
  /usr/include/c++/4.2.1/bits/cpp_type_traits.h \
  /usr/include/c++/4.2.1/ext/type_traits.h /usr/include/c++/4.2.1/utility \
  /usr/include/c++/4.2.1/bits/stl_relops.h \
  /usr/include/c++/4.2.1/bits/stl_iterator_base_types.h \
  /usr/include/c++/4.2.1/bits/stl_iterator_base_funcs.h \
  /usr/include/c++/4.2.1/bits/concept_check.h \
  /usr/include/c++/4.2.1/bits/stl_iterator.h \
  /usr/include/c++/4.2.1/debug/debug.h \
  /usr/include/c++/4.2.1/bits/localefwd.h \
  /usr/include/c++/4.2.1/bits/ios_base.h \
  /usr/include/c++/4.2.1/ext/atomicity.h \
  /usr/include/c++/4.2.1/bits/atomic_word.h \
  /usr/include/c++/4.2.1/bits/locale_classes.h \
  /usr/include/c++/4.2.1/string /usr/include/c++/4.2.1/memory \
  /usr/include/c++/4.2.1/bits/allocator.h \
  /usr/include/c++/4.2.1/bits/c++allocator.h \
  /usr/include/c++/4.2.1/ext/new_allocator.h /usr/include/c++/4.2.1/new \
  /usr/include/c++/4.2.1/bits/stl_construct.h \
  /usr/include/c++/4.2.1/bits/stl_uninitialized.h \
  /usr/include/c++/4.2.1/bits/stl_raw_storage_iter.h \
  /usr/include/c++/4.2.1/limits \
  /usr/include/c++/4.2.1/bits/ostream_insert.h \
  /usr/include/c++/4.2.1/bits/stl_function.h \
  /usr/include/c++/4.2.1/bits/basic_string.h \
  /usr/include/c++/4.2.1/algorithm /usr/include/c++/4.2.1/bits/stl_algo.h \
  /usr/include/c++/4.2.1/bits/stl_heap.h \
  /usr/include/c++/4.2.1/bits/stl_tempbuf.h \
  /usr/include/c++/4.2.1/bits/basic_string.tcc \
  /usr/include/c++/4.2.1/streambuf \
  /usr/include/c++/4.2.1/bits/streambuf.tcc \
  /usr/include/c++/4.2.1/bits/basic_ios.h \
  /usr/include/c++/4.2.1/bits/streambuf_iterator.h \
  /usr/include/c++/4.2.1/bits/locale_facets.h \
  /usr/include/c++/4.2.1/cwctype /usr/include/wctype.h \
  /usr/include/c++/4.2.1/bits/ctype_base.h \
  /usr/include/c++/4.2.1/bits/ctype_inline.h \
  /usr/include/c++/4.2.1/bits/codecvt.h \
  /usr/include/c++/4.2.1/bits/time_members.h \
  /usr/include/c++/4.2.1/bits/messages_members.h \
  /usr/include/c++/4.2.1/bits/basic_ios.tcc \
  /usr/include/c++/4.2.1/bits/ostream.tcc /usr/include/c++/4.2.1/locale \
  /usr/include/c++/4.2.1/bits/locale_facets.tcc \
  /usr/include/c++/4.2.1/typeinfo /usr/include/c++/4.2.1/istream \
  /usr/include/c++/4.2.1/bits/istream.tcc sli_type.h sli_name.h \
  /usr/include/c++/4.2.1/cassert /usr/include/assert.h \
  /usr/include/c++/4.2.1/map /usr/include/c++/4.2.1/bits/stl_tree.h \
  /usr/include/c++/4.2.1/bits/stl_map.h \
  /usr/include/c++/4.2.1/bits/stl_multimap.h /usr/include/c++/4.2.1/deque \
  /usr/include/c++/4.2.1/bits/stl_deque.h \
  /usr/include/c++/4.2.1/bits/deque.tcc sli_exceptions.h \
  /usr/include/c++/4.2.1/vector /usr/include/c++/4.2.1/bits/stl_vector.h \
  /usr/include/c++/4.2.1/bits/stl_bvector.h \
  /usr/include/c++/4.2.1/bits/vector.tcc sli_dicttype.h sli_dictionary.h \
  sli_allocator.h
sli_functiontype.o: sli_functiontype.cpp sli_functiontype.h sli_type.h \
  /usr/include/c++/4.2.1/string /usr/include/c++/4.2.1/bits/c++config.h \
  /usr/include/c++/4.2.1/bits/os_defines.h /usr/include/unistd.h \
  /usr/include/_types.h /usr/include/sys/_types.h \
  /usr/include/sys/cdefs.h /usr/include/sys/_symbol_aliasing.h \
  /usr/include/sys/_posix_availability.h /usr/include/machine/_types.h \
  /usr/include/i386/_types.h /usr/include/sys/unistd.h \
  /usr/include/Availability.h /usr/include/AvailabilityInternal.h \
  /usr/include/sys/select.h /usr/include/sys/appleapiopts.h \
  /usr/include/sys/_structs.h /usr/include/sys/_select.h \
  /usr/include/c++/4.2.1/bits/cpu_defines.h \
  /usr/include/c++/4.2.1/bits/stringfwd.h \
  /usr/include/c++/4.2.1/bits/char_traits.h \
  /usr/include/c++/4.2.1/cstring /usr/include/c++/4.2.1/cstddef \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/stddef.h \
  /usr/include/string.h /usr/include/strings.h \
  /usr/include/c++/4.2.1/bits/stl_algobase.h \
  /usr/include/c++/4.2.1/climits \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/limits.h \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/syslimits.h \
  /usr/include/limits.h /usr/include/machine/limits.h \
  /usr/include/i386/limits.h /usr/include/i386/_limits.h \
  /usr/include/sys/syslimits.h /usr/include/c++/4.2.1/cstdlib \
  /usr/include/stdlib.h /usr/include/sys/wait.h /usr/include/sys/signal.h \
  /usr/include/machine/signal.h /usr/include/i386/signal.h \
  /usr/include/i386/_structs.h /usr/include/machine/_structs.h \
  /usr/include/mach/i386/_structs.h /usr/include/sys/resource.h \
  /usr/include/machine/endian.h /usr/include/i386/endian.h \
  /usr/include/sys/_endian.h /usr/include/libkern/_OSByteOrder.h \
  /usr/include/libkern/i386/_OSByteOrder.h /usr/include/alloca.h \
  /usr/include/machine/types.h /usr/include/i386/types.h \
  /usr/include/i386/_types.h /usr/include/c++/4.2.1/iosfwd \
  /usr/include/c++/4.2.1/bits/c++locale.h /usr/include/c++/4.2.1/clocale \
  /usr/include/locale.h /usr/include/_locale.h \
  /usr/include/c++/4.2.1/cstdio /usr/include/stdio.h \
  /usr/include/c++/4.2.1/cstdarg \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/stdarg.h \
  /usr/include/c++/4.2.1/bits/c++io.h /usr/include/c++/4.2.1/bits/gthr.h \
  /usr/include/c++/4.2.1/bits/gthr-default.h /usr/include/pthread.h \
  /usr/include/pthread_impl.h /usr/include/sched.h /usr/include/time.h \
  /usr/include/_structs.h /usr/include/c++/4.2.1/cctype \
  /usr/include/ctype.h /usr/include/runetype.h \
  /usr/include/c++/4.2.1/bits/postypes.h /usr/include/c++/4.2.1/cwchar \
  /usr/include/c++/4.2.1/ctime /usr/include/wchar.h \
  /usr/include/_wctype.h /usr/include/stdint.h \
  /usr/include/c++/4.2.1/bits/functexcept.h \
  /usr/include/c++/4.2.1/exception_defines.h \
  /usr/include/c++/4.2.1/bits/stl_pair.h \
  /usr/include/c++/4.2.1/bits/cpp_type_traits.h \
  /usr/include/c++/4.2.1/ext/type_traits.h /usr/include/c++/4.2.1/utility \
  /usr/include/c++/4.2.1/bits/stl_relops.h \
  /usr/include/c++/4.2.1/bits/stl_iterator_base_types.h \
  /usr/include/c++/4.2.1/bits/stl_iterator_base_funcs.h \
  /usr/include/c++/4.2.1/bits/concept_check.h \
  /usr/include/c++/4.2.1/bits/stl_iterator.h \
  /usr/include/c++/4.2.1/debug/debug.h /usr/include/c++/4.2.1/memory \
  /usr/include/c++/4.2.1/bits/allocator.h \
  /usr/include/c++/4.2.1/bits/c++allocator.h \
  /usr/include/c++/4.2.1/ext/new_allocator.h /usr/include/c++/4.2.1/new \
  /usr/include/c++/4.2.1/exception \
  /usr/include/c++/4.2.1/bits/stl_construct.h \
  /usr/include/c++/4.2.1/bits/stl_uninitialized.h \
  /usr/include/c++/4.2.1/bits/stl_raw_storage_iter.h \
  /usr/include/c++/4.2.1/limits \
  /usr/include/c++/4.2.1/bits/ostream_insert.h \
  /usr/include/c++/4.2.1/bits/stl_function.h \
  /usr/include/c++/4.2.1/bits/basic_string.h \
  /usr/include/c++/4.2.1/ext/atomicity.h \
  /usr/include/c++/4.2.1/bits/atomic_word.h \
  /usr/include/c++/4.2.1/algorithm /usr/include/c++/4.2.1/bits/stl_algo.h \
  /usr/include/c++/4.2.1/bits/stl_heap.h \
  /usr/include/c++/4.2.1/bits/stl_tempbuf.h \
  /usr/include/c++/4.2.1/bits/basic_string.tcc sli_name.h \
  /usr/include/c++/4.2.1/cassert /usr/include/assert.h \
  /usr/include/c++/4.2.1/map /usr/include/c++/4.2.1/bits/stl_tree.h \
  /usr/include/c++/4.2.1/bits/stl_map.h \
  /usr/include/c++/4.2.1/bits/stl_multimap.h /usr/include/c++/4.2.1/deque \
  /usr/include/c++/4.2.1/bits/stl_deque.h \
  /usr/include/c++/4.2.1/bits/deque.tcc /usr/include/c++/4.2.1/iostream \
  /usr/include/c++/4.2.1/ostream /usr/include/c++/4.2.1/ios \
  /usr/include/c++/4.2.1/bits/localefwd.h \
  /usr/include/c++/4.2.1/bits/ios_base.h \
  /usr/include/c++/4.2.1/bits/locale_classes.h \
  /usr/include/c++/4.2.1/streambuf \
  /usr/include/c++/4.2.1/bits/streambuf.tcc \
  /usr/include/c++/4.2.1/bits/basic_ios.h \
  /usr/include/c++/4.2.1/bits/streambuf_iterator.h \
  /usr/include/c++/4.2.1/bits/locale_facets.h \
  /usr/include/c++/4.2.1/cwctype /usr/include/wctype.h \
  /usr/include/c++/4.2.1/bits/ctype_base.h \
  /usr/include/c++/4.2.1/bits/ctype_inline.h \
  /usr/include/c++/4.2.1/bits/codecvt.h \
  /usr/include/c++/4.2.1/bits/time_members.h \
  /usr/include/c++/4.2.1/bits/messages_members.h \
  /usr/include/c++/4.2.1/bits/basic_ios.tcc \
  /usr/include/c++/4.2.1/bits/ostream.tcc /usr/include/c++/4.2.1/locale \
  /usr/include/c++/4.2.1/bits/locale_facets.tcc \
  /usr/include/c++/4.2.1/typeinfo /usr/include/c++/4.2.1/istream \
  /usr/include/c++/4.2.1/bits/istream.tcc sli_interpreter.h sli_token.h \
  sli_exceptions.h /usr/include/c++/4.2.1/vector \
  /usr/include/c++/4.2.1/bits/stl_vector.h \
  /usr/include/c++/4.2.1/bits/stl_bvector.h \
  /usr/include/c++/4.2.1/bits/vector.tcc sli_allocator.h sli_arraytype.h \
  sli_array.h sli_integertype.h SLI_token.h sli_tokenstack.h sli_string.h \
  sli_dictionary.h sli_dictstack.h /usr/include/c++/4.2.1/list \
  /usr/include/c++/4.2.1/bits/stl_list.h \
  /usr/include/c++/4.2.1/bits/list.tcc sli_builtins.h sli_function.h \
  sli_module.h
sli_allocator.o: sli_allocator.cpp sli_allocator.h \
  /usr/include/c++/4.2.1/cassert /usr/include/assert.h \
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
  /usr/include/i386/_types.h /usr/include/c++/4.2.1/cstdlib \
  /usr/include/c++/4.2.1/bits/c++config.h \
  /usr/include/c++/4.2.1/bits/os_defines.h /usr/include/unistd.h \
  /usr/include/sys/unistd.h /usr/include/sys/select.h \
  /usr/include/sys/_select.h /usr/include/c++/4.2.1/bits/cpu_defines.h \
  /usr/include/c++/4.2.1/cstddef \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/stddef.h \
  /usr/include/c++/4.2.1/string /usr/include/c++/4.2.1/bits/stringfwd.h \
  /usr/include/c++/4.2.1/bits/char_traits.h \
  /usr/include/c++/4.2.1/cstring /usr/include/string.h \
  /usr/include/strings.h /usr/include/c++/4.2.1/bits/stl_algobase.h \
  /usr/include/c++/4.2.1/climits \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/limits.h \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/syslimits.h \
  /usr/include/limits.h /usr/include/machine/limits.h \
  /usr/include/i386/limits.h /usr/include/i386/_limits.h \
  /usr/include/sys/syslimits.h /usr/include/c++/4.2.1/iosfwd \
  /usr/include/c++/4.2.1/bits/c++locale.h /usr/include/c++/4.2.1/clocale \
  /usr/include/locale.h /usr/include/_locale.h \
  /usr/include/c++/4.2.1/cstdio /usr/include/stdio.h \
  /usr/include/c++/4.2.1/cstdarg \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/stdarg.h \
  /usr/include/c++/4.2.1/bits/c++io.h /usr/include/c++/4.2.1/bits/gthr.h \
  /usr/include/c++/4.2.1/bits/gthr-default.h /usr/include/pthread.h \
  /usr/include/pthread_impl.h /usr/include/sched.h /usr/include/time.h \
  /usr/include/_structs.h /usr/include/c++/4.2.1/cctype \
  /usr/include/ctype.h /usr/include/runetype.h \
  /usr/include/c++/4.2.1/bits/postypes.h /usr/include/c++/4.2.1/cwchar \
  /usr/include/c++/4.2.1/ctime /usr/include/wchar.h \
  /usr/include/_wctype.h /usr/include/stdint.h \
  /usr/include/c++/4.2.1/bits/functexcept.h \
  /usr/include/c++/4.2.1/exception_defines.h \
  /usr/include/c++/4.2.1/bits/stl_pair.h \
  /usr/include/c++/4.2.1/bits/cpp_type_traits.h \
  /usr/include/c++/4.2.1/ext/type_traits.h /usr/include/c++/4.2.1/utility \
  /usr/include/c++/4.2.1/bits/stl_relops.h \
  /usr/include/c++/4.2.1/bits/stl_iterator_base_types.h \
  /usr/include/c++/4.2.1/bits/stl_iterator_base_funcs.h \
  /usr/include/c++/4.2.1/bits/concept_check.h \
  /usr/include/c++/4.2.1/bits/stl_iterator.h \
  /usr/include/c++/4.2.1/debug/debug.h /usr/include/c++/4.2.1/memory \
  /usr/include/c++/4.2.1/bits/allocator.h \
  /usr/include/c++/4.2.1/bits/c++allocator.h \
  /usr/include/c++/4.2.1/ext/new_allocator.h /usr/include/c++/4.2.1/new \
  /usr/include/c++/4.2.1/exception \
  /usr/include/c++/4.2.1/bits/stl_construct.h \
  /usr/include/c++/4.2.1/bits/stl_uninitialized.h \
  /usr/include/c++/4.2.1/bits/stl_raw_storage_iter.h \
  /usr/include/c++/4.2.1/limits \
  /usr/include/c++/4.2.1/bits/ostream_insert.h \
  /usr/include/c++/4.2.1/bits/stl_function.h \
  /usr/include/c++/4.2.1/bits/basic_string.h \
  /usr/include/c++/4.2.1/ext/atomicity.h \
  /usr/include/c++/4.2.1/bits/atomic_word.h \
  /usr/include/c++/4.2.1/algorithm /usr/include/c++/4.2.1/bits/stl_algo.h \
  /usr/include/c++/4.2.1/bits/stl_heap.h \
  /usr/include/c++/4.2.1/bits/stl_tempbuf.h \
  /usr/include/c++/4.2.1/bits/basic_string.tcc
sli_tokenstack.o: sli_tokenstack.cpp sli_tokenstack.h sli_token.h \
  /usr/include/c++/4.2.1/iostream /usr/include/c++/4.2.1/bits/c++config.h \
  /usr/include/c++/4.2.1/bits/os_defines.h /usr/include/unistd.h \
  /usr/include/_types.h /usr/include/sys/_types.h \
  /usr/include/sys/cdefs.h /usr/include/sys/_symbol_aliasing.h \
  /usr/include/sys/_posix_availability.h /usr/include/machine/_types.h \
  /usr/include/i386/_types.h /usr/include/sys/unistd.h \
  /usr/include/Availability.h /usr/include/AvailabilityInternal.h \
  /usr/include/sys/select.h /usr/include/sys/appleapiopts.h \
  /usr/include/sys/_structs.h /usr/include/sys/_select.h \
  /usr/include/c++/4.2.1/bits/cpu_defines.h \
  /usr/include/c++/4.2.1/ostream /usr/include/c++/4.2.1/ios \
  /usr/include/c++/4.2.1/iosfwd /usr/include/c++/4.2.1/bits/c++locale.h \
  /usr/include/c++/4.2.1/clocale /usr/include/locale.h \
  /usr/include/_locale.h /usr/include/c++/4.2.1/cstring \
  /usr/include/c++/4.2.1/cstddef \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/stddef.h \
  /usr/include/string.h /usr/include/strings.h \
  /usr/include/c++/4.2.1/cstdio /usr/include/stdio.h \
  /usr/include/c++/4.2.1/cstdarg \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/stdarg.h \
  /usr/include/c++/4.2.1/bits/c++io.h /usr/include/c++/4.2.1/bits/gthr.h \
  /usr/include/c++/4.2.1/bits/gthr-default.h /usr/include/pthread.h \
  /usr/include/pthread_impl.h /usr/include/sched.h /usr/include/time.h \
  /usr/include/_structs.h /usr/include/c++/4.2.1/cctype \
  /usr/include/ctype.h /usr/include/runetype.h \
  /usr/include/c++/4.2.1/bits/stringfwd.h \
  /usr/include/c++/4.2.1/bits/postypes.h /usr/include/c++/4.2.1/cwchar \
  /usr/include/c++/4.2.1/ctime /usr/include/wchar.h \
  /usr/include/_wctype.h /usr/include/stdint.h \
  /usr/include/c++/4.2.1/bits/functexcept.h \
  /usr/include/c++/4.2.1/exception_defines.h \
  /usr/include/c++/4.2.1/exception \
  /usr/include/c++/4.2.1/bits/char_traits.h \
  /usr/include/c++/4.2.1/bits/stl_algobase.h \
  /usr/include/c++/4.2.1/climits \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/limits.h \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/syslimits.h \
  /usr/include/limits.h /usr/include/machine/limits.h \
  /usr/include/i386/limits.h /usr/include/i386/_limits.h \
  /usr/include/sys/syslimits.h /usr/include/c++/4.2.1/cstdlib \
  /usr/include/stdlib.h /usr/include/sys/wait.h /usr/include/sys/signal.h \
  /usr/include/machine/signal.h /usr/include/i386/signal.h \
  /usr/include/i386/_structs.h /usr/include/machine/_structs.h \
  /usr/include/mach/i386/_structs.h /usr/include/sys/resource.h \
  /usr/include/machine/endian.h /usr/include/i386/endian.h \
  /usr/include/sys/_endian.h /usr/include/libkern/_OSByteOrder.h \
  /usr/include/libkern/i386/_OSByteOrder.h /usr/include/alloca.h \
  /usr/include/machine/types.h /usr/include/i386/types.h \
  /usr/include/i386/_types.h /usr/include/c++/4.2.1/bits/stl_pair.h \
  /usr/include/c++/4.2.1/bits/cpp_type_traits.h \
  /usr/include/c++/4.2.1/ext/type_traits.h /usr/include/c++/4.2.1/utility \
  /usr/include/c++/4.2.1/bits/stl_relops.h \
  /usr/include/c++/4.2.1/bits/stl_iterator_base_types.h \
  /usr/include/c++/4.2.1/bits/stl_iterator_base_funcs.h \
  /usr/include/c++/4.2.1/bits/concept_check.h \
  /usr/include/c++/4.2.1/bits/stl_iterator.h \
  /usr/include/c++/4.2.1/debug/debug.h \
  /usr/include/c++/4.2.1/bits/localefwd.h \
  /usr/include/c++/4.2.1/bits/ios_base.h \
  /usr/include/c++/4.2.1/ext/atomicity.h \
  /usr/include/c++/4.2.1/bits/atomic_word.h \
  /usr/include/c++/4.2.1/bits/locale_classes.h \
  /usr/include/c++/4.2.1/string /usr/include/c++/4.2.1/memory \
  /usr/include/c++/4.2.1/bits/allocator.h \
  /usr/include/c++/4.2.1/bits/c++allocator.h \
  /usr/include/c++/4.2.1/ext/new_allocator.h /usr/include/c++/4.2.1/new \
  /usr/include/c++/4.2.1/bits/stl_construct.h \
  /usr/include/c++/4.2.1/bits/stl_uninitialized.h \
  /usr/include/c++/4.2.1/bits/stl_raw_storage_iter.h \
  /usr/include/c++/4.2.1/limits \
  /usr/include/c++/4.2.1/bits/ostream_insert.h \
  /usr/include/c++/4.2.1/bits/stl_function.h \
  /usr/include/c++/4.2.1/bits/basic_string.h \
  /usr/include/c++/4.2.1/algorithm /usr/include/c++/4.2.1/bits/stl_algo.h \
  /usr/include/c++/4.2.1/bits/stl_heap.h \
  /usr/include/c++/4.2.1/bits/stl_tempbuf.h \
  /usr/include/c++/4.2.1/bits/basic_string.tcc \
  /usr/include/c++/4.2.1/streambuf \
  /usr/include/c++/4.2.1/bits/streambuf.tcc \
  /usr/include/c++/4.2.1/bits/basic_ios.h \
  /usr/include/c++/4.2.1/bits/streambuf_iterator.h \
  /usr/include/c++/4.2.1/bits/locale_facets.h \
  /usr/include/c++/4.2.1/cwctype /usr/include/wctype.h \
  /usr/include/c++/4.2.1/bits/ctype_base.h \
  /usr/include/c++/4.2.1/bits/ctype_inline.h \
  /usr/include/c++/4.2.1/bits/codecvt.h \
  /usr/include/c++/4.2.1/bits/time_members.h \
  /usr/include/c++/4.2.1/bits/messages_members.h \
  /usr/include/c++/4.2.1/bits/basic_ios.tcc \
  /usr/include/c++/4.2.1/bits/ostream.tcc /usr/include/c++/4.2.1/locale \
  /usr/include/c++/4.2.1/bits/locale_facets.tcc \
  /usr/include/c++/4.2.1/typeinfo /usr/include/c++/4.2.1/istream \
  /usr/include/c++/4.2.1/bits/istream.tcc sli_type.h sli_name.h \
  /usr/include/c++/4.2.1/cassert /usr/include/assert.h \
  /usr/include/c++/4.2.1/map /usr/include/c++/4.2.1/bits/stl_tree.h \
  /usr/include/c++/4.2.1/bits/stl_map.h \
  /usr/include/c++/4.2.1/bits/stl_multimap.h /usr/include/c++/4.2.1/deque \
  /usr/include/c++/4.2.1/bits/stl_deque.h \
  /usr/include/c++/4.2.1/bits/deque.tcc sli_exceptions.h \
  /usr/include/c++/4.2.1/vector /usr/include/c++/4.2.1/bits/stl_vector.h \
  /usr/include/c++/4.2.1/bits/stl_bvector.h \
  /usr/include/c++/4.2.1/bits/vector.tcc sli_array.h sli_allocator.h
sli_name.o: sli_name.cpp sli_name.h /usr/include/c++/4.2.1/cassert \
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
  /usr/include/i386/_types.h /usr/include/c++/4.2.1/map \
  /usr/include/c++/4.2.1/bits/stl_tree.h \
  /usr/include/c++/4.2.1/bits/stl_algobase.h \
  /usr/include/c++/4.2.1/bits/c++config.h \
  /usr/include/c++/4.2.1/bits/os_defines.h /usr/include/unistd.h \
  /usr/include/sys/unistd.h /usr/include/sys/select.h \
  /usr/include/sys/_select.h /usr/include/c++/4.2.1/bits/cpu_defines.h \
  /usr/include/c++/4.2.1/cstring /usr/include/c++/4.2.1/cstddef \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/stddef.h \
  /usr/include/string.h /usr/include/strings.h \
  /usr/include/c++/4.2.1/climits \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/limits.h \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/syslimits.h \
  /usr/include/limits.h /usr/include/machine/limits.h \
  /usr/include/i386/limits.h /usr/include/i386/_limits.h \
  /usr/include/sys/syslimits.h /usr/include/c++/4.2.1/cstdlib \
  /usr/include/c++/4.2.1/iosfwd /usr/include/c++/4.2.1/bits/c++locale.h \
  /usr/include/c++/4.2.1/clocale /usr/include/locale.h \
  /usr/include/_locale.h /usr/include/c++/4.2.1/cstdio \
  /usr/include/stdio.h /usr/include/c++/4.2.1/cstdarg \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/stdarg.h \
  /usr/include/c++/4.2.1/bits/c++io.h /usr/include/c++/4.2.1/bits/gthr.h \
  /usr/include/c++/4.2.1/bits/gthr-default.h /usr/include/pthread.h \
  /usr/include/pthread_impl.h /usr/include/sched.h /usr/include/time.h \
  /usr/include/_structs.h /usr/include/c++/4.2.1/cctype \
  /usr/include/ctype.h /usr/include/runetype.h \
  /usr/include/c++/4.2.1/bits/stringfwd.h \
  /usr/include/c++/4.2.1/bits/postypes.h /usr/include/c++/4.2.1/cwchar \
  /usr/include/c++/4.2.1/ctime /usr/include/wchar.h \
  /usr/include/_wctype.h /usr/include/stdint.h \
  /usr/include/c++/4.2.1/bits/functexcept.h \
  /usr/include/c++/4.2.1/exception_defines.h \
  /usr/include/c++/4.2.1/bits/stl_pair.h \
  /usr/include/c++/4.2.1/bits/cpp_type_traits.h \
  /usr/include/c++/4.2.1/ext/type_traits.h /usr/include/c++/4.2.1/utility \
  /usr/include/c++/4.2.1/bits/stl_relops.h \
  /usr/include/c++/4.2.1/bits/stl_iterator_base_types.h \
  /usr/include/c++/4.2.1/bits/stl_iterator_base_funcs.h \
  /usr/include/c++/4.2.1/bits/concept_check.h \
  /usr/include/c++/4.2.1/bits/stl_iterator.h \
  /usr/include/c++/4.2.1/debug/debug.h \
  /usr/include/c++/4.2.1/bits/allocator.h \
  /usr/include/c++/4.2.1/bits/c++allocator.h \
  /usr/include/c++/4.2.1/ext/new_allocator.h /usr/include/c++/4.2.1/new \
  /usr/include/c++/4.2.1/exception \
  /usr/include/c++/4.2.1/bits/stl_construct.h \
  /usr/include/c++/4.2.1/bits/stl_function.h \
  /usr/include/c++/4.2.1/bits/stl_map.h \
  /usr/include/c++/4.2.1/bits/stl_multimap.h \
  /usr/include/c++/4.2.1/string /usr/include/c++/4.2.1/bits/char_traits.h \
  /usr/include/c++/4.2.1/memory \
  /usr/include/c++/4.2.1/bits/stl_uninitialized.h \
  /usr/include/c++/4.2.1/bits/stl_raw_storage_iter.h \
  /usr/include/c++/4.2.1/limits \
  /usr/include/c++/4.2.1/bits/ostream_insert.h \
  /usr/include/c++/4.2.1/bits/basic_string.h \
  /usr/include/c++/4.2.1/ext/atomicity.h \
  /usr/include/c++/4.2.1/bits/atomic_word.h \
  /usr/include/c++/4.2.1/algorithm /usr/include/c++/4.2.1/bits/stl_algo.h \
  /usr/include/c++/4.2.1/bits/stl_heap.h \
  /usr/include/c++/4.2.1/bits/stl_tempbuf.h \
  /usr/include/c++/4.2.1/bits/basic_string.tcc \
  /usr/include/c++/4.2.1/deque /usr/include/c++/4.2.1/bits/stl_deque.h \
  /usr/include/c++/4.2.1/bits/deque.tcc /usr/include/c++/4.2.1/iostream \
  /usr/include/c++/4.2.1/ostream /usr/include/c++/4.2.1/ios \
  /usr/include/c++/4.2.1/bits/localefwd.h \
  /usr/include/c++/4.2.1/bits/ios_base.h \
  /usr/include/c++/4.2.1/bits/locale_classes.h \
  /usr/include/c++/4.2.1/streambuf \
  /usr/include/c++/4.2.1/bits/streambuf.tcc \
  /usr/include/c++/4.2.1/bits/basic_ios.h \
  /usr/include/c++/4.2.1/bits/streambuf_iterator.h \
  /usr/include/c++/4.2.1/bits/locale_facets.h \
  /usr/include/c++/4.2.1/cwctype /usr/include/wctype.h \
  /usr/include/c++/4.2.1/bits/ctype_base.h \
  /usr/include/c++/4.2.1/bits/ctype_inline.h \
  /usr/include/c++/4.2.1/bits/codecvt.h \
  /usr/include/c++/4.2.1/bits/time_members.h \
  /usr/include/c++/4.2.1/bits/messages_members.h \
  /usr/include/c++/4.2.1/bits/basic_ios.tcc \
  /usr/include/c++/4.2.1/bits/ostream.tcc /usr/include/c++/4.2.1/locale \
  /usr/include/c++/4.2.1/bits/locale_facets.tcc \
  /usr/include/c++/4.2.1/typeinfo /usr/include/c++/4.2.1/istream \
  /usr/include/c++/4.2.1/bits/istream.tcc /usr/include/c++/4.2.1/iomanip \
  /usr/include/c++/4.2.1/functional
sli_dictionary.o: sli_dictionary.cpp sli_dictionary.h sli_name.h \
  /usr/include/c++/4.2.1/cassert /usr/include/assert.h \
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
  /usr/include/i386/_types.h /usr/include/c++/4.2.1/map \
  /usr/include/c++/4.2.1/bits/stl_tree.h \
  /usr/include/c++/4.2.1/bits/stl_algobase.h \
  /usr/include/c++/4.2.1/bits/c++config.h \
  /usr/include/c++/4.2.1/bits/os_defines.h /usr/include/unistd.h \
  /usr/include/sys/unistd.h /usr/include/sys/select.h \
  /usr/include/sys/_select.h /usr/include/c++/4.2.1/bits/cpu_defines.h \
  /usr/include/c++/4.2.1/cstring /usr/include/c++/4.2.1/cstddef \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/stddef.h \
  /usr/include/string.h /usr/include/strings.h \
  /usr/include/c++/4.2.1/climits \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/limits.h \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/syslimits.h \
  /usr/include/limits.h /usr/include/machine/limits.h \
  /usr/include/i386/limits.h /usr/include/i386/_limits.h \
  /usr/include/sys/syslimits.h /usr/include/c++/4.2.1/cstdlib \
  /usr/include/c++/4.2.1/iosfwd /usr/include/c++/4.2.1/bits/c++locale.h \
  /usr/include/c++/4.2.1/clocale /usr/include/locale.h \
  /usr/include/_locale.h /usr/include/c++/4.2.1/cstdio \
  /usr/include/stdio.h /usr/include/c++/4.2.1/cstdarg \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/stdarg.h \
  /usr/include/c++/4.2.1/bits/c++io.h /usr/include/c++/4.2.1/bits/gthr.h \
  /usr/include/c++/4.2.1/bits/gthr-default.h /usr/include/pthread.h \
  /usr/include/pthread_impl.h /usr/include/sched.h /usr/include/time.h \
  /usr/include/_structs.h /usr/include/c++/4.2.1/cctype \
  /usr/include/ctype.h /usr/include/runetype.h \
  /usr/include/c++/4.2.1/bits/stringfwd.h \
  /usr/include/c++/4.2.1/bits/postypes.h /usr/include/c++/4.2.1/cwchar \
  /usr/include/c++/4.2.1/ctime /usr/include/wchar.h \
  /usr/include/_wctype.h /usr/include/stdint.h \
  /usr/include/c++/4.2.1/bits/functexcept.h \
  /usr/include/c++/4.2.1/exception_defines.h \
  /usr/include/c++/4.2.1/bits/stl_pair.h \
  /usr/include/c++/4.2.1/bits/cpp_type_traits.h \
  /usr/include/c++/4.2.1/ext/type_traits.h /usr/include/c++/4.2.1/utility \
  /usr/include/c++/4.2.1/bits/stl_relops.h \
  /usr/include/c++/4.2.1/bits/stl_iterator_base_types.h \
  /usr/include/c++/4.2.1/bits/stl_iterator_base_funcs.h \
  /usr/include/c++/4.2.1/bits/concept_check.h \
  /usr/include/c++/4.2.1/bits/stl_iterator.h \
  /usr/include/c++/4.2.1/debug/debug.h \
  /usr/include/c++/4.2.1/bits/allocator.h \
  /usr/include/c++/4.2.1/bits/c++allocator.h \
  /usr/include/c++/4.2.1/ext/new_allocator.h /usr/include/c++/4.2.1/new \
  /usr/include/c++/4.2.1/exception \
  /usr/include/c++/4.2.1/bits/stl_construct.h \
  /usr/include/c++/4.2.1/bits/stl_function.h \
  /usr/include/c++/4.2.1/bits/stl_map.h \
  /usr/include/c++/4.2.1/bits/stl_multimap.h \
  /usr/include/c++/4.2.1/string /usr/include/c++/4.2.1/bits/char_traits.h \
  /usr/include/c++/4.2.1/memory \
  /usr/include/c++/4.2.1/bits/stl_uninitialized.h \
  /usr/include/c++/4.2.1/bits/stl_raw_storage_iter.h \
  /usr/include/c++/4.2.1/limits \
  /usr/include/c++/4.2.1/bits/ostream_insert.h \
  /usr/include/c++/4.2.1/bits/basic_string.h \
  /usr/include/c++/4.2.1/ext/atomicity.h \
  /usr/include/c++/4.2.1/bits/atomic_word.h \
  /usr/include/c++/4.2.1/algorithm /usr/include/c++/4.2.1/bits/stl_algo.h \
  /usr/include/c++/4.2.1/bits/stl_heap.h \
  /usr/include/c++/4.2.1/bits/stl_tempbuf.h \
  /usr/include/c++/4.2.1/bits/basic_string.tcc \
  /usr/include/c++/4.2.1/deque /usr/include/c++/4.2.1/bits/stl_deque.h \
  /usr/include/c++/4.2.1/bits/deque.tcc /usr/include/c++/4.2.1/iostream \
  /usr/include/c++/4.2.1/ostream /usr/include/c++/4.2.1/ios \
  /usr/include/c++/4.2.1/bits/localefwd.h \
  /usr/include/c++/4.2.1/bits/ios_base.h \
  /usr/include/c++/4.2.1/bits/locale_classes.h \
  /usr/include/c++/4.2.1/streambuf \
  /usr/include/c++/4.2.1/bits/streambuf.tcc \
  /usr/include/c++/4.2.1/bits/basic_ios.h \
  /usr/include/c++/4.2.1/bits/streambuf_iterator.h \
  /usr/include/c++/4.2.1/bits/locale_facets.h \
  /usr/include/c++/4.2.1/cwctype /usr/include/wctype.h \
  /usr/include/c++/4.2.1/bits/ctype_base.h \
  /usr/include/c++/4.2.1/bits/ctype_inline.h \
  /usr/include/c++/4.2.1/bits/codecvt.h \
  /usr/include/c++/4.2.1/bits/time_members.h \
  /usr/include/c++/4.2.1/bits/messages_members.h \
  /usr/include/c++/4.2.1/bits/basic_ios.tcc \
  /usr/include/c++/4.2.1/bits/ostream.tcc /usr/include/c++/4.2.1/locale \
  /usr/include/c++/4.2.1/bits/locale_facets.tcc \
  /usr/include/c++/4.2.1/typeinfo /usr/include/c++/4.2.1/istream \
  /usr/include/c++/4.2.1/bits/istream.tcc sli_token.h sli_type.h \
  sli_exceptions.h /usr/include/c++/4.2.1/vector \
  /usr/include/c++/4.2.1/bits/stl_vector.h \
  /usr/include/c++/4.2.1/bits/stl_bvector.h \
  /usr/include/c++/4.2.1/bits/vector.tcc sli_allocator.h \
  /usr/include/c++/4.2.1/iomanip /usr/include/c++/4.2.1/functional
sli_exceptions.o: sli_exceptions.cpp config.h sli_exceptions.h sli_name.h \
  /usr/include/c++/4.2.1/cassert /usr/include/assert.h \
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
  /usr/include/i386/_types.h /usr/include/c++/4.2.1/map \
  /usr/include/c++/4.2.1/bits/stl_tree.h \
  /usr/include/c++/4.2.1/bits/stl_algobase.h \
  /usr/include/c++/4.2.1/bits/c++config.h \
  /usr/include/c++/4.2.1/bits/os_defines.h /usr/include/unistd.h \
  /usr/include/sys/unistd.h /usr/include/sys/select.h \
  /usr/include/sys/_select.h /usr/include/c++/4.2.1/bits/cpu_defines.h \
  /usr/include/c++/4.2.1/cstring /usr/include/c++/4.2.1/cstddef \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/stddef.h \
  /usr/include/string.h /usr/include/strings.h \
  /usr/include/c++/4.2.1/climits \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/limits.h \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/syslimits.h \
  /usr/include/limits.h /usr/include/machine/limits.h \
  /usr/include/i386/limits.h /usr/include/i386/_limits.h \
  /usr/include/sys/syslimits.h /usr/include/c++/4.2.1/cstdlib \
  /usr/include/c++/4.2.1/iosfwd /usr/include/c++/4.2.1/bits/c++locale.h \
  /usr/include/c++/4.2.1/clocale /usr/include/locale.h \
  /usr/include/_locale.h /usr/include/c++/4.2.1/cstdio \
  /usr/include/stdio.h /usr/include/c++/4.2.1/cstdarg \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/stdarg.h \
  /usr/include/c++/4.2.1/bits/c++io.h /usr/include/c++/4.2.1/bits/gthr.h \
  /usr/include/c++/4.2.1/bits/gthr-default.h /usr/include/pthread.h \
  /usr/include/pthread_impl.h /usr/include/sched.h /usr/include/time.h \
  /usr/include/_structs.h /usr/include/c++/4.2.1/cctype \
  /usr/include/ctype.h /usr/include/runetype.h \
  /usr/include/c++/4.2.1/bits/stringfwd.h \
  /usr/include/c++/4.2.1/bits/postypes.h /usr/include/c++/4.2.1/cwchar \
  /usr/include/c++/4.2.1/ctime /usr/include/wchar.h \
  /usr/include/_wctype.h /usr/include/stdint.h \
  /usr/include/c++/4.2.1/bits/functexcept.h \
  /usr/include/c++/4.2.1/exception_defines.h \
  /usr/include/c++/4.2.1/bits/stl_pair.h \
  /usr/include/c++/4.2.1/bits/cpp_type_traits.h \
  /usr/include/c++/4.2.1/ext/type_traits.h /usr/include/c++/4.2.1/utility \
  /usr/include/c++/4.2.1/bits/stl_relops.h \
  /usr/include/c++/4.2.1/bits/stl_iterator_base_types.h \
  /usr/include/c++/4.2.1/bits/stl_iterator_base_funcs.h \
  /usr/include/c++/4.2.1/bits/concept_check.h \
  /usr/include/c++/4.2.1/bits/stl_iterator.h \
  /usr/include/c++/4.2.1/debug/debug.h \
  /usr/include/c++/4.2.1/bits/allocator.h \
  /usr/include/c++/4.2.1/bits/c++allocator.h \
  /usr/include/c++/4.2.1/ext/new_allocator.h /usr/include/c++/4.2.1/new \
  /usr/include/c++/4.2.1/exception \
  /usr/include/c++/4.2.1/bits/stl_construct.h \
  /usr/include/c++/4.2.1/bits/stl_function.h \
  /usr/include/c++/4.2.1/bits/stl_map.h \
  /usr/include/c++/4.2.1/bits/stl_multimap.h \
  /usr/include/c++/4.2.1/string /usr/include/c++/4.2.1/bits/char_traits.h \
  /usr/include/c++/4.2.1/memory \
  /usr/include/c++/4.2.1/bits/stl_uninitialized.h \
  /usr/include/c++/4.2.1/bits/stl_raw_storage_iter.h \
  /usr/include/c++/4.2.1/limits \
  /usr/include/c++/4.2.1/bits/ostream_insert.h \
  /usr/include/c++/4.2.1/bits/basic_string.h \
  /usr/include/c++/4.2.1/ext/atomicity.h \
  /usr/include/c++/4.2.1/bits/atomic_word.h \
  /usr/include/c++/4.2.1/algorithm /usr/include/c++/4.2.1/bits/stl_algo.h \
  /usr/include/c++/4.2.1/bits/stl_heap.h \
  /usr/include/c++/4.2.1/bits/stl_tempbuf.h \
  /usr/include/c++/4.2.1/bits/basic_string.tcc \
  /usr/include/c++/4.2.1/deque /usr/include/c++/4.2.1/bits/stl_deque.h \
  /usr/include/c++/4.2.1/bits/deque.tcc /usr/include/c++/4.2.1/iostream \
  /usr/include/c++/4.2.1/ostream /usr/include/c++/4.2.1/ios \
  /usr/include/c++/4.2.1/bits/localefwd.h \
  /usr/include/c++/4.2.1/bits/ios_base.h \
  /usr/include/c++/4.2.1/bits/locale_classes.h \
  /usr/include/c++/4.2.1/streambuf \
  /usr/include/c++/4.2.1/bits/streambuf.tcc \
  /usr/include/c++/4.2.1/bits/basic_ios.h \
  /usr/include/c++/4.2.1/bits/streambuf_iterator.h \
  /usr/include/c++/4.2.1/bits/locale_facets.h \
  /usr/include/c++/4.2.1/cwctype /usr/include/wctype.h \
  /usr/include/c++/4.2.1/bits/ctype_base.h \
  /usr/include/c++/4.2.1/bits/ctype_inline.h \
  /usr/include/c++/4.2.1/bits/codecvt.h \
  /usr/include/c++/4.2.1/bits/time_members.h \
  /usr/include/c++/4.2.1/bits/messages_members.h \
  /usr/include/c++/4.2.1/bits/basic_ios.tcc \
  /usr/include/c++/4.2.1/bits/ostream.tcc /usr/include/c++/4.2.1/locale \
  /usr/include/c++/4.2.1/bits/locale_facets.tcc \
  /usr/include/c++/4.2.1/typeinfo /usr/include/c++/4.2.1/istream \
  /usr/include/c++/4.2.1/bits/istream.tcc /usr/include/c++/4.2.1/vector \
  /usr/include/c++/4.2.1/bits/stl_vector.h \
  /usr/include/c++/4.2.1/bits/stl_bvector.h \
  /usr/include/c++/4.2.1/bits/vector.tcc sli_interpreter.h sli_type.h \
  sli_token.h sli_allocator.h sli_arraytype.h sli_array.h \
  sli_integertype.h SLI_token.h sli_tokenstack.h sli_string.h \
  sli_dictionary.h sli_dictstack.h /usr/include/c++/4.2.1/list \
  /usr/include/c++/4.2.1/bits/stl_list.h \
  /usr/include/c++/4.2.1/bits/list.tcc sli_builtins.h sli_function.h \
  sli_module.h /usr/include/c++/4.2.1/sstream \
  /usr/include/c++/4.2.1/bits/sstream.tcc
sli_dictstack.o: sli_dictstack.cpp sli_interpreter.h sli_type.h \
  /usr/include/c++/4.2.1/string /usr/include/c++/4.2.1/bits/c++config.h \
  /usr/include/c++/4.2.1/bits/os_defines.h /usr/include/unistd.h \
  /usr/include/_types.h /usr/include/sys/_types.h \
  /usr/include/sys/cdefs.h /usr/include/sys/_symbol_aliasing.h \
  /usr/include/sys/_posix_availability.h /usr/include/machine/_types.h \
  /usr/include/i386/_types.h /usr/include/sys/unistd.h \
  /usr/include/Availability.h /usr/include/AvailabilityInternal.h \
  /usr/include/sys/select.h /usr/include/sys/appleapiopts.h \
  /usr/include/sys/_structs.h /usr/include/sys/_select.h \
  /usr/include/c++/4.2.1/bits/cpu_defines.h \
  /usr/include/c++/4.2.1/bits/stringfwd.h \
  /usr/include/c++/4.2.1/bits/char_traits.h \
  /usr/include/c++/4.2.1/cstring /usr/include/c++/4.2.1/cstddef \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/stddef.h \
  /usr/include/string.h /usr/include/strings.h \
  /usr/include/c++/4.2.1/bits/stl_algobase.h \
  /usr/include/c++/4.2.1/climits \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/limits.h \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/syslimits.h \
  /usr/include/limits.h /usr/include/machine/limits.h \
  /usr/include/i386/limits.h /usr/include/i386/_limits.h \
  /usr/include/sys/syslimits.h /usr/include/c++/4.2.1/cstdlib \
  /usr/include/stdlib.h /usr/include/sys/wait.h /usr/include/sys/signal.h \
  /usr/include/machine/signal.h /usr/include/i386/signal.h \
  /usr/include/i386/_structs.h /usr/include/machine/_structs.h \
  /usr/include/mach/i386/_structs.h /usr/include/sys/resource.h \
  /usr/include/machine/endian.h /usr/include/i386/endian.h \
  /usr/include/sys/_endian.h /usr/include/libkern/_OSByteOrder.h \
  /usr/include/libkern/i386/_OSByteOrder.h /usr/include/alloca.h \
  /usr/include/machine/types.h /usr/include/i386/types.h \
  /usr/include/i386/_types.h /usr/include/c++/4.2.1/iosfwd \
  /usr/include/c++/4.2.1/bits/c++locale.h /usr/include/c++/4.2.1/clocale \
  /usr/include/locale.h /usr/include/_locale.h \
  /usr/include/c++/4.2.1/cstdio /usr/include/stdio.h \
  /usr/include/c++/4.2.1/cstdarg \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/stdarg.h \
  /usr/include/c++/4.2.1/bits/c++io.h /usr/include/c++/4.2.1/bits/gthr.h \
  /usr/include/c++/4.2.1/bits/gthr-default.h /usr/include/pthread.h \
  /usr/include/pthread_impl.h /usr/include/sched.h /usr/include/time.h \
  /usr/include/_structs.h /usr/include/c++/4.2.1/cctype \
  /usr/include/ctype.h /usr/include/runetype.h \
  /usr/include/c++/4.2.1/bits/postypes.h /usr/include/c++/4.2.1/cwchar \
  /usr/include/c++/4.2.1/ctime /usr/include/wchar.h \
  /usr/include/_wctype.h /usr/include/stdint.h \
  /usr/include/c++/4.2.1/bits/functexcept.h \
  /usr/include/c++/4.2.1/exception_defines.h \
  /usr/include/c++/4.2.1/bits/stl_pair.h \
  /usr/include/c++/4.2.1/bits/cpp_type_traits.h \
  /usr/include/c++/4.2.1/ext/type_traits.h /usr/include/c++/4.2.1/utility \
  /usr/include/c++/4.2.1/bits/stl_relops.h \
  /usr/include/c++/4.2.1/bits/stl_iterator_base_types.h \
  /usr/include/c++/4.2.1/bits/stl_iterator_base_funcs.h \
  /usr/include/c++/4.2.1/bits/concept_check.h \
  /usr/include/c++/4.2.1/bits/stl_iterator.h \
  /usr/include/c++/4.2.1/debug/debug.h /usr/include/c++/4.2.1/memory \
  /usr/include/c++/4.2.1/bits/allocator.h \
  /usr/include/c++/4.2.1/bits/c++allocator.h \
  /usr/include/c++/4.2.1/ext/new_allocator.h /usr/include/c++/4.2.1/new \
  /usr/include/c++/4.2.1/exception \
  /usr/include/c++/4.2.1/bits/stl_construct.h \
  /usr/include/c++/4.2.1/bits/stl_uninitialized.h \
  /usr/include/c++/4.2.1/bits/stl_raw_storage_iter.h \
  /usr/include/c++/4.2.1/limits \
  /usr/include/c++/4.2.1/bits/ostream_insert.h \
  /usr/include/c++/4.2.1/bits/stl_function.h \
  /usr/include/c++/4.2.1/bits/basic_string.h \
  /usr/include/c++/4.2.1/ext/atomicity.h \
  /usr/include/c++/4.2.1/bits/atomic_word.h \
  /usr/include/c++/4.2.1/algorithm /usr/include/c++/4.2.1/bits/stl_algo.h \
  /usr/include/c++/4.2.1/bits/stl_heap.h \
  /usr/include/c++/4.2.1/bits/stl_tempbuf.h \
  /usr/include/c++/4.2.1/bits/basic_string.tcc sli_name.h \
  /usr/include/c++/4.2.1/cassert /usr/include/assert.h \
  /usr/include/c++/4.2.1/map /usr/include/c++/4.2.1/bits/stl_tree.h \
  /usr/include/c++/4.2.1/bits/stl_map.h \
  /usr/include/c++/4.2.1/bits/stl_multimap.h /usr/include/c++/4.2.1/deque \
  /usr/include/c++/4.2.1/bits/stl_deque.h \
  /usr/include/c++/4.2.1/bits/deque.tcc /usr/include/c++/4.2.1/iostream \
  /usr/include/c++/4.2.1/ostream /usr/include/c++/4.2.1/ios \
  /usr/include/c++/4.2.1/bits/localefwd.h \
  /usr/include/c++/4.2.1/bits/ios_base.h \
  /usr/include/c++/4.2.1/bits/locale_classes.h \
  /usr/include/c++/4.2.1/streambuf \
  /usr/include/c++/4.2.1/bits/streambuf.tcc \
  /usr/include/c++/4.2.1/bits/basic_ios.h \
  /usr/include/c++/4.2.1/bits/streambuf_iterator.h \
  /usr/include/c++/4.2.1/bits/locale_facets.h \
  /usr/include/c++/4.2.1/cwctype /usr/include/wctype.h \
  /usr/include/c++/4.2.1/bits/ctype_base.h \
  /usr/include/c++/4.2.1/bits/ctype_inline.h \
  /usr/include/c++/4.2.1/bits/codecvt.h \
  /usr/include/c++/4.2.1/bits/time_members.h \
  /usr/include/c++/4.2.1/bits/messages_members.h \
  /usr/include/c++/4.2.1/bits/basic_ios.tcc \
  /usr/include/c++/4.2.1/bits/ostream.tcc /usr/include/c++/4.2.1/locale \
  /usr/include/c++/4.2.1/bits/locale_facets.tcc \
  /usr/include/c++/4.2.1/typeinfo /usr/include/c++/4.2.1/istream \
  /usr/include/c++/4.2.1/bits/istream.tcc sli_token.h sli_exceptions.h \
  /usr/include/c++/4.2.1/vector /usr/include/c++/4.2.1/bits/stl_vector.h \
  /usr/include/c++/4.2.1/bits/stl_bvector.h \
  /usr/include/c++/4.2.1/bits/vector.tcc sli_allocator.h sli_arraytype.h \
  sli_array.h sli_integertype.h SLI_token.h sli_tokenstack.h sli_string.h \
  sli_dictionary.h sli_dictstack.h /usr/include/c++/4.2.1/list \
  /usr/include/c++/4.2.1/bits/stl_list.h \
  /usr/include/c++/4.2.1/bits/list.tcc sli_builtins.h sli_function.h \
  sli_module.h
sli_charcode.o: sli_charcode.cpp sli_charcode.h \
  /usr/include/c++/4.2.1/vector /usr/include/c++/4.2.1/bits/functexcept.h \
  /usr/include/c++/4.2.1/bits/c++config.h \
  /usr/include/c++/4.2.1/bits/os_defines.h /usr/include/unistd.h \
  /usr/include/_types.h /usr/include/sys/_types.h \
  /usr/include/sys/cdefs.h /usr/include/sys/_symbol_aliasing.h \
  /usr/include/sys/_posix_availability.h /usr/include/machine/_types.h \
  /usr/include/i386/_types.h /usr/include/sys/unistd.h \
  /usr/include/Availability.h /usr/include/AvailabilityInternal.h \
  /usr/include/sys/select.h /usr/include/sys/appleapiopts.h \
  /usr/include/sys/_structs.h /usr/include/sys/_select.h \
  /usr/include/c++/4.2.1/bits/cpu_defines.h \
  /usr/include/c++/4.2.1/exception_defines.h \
  /usr/include/c++/4.2.1/bits/stl_algobase.h \
  /usr/include/c++/4.2.1/cstring /usr/include/c++/4.2.1/cstddef \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/stddef.h \
  /usr/include/string.h /usr/include/strings.h \
  /usr/include/c++/4.2.1/climits \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/limits.h \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/syslimits.h \
  /usr/include/limits.h /usr/include/machine/limits.h \
  /usr/include/i386/limits.h /usr/include/i386/_limits.h \
  /usr/include/sys/syslimits.h /usr/include/c++/4.2.1/cstdlib \
  /usr/include/stdlib.h /usr/include/sys/wait.h /usr/include/sys/signal.h \
  /usr/include/machine/signal.h /usr/include/i386/signal.h \
  /usr/include/i386/_structs.h /usr/include/machine/_structs.h \
  /usr/include/mach/i386/_structs.h /usr/include/sys/resource.h \
  /usr/include/machine/endian.h /usr/include/i386/endian.h \
  /usr/include/sys/_endian.h /usr/include/libkern/_OSByteOrder.h \
  /usr/include/libkern/i386/_OSByteOrder.h /usr/include/alloca.h \
  /usr/include/machine/types.h /usr/include/i386/types.h \
  /usr/include/i386/_types.h /usr/include/c++/4.2.1/iosfwd \
  /usr/include/c++/4.2.1/bits/c++locale.h /usr/include/c++/4.2.1/clocale \
  /usr/include/locale.h /usr/include/_locale.h \
  /usr/include/c++/4.2.1/cstdio /usr/include/stdio.h \
  /usr/include/c++/4.2.1/cstdarg \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/stdarg.h \
  /usr/include/c++/4.2.1/bits/c++io.h /usr/include/c++/4.2.1/bits/gthr.h \
  /usr/include/c++/4.2.1/bits/gthr-default.h /usr/include/pthread.h \
  /usr/include/pthread_impl.h /usr/include/sched.h /usr/include/time.h \
  /usr/include/_structs.h /usr/include/c++/4.2.1/cctype \
  /usr/include/ctype.h /usr/include/runetype.h \
  /usr/include/c++/4.2.1/bits/stringfwd.h \
  /usr/include/c++/4.2.1/bits/postypes.h /usr/include/c++/4.2.1/cwchar \
  /usr/include/c++/4.2.1/ctime /usr/include/wchar.h \
  /usr/include/_wctype.h /usr/include/stdint.h \
  /usr/include/c++/4.2.1/bits/stl_pair.h \
  /usr/include/c++/4.2.1/bits/cpp_type_traits.h \
  /usr/include/c++/4.2.1/ext/type_traits.h /usr/include/c++/4.2.1/utility \
  /usr/include/c++/4.2.1/bits/stl_relops.h \
  /usr/include/c++/4.2.1/bits/stl_iterator_base_types.h \
  /usr/include/c++/4.2.1/bits/stl_iterator_base_funcs.h \
  /usr/include/c++/4.2.1/bits/concept_check.h \
  /usr/include/c++/4.2.1/bits/stl_iterator.h \
  /usr/include/c++/4.2.1/debug/debug.h \
  /usr/include/c++/4.2.1/bits/allocator.h \
  /usr/include/c++/4.2.1/bits/c++allocator.h \
  /usr/include/c++/4.2.1/ext/new_allocator.h /usr/include/c++/4.2.1/new \
  /usr/include/c++/4.2.1/exception \
  /usr/include/c++/4.2.1/bits/stl_construct.h \
  /usr/include/c++/4.2.1/bits/stl_uninitialized.h \
  /usr/include/c++/4.2.1/bits/stl_vector.h \
  /usr/include/c++/4.2.1/bits/stl_bvector.h \
  /usr/include/c++/4.2.1/bits/vector.tcc /usr/include/c++/4.2.1/cassert \
  /usr/include/assert.h
sli_scanner.o: sli_scanner.cpp sli_scanner.h sli_token.h \
  /usr/include/c++/4.2.1/iostream /usr/include/c++/4.2.1/bits/c++config.h \
  /usr/include/c++/4.2.1/bits/os_defines.h /usr/include/unistd.h \
  /usr/include/_types.h /usr/include/sys/_types.h \
  /usr/include/sys/cdefs.h /usr/include/sys/_symbol_aliasing.h \
  /usr/include/sys/_posix_availability.h /usr/include/machine/_types.h \
  /usr/include/i386/_types.h /usr/include/sys/unistd.h \
  /usr/include/Availability.h /usr/include/AvailabilityInternal.h \
  /usr/include/sys/select.h /usr/include/sys/appleapiopts.h \
  /usr/include/sys/_structs.h /usr/include/sys/_select.h \
  /usr/include/c++/4.2.1/bits/cpu_defines.h \
  /usr/include/c++/4.2.1/ostream /usr/include/c++/4.2.1/ios \
  /usr/include/c++/4.2.1/iosfwd /usr/include/c++/4.2.1/bits/c++locale.h \
  /usr/include/c++/4.2.1/clocale /usr/include/locale.h \
  /usr/include/_locale.h /usr/include/c++/4.2.1/cstring \
  /usr/include/c++/4.2.1/cstddef \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/stddef.h \
  /usr/include/string.h /usr/include/strings.h \
  /usr/include/c++/4.2.1/cstdio /usr/include/stdio.h \
  /usr/include/c++/4.2.1/cstdarg \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/stdarg.h \
  /usr/include/c++/4.2.1/bits/c++io.h /usr/include/c++/4.2.1/bits/gthr.h \
  /usr/include/c++/4.2.1/bits/gthr-default.h /usr/include/pthread.h \
  /usr/include/pthread_impl.h /usr/include/sched.h /usr/include/time.h \
  /usr/include/_structs.h /usr/include/c++/4.2.1/cctype \
  /usr/include/ctype.h /usr/include/runetype.h \
  /usr/include/c++/4.2.1/bits/stringfwd.h \
  /usr/include/c++/4.2.1/bits/postypes.h /usr/include/c++/4.2.1/cwchar \
  /usr/include/c++/4.2.1/ctime /usr/include/wchar.h \
  /usr/include/_wctype.h /usr/include/stdint.h \
  /usr/include/c++/4.2.1/bits/functexcept.h \
  /usr/include/c++/4.2.1/exception_defines.h \
  /usr/include/c++/4.2.1/exception \
  /usr/include/c++/4.2.1/bits/char_traits.h \
  /usr/include/c++/4.2.1/bits/stl_algobase.h \
  /usr/include/c++/4.2.1/climits \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/limits.h \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/syslimits.h \
  /usr/include/limits.h /usr/include/machine/limits.h \
  /usr/include/i386/limits.h /usr/include/i386/_limits.h \
  /usr/include/sys/syslimits.h /usr/include/c++/4.2.1/cstdlib \
  /usr/include/stdlib.h /usr/include/sys/wait.h /usr/include/sys/signal.h \
  /usr/include/machine/signal.h /usr/include/i386/signal.h \
  /usr/include/i386/_structs.h /usr/include/machine/_structs.h \
  /usr/include/mach/i386/_structs.h /usr/include/sys/resource.h \
  /usr/include/machine/endian.h /usr/include/i386/endian.h \
  /usr/include/sys/_endian.h /usr/include/libkern/_OSByteOrder.h \
  /usr/include/libkern/i386/_OSByteOrder.h /usr/include/alloca.h \
  /usr/include/machine/types.h /usr/include/i386/types.h \
  /usr/include/i386/_types.h /usr/include/c++/4.2.1/bits/stl_pair.h \
  /usr/include/c++/4.2.1/bits/cpp_type_traits.h \
  /usr/include/c++/4.2.1/ext/type_traits.h /usr/include/c++/4.2.1/utility \
  /usr/include/c++/4.2.1/bits/stl_relops.h \
  /usr/include/c++/4.2.1/bits/stl_iterator_base_types.h \
  /usr/include/c++/4.2.1/bits/stl_iterator_base_funcs.h \
  /usr/include/c++/4.2.1/bits/concept_check.h \
  /usr/include/c++/4.2.1/bits/stl_iterator.h \
  /usr/include/c++/4.2.1/debug/debug.h \
  /usr/include/c++/4.2.1/bits/localefwd.h \
  /usr/include/c++/4.2.1/bits/ios_base.h \
  /usr/include/c++/4.2.1/ext/atomicity.h \
  /usr/include/c++/4.2.1/bits/atomic_word.h \
  /usr/include/c++/4.2.1/bits/locale_classes.h \
  /usr/include/c++/4.2.1/string /usr/include/c++/4.2.1/memory \
  /usr/include/c++/4.2.1/bits/allocator.h \
  /usr/include/c++/4.2.1/bits/c++allocator.h \
  /usr/include/c++/4.2.1/ext/new_allocator.h /usr/include/c++/4.2.1/new \
  /usr/include/c++/4.2.1/bits/stl_construct.h \
  /usr/include/c++/4.2.1/bits/stl_uninitialized.h \
  /usr/include/c++/4.2.1/bits/stl_raw_storage_iter.h \
  /usr/include/c++/4.2.1/limits \
  /usr/include/c++/4.2.1/bits/ostream_insert.h \
  /usr/include/c++/4.2.1/bits/stl_function.h \
  /usr/include/c++/4.2.1/bits/basic_string.h \
  /usr/include/c++/4.2.1/algorithm /usr/include/c++/4.2.1/bits/stl_algo.h \
  /usr/include/c++/4.2.1/bits/stl_heap.h \
  /usr/include/c++/4.2.1/bits/stl_tempbuf.h \
  /usr/include/c++/4.2.1/bits/basic_string.tcc \
  /usr/include/c++/4.2.1/streambuf \
  /usr/include/c++/4.2.1/bits/streambuf.tcc \
  /usr/include/c++/4.2.1/bits/basic_ios.h \
  /usr/include/c++/4.2.1/bits/streambuf_iterator.h \
  /usr/include/c++/4.2.1/bits/locale_facets.h \
  /usr/include/c++/4.2.1/cwctype /usr/include/wctype.h \
  /usr/include/c++/4.2.1/bits/ctype_base.h \
  /usr/include/c++/4.2.1/bits/ctype_inline.h \
  /usr/include/c++/4.2.1/bits/codecvt.h \
  /usr/include/c++/4.2.1/bits/time_members.h \
  /usr/include/c++/4.2.1/bits/messages_members.h \
  /usr/include/c++/4.2.1/bits/basic_ios.tcc \
  /usr/include/c++/4.2.1/bits/ostream.tcc /usr/include/c++/4.2.1/locale \
  /usr/include/c++/4.2.1/bits/locale_facets.tcc \
  /usr/include/c++/4.2.1/typeinfo /usr/include/c++/4.2.1/istream \
  /usr/include/c++/4.2.1/bits/istream.tcc sli_type.h sli_name.h \
  /usr/include/c++/4.2.1/cassert /usr/include/assert.h \
  /usr/include/c++/4.2.1/map /usr/include/c++/4.2.1/bits/stl_tree.h \
  /usr/include/c++/4.2.1/bits/stl_map.h \
  /usr/include/c++/4.2.1/bits/stl_multimap.h /usr/include/c++/4.2.1/deque \
  /usr/include/c++/4.2.1/bits/stl_deque.h \
  /usr/include/c++/4.2.1/bits/deque.tcc sli_exceptions.h \
  /usr/include/c++/4.2.1/vector /usr/include/c++/4.2.1/bits/stl_vector.h \
  /usr/include/c++/4.2.1/bits/stl_bvector.h \
  /usr/include/c++/4.2.1/bits/vector.tcc sli_charcode.h sli_interpreter.h \
  sli_allocator.h sli_arraytype.h sli_array.h sli_integertype.h \
  SLI_token.h sli_tokenstack.h sli_string.h sli_dictionary.h \
  sli_dictstack.h /usr/include/c++/4.2.1/list \
  /usr/include/c++/4.2.1/bits/stl_list.h \
  /usr/include/c++/4.2.1/bits/list.tcc sli_builtins.h sli_function.h \
  sli_module.h /usr/include/c++/4.2.1/cmath /usr/include/math.h \
  /usr/include/architecture/i386/math.h \
  /usr/include/c++/4.2.1/bits/cmath.tcc /usr/include/c++/4.2.1/sstream \
  /usr/include/c++/4.2.1/bits/sstream.tcc
sli_parser.o: sli_parser.cpp sli_interpreter.h sli_type.h \
  /usr/include/c++/4.2.1/string /usr/include/c++/4.2.1/bits/c++config.h \
  /usr/include/c++/4.2.1/bits/os_defines.h /usr/include/unistd.h \
  /usr/include/_types.h /usr/include/sys/_types.h \
  /usr/include/sys/cdefs.h /usr/include/sys/_symbol_aliasing.h \
  /usr/include/sys/_posix_availability.h /usr/include/machine/_types.h \
  /usr/include/i386/_types.h /usr/include/sys/unistd.h \
  /usr/include/Availability.h /usr/include/AvailabilityInternal.h \
  /usr/include/sys/select.h /usr/include/sys/appleapiopts.h \
  /usr/include/sys/_structs.h /usr/include/sys/_select.h \
  /usr/include/c++/4.2.1/bits/cpu_defines.h \
  /usr/include/c++/4.2.1/bits/stringfwd.h \
  /usr/include/c++/4.2.1/bits/char_traits.h \
  /usr/include/c++/4.2.1/cstring /usr/include/c++/4.2.1/cstddef \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/stddef.h \
  /usr/include/string.h /usr/include/strings.h \
  /usr/include/c++/4.2.1/bits/stl_algobase.h \
  /usr/include/c++/4.2.1/climits \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/limits.h \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/syslimits.h \
  /usr/include/limits.h /usr/include/machine/limits.h \
  /usr/include/i386/limits.h /usr/include/i386/_limits.h \
  /usr/include/sys/syslimits.h /usr/include/c++/4.2.1/cstdlib \
  /usr/include/stdlib.h /usr/include/sys/wait.h /usr/include/sys/signal.h \
  /usr/include/machine/signal.h /usr/include/i386/signal.h \
  /usr/include/i386/_structs.h /usr/include/machine/_structs.h \
  /usr/include/mach/i386/_structs.h /usr/include/sys/resource.h \
  /usr/include/machine/endian.h /usr/include/i386/endian.h \
  /usr/include/sys/_endian.h /usr/include/libkern/_OSByteOrder.h \
  /usr/include/libkern/i386/_OSByteOrder.h /usr/include/alloca.h \
  /usr/include/machine/types.h /usr/include/i386/types.h \
  /usr/include/i386/_types.h /usr/include/c++/4.2.1/iosfwd \
  /usr/include/c++/4.2.1/bits/c++locale.h /usr/include/c++/4.2.1/clocale \
  /usr/include/locale.h /usr/include/_locale.h \
  /usr/include/c++/4.2.1/cstdio /usr/include/stdio.h \
  /usr/include/c++/4.2.1/cstdarg \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/stdarg.h \
  /usr/include/c++/4.2.1/bits/c++io.h /usr/include/c++/4.2.1/bits/gthr.h \
  /usr/include/c++/4.2.1/bits/gthr-default.h /usr/include/pthread.h \
  /usr/include/pthread_impl.h /usr/include/sched.h /usr/include/time.h \
  /usr/include/_structs.h /usr/include/c++/4.2.1/cctype \
  /usr/include/ctype.h /usr/include/runetype.h \
  /usr/include/c++/4.2.1/bits/postypes.h /usr/include/c++/4.2.1/cwchar \
  /usr/include/c++/4.2.1/ctime /usr/include/wchar.h \
  /usr/include/_wctype.h /usr/include/stdint.h \
  /usr/include/c++/4.2.1/bits/functexcept.h \
  /usr/include/c++/4.2.1/exception_defines.h \
  /usr/include/c++/4.2.1/bits/stl_pair.h \
  /usr/include/c++/4.2.1/bits/cpp_type_traits.h \
  /usr/include/c++/4.2.1/ext/type_traits.h /usr/include/c++/4.2.1/utility \
  /usr/include/c++/4.2.1/bits/stl_relops.h \
  /usr/include/c++/4.2.1/bits/stl_iterator_base_types.h \
  /usr/include/c++/4.2.1/bits/stl_iterator_base_funcs.h \
  /usr/include/c++/4.2.1/bits/concept_check.h \
  /usr/include/c++/4.2.1/bits/stl_iterator.h \
  /usr/include/c++/4.2.1/debug/debug.h /usr/include/c++/4.2.1/memory \
  /usr/include/c++/4.2.1/bits/allocator.h \
  /usr/include/c++/4.2.1/bits/c++allocator.h \
  /usr/include/c++/4.2.1/ext/new_allocator.h /usr/include/c++/4.2.1/new \
  /usr/include/c++/4.2.1/exception \
  /usr/include/c++/4.2.1/bits/stl_construct.h \
  /usr/include/c++/4.2.1/bits/stl_uninitialized.h \
  /usr/include/c++/4.2.1/bits/stl_raw_storage_iter.h \
  /usr/include/c++/4.2.1/limits \
  /usr/include/c++/4.2.1/bits/ostream_insert.h \
  /usr/include/c++/4.2.1/bits/stl_function.h \
  /usr/include/c++/4.2.1/bits/basic_string.h \
  /usr/include/c++/4.2.1/ext/atomicity.h \
  /usr/include/c++/4.2.1/bits/atomic_word.h \
  /usr/include/c++/4.2.1/algorithm /usr/include/c++/4.2.1/bits/stl_algo.h \
  /usr/include/c++/4.2.1/bits/stl_heap.h \
  /usr/include/c++/4.2.1/bits/stl_tempbuf.h \
  /usr/include/c++/4.2.1/bits/basic_string.tcc sli_name.h \
  /usr/include/c++/4.2.1/cassert /usr/include/assert.h \
  /usr/include/c++/4.2.1/map /usr/include/c++/4.2.1/bits/stl_tree.h \
  /usr/include/c++/4.2.1/bits/stl_map.h \
  /usr/include/c++/4.2.1/bits/stl_multimap.h /usr/include/c++/4.2.1/deque \
  /usr/include/c++/4.2.1/bits/stl_deque.h \
  /usr/include/c++/4.2.1/bits/deque.tcc /usr/include/c++/4.2.1/iostream \
  /usr/include/c++/4.2.1/ostream /usr/include/c++/4.2.1/ios \
  /usr/include/c++/4.2.1/bits/localefwd.h \
  /usr/include/c++/4.2.1/bits/ios_base.h \
  /usr/include/c++/4.2.1/bits/locale_classes.h \
  /usr/include/c++/4.2.1/streambuf \
  /usr/include/c++/4.2.1/bits/streambuf.tcc \
  /usr/include/c++/4.2.1/bits/basic_ios.h \
  /usr/include/c++/4.2.1/bits/streambuf_iterator.h \
  /usr/include/c++/4.2.1/bits/locale_facets.h \
  /usr/include/c++/4.2.1/cwctype /usr/include/wctype.h \
  /usr/include/c++/4.2.1/bits/ctype_base.h \
  /usr/include/c++/4.2.1/bits/ctype_inline.h \
  /usr/include/c++/4.2.1/bits/codecvt.h \
  /usr/include/c++/4.2.1/bits/time_members.h \
  /usr/include/c++/4.2.1/bits/messages_members.h \
  /usr/include/c++/4.2.1/bits/basic_ios.tcc \
  /usr/include/c++/4.2.1/bits/ostream.tcc /usr/include/c++/4.2.1/locale \
  /usr/include/c++/4.2.1/bits/locale_facets.tcc \
  /usr/include/c++/4.2.1/typeinfo /usr/include/c++/4.2.1/istream \
  /usr/include/c++/4.2.1/bits/istream.tcc sli_token.h sli_exceptions.h \
  /usr/include/c++/4.2.1/vector /usr/include/c++/4.2.1/bits/stl_vector.h \
  /usr/include/c++/4.2.1/bits/stl_bvector.h \
  /usr/include/c++/4.2.1/bits/vector.tcc sli_allocator.h sli_arraytype.h \
  sli_array.h sli_integertype.h SLI_token.h sli_tokenstack.h sli_string.h \
  sli_dictionary.h sli_dictstack.h /usr/include/c++/4.2.1/list \
  /usr/include/c++/4.2.1/bits/stl_list.h \
  /usr/include/c++/4.2.1/bits/list.tcc sli_builtins.h sli_function.h \
  sli_module.h sli_scanner.h sli_charcode.h sli_parser.h sli_nametype.h
sli_interpreter.o: sli_interpreter.cpp sli_interpreter.h sli_type.h \
  /usr/include/c++/4.2.1/string /usr/include/c++/4.2.1/bits/c++config.h \
  /usr/include/c++/4.2.1/bits/os_defines.h /usr/include/unistd.h \
  /usr/include/_types.h /usr/include/sys/_types.h \
  /usr/include/sys/cdefs.h /usr/include/sys/_symbol_aliasing.h \
  /usr/include/sys/_posix_availability.h /usr/include/machine/_types.h \
  /usr/include/i386/_types.h /usr/include/sys/unistd.h \
  /usr/include/Availability.h /usr/include/AvailabilityInternal.h \
  /usr/include/sys/select.h /usr/include/sys/appleapiopts.h \
  /usr/include/sys/_structs.h /usr/include/sys/_select.h \
  /usr/include/c++/4.2.1/bits/cpu_defines.h \
  /usr/include/c++/4.2.1/bits/stringfwd.h \
  /usr/include/c++/4.2.1/bits/char_traits.h \
  /usr/include/c++/4.2.1/cstring /usr/include/c++/4.2.1/cstddef \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/stddef.h \
  /usr/include/string.h /usr/include/strings.h \
  /usr/include/c++/4.2.1/bits/stl_algobase.h \
  /usr/include/c++/4.2.1/climits \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/limits.h \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/syslimits.h \
  /usr/include/limits.h /usr/include/machine/limits.h \
  /usr/include/i386/limits.h /usr/include/i386/_limits.h \
  /usr/include/sys/syslimits.h /usr/include/c++/4.2.1/cstdlib \
  /usr/include/stdlib.h /usr/include/sys/wait.h /usr/include/sys/signal.h \
  /usr/include/machine/signal.h /usr/include/i386/signal.h \
  /usr/include/i386/_structs.h /usr/include/machine/_structs.h \
  /usr/include/mach/i386/_structs.h /usr/include/sys/resource.h \
  /usr/include/machine/endian.h /usr/include/i386/endian.h \
  /usr/include/sys/_endian.h /usr/include/libkern/_OSByteOrder.h \
  /usr/include/libkern/i386/_OSByteOrder.h /usr/include/alloca.h \
  /usr/include/machine/types.h /usr/include/i386/types.h \
  /usr/include/i386/_types.h /usr/include/c++/4.2.1/iosfwd \
  /usr/include/c++/4.2.1/bits/c++locale.h /usr/include/c++/4.2.1/clocale \
  /usr/include/locale.h /usr/include/_locale.h \
  /usr/include/c++/4.2.1/cstdio /usr/include/stdio.h \
  /usr/include/c++/4.2.1/cstdarg \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/stdarg.h \
  /usr/include/c++/4.2.1/bits/c++io.h /usr/include/c++/4.2.1/bits/gthr.h \
  /usr/include/c++/4.2.1/bits/gthr-default.h /usr/include/pthread.h \
  /usr/include/pthread_impl.h /usr/include/sched.h /usr/include/time.h \
  /usr/include/_structs.h /usr/include/c++/4.2.1/cctype \
  /usr/include/ctype.h /usr/include/runetype.h \
  /usr/include/c++/4.2.1/bits/postypes.h /usr/include/c++/4.2.1/cwchar \
  /usr/include/c++/4.2.1/ctime /usr/include/wchar.h \
  /usr/include/_wctype.h /usr/include/stdint.h \
  /usr/include/c++/4.2.1/bits/functexcept.h \
  /usr/include/c++/4.2.1/exception_defines.h \
  /usr/include/c++/4.2.1/bits/stl_pair.h \
  /usr/include/c++/4.2.1/bits/cpp_type_traits.h \
  /usr/include/c++/4.2.1/ext/type_traits.h /usr/include/c++/4.2.1/utility \
  /usr/include/c++/4.2.1/bits/stl_relops.h \
  /usr/include/c++/4.2.1/bits/stl_iterator_base_types.h \
  /usr/include/c++/4.2.1/bits/stl_iterator_base_funcs.h \
  /usr/include/c++/4.2.1/bits/concept_check.h \
  /usr/include/c++/4.2.1/bits/stl_iterator.h \
  /usr/include/c++/4.2.1/debug/debug.h /usr/include/c++/4.2.1/memory \
  /usr/include/c++/4.2.1/bits/allocator.h \
  /usr/include/c++/4.2.1/bits/c++allocator.h \
  /usr/include/c++/4.2.1/ext/new_allocator.h /usr/include/c++/4.2.1/new \
  /usr/include/c++/4.2.1/exception \
  /usr/include/c++/4.2.1/bits/stl_construct.h \
  /usr/include/c++/4.2.1/bits/stl_uninitialized.h \
  /usr/include/c++/4.2.1/bits/stl_raw_storage_iter.h \
  /usr/include/c++/4.2.1/limits \
  /usr/include/c++/4.2.1/bits/ostream_insert.h \
  /usr/include/c++/4.2.1/bits/stl_function.h \
  /usr/include/c++/4.2.1/bits/basic_string.h \
  /usr/include/c++/4.2.1/ext/atomicity.h \
  /usr/include/c++/4.2.1/bits/atomic_word.h \
  /usr/include/c++/4.2.1/algorithm /usr/include/c++/4.2.1/bits/stl_algo.h \
  /usr/include/c++/4.2.1/bits/stl_heap.h \
  /usr/include/c++/4.2.1/bits/stl_tempbuf.h \
  /usr/include/c++/4.2.1/bits/basic_string.tcc sli_name.h \
  /usr/include/c++/4.2.1/cassert /usr/include/assert.h \
  /usr/include/c++/4.2.1/map /usr/include/c++/4.2.1/bits/stl_tree.h \
  /usr/include/c++/4.2.1/bits/stl_map.h \
  /usr/include/c++/4.2.1/bits/stl_multimap.h /usr/include/c++/4.2.1/deque \
  /usr/include/c++/4.2.1/bits/stl_deque.h \
  /usr/include/c++/4.2.1/bits/deque.tcc /usr/include/c++/4.2.1/iostream \
  /usr/include/c++/4.2.1/ostream /usr/include/c++/4.2.1/ios \
  /usr/include/c++/4.2.1/bits/localefwd.h \
  /usr/include/c++/4.2.1/bits/ios_base.h \
  /usr/include/c++/4.2.1/bits/locale_classes.h \
  /usr/include/c++/4.2.1/streambuf \
  /usr/include/c++/4.2.1/bits/streambuf.tcc \
  /usr/include/c++/4.2.1/bits/basic_ios.h \
  /usr/include/c++/4.2.1/bits/streambuf_iterator.h \
  /usr/include/c++/4.2.1/bits/locale_facets.h \
  /usr/include/c++/4.2.1/cwctype /usr/include/wctype.h \
  /usr/include/c++/4.2.1/bits/ctype_base.h \
  /usr/include/c++/4.2.1/bits/ctype_inline.h \
  /usr/include/c++/4.2.1/bits/codecvt.h \
  /usr/include/c++/4.2.1/bits/time_members.h \
  /usr/include/c++/4.2.1/bits/messages_members.h \
  /usr/include/c++/4.2.1/bits/basic_ios.tcc \
  /usr/include/c++/4.2.1/bits/ostream.tcc /usr/include/c++/4.2.1/locale \
  /usr/include/c++/4.2.1/bits/locale_facets.tcc \
  /usr/include/c++/4.2.1/typeinfo /usr/include/c++/4.2.1/istream \
  /usr/include/c++/4.2.1/bits/istream.tcc sli_token.h sli_exceptions.h \
  /usr/include/c++/4.2.1/vector /usr/include/c++/4.2.1/bits/stl_vector.h \
  /usr/include/c++/4.2.1/bits/stl_bvector.h \
  /usr/include/c++/4.2.1/bits/vector.tcc sli_allocator.h sli_arraytype.h \
  sli_array.h sli_integertype.h SLI_token.h sli_tokenstack.h sli_string.h \
  sli_dictionary.h sli_dictstack.h /usr/include/c++/4.2.1/list \
  /usr/include/c++/4.2.1/bits/stl_list.h \
  /usr/include/c++/4.2.1/bits/list.tcc sli_builtins.h sli_function.h \
  sli_module.h sli_nametype.h sli_dicttype.h sli_functiontype.h \
  sli_stringtype.h compose.hpp /usr/include/c++/4.2.1/sstream \
  /usr/include/c++/4.2.1/bits/sstream.tcc sli_numerics.h \
  /usr/include/c++/4.2.1/cmath /usr/include/math.h \
  /usr/include/architecture/i386/math.h \
  /usr/include/c++/4.2.1/bits/cmath.tcc sli_parser.h sli_scanner.h \
  sli_charcode.h sli_control.h sli_math.h sli_stack.h
sli_numerics.o: sli_numerics.cpp config.h sli_numerics.h \
  /usr/include/c++/4.2.1/limits /usr/include/c++/4.2.1/bits/c++config.h \
  /usr/include/c++/4.2.1/bits/os_defines.h /usr/include/unistd.h \
  /usr/include/_types.h /usr/include/sys/_types.h \
  /usr/include/sys/cdefs.h /usr/include/sys/_symbol_aliasing.h \
  /usr/include/sys/_posix_availability.h /usr/include/machine/_types.h \
  /usr/include/i386/_types.h /usr/include/sys/unistd.h \
  /usr/include/Availability.h /usr/include/AvailabilityInternal.h \
  /usr/include/sys/select.h /usr/include/sys/appleapiopts.h \
  /usr/include/sys/_structs.h /usr/include/sys/_select.h \
  /usr/include/c++/4.2.1/bits/cpu_defines.h /usr/include/c++/4.2.1/cmath \
  /usr/include/c++/4.2.1/bits/cpp_type_traits.h \
  /usr/include/c++/4.2.1/ext/type_traits.h /usr/include/c++/4.2.1/cstddef \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/stddef.h \
  /usr/include/c++/4.2.1/utility /usr/include/c++/4.2.1/bits/stl_relops.h \
  /usr/include/c++/4.2.1/bits/stl_pair.h /usr/include/math.h \
  /usr/include/architecture/i386/math.h \
  /usr/include/c++/4.2.1/bits/cmath.tcc
sli_builtins.o: sli_builtins.cpp sli_builtins.h sli_function.h sli_name.h \
  /usr/include/c++/4.2.1/cassert /usr/include/assert.h \
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
  /usr/include/i386/_types.h /usr/include/c++/4.2.1/map \
  /usr/include/c++/4.2.1/bits/stl_tree.h \
  /usr/include/c++/4.2.1/bits/stl_algobase.h \
  /usr/include/c++/4.2.1/bits/c++config.h \
  /usr/include/c++/4.2.1/bits/os_defines.h /usr/include/unistd.h \
  /usr/include/sys/unistd.h /usr/include/sys/select.h \
  /usr/include/sys/_select.h /usr/include/c++/4.2.1/bits/cpu_defines.h \
  /usr/include/c++/4.2.1/cstring /usr/include/c++/4.2.1/cstddef \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/stddef.h \
  /usr/include/string.h /usr/include/strings.h \
  /usr/include/c++/4.2.1/climits \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/limits.h \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/syslimits.h \
  /usr/include/limits.h /usr/include/machine/limits.h \
  /usr/include/i386/limits.h /usr/include/i386/_limits.h \
  /usr/include/sys/syslimits.h /usr/include/c++/4.2.1/cstdlib \
  /usr/include/c++/4.2.1/iosfwd /usr/include/c++/4.2.1/bits/c++locale.h \
  /usr/include/c++/4.2.1/clocale /usr/include/locale.h \
  /usr/include/_locale.h /usr/include/c++/4.2.1/cstdio \
  /usr/include/stdio.h /usr/include/c++/4.2.1/cstdarg \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/stdarg.h \
  /usr/include/c++/4.2.1/bits/c++io.h /usr/include/c++/4.2.1/bits/gthr.h \
  /usr/include/c++/4.2.1/bits/gthr-default.h /usr/include/pthread.h \
  /usr/include/pthread_impl.h /usr/include/sched.h /usr/include/time.h \
  /usr/include/_structs.h /usr/include/c++/4.2.1/cctype \
  /usr/include/ctype.h /usr/include/runetype.h \
  /usr/include/c++/4.2.1/bits/stringfwd.h \
  /usr/include/c++/4.2.1/bits/postypes.h /usr/include/c++/4.2.1/cwchar \
  /usr/include/c++/4.2.1/ctime /usr/include/wchar.h \
  /usr/include/_wctype.h /usr/include/stdint.h \
  /usr/include/c++/4.2.1/bits/functexcept.h \
  /usr/include/c++/4.2.1/exception_defines.h \
  /usr/include/c++/4.2.1/bits/stl_pair.h \
  /usr/include/c++/4.2.1/bits/cpp_type_traits.h \
  /usr/include/c++/4.2.1/ext/type_traits.h /usr/include/c++/4.2.1/utility \
  /usr/include/c++/4.2.1/bits/stl_relops.h \
  /usr/include/c++/4.2.1/bits/stl_iterator_base_types.h \
  /usr/include/c++/4.2.1/bits/stl_iterator_base_funcs.h \
  /usr/include/c++/4.2.1/bits/concept_check.h \
  /usr/include/c++/4.2.1/bits/stl_iterator.h \
  /usr/include/c++/4.2.1/debug/debug.h \
  /usr/include/c++/4.2.1/bits/allocator.h \
  /usr/include/c++/4.2.1/bits/c++allocator.h \
  /usr/include/c++/4.2.1/ext/new_allocator.h /usr/include/c++/4.2.1/new \
  /usr/include/c++/4.2.1/exception \
  /usr/include/c++/4.2.1/bits/stl_construct.h \
  /usr/include/c++/4.2.1/bits/stl_function.h \
  /usr/include/c++/4.2.1/bits/stl_map.h \
  /usr/include/c++/4.2.1/bits/stl_multimap.h \
  /usr/include/c++/4.2.1/string /usr/include/c++/4.2.1/bits/char_traits.h \
  /usr/include/c++/4.2.1/memory \
  /usr/include/c++/4.2.1/bits/stl_uninitialized.h \
  /usr/include/c++/4.2.1/bits/stl_raw_storage_iter.h \
  /usr/include/c++/4.2.1/limits \
  /usr/include/c++/4.2.1/bits/ostream_insert.h \
  /usr/include/c++/4.2.1/bits/basic_string.h \
  /usr/include/c++/4.2.1/ext/atomicity.h \
  /usr/include/c++/4.2.1/bits/atomic_word.h \
  /usr/include/c++/4.2.1/algorithm /usr/include/c++/4.2.1/bits/stl_algo.h \
  /usr/include/c++/4.2.1/bits/stl_heap.h \
  /usr/include/c++/4.2.1/bits/stl_tempbuf.h \
  /usr/include/c++/4.2.1/bits/basic_string.tcc \
  /usr/include/c++/4.2.1/deque /usr/include/c++/4.2.1/bits/stl_deque.h \
  /usr/include/c++/4.2.1/bits/deque.tcc /usr/include/c++/4.2.1/iostream \
  /usr/include/c++/4.2.1/ostream /usr/include/c++/4.2.1/ios \
  /usr/include/c++/4.2.1/bits/localefwd.h \
  /usr/include/c++/4.2.1/bits/ios_base.h \
  /usr/include/c++/4.2.1/bits/locale_classes.h \
  /usr/include/c++/4.2.1/streambuf \
  /usr/include/c++/4.2.1/bits/streambuf.tcc \
  /usr/include/c++/4.2.1/bits/basic_ios.h \
  /usr/include/c++/4.2.1/bits/streambuf_iterator.h \
  /usr/include/c++/4.2.1/bits/locale_facets.h \
  /usr/include/c++/4.2.1/cwctype /usr/include/wctype.h \
  /usr/include/c++/4.2.1/bits/ctype_base.h \
  /usr/include/c++/4.2.1/bits/ctype_inline.h \
  /usr/include/c++/4.2.1/bits/codecvt.h \
  /usr/include/c++/4.2.1/bits/time_members.h \
  /usr/include/c++/4.2.1/bits/messages_members.h \
  /usr/include/c++/4.2.1/bits/basic_ios.tcc \
  /usr/include/c++/4.2.1/bits/ostream.tcc /usr/include/c++/4.2.1/locale \
  /usr/include/c++/4.2.1/bits/locale_facets.tcc \
  /usr/include/c++/4.2.1/typeinfo /usr/include/c++/4.2.1/istream \
  /usr/include/c++/4.2.1/bits/istream.tcc sli_interpreter.h sli_type.h \
  sli_token.h sli_exceptions.h /usr/include/c++/4.2.1/vector \
  /usr/include/c++/4.2.1/bits/stl_vector.h \
  /usr/include/c++/4.2.1/bits/stl_bvector.h \
  /usr/include/c++/4.2.1/bits/vector.tcc sli_allocator.h sli_arraytype.h \
  sli_array.h sli_integertype.h SLI_token.h sli_tokenstack.h sli_string.h \
  sli_dictionary.h sli_dictstack.h /usr/include/c++/4.2.1/list \
  /usr/include/c++/4.2.1/bits/stl_list.h \
  /usr/include/c++/4.2.1/bits/list.tcc sli_module.h sli_iostream.h \
  sli_lockptr.h
sli_control.o: sli_control.cpp sli_control.h sli_interpreter.h sli_type.h \
  /usr/include/c++/4.2.1/string /usr/include/c++/4.2.1/bits/c++config.h \
  /usr/include/c++/4.2.1/bits/os_defines.h /usr/include/unistd.h \
  /usr/include/_types.h /usr/include/sys/_types.h \
  /usr/include/sys/cdefs.h /usr/include/sys/_symbol_aliasing.h \
  /usr/include/sys/_posix_availability.h /usr/include/machine/_types.h \
  /usr/include/i386/_types.h /usr/include/sys/unistd.h \
  /usr/include/Availability.h /usr/include/AvailabilityInternal.h \
  /usr/include/sys/select.h /usr/include/sys/appleapiopts.h \
  /usr/include/sys/_structs.h /usr/include/sys/_select.h \
  /usr/include/c++/4.2.1/bits/cpu_defines.h \
  /usr/include/c++/4.2.1/bits/stringfwd.h \
  /usr/include/c++/4.2.1/bits/char_traits.h \
  /usr/include/c++/4.2.1/cstring /usr/include/c++/4.2.1/cstddef \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/stddef.h \
  /usr/include/string.h /usr/include/strings.h \
  /usr/include/c++/4.2.1/bits/stl_algobase.h \
  /usr/include/c++/4.2.1/climits \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/limits.h \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/syslimits.h \
  /usr/include/limits.h /usr/include/machine/limits.h \
  /usr/include/i386/limits.h /usr/include/i386/_limits.h \
  /usr/include/sys/syslimits.h /usr/include/c++/4.2.1/cstdlib \
  /usr/include/stdlib.h /usr/include/sys/wait.h /usr/include/sys/signal.h \
  /usr/include/machine/signal.h /usr/include/i386/signal.h \
  /usr/include/i386/_structs.h /usr/include/machine/_structs.h \
  /usr/include/mach/i386/_structs.h /usr/include/sys/resource.h \
  /usr/include/machine/endian.h /usr/include/i386/endian.h \
  /usr/include/sys/_endian.h /usr/include/libkern/_OSByteOrder.h \
  /usr/include/libkern/i386/_OSByteOrder.h /usr/include/alloca.h \
  /usr/include/machine/types.h /usr/include/i386/types.h \
  /usr/include/i386/_types.h /usr/include/c++/4.2.1/iosfwd \
  /usr/include/c++/4.2.1/bits/c++locale.h /usr/include/c++/4.2.1/clocale \
  /usr/include/locale.h /usr/include/_locale.h \
  /usr/include/c++/4.2.1/cstdio /usr/include/stdio.h \
  /usr/include/c++/4.2.1/cstdarg \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/stdarg.h \
  /usr/include/c++/4.2.1/bits/c++io.h /usr/include/c++/4.2.1/bits/gthr.h \
  /usr/include/c++/4.2.1/bits/gthr-default.h /usr/include/pthread.h \
  /usr/include/pthread_impl.h /usr/include/sched.h /usr/include/time.h \
  /usr/include/_structs.h /usr/include/c++/4.2.1/cctype \
  /usr/include/ctype.h /usr/include/runetype.h \
  /usr/include/c++/4.2.1/bits/postypes.h /usr/include/c++/4.2.1/cwchar \
  /usr/include/c++/4.2.1/ctime /usr/include/wchar.h \
  /usr/include/_wctype.h /usr/include/stdint.h \
  /usr/include/c++/4.2.1/bits/functexcept.h \
  /usr/include/c++/4.2.1/exception_defines.h \
  /usr/include/c++/4.2.1/bits/stl_pair.h \
  /usr/include/c++/4.2.1/bits/cpp_type_traits.h \
  /usr/include/c++/4.2.1/ext/type_traits.h /usr/include/c++/4.2.1/utility \
  /usr/include/c++/4.2.1/bits/stl_relops.h \
  /usr/include/c++/4.2.1/bits/stl_iterator_base_types.h \
  /usr/include/c++/4.2.1/bits/stl_iterator_base_funcs.h \
  /usr/include/c++/4.2.1/bits/concept_check.h \
  /usr/include/c++/4.2.1/bits/stl_iterator.h \
  /usr/include/c++/4.2.1/debug/debug.h /usr/include/c++/4.2.1/memory \
  /usr/include/c++/4.2.1/bits/allocator.h \
  /usr/include/c++/4.2.1/bits/c++allocator.h \
  /usr/include/c++/4.2.1/ext/new_allocator.h /usr/include/c++/4.2.1/new \
  /usr/include/c++/4.2.1/exception \
  /usr/include/c++/4.2.1/bits/stl_construct.h \
  /usr/include/c++/4.2.1/bits/stl_uninitialized.h \
  /usr/include/c++/4.2.1/bits/stl_raw_storage_iter.h \
  /usr/include/c++/4.2.1/limits \
  /usr/include/c++/4.2.1/bits/ostream_insert.h \
  /usr/include/c++/4.2.1/bits/stl_function.h \
  /usr/include/c++/4.2.1/bits/basic_string.h \
  /usr/include/c++/4.2.1/ext/atomicity.h \
  /usr/include/c++/4.2.1/bits/atomic_word.h \
  /usr/include/c++/4.2.1/algorithm /usr/include/c++/4.2.1/bits/stl_algo.h \
  /usr/include/c++/4.2.1/bits/stl_heap.h \
  /usr/include/c++/4.2.1/bits/stl_tempbuf.h \
  /usr/include/c++/4.2.1/bits/basic_string.tcc sli_name.h \
  /usr/include/c++/4.2.1/cassert /usr/include/assert.h \
  /usr/include/c++/4.2.1/map /usr/include/c++/4.2.1/bits/stl_tree.h \
  /usr/include/c++/4.2.1/bits/stl_map.h \
  /usr/include/c++/4.2.1/bits/stl_multimap.h /usr/include/c++/4.2.1/deque \
  /usr/include/c++/4.2.1/bits/stl_deque.h \
  /usr/include/c++/4.2.1/bits/deque.tcc /usr/include/c++/4.2.1/iostream \
  /usr/include/c++/4.2.1/ostream /usr/include/c++/4.2.1/ios \
  /usr/include/c++/4.2.1/bits/localefwd.h \
  /usr/include/c++/4.2.1/bits/ios_base.h \
  /usr/include/c++/4.2.1/bits/locale_classes.h \
  /usr/include/c++/4.2.1/streambuf \
  /usr/include/c++/4.2.1/bits/streambuf.tcc \
  /usr/include/c++/4.2.1/bits/basic_ios.h \
  /usr/include/c++/4.2.1/bits/streambuf_iterator.h \
  /usr/include/c++/4.2.1/bits/locale_facets.h \
  /usr/include/c++/4.2.1/cwctype /usr/include/wctype.h \
  /usr/include/c++/4.2.1/bits/ctype_base.h \
  /usr/include/c++/4.2.1/bits/ctype_inline.h \
  /usr/include/c++/4.2.1/bits/codecvt.h \
  /usr/include/c++/4.2.1/bits/time_members.h \
  /usr/include/c++/4.2.1/bits/messages_members.h \
  /usr/include/c++/4.2.1/bits/basic_ios.tcc \
  /usr/include/c++/4.2.1/bits/ostream.tcc /usr/include/c++/4.2.1/locale \
  /usr/include/c++/4.2.1/bits/locale_facets.tcc \
  /usr/include/c++/4.2.1/typeinfo /usr/include/c++/4.2.1/istream \
  /usr/include/c++/4.2.1/bits/istream.tcc sli_token.h sli_exceptions.h \
  /usr/include/c++/4.2.1/vector /usr/include/c++/4.2.1/bits/stl_vector.h \
  /usr/include/c++/4.2.1/bits/stl_bvector.h \
  /usr/include/c++/4.2.1/bits/vector.tcc sli_allocator.h sli_arraytype.h \
  sli_array.h sli_integertype.h SLI_token.h sli_tokenstack.h sli_string.h \
  sli_dictionary.h sli_dictstack.h /usr/include/c++/4.2.1/list \
  /usr/include/c++/4.2.1/bits/stl_list.h \
  /usr/include/c++/4.2.1/bits/list.tcc sli_builtins.h sli_function.h \
  sli_module.h sli_scanner.h sli_charcode.h sli_parser.h \
  sli_iostreamtype.h sli_iostream.h sli_lockptr.h sli_stringtype.h \
  sli_functiontype.h /usr/include/sys/time.h /usr/include/sys/times.h \
  /usr/include/c++/4.2.1/sstream /usr/include/c++/4.2.1/bits/sstream.tcc
sli_iostreamtype.o: sli_iostreamtype.cpp sli_functiontype.h sli_type.h \
  /usr/include/c++/4.2.1/string /usr/include/c++/4.2.1/bits/c++config.h \
  /usr/include/c++/4.2.1/bits/os_defines.h /usr/include/unistd.h \
  /usr/include/_types.h /usr/include/sys/_types.h \
  /usr/include/sys/cdefs.h /usr/include/sys/_symbol_aliasing.h \
  /usr/include/sys/_posix_availability.h /usr/include/machine/_types.h \
  /usr/include/i386/_types.h /usr/include/sys/unistd.h \
  /usr/include/Availability.h /usr/include/AvailabilityInternal.h \
  /usr/include/sys/select.h /usr/include/sys/appleapiopts.h \
  /usr/include/sys/_structs.h /usr/include/sys/_select.h \
  /usr/include/c++/4.2.1/bits/cpu_defines.h \
  /usr/include/c++/4.2.1/bits/stringfwd.h \
  /usr/include/c++/4.2.1/bits/char_traits.h \
  /usr/include/c++/4.2.1/cstring /usr/include/c++/4.2.1/cstddef \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/stddef.h \
  /usr/include/string.h /usr/include/strings.h \
  /usr/include/c++/4.2.1/bits/stl_algobase.h \
  /usr/include/c++/4.2.1/climits \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/limits.h \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/syslimits.h \
  /usr/include/limits.h /usr/include/machine/limits.h \
  /usr/include/i386/limits.h /usr/include/i386/_limits.h \
  /usr/include/sys/syslimits.h /usr/include/c++/4.2.1/cstdlib \
  /usr/include/stdlib.h /usr/include/sys/wait.h /usr/include/sys/signal.h \
  /usr/include/machine/signal.h /usr/include/i386/signal.h \
  /usr/include/i386/_structs.h /usr/include/machine/_structs.h \
  /usr/include/mach/i386/_structs.h /usr/include/sys/resource.h \
  /usr/include/machine/endian.h /usr/include/i386/endian.h \
  /usr/include/sys/_endian.h /usr/include/libkern/_OSByteOrder.h \
  /usr/include/libkern/i386/_OSByteOrder.h /usr/include/alloca.h \
  /usr/include/machine/types.h /usr/include/i386/types.h \
  /usr/include/i386/_types.h /usr/include/c++/4.2.1/iosfwd \
  /usr/include/c++/4.2.1/bits/c++locale.h /usr/include/c++/4.2.1/clocale \
  /usr/include/locale.h /usr/include/_locale.h \
  /usr/include/c++/4.2.1/cstdio /usr/include/stdio.h \
  /usr/include/c++/4.2.1/cstdarg \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/stdarg.h \
  /usr/include/c++/4.2.1/bits/c++io.h /usr/include/c++/4.2.1/bits/gthr.h \
  /usr/include/c++/4.2.1/bits/gthr-default.h /usr/include/pthread.h \
  /usr/include/pthread_impl.h /usr/include/sched.h /usr/include/time.h \
  /usr/include/_structs.h /usr/include/c++/4.2.1/cctype \
  /usr/include/ctype.h /usr/include/runetype.h \
  /usr/include/c++/4.2.1/bits/postypes.h /usr/include/c++/4.2.1/cwchar \
  /usr/include/c++/4.2.1/ctime /usr/include/wchar.h \
  /usr/include/_wctype.h /usr/include/stdint.h \
  /usr/include/c++/4.2.1/bits/functexcept.h \
  /usr/include/c++/4.2.1/exception_defines.h \
  /usr/include/c++/4.2.1/bits/stl_pair.h \
  /usr/include/c++/4.2.1/bits/cpp_type_traits.h \
  /usr/include/c++/4.2.1/ext/type_traits.h /usr/include/c++/4.2.1/utility \
  /usr/include/c++/4.2.1/bits/stl_relops.h \
  /usr/include/c++/4.2.1/bits/stl_iterator_base_types.h \
  /usr/include/c++/4.2.1/bits/stl_iterator_base_funcs.h \
  /usr/include/c++/4.2.1/bits/concept_check.h \
  /usr/include/c++/4.2.1/bits/stl_iterator.h \
  /usr/include/c++/4.2.1/debug/debug.h /usr/include/c++/4.2.1/memory \
  /usr/include/c++/4.2.1/bits/allocator.h \
  /usr/include/c++/4.2.1/bits/c++allocator.h \
  /usr/include/c++/4.2.1/ext/new_allocator.h /usr/include/c++/4.2.1/new \
  /usr/include/c++/4.2.1/exception \
  /usr/include/c++/4.2.1/bits/stl_construct.h \
  /usr/include/c++/4.2.1/bits/stl_uninitialized.h \
  /usr/include/c++/4.2.1/bits/stl_raw_storage_iter.h \
  /usr/include/c++/4.2.1/limits \
  /usr/include/c++/4.2.1/bits/ostream_insert.h \
  /usr/include/c++/4.2.1/bits/stl_function.h \
  /usr/include/c++/4.2.1/bits/basic_string.h \
  /usr/include/c++/4.2.1/ext/atomicity.h \
  /usr/include/c++/4.2.1/bits/atomic_word.h \
  /usr/include/c++/4.2.1/algorithm /usr/include/c++/4.2.1/bits/stl_algo.h \
  /usr/include/c++/4.2.1/bits/stl_heap.h \
  /usr/include/c++/4.2.1/bits/stl_tempbuf.h \
  /usr/include/c++/4.2.1/bits/basic_string.tcc sli_name.h \
  /usr/include/c++/4.2.1/cassert /usr/include/assert.h \
  /usr/include/c++/4.2.1/map /usr/include/c++/4.2.1/bits/stl_tree.h \
  /usr/include/c++/4.2.1/bits/stl_map.h \
  /usr/include/c++/4.2.1/bits/stl_multimap.h /usr/include/c++/4.2.1/deque \
  /usr/include/c++/4.2.1/bits/stl_deque.h \
  /usr/include/c++/4.2.1/bits/deque.tcc /usr/include/c++/4.2.1/iostream \
  /usr/include/c++/4.2.1/ostream /usr/include/c++/4.2.1/ios \
  /usr/include/c++/4.2.1/bits/localefwd.h \
  /usr/include/c++/4.2.1/bits/ios_base.h \
  /usr/include/c++/4.2.1/bits/locale_classes.h \
  /usr/include/c++/4.2.1/streambuf \
  /usr/include/c++/4.2.1/bits/streambuf.tcc \
  /usr/include/c++/4.2.1/bits/basic_ios.h \
  /usr/include/c++/4.2.1/bits/streambuf_iterator.h \
  /usr/include/c++/4.2.1/bits/locale_facets.h \
  /usr/include/c++/4.2.1/cwctype /usr/include/wctype.h \
  /usr/include/c++/4.2.1/bits/ctype_base.h \
  /usr/include/c++/4.2.1/bits/ctype_inline.h \
  /usr/include/c++/4.2.1/bits/codecvt.h \
  /usr/include/c++/4.2.1/bits/time_members.h \
  /usr/include/c++/4.2.1/bits/messages_members.h \
  /usr/include/c++/4.2.1/bits/basic_ios.tcc \
  /usr/include/c++/4.2.1/bits/ostream.tcc /usr/include/c++/4.2.1/locale \
  /usr/include/c++/4.2.1/bits/locale_facets.tcc \
  /usr/include/c++/4.2.1/typeinfo /usr/include/c++/4.2.1/istream \
  /usr/include/c++/4.2.1/bits/istream.tcc sli_interpreter.h sli_token.h \
  sli_exceptions.h /usr/include/c++/4.2.1/vector \
  /usr/include/c++/4.2.1/bits/stl_vector.h \
  /usr/include/c++/4.2.1/bits/stl_bvector.h \
  /usr/include/c++/4.2.1/bits/vector.tcc sli_allocator.h sli_arraytype.h \
  sli_array.h sli_integertype.h SLI_token.h sli_tokenstack.h sli_string.h \
  sli_dictionary.h sli_dictstack.h /usr/include/c++/4.2.1/list \
  /usr/include/c++/4.2.1/bits/stl_list.h \
  /usr/include/c++/4.2.1/bits/list.tcc sli_builtins.h sli_function.h \
  sli_module.h sli_iostreamtype.h
sli_stack.o: sli_stack.cpp sli_stack.h sli_function.h sli_name.h \
  /usr/include/c++/4.2.1/cassert /usr/include/assert.h \
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
  /usr/include/i386/_types.h /usr/include/c++/4.2.1/map \
  /usr/include/c++/4.2.1/bits/stl_tree.h \
  /usr/include/c++/4.2.1/bits/stl_algobase.h \
  /usr/include/c++/4.2.1/bits/c++config.h \
  /usr/include/c++/4.2.1/bits/os_defines.h /usr/include/unistd.h \
  /usr/include/sys/unistd.h /usr/include/sys/select.h \
  /usr/include/sys/_select.h /usr/include/c++/4.2.1/bits/cpu_defines.h \
  /usr/include/c++/4.2.1/cstring /usr/include/c++/4.2.1/cstddef \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/stddef.h \
  /usr/include/string.h /usr/include/strings.h \
  /usr/include/c++/4.2.1/climits \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/limits.h \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/syslimits.h \
  /usr/include/limits.h /usr/include/machine/limits.h \
  /usr/include/i386/limits.h /usr/include/i386/_limits.h \
  /usr/include/sys/syslimits.h /usr/include/c++/4.2.1/cstdlib \
  /usr/include/c++/4.2.1/iosfwd /usr/include/c++/4.2.1/bits/c++locale.h \
  /usr/include/c++/4.2.1/clocale /usr/include/locale.h \
  /usr/include/_locale.h /usr/include/c++/4.2.1/cstdio \
  /usr/include/stdio.h /usr/include/c++/4.2.1/cstdarg \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/stdarg.h \
  /usr/include/c++/4.2.1/bits/c++io.h /usr/include/c++/4.2.1/bits/gthr.h \
  /usr/include/c++/4.2.1/bits/gthr-default.h /usr/include/pthread.h \
  /usr/include/pthread_impl.h /usr/include/sched.h /usr/include/time.h \
  /usr/include/_structs.h /usr/include/c++/4.2.1/cctype \
  /usr/include/ctype.h /usr/include/runetype.h \
  /usr/include/c++/4.2.1/bits/stringfwd.h \
  /usr/include/c++/4.2.1/bits/postypes.h /usr/include/c++/4.2.1/cwchar \
  /usr/include/c++/4.2.1/ctime /usr/include/wchar.h \
  /usr/include/_wctype.h /usr/include/stdint.h \
  /usr/include/c++/4.2.1/bits/functexcept.h \
  /usr/include/c++/4.2.1/exception_defines.h \
  /usr/include/c++/4.2.1/bits/stl_pair.h \
  /usr/include/c++/4.2.1/bits/cpp_type_traits.h \
  /usr/include/c++/4.2.1/ext/type_traits.h /usr/include/c++/4.2.1/utility \
  /usr/include/c++/4.2.1/bits/stl_relops.h \
  /usr/include/c++/4.2.1/bits/stl_iterator_base_types.h \
  /usr/include/c++/4.2.1/bits/stl_iterator_base_funcs.h \
  /usr/include/c++/4.2.1/bits/concept_check.h \
  /usr/include/c++/4.2.1/bits/stl_iterator.h \
  /usr/include/c++/4.2.1/debug/debug.h \
  /usr/include/c++/4.2.1/bits/allocator.h \
  /usr/include/c++/4.2.1/bits/c++allocator.h \
  /usr/include/c++/4.2.1/ext/new_allocator.h /usr/include/c++/4.2.1/new \
  /usr/include/c++/4.2.1/exception \
  /usr/include/c++/4.2.1/bits/stl_construct.h \
  /usr/include/c++/4.2.1/bits/stl_function.h \
  /usr/include/c++/4.2.1/bits/stl_map.h \
  /usr/include/c++/4.2.1/bits/stl_multimap.h \
  /usr/include/c++/4.2.1/string /usr/include/c++/4.2.1/bits/char_traits.h \
  /usr/include/c++/4.2.1/memory \
  /usr/include/c++/4.2.1/bits/stl_uninitialized.h \
  /usr/include/c++/4.2.1/bits/stl_raw_storage_iter.h \
  /usr/include/c++/4.2.1/limits \
  /usr/include/c++/4.2.1/bits/ostream_insert.h \
  /usr/include/c++/4.2.1/bits/basic_string.h \
  /usr/include/c++/4.2.1/ext/atomicity.h \
  /usr/include/c++/4.2.1/bits/atomic_word.h \
  /usr/include/c++/4.2.1/algorithm /usr/include/c++/4.2.1/bits/stl_algo.h \
  /usr/include/c++/4.2.1/bits/stl_heap.h \
  /usr/include/c++/4.2.1/bits/stl_tempbuf.h \
  /usr/include/c++/4.2.1/bits/basic_string.tcc \
  /usr/include/c++/4.2.1/deque /usr/include/c++/4.2.1/bits/stl_deque.h \
  /usr/include/c++/4.2.1/bits/deque.tcc /usr/include/c++/4.2.1/iostream \
  /usr/include/c++/4.2.1/ostream /usr/include/c++/4.2.1/ios \
  /usr/include/c++/4.2.1/bits/localefwd.h \
  /usr/include/c++/4.2.1/bits/ios_base.h \
  /usr/include/c++/4.2.1/bits/locale_classes.h \
  /usr/include/c++/4.2.1/streambuf \
  /usr/include/c++/4.2.1/bits/streambuf.tcc \
  /usr/include/c++/4.2.1/bits/basic_ios.h \
  /usr/include/c++/4.2.1/bits/streambuf_iterator.h \
  /usr/include/c++/4.2.1/bits/locale_facets.h \
  /usr/include/c++/4.2.1/cwctype /usr/include/wctype.h \
  /usr/include/c++/4.2.1/bits/ctype_base.h \
  /usr/include/c++/4.2.1/bits/ctype_inline.h \
  /usr/include/c++/4.2.1/bits/codecvt.h \
  /usr/include/c++/4.2.1/bits/time_members.h \
  /usr/include/c++/4.2.1/bits/messages_members.h \
  /usr/include/c++/4.2.1/bits/basic_ios.tcc \
  /usr/include/c++/4.2.1/bits/ostream.tcc /usr/include/c++/4.2.1/locale \
  /usr/include/c++/4.2.1/bits/locale_facets.tcc \
  /usr/include/c++/4.2.1/typeinfo /usr/include/c++/4.2.1/istream \
  /usr/include/c++/4.2.1/bits/istream.tcc sli_interpreter.h sli_type.h \
  sli_token.h sli_exceptions.h /usr/include/c++/4.2.1/vector \
  /usr/include/c++/4.2.1/bits/stl_vector.h \
  /usr/include/c++/4.2.1/bits/stl_bvector.h \
  /usr/include/c++/4.2.1/bits/vector.tcc sli_allocator.h sli_arraytype.h \
  sli_array.h sli_integertype.h SLI_token.h sli_tokenstack.h sli_string.h \
  sli_dictionary.h sli_dictstack.h /usr/include/c++/4.2.1/list \
  /usr/include/c++/4.2.1/bits/stl_list.h \
  /usr/include/c++/4.2.1/bits/list.tcc sli_builtins.h sli_module.h
sli_math.o: sli_math.cpp sli_math.h sli_function.h sli_name.h \
  /usr/include/c++/4.2.1/cassert /usr/include/assert.h \
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
  /usr/include/i386/_types.h /usr/include/c++/4.2.1/map \
  /usr/include/c++/4.2.1/bits/stl_tree.h \
  /usr/include/c++/4.2.1/bits/stl_algobase.h \
  /usr/include/c++/4.2.1/bits/c++config.h \
  /usr/include/c++/4.2.1/bits/os_defines.h /usr/include/unistd.h \
  /usr/include/sys/unistd.h /usr/include/sys/select.h \
  /usr/include/sys/_select.h /usr/include/c++/4.2.1/bits/cpu_defines.h \
  /usr/include/c++/4.2.1/cstring /usr/include/c++/4.2.1/cstddef \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/stddef.h \
  /usr/include/string.h /usr/include/strings.h \
  /usr/include/c++/4.2.1/climits \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/limits.h \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/syslimits.h \
  /usr/include/limits.h /usr/include/machine/limits.h \
  /usr/include/i386/limits.h /usr/include/i386/_limits.h \
  /usr/include/sys/syslimits.h /usr/include/c++/4.2.1/cstdlib \
  /usr/include/c++/4.2.1/iosfwd /usr/include/c++/4.2.1/bits/c++locale.h \
  /usr/include/c++/4.2.1/clocale /usr/include/locale.h \
  /usr/include/_locale.h /usr/include/c++/4.2.1/cstdio \
  /usr/include/stdio.h /usr/include/c++/4.2.1/cstdarg \
  /Developer/usr/llvm-gcc-4.2/lib/gcc/i686-apple-darwin11/4.2.1/include/stdarg.h \
  /usr/include/c++/4.2.1/bits/c++io.h /usr/include/c++/4.2.1/bits/gthr.h \
  /usr/include/c++/4.2.1/bits/gthr-default.h /usr/include/pthread.h \
  /usr/include/pthread_impl.h /usr/include/sched.h /usr/include/time.h \
  /usr/include/_structs.h /usr/include/c++/4.2.1/cctype \
  /usr/include/ctype.h /usr/include/runetype.h \
  /usr/include/c++/4.2.1/bits/stringfwd.h \
  /usr/include/c++/4.2.1/bits/postypes.h /usr/include/c++/4.2.1/cwchar \
  /usr/include/c++/4.2.1/ctime /usr/include/wchar.h \
  /usr/include/_wctype.h /usr/include/stdint.h \
  /usr/include/c++/4.2.1/bits/functexcept.h \
  /usr/include/c++/4.2.1/exception_defines.h \
  /usr/include/c++/4.2.1/bits/stl_pair.h \
  /usr/include/c++/4.2.1/bits/cpp_type_traits.h \
  /usr/include/c++/4.2.1/ext/type_traits.h /usr/include/c++/4.2.1/utility \
  /usr/include/c++/4.2.1/bits/stl_relops.h \
  /usr/include/c++/4.2.1/bits/stl_iterator_base_types.h \
  /usr/include/c++/4.2.1/bits/stl_iterator_base_funcs.h \
  /usr/include/c++/4.2.1/bits/concept_check.h \
  /usr/include/c++/4.2.1/bits/stl_iterator.h \
  /usr/include/c++/4.2.1/debug/debug.h \
  /usr/include/c++/4.2.1/bits/allocator.h \
  /usr/include/c++/4.2.1/bits/c++allocator.h \
  /usr/include/c++/4.2.1/ext/new_allocator.h /usr/include/c++/4.2.1/new \
  /usr/include/c++/4.2.1/exception \
  /usr/include/c++/4.2.1/bits/stl_construct.h \
  /usr/include/c++/4.2.1/bits/stl_function.h \
  /usr/include/c++/4.2.1/bits/stl_map.h \
  /usr/include/c++/4.2.1/bits/stl_multimap.h \
  /usr/include/c++/4.2.1/string /usr/include/c++/4.2.1/bits/char_traits.h \
  /usr/include/c++/4.2.1/memory \
  /usr/include/c++/4.2.1/bits/stl_uninitialized.h \
  /usr/include/c++/4.2.1/bits/stl_raw_storage_iter.h \
  /usr/include/c++/4.2.1/limits \
  /usr/include/c++/4.2.1/bits/ostream_insert.h \
  /usr/include/c++/4.2.1/bits/basic_string.h \
  /usr/include/c++/4.2.1/ext/atomicity.h \
  /usr/include/c++/4.2.1/bits/atomic_word.h \
  /usr/include/c++/4.2.1/algorithm /usr/include/c++/4.2.1/bits/stl_algo.h \
  /usr/include/c++/4.2.1/bits/stl_heap.h \
  /usr/include/c++/4.2.1/bits/stl_tempbuf.h \
  /usr/include/c++/4.2.1/bits/basic_string.tcc \
  /usr/include/c++/4.2.1/deque /usr/include/c++/4.2.1/bits/stl_deque.h \
  /usr/include/c++/4.2.1/bits/deque.tcc /usr/include/c++/4.2.1/iostream \
  /usr/include/c++/4.2.1/ostream /usr/include/c++/4.2.1/ios \
  /usr/include/c++/4.2.1/bits/localefwd.h \
  /usr/include/c++/4.2.1/bits/ios_base.h \
  /usr/include/c++/4.2.1/bits/locale_classes.h \
  /usr/include/c++/4.2.1/streambuf \
  /usr/include/c++/4.2.1/bits/streambuf.tcc \
  /usr/include/c++/4.2.1/bits/basic_ios.h \
  /usr/include/c++/4.2.1/bits/streambuf_iterator.h \
  /usr/include/c++/4.2.1/bits/locale_facets.h \
  /usr/include/c++/4.2.1/cwctype /usr/include/wctype.h \
  /usr/include/c++/4.2.1/bits/ctype_base.h \
  /usr/include/c++/4.2.1/bits/ctype_inline.h \
  /usr/include/c++/4.2.1/bits/codecvt.h \
  /usr/include/c++/4.2.1/bits/time_members.h \
  /usr/include/c++/4.2.1/bits/messages_members.h \
  /usr/include/c++/4.2.1/bits/basic_ios.tcc \
  /usr/include/c++/4.2.1/bits/ostream.tcc /usr/include/c++/4.2.1/locale \
  /usr/include/c++/4.2.1/bits/locale_facets.tcc \
  /usr/include/c++/4.2.1/typeinfo /usr/include/c++/4.2.1/istream \
  /usr/include/c++/4.2.1/bits/istream.tcc sli_type.h sli_interpreter.h \
  sli_token.h sli_exceptions.h /usr/include/c++/4.2.1/vector \
  /usr/include/c++/4.2.1/bits/stl_vector.h \
  /usr/include/c++/4.2.1/bits/stl_bvector.h \
  /usr/include/c++/4.2.1/bits/vector.tcc sli_allocator.h sli_arraytype.h \
  sli_array.h sli_integertype.h SLI_token.h sli_tokenstack.h sli_string.h \
  sli_dictionary.h sli_dictstack.h /usr/include/c++/4.2.1/list \
  /usr/include/c++/4.2.1/bits/stl_list.h \
  /usr/include/c++/4.2.1/bits/list.tcc sli_builtins.h sli_module.h \
  /usr/include/c++/4.2.1/cmath /usr/include/math.h \
  /usr/include/architecture/i386/math.h \
  /usr/include/c++/4.2.1/bits/cmath.tcc
