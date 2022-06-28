SHELL = /bin/sh
#CC      = gcc
CC      = g++
AR      = ar
ARFL    = rv
RANLIB  = ranlib

CFLAGS = -c -g -W -Wall -Wcast-qual -Wcast-align \
-Waggregate-return \
-D_REENTRANT -D_POSIX_PTHREAD_SEMANTICS -D_USE_FAST_MACRO \
-Wno-long-long \
-Wpointer-arith -Werror -Wshadow -pedantic -O2

###########################################################
#Check system:
#       Linux, SunOS, Solaris, BSD variants, AIX, HP-UX
SYSLIB =
RPATH =

CHECKSYSRES = @echo "Unknow system type!";exit 1
UNIXNAME = $(shell uname -sm)
OSTYPE = $(shell uname -p)

#Path for Linux
ifeq ($(findstring Linux, $(UNIXNAME)), Linux)
#	ifeq ($CC, "gcc")
	ifeq ($(findstring i686, $(OSTYPE)), i686)
		RPATH = linux32
	endif
	ifeq ($(findstring x86_64, $(OSTYPE)), x86_64)
		RPATH = linux64
	endif
	ifeq ($(findstring gcc, $(CC)), gcc)
		CFLAGS += -Wstrict-prototypes -Wmissing-prototypes
	endif
	CFLAGS += -DLINUX2
	SYSLIB = -lcrypt -lpthread
endif

#Path for SunOS
ifeq ($(findstring SunOS, $(UNIXNAME)), SunOS)
	ifeq ($(findstring i386, $(OSTYPE)), i386)
		RPATH = sunos_x86
	endif
	ifeq ($(findstring 86, $(UNIXNAME)), 86)
		SYSLIB = -lsocket -lnsl -lrt
	endif
	ifeq ($(findstring sun4u, $(UNIXNAME)), sun4u)
		SYSLIB = -lsocket -lnsl -lrt
	endif
#	ifeq ($CC, "gcc")
	ifeq ($(findstring gcc, $(CC)), gcc)
		CFLAGS += -Wstrict-prototypes
	endif
	CFLAGS += -DSUNOS5
endif

#Path for HP-UX
ifeq ($(findstring HP-UX, $(UNIXNAME)), HP-UX)
#	ifeq ($CC, "gcc")
	ifeq ($(findstring gcc, $(CC)), gcc)
		CFLAGS += -Wstrict-prototypes
	endif
	SYSLIB = -lpthread
	CFLAGS += -DHP_UX -DHPUX11
	PLAT_NAME=hp-ux
endif

#Find system type.
ifneq ($(SYSPATH),)
	CHECKSYSRES = @echo "System is $(shell uname -sm)"
endif
###########################################################
ACL_PATH = ../../lib_all/lib_acl
ACL_LIB  = $(ACL_PATH)/lib
ACL_INC  = $(ACL_PATH)/include

ALL_LIBS = -L$(ACL_LIB) -l_acl $(SYSLIB)

INCLUDE = -I$(ACL_INC)
CFLAGS += $(INCLUDE) -DHAVE_NO_STRCASESTR

OBJ_OUTPATH = ./Debug

#Project's objs
SOURCES = $(wildcard *.cpp)
OBJS = $(patsubst %.cpp, $(OBJ_OUTPATH)/%.o, $(notdir $(SOURCES)))

###########################################################
PROG_NAME  = dbench

.PHONY = RM all clean
COMPILE = $(CC) $(CFLAGS)

all: RM $(PROG_NAME)

RM:
	rm -f $(PROG_NAME)

$(PROG_NAME): $(OBJS)
	$(CC) -o $(PROG_NAME) $(OBJS) $(LIB_NAME_PATH) $(ALL_LIBS)

$(OBJ_OUTPATH)/%.o: %.cpp
	$(COMPILE) -o $@ $<

clean:
	rm -f $(OBJS) $(PROG_NAME)

rebuild: clean all
