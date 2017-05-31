SHELL = /bin/sh
CC      = gcc
#CC      = g++
AR      = ar
ARFL    = rv
RANLIB  = ranlib

CFLAGS = -c -g -W -Wcast-qual -Wcast-align \
-Waggregate-return -Wmissing-prototypes \
-Wpointer-arith -Werror -Wshadow -O2 \
-D_REENTRANT -D_POSIX_PTHREAD_SEMANTICS -D_USE_FAST_MACRO \
-Wno-long-long \
-pedantic -Wall 
###########################################################
#Check system:
#       Linux, SunOS, Solaris, BSD variants, AIX, HP-UX
SYSLIB =
CHECKSYSRES = @echo "Unknow system type!";exit 1
UNIXNAME = $(shell uname -sm)

ifeq ($(findstring gcc, $(CC)), gcc)
	CFLAGS += -Wstrict-prototypes
endif

# For FreeBSD
ifeq ($(findstring FreeBSD, $(UNIXNAME)), FreeBSD)
	CFLAGS += -DFREEBSD -D_REENTRANT -pedantic
	SYSLIB = -lcrypt -lpthread
endif

# For Darwin
ifeq ($(findstring Darwin, $(UNIXNAME)), Darwin)
	CFLAGS += -D_REENTRANT -pedantic -DMACOSX
	SYSLIB = -lpthread
endif

#Path for Linux
ifeq ($(findstring Linux, $(UNIXNAME)), Linux)
	CFLAGS += -DLINUX2
	SYSLIB = -lcrypt -lpthread
endif

# For MINGW
ifeq ($(findstring MINGW, $(UNIXNAME)), MINGW)
	SYSLIB = -lpthread-2
	CFLAGS += -DLINUX2 -DMINGW
	UNIXTYPE = LINUX
endif

# For MSYS
ifeq ($(findstring MSYS, $(UNIXNAME)), MSYS)
	SYSLIB = -lpthread-2
	CFLAGS += -DLINUX2 -DMINGW
	UNIXTYPE = LINUX
endif

#Path for SunOS
ifeq ($(findstring SunOS, $(UNIXNAME)), SunOS)
	ifeq ($(findstring 86, $(UNIXNAME)), 86)
		SYSLIB = -lsocket -lnsl -lrt
	endif
	ifeq ($(findstring sun4u, $(UNIXNAME)), sun4u)
		SYSLIB = -lsocket -lnsl -lrt
	endif
	CFLAGS += -DSUNOS5
endif

#Path for HP-UX
ifeq ($(findstring HP-UX, $(UNIXNAME)), HP-UX)
	SYSLIB = -lpthread
	CFLAGS += -DHP_UX -DHPUX11
	PLAT_NAME=hp-ux
endif

#Find system type.
ifneq ($(SYSPATH),)
	CHECKSYSRES = @echo "System is $(shell uname -sm)"
endif
###########################################################

LIB_BASE_PATH = ../../../lib_acl
LIB_NAME_PATH = -L$(LIB_BASE_PATH)/lib
LIB_INCL_PATH = $(LIB_BASE_PATH)/include

ALL_LIBS = -lacl $(SYSLIB)

BASE_PATH  = .
INCLUDEDIR = $(BASE_PATH)
INCLUDE = -I$(INCLUDEDIR) -I$(LIB_INCL_PATH)
CFLAGS += $(INCLUDE)

COMPILE = $(CC) $(CFLAGS)
###########################################################

PROG_PARENT = parent
PROG_CHILD = child

.PHONY = all clean

all: $(PROG_PARENT) $(PROG_CHILD)

$(PROG_PARENT): parent.o
	$(CC) -o $(PROG_PARENT) parent.o $(LIB_NAME_PATH) $(ALL_LIBS)
$(PROG_CHILD): child.o
	$(CC) -o $(PROG_CHILD) child.o $(LIB_NAME_PATH) $(ALL_LIBS)

parent.o: parent.c
	$(COMPILE) -o parent.o parent.c
child.o: child.c
	$(COMPILE) -o child.o child.c
clean:
	rm -f $(PROG_PARENT) $(PROG_CHILD)
	rm -f *.o

rebuild: clean all
