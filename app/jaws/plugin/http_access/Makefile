SHELL = /bin/sh
CC      = gcc
#CC      = g++
AR      = ar
ARFL    = rv
RANLIB  = ranlib

CFLAGS = -c -g -W -Wall -Wcast-align \
-Waggregate-return -Wmissing-prototypes \
-D_REENTRANT -D_POSIX_PTHREAD_SEMANTICS -D_USE_FAST_MACRO \
-Wno-long-long \
-Wpointer-arith -Werror -Wshadow -pedantic -O2 \
-fPIC -DPIC
#-Wcast-qual 
###########################################################
#Check system:
#       Linux, SunOS, Solaris, BSD variants, AIX, HP-UX
SYSLIB = -ldl
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
		CFLAGS += -Wstrict-prototypes
	endif
	CFLAGS += -DLINUX2
	SYSLIB += -lcrypt -lpthread
endif

#Path for SunOS
ifeq ($(findstring SunOS, $(UNIXNAME)), SunOS)
	ifeq ($(findstring i386, $(OSTYPE)), i386)
		RPATH = sunos_x86
	endif
	ifeq ($(findstring 86, $(UNIXNAME)), 86)
		SYSLIB += -lsocket -lnsl -lrt
	endif
	ifeq ($(findstring sun4u, $(UNIXNAME)), sun4u)
		SYSLIB += -lsocket -lnsl -lrt
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
	SYSLIB += -lpthread
	CFLAGS += -DHP_UX -DHPUX11
	PLAT_NAME=hp-ux
endif

#Find system type.
ifneq ($(SYSPATH),)
	CHECKSYSRES = @echo "System is $(shell uname -sm)"
endif
###########################################################
ACL_PATH = ../../../../lib_acl
ACL_LIB  = $(ACL_PATH)/lib
ACL_INC  = $(ACL_PATH)/include

PROTO_PATH = ../../../../lib_protocol
PROTO_LIB  = $(PROTO_PATH)/lib
PROTO_INC  = $(PROTO_PATH)/include

GLOBAL_PATH = ../../global
GLOBAL_LIB  = $(GLOBAL_PATH)
GLOBAL_INC  = $(GLOBAL_PATH)


#ALL_LIBS = -L$(GLOBAL_LIB) -l_global -L$(PROTO_LIB) -l_protocol -L$(ACL_LIB) -l_acl $(SYSLIB)
ALL_LIBS = -L$(PROTO_LIB) -l_protocol -L$(ACL_LIB) -l_acl $(SYSLIB)

#INCLUDE = -I$(GLOBAL_INC) -I$(PROTO_INC) -I$(ACL_INC)
INCLUDE = -I$(PROTO_INC) -I$(ACL_INC)
CFLAGS += $(INCLUDE) -DHAVE_NO_STRCASESTR

OBJ_OUTPATH = ./debug

#Project's objs
SOURCES = $(wildcard *.c)
OBJS = $(patsubst %.c, $(OBJ_OUTPATH)/%.o, $(notdir $(SOURCES)))

###########################################################
LIB_NAME  = http_access.so

.PHONY = RM all clean
COMPILE = $(CC) $(CFLAGS)

all: RM $(LIB_NAME)

RM:
	rm -f $(LIB_NAME)

$(LIB_NAME): $(OBJS)
	$(CC) -shared -o $(LIB_NAME) $(OBJS) $(ALL_LIBS)
	cp -f $(LIB_NAME) ../../dist/unix_setup/plugin/$(RPATH)/
#	$(CC) -shared -o $(LIB_NAME) $(OBJS)

$(OBJ_OUTPATH)/%.o: ./%.c
	$(COMPILE) -o $@ $<

clean:
	rm -f $(OBJS) $(LIB_NAME)
	rm -f $(LIB_NAME) ../../dist/unix_setup/plugin/$(RPATH)/$(LIB_NAME)

rebuild: clean all
