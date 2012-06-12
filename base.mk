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
CC=g++
GDB=-g
CCFLAGS= $(GDB) -Wall -O2

## Linker flags
LDLIBS= -lstdc++
LDFLAGS= $(GDB) $(LDLIBS) $(LIBREADLINE)

TARGET= sli3
OBJS= 	sli_main.o\
	sli_array.o\
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
	sli_interpreter.o

SOURCES = ${OBJS:.o=.cpp}

all:
	${MAKE} -f base.mk makefile "CFLAGS=${CFLAGS}"
	${MAKE} ${OBJS}
	${MAKE} sli

sli: ${OBJS}
	${CC} -o $@ ${OBJS} ${LDFLAGS}

.cc.o:
	${CC} ${CFLAGS} -c $*.cc

makefile: base.mk
	@ echo 'Generating makefile...'
	@ rm -f $@
	@ cp base.mk $@
	@ echo '# Automatically-generated dependency list:' >> $@
	@ ${CC} ${CFLAGS} -M ${SOURCES} >> $@
	@ chmod -w $@
