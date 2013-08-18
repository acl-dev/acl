SHELL = /bin/sh
CC      = g++
CC      = gcc
#CC	= ${MY_ENV_CC}
AR      = ar
ARFL    = rv
#ARFL    = cru
RANLIB  = ranlib

CFLAGS = -c -g -W -Wall -Wcast-qual -Wcast-align \
-Waggregate-return \
-Wno-long-long \
-Wpointer-arith -Werror -Wshadow -O2 \
-D_REENTRANT -D_POSIX_PTHREAD_SEMANTICS -D_USE_FAST_MACRO

#CFLAGS = -c -g -W -Wall -Wcast-qual -Wcast-align \
#-Waggregate-return -Wmissing-prototypes \
#-Wpointer-arith -Werror -Wshadow -O2 \
#-D_POSIX_PTHREAD_SEMANTICS -D_USE_FAST_MACRO
###########################################################
#Check system:
#       Linux, SunOS, Solaris, BSD variants, AIX, HP-UX
SYSLIB = -lpthread -lcrypt -lz
CHECKSYSRES = @echo "Unknow system type!";exit 1
UNIXNAME = $(shell uname -sm)
OSTYPE = $(shell uname -p)
RPATH = 

ifeq ($(CC),)
        CC = gcc
endif

#Path for Linux
ifeq ($(findstring Linux, $(UNIXNAME)), Linux)
	ifeq ($CC, "gcc")
		CFLAGS += -Wstrict-prototypes
	endif
	CFLAGS += -DLINUX2 -D_REENTRANT -pedantic

	ifeq ($(findstring i686, $(OSTYPE)), i686)
		RPATH = linux32
		FLAG_OS = LINUX32
	endif
	ifeq ($(findstring x86_64, $(OSTYPE)), x86_64)
		RPATH = linux64
		FLAG_OS = LINUX64
	endif
endif

#Path for SunOS
ifeq ($(findstring SunOS, $(UNIXNAME)), SunOS)
	ifeq ($(findstring 86, $(UNIXNAME)), 86)
		SYSLIB += -lsocket -lnsl -lrt
	endif
	ifeq ($(findstring sun4u, $(UNIXNAME)), sun4u)
		SYSLIB += -lsocket -lnsl -lrt
	endif
	ifeq ($CC, "gcc")
		CFLAGS += -Wstrict-prototypes
	endif
	CFLAGS += -DSUNOS5 -D_REENTRANT -pedantic
endif

#Path for HP-UX
ifeq ($(findstring HP-UX, $(UNIXNAME)), HP-UX)
	ifeq ($CC, "gcc")
		CFLAGS += -Wstrict-prototypes
	endif
	CFLAGS += -DHP_UX -DHPUX11
	PLAT_NAME=hp-ux
endif

#Find system type.
ifneq ($(SYSPATH),)
	CHECKSYSRES = @echo "System is $(shell uname -sm)"
endif
###########################################################
BASE_PATH  = ../../../build

INC_COMPILE = -I./include -I../../../lib_acl/include \
		-I../../../lib_protocol/include
LIB_COMPILE = -L../../../lib_acl/lib -L../../../lib_protocol/lib -l_protocol -l_acl

CFLAGS += $(INC_COMPILE)

###########################################################
OUT_PATH = .
OBJ_PATH = $(OUT_PATH)/debug

#Project's objs
SRC = $(wildcard src/*.c) $(wildcard unix/*.c)
OBJ = $(patsubst %.c, $(OBJ_PATH)/%.o, $(notdir $(SRC)))
###########################################################

PROG  = gid_server

all: rm gid_server
.PHONY = gid_server clean
COMPILE = $(CC) $(CFLAGS)

$(PROG): $(OBJ)
	$(CC) $(OBJ) $(LIB_COMPILE) $(SYSLIB) -o $(PROG)
#	cp $(OBJ_PATH)/$(PROG) $(BASE_PATH)/dist/master/libexec
	@echo ""
	@echo "All ok! Output:$(PROG)"
	@echo ""

$(OBJ_PATH)/%.o: src/%.c
	$(COMPILE) $< -o $@
$(OBJ_PATH)/%.o: unix/%.c
	$(COMPILE) $< -o $@

rm:
	rm -f $(PROG)
clean:
	rm -f $(PROG)
	rm -f $(OBJ)
