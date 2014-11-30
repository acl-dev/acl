SHELL = /bin/sh
#CC      = gcc
CC      = g++
AR      = ar
ARFL    = rv
RANLIB  = ranlib

CFLAGS = -c -g -W -Wcast-qual -Wcast-align \
-Waggregate-return \
-Wpointer-arith -Werror -Wshadow -O2 \
-D_REENTRANT -D_POSIX_PTHREAD_SEMANTICS -D_USE_FAST_MACRO \
-Wno-long-long
# -pedantic -Wall 
# -Wmissing-prototypes
###########################################################
#Check system:
#       Linux, SunOS, Solaris, BSD variants, AIX, HP-UX
SYSLIB =
CHECKSYSRES = @echo "Unknow system type!";exit 1
UNIXNAME = $(shell uname -sm)

# For FreeBSD
ifeq ($(findstring FreeBSD, $(UNIXNAME)), FreeBSD)
	ifeq ($(findstring gcc, $(CC)), gcc)
		CFLAGS += -Wstrict-prototypes
	endif
	CFLAGS += -DFREEBSD -D_REENTRANT -pedantic
	SYSLIB = -lcrypt -lpthread
endif

#Path for Linux
ifeq ($(findstring Linux, $(UNIXNAME)), Linux)
	ifeq ($CC, "gcc")
		CFLAGS += -Wstrict-prototypes
	endif
	CFLAGS += -DLINUX2
	SYSLIB = -lcrypt -lpthread
endif

#Path for SunOS
ifeq ($(findstring SunOS, $(UNIXNAME)), SunOS)
	ifeq ($(findstring 86, $(UNIXNAME)), 86)
		SYSLIB = -lsocket -lnsl -lrt
	endif
	ifeq ($(findstring sun4u, $(UNIXNAME)), sun4u)
		SYSLIB = -lsocket -lnsl -lrt
	endif
	ifeq ($CC, "gcc")
		CFLAGS += -Wstrict-prototypes
	endif
	CFLAGS += -DSUNOS5
endif

#Path for HP-UX
ifeq ($(findstring HP-UX, $(UNIXNAME)), HP-UX)
	ifeq ($CC, "gcc")
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

LIB_BASE_PATH = ../../lib_acl
LIB_NAME_PATH = -L$(LIB_BASE_PATH)/lib
LIB_INCL_PATH = $(LIB_BASE_PATH)/include

ALL_LIBS = -l_acl $(SYSLIB)

BASE_PATH  = .
INCLUDEDIR = $(BASE_PATH)
INCLUDE = -I$(INCLUDEDIR) -I$(LIB_INCL_PATH)
CFLAGS += $(INCLUDE)

OUTPATH = ./
OBJ_OUTPATH = $(OUTPATH)

#Project's objs
SOURCES = $(wildcard *.c)
INCLUDES = $(wildcard *.h)
OBJS = $(patsubst %.c,$(OBJ_OUTPATH)%.o,$(SOURCES))

###########################################################

PROG_NAME  = cgi_env

.PHONY = clean
COMPILE = $(CC) $(CFLAGS)

all: RM $(PROG_NAME)

RM:
	rm -f $(PROG_NAME)

$(PROG_NAME): $(OBJS)
	$(CC) -o $(PROG_NAME) $(OBJS) $(LIB_NAME_PATH) $(ALL_LIBS)

$(OBJ_OUTPATH)%.o: %.c *.h
	$(COMPILE) -o $@ $<

clean:
	rm -f $(OBJS) $(PROG_NAME)

rebuild: clean all
