SHELL = /bin/sh
CC      = gcc
#CC      = g++
AR      = ar
ARFL    = rv
RANLIB  = ranlib

#CFLAGS = -c -g -W -Wall -Wcast-qual -Wcast-align
CFLAGS = -c -g -W -Wall -Wcast-align \
-Waggregate-return -Wmissing-prototypes \
-Wpointer-arith -Werror -Wshadow -O2 \
-D_REENTRANT -D_POSIX_PTHREAD_SEMANTICS -D_USE_FAST_MACRO \
-Wno-long-long
#-Wno-long-long -DHAS_DB -D_FILE_OFFSET_BITS -D_LARGEFILE64_SOURCE
#-Wno-long-long -DHAS_DB -D__USE_FILE_OFFSET64 -D__USE_LARGEFILE64
#-Wno-long-long -DHAS_DB -D_FILE_OFFSET_BITS
###########################################################
#Check system:
#       Linux, SunOS, Solaris, BSD variants, AIX, HP-UX
SYSLIB =
RPATH =
CHECKSYSRES = @echo "Unknow system type!";exit 1
UNIXNAME = $(shell uname -sm)
OSTYPE = $(shell uname -p)

# For FreeBSD
ifeq ($(findstring FreeBSD, $(UNIXNAME)), FreeBSD)
	ifeq ($(findstring gcc, $(CC)), gcc)
		CFLAGS += -Wstrict-prototypes
	endif
	CFLAGS += -DFREEBSD -D_REENTRANT -pedantic
	SYSLIB = -lpthread
	RPATH = freebsd
endif

#Path for Linux
ifeq ($(findstring Linux, $(UNIXNAME)), Linux)
	ifeq ($CC, "gcc")
		CFLAGS += -Wstrict-prototypes
	endif
	CFLAGS += -DLINUX2
	SYSLIB = -lcrypt -lpthread
	ifeq ($(findstring i686, $(OSTYPE)), i686)
		RPATH = linux32
	endif
	ifeq ($(findstring x86_64, $(OSTYPE)), x86_64)
		RPATH = linux64
	endif
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
	ifeq ($(findstring i386, $(OSTYPE)), i386)
		RPATH = sunos_x86
	endif
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

ACL_PATH = ../../lib_acl
ACL_LIB  = $(ACL_PATH)/lib
ACL_INC  = $(ACL_PATH)/include

PRO_PATH = ../../lib_protocol
PRO_LIB  = $(PRO_PATH)/lib
PRO_INC  = $(PRO_PATH)/include

DICT_PATH = ../../lib_dict
DICT_LIB = $(DICT_PATH)/lib
DICT_INC = $(DICT_PATH)/include

ALL_LIBS = -L$(DICT_LIB) -l_dict \
	-L$(PRO_LIB) -l_protocol \
	-L$(ACL_LIB) -l_acl \
	-ldb $(SYSLIB)

INCLUDE = -I$(DICT_INC) -I$(ACL_INC) -I$(PRO_INC)
CFLAGS += $(INCLUDE)

OUTPATH = .
OBJ_PATH = $(OUTPATH)/unix

#Project's objs
SOURCES = $(wildcard unix/*.c)
OBJS = $(patsubst %.c, $(OBJ_PATH)/%.o, $(notdir $(SOURCES)))

###########################################################

VERSION = 1.0.0
DATE_NOW = 20`date +%y`.`date +%m`.`date +%d`

PROG_NAME  = dictd

.PHONY = all clean RM clean_all
COMPILE = $(CC) $(CFLAGS)

all: RM $(PROG_NAME)

$(PROG_NAME): $(OBJS)
	$(CC) -o $(PROG_NAME) $(OBJS) $(ALL_LIBS)

$(OBJ_PATH)/%.o: unix/%.c
	$(COMPILE) $< -o $@

RM:
	@(rm -f $(PROG_NAME))

clean:
	rm -f $(OBJS) $(PROG_NAME)

rebuild: clean all
clean_all:
	@(make -f Makefile clean)
	@(make -f Makefile.aio clean)
	@(make -f Makefile.tools clean)
