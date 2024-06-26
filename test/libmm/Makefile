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
CHECKSYSRES = @echo "Unknow system type!";exit 1
UNIXNAME = $(shell uname -sm)
shared_ldflags =

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
	CFLAGS += -Wno-invalid-source-encoding \
		  -Wno-invalid-offsetof
endif

# For Linux
ifeq ($(findstring Linux, $(UNIXNAME)), Linux)
	shared_ldflags += -lunwind -lunwind-x86_64 -lz -lpthread
endif

# For Darwin
ifeq ($(findstring Darwin, $(UNIXNAME)), Darwin)
	CFLAGS += -D_XOPEN_SOURCE -Wno-deprecated-declarations
	shared_ldflags += -lz -lpthread
endif

#Find system type.
ifneq ($(SYSPATH),)
	CHECKSYSRES = @echo "System is $(shell uname -sm)"
endif
###########################################################

OBJ_PATH_DST = ./debug
LIB_PATH_DST = .
DEF_PATH_DST = ./debug

CFLAGS += -I.

#Project's objs
OBJS_DST = $(patsubst %.cpp, $(OBJ_PATH_DST)/%.o, $(notdir $(wildcard ./*.cpp)))

###########################################################
.PHONY = all shared clean

SHARED_LIBNAME = libmm.so

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

# src
$(OBJ_PATH_DST)/%.o: ./%.cpp
	$(COMPILE_OBJ)

clean cl:
	rm -f $(LIB_PATH_DST)/$(SHARED_LIBNAME)
	rm -f $(OBJS_DST)

rebuild rb: clean static
