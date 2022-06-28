SHELL = /bin/sh
CC      = gcc
#CC      = g++
AR      = ar
ARFL    = rv
RANLIB  = ranlib

CFLAGS = -c -g -W -Wall -Wcast-qual -Wcast-align \
-Waggregate-return -Wmissing-prototypes \
-Wpointer-arith -Werror -Wshadow -pedantic -O2 \
-D_REENTRANT -D_POSIX_PTHREAD_SEMANTICS -D_USE_FAST_MACRO \
-Wno-long-long \
-DUSE_TLS
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

ACL_PATH = ../../../../lib_acl
ACL_INC = $(ACL_PATH)/include
ACL_LIB = $(ACL_PATH)/lib

TLS_PATH = ../../../../lib_tls
TLS_INC = $(TLS_PATH)/include
TLS_LIB = $(TLS_PATH)/lib

DICT_PATH = ../../../../lib_dict
DICT_INC = $(DICT_PATH)/include
DICT_LIB = $(DICT_PATH)/lib

LD_RUN_PATH = -Wl,-rpath,/opt/messenger/lib/mysql/linux/lib

ALL_LIBS = -L$(TLS_LIB) -l_tls -L$(DICT_LIB) -l_dict -L$(ACL_LIB) -l_acl \
	-lssl -ldb $(SYSLIB)
#	-L/home/zsx/download/openssl/bin/lib -lssl \
#	-L/home/zsx/download/openssl/bin/lib -lcrypto \
#	-ldb $(SYSLIB)

INCLUDE = -I. -I../.. -I$(TLS_INC) -I$(DICT_INC) -I$(ACL_INC)
CFLAGS += $(INCLUDE)

OUTPATH = ./
OBJ_OUTPATH = $(OUTPATH)

#Project's objs
SOURCES = $(wildcard *.c)
INCLUDES = $(wildcard *.h)
OBJS = $(patsubst %.c,$(OBJ_OUTPATH)%.o,$(SOURCES))

###########################################################

PROG_NAME  = tlscli

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
