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
-Wno-long-long
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

LIB_ACL_PATH = ../../lib_acl
LIB_PATH = -L$(LIB_ACL_PATH)/lib
INCL_PATH = $(LIB_ACL_PATH)/include

ALL_LIBS = -l_acl $(SYSLIB)

INCLUDE = -I$(INCL_PATH)
CFLAGS += $(INCLUDE)

OUTPATH = ./
OBJ_OUTPATH = $(OUTPATH)

#Project's objs
SOURCES = $(wildcard *.c)
INCLUDES = $(wildcard *.h)
OBJS = $(patsubst %.c,$(OBJ_OUTPATH)%.o,$(SOURCES))

DIST_PATH = ../../dist/master/libexec
###########################################################

.PHONY = all clean install uninstall
#PROG_NAME  = acl_master
#PATH_RELEASE = ./acl/libexec

COMPILE = $(CC) $(CFLAGS)

all: $(PROG_NAME)

#$(PROG_NAME): $(OBJS)
#	$(CC) -o $(PROG_NAME) $(OBJS) $(LIB_PATH) $(ALL_LIBS)
#	cp $(PROG_NAME) $(PATH_RELEASE)/
all: build
build:
	@(cd single_echo; make)
	@(cd ioctl_echo2; make)
	@(cd ioctl_echo3; make)
	@(cd master_threads; make)
	@(cd trigger; make)
	@(cd single_proxy; make)
	@(cd aio_echo; make)
	@(cd aio_proxy; make)
	@(cd udp_echo; make)
#	@(cd master_notify; make)

$(OBJ_OUTPATH)%.o: %.c *.h
	$(COMPILE) -o $@ $<

clean:
	@(cd single_echo; make clean)
	@(cd ioctl_echo2; make clean)
	@(cd ioctl_echo3; make clean)
	@(cd master_threads; make clean)
	@(cd trigger; make clean)
	@(cd single_proxy; make clean)
	@(cd aio_proxy; make clean)
	@(cd aio_echo; make clean)
	@(cd master_notify; make clean)
	@(cd udp_echo; make clean)

install:
	cp single_echo/single_echo ${DIST_PATH}/
	cp ioctl_echo/ioctl_echo ${DIST_PATH}/
	cp trigger/trigger ${DIST_PATH}/
	cp single_proxy/single_proxy ${DIST_PATH}/
	cp aio_proxy/aio_proxy ${DIST_PATH}/
	cp udp_echo/udp_echo ${DIST_PATH}/
#	cp master_notify/acl_notify ${DIST_PATH}/
#	cp aio_echo/aio_echo ${DIST_PATH}/

uninstall:
	rm -f ${DIST_PATH}/single_echo
	rm -f ${DIST_PATH}/ioctl_echo
	rm -f ${DIST_PATH}/trigger
	rm -f ${DIST_PATH}/single_proxy
#	rm -f ${DIST_PATH}/acl_notify
#	rm -f ${DIST_PATH}/aio_echo

rebuild: clean all
