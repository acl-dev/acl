SHELL = /bin/sh
CC	= ${ENV_CPP}
AR      = ${ENV_AR}
ARFL    = rv
RANLIB  = ${ENV_RANLIB}

FLAGS   = ${ENV_FLAGS}

CFLAGS = -c -g -W \
-O3 \
-fPIC \
-Wall \
-Werror \
-Wpointer-arith \
-D_REENTRANT \
-D_POSIX_PTHREAD_SEMANTICS \
-Wno-long-long \
-Wformat \

###########################################################
#Check system:
SYSLIB =
CHECKSYSRES = @echo "Unknow system type!";exit 1
UNIXNAME = $(shell uname -sm)
UNIXTYPE = LINUX
polarssl =

ifeq ($(CC),)
        CC = g++
endif

ifeq ($(AR),)
	AR = ar
endif

ifeq ($(RANLIB),)
	RANLIB = ranlib
endif

ifeq ($(findstring clang++, $(CC)), clang++)
	CFLAGS += -Wno-invalid-source-encoding -Wno-invalid-offsetof
endif

STRIP_ARGS =
ifeq ($(findstring Darwin, $(UNIXNAME)), Darwin)
	STRIP_ARGS = -x -S
endif

#Find system type.
ifneq ($(SYSPATH),)
	CHECKSYSRES = @echo "System is $(shell uname -sm)"
endif
###########################################################

OBJ_PATH_DST = ./debug
LIB_PATH_DST = .
DEF_PATH_DST = ./debug

CFLAGS += -I. -I../../lib_protocol/include -I../../lib_acl/include
shared_ldflags = ../../build/lib/libprotocol.a ../../build/lib/libacl.a -lz -lpthread

#Project's objs
OBJS_DST = $(patsubst %.cpp, $(OBJ_PATH_DST)/%.o, $(notdir $(wildcard ./*.cpp)))

###########################################################
.PHONY = all shared clean

SHARED_LIBNAME = libhttp.so

all: shared

$(shell mkdir -p $(DEF_PATH_DST))

COMPILE = $(CC) $(CFLAGS)
COMPILE_OBJ = @(echo 'building $<'; $(COMPILE) $< -o $@)
CREATE_DEF = @(echo 'creating $@'; rm -f $@; \
	$(COMPILE) -MM $< > $@.$$$$; \
	sed 's,.*.o\( \)*:,$(patsubst %.inc,%.o,$@) $@ :,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$)

###########################################################
rpath = .

shared: $(OBJS_DST)
	@echo 'creating $(SHARED_LIBNAME)'
	$(CC) -shared -o $(rpath)/$(SHARED_LIBNAME) $(OBJS_DST) \
		$(shared_ldflags); \
	echo 'build $(rpath)/$(SHARED_LIBNAME) ok!';
	strip $(STRIP_ARGS) $(rpath)/$(SHARED_LIBNAME)
	ls -lh $(rpath)/$(SHARED_LIBNAME)

# src
$(OBJ_PATH_DST)/%.o: ./%.cpp
	$(COMPILE_OBJ)

clean cl:
	rm -f $(LIB_PATH_DST)/$(SHARED_LIBNAME)
	rm -f $(OBJS_DST)

rebuild rb: clean static
