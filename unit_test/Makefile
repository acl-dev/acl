SHELL = /bin/sh
#CC      = gcc
#CC      = g++
CC	= ${MY_ENV_CC}
AR      = ar
ARFL    = rv
#ARFL    = cru
RANLIB  = ranlib

CFLAGS = -c -g -W \
-O3 \
-Wall \
-Werror \
-Wshadow \
-Wcast-qual \
-D_REENTRANT \
-D_POSIX_PTHREAD_SEMANTICS \
-D_USE_FAST_MACRO \
-Wpointer-arith \
-Waggregate-return \
-Wmissing-prototypes \
-Wno-long-long
#-Wno-invalid-source-encoding
# -Wcast-align
###########################################################
#Check system:
#       Linux, SunOS, Solaris, BSD variants, AIX, HP-UX
SYSLIB = -lpthread
CHECKSYSRES = @echo "Unknow system type!";exit 1
UNIXNAME = $(shell uname -sm)

ifeq ($(CC),)
        CC = gcc
endif

ifeq ($(findstring gcc, $(CC)), gcc)
	CFLAGS += -Wstrict-prototypes
endif

# For FreeBSD
ifeq ($(findstring FreeBSD, $(UNIXNAME)), FreeBSD)
	CFLAGS += -DFREEBSD -pedantic
	SYSLIB = -lcrypt -lpthread
endif

# For Darwin
ifeq ($(findstring Darwin, $(UNIXNAME)), Darwin)
	CFLAGS += -DMACOSX -Wno-invalid-source-encoding -Wno-invalid-offsetof
	UNIXTYPE = MACOSX
endif

#Path for Linux
ifeq ($(findstring Linux, $(UNIXNAME)), Linux)
	CFLAGS += -DLINUX2 -pedantic
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
	CFLAGS += -DSUNOS5 -pedantic
endif

#Path for HP-UX
ifeq ($(findstring HP-UX, $(UNIXNAME)), HP-UX)
	CFLAGS += -DHP_UX -DHPUX11
	PLAT_NAME=hp-ux
endif

#Find system type.
ifneq ($(SYSPATH),)
	CHECKSYSRES = @echo "System is $(shell uname -sm)"
endif
###########################################################
ACL_HOME     = ../lib_acl
ACL_INC_PATH = $(ACL_HOME)/include
ACL_LIB_PATH = $(ACL_HOME)/lib

CFLAGS += -I$(ACL_INC_PATH)

LINK_LIBS = -L$(ACL_LIB_PATH) -lacl $(SYSLIB)

###########################################################
OUT_PATH = .
OBJ_PATH_DST = $(OUT_PATH)/debug
PROG_PATH_DST = $(OUT_PATH)/bin

BASE_PATH  = .
SRC_PATH_SRC = $(BASE_PATH)

INC_COMPILE  = -I. -I./stdlib -I./net -I./unit_test
CFLAGS += $(INC_COMPILE)

#Project's source
BASE_SRC        = $(wildcard *.c)
STDLIB_SRC	= $(wildcard $(SRC_PATH_SRC)/stdlib/*.c)
UNIT_TEST_SRC	= $(wildcard $(SRC_PATH_SRC)/unit_test/*.c)
NET_SRC		= $(wildcard $(SRC_PATH_SRC)/net/*.c)
SOURCES_SRC	= $(BASE_SRC) $(STDLIB_SRC) $(UNIT_TEST_SRC) $(NET_SRC)

#Project's obj
OBJS_DST        = $(patsubst %.c, $(OBJ_PATH_DST)/%.o, $(notdir $(SOURCES_SRC)))

###########################################################
.PHONY = all clean myecho

PROG_NAME  = test_main

COMPILE = $(CC) $(CFLAGS)
LINK    = $(CC) -o $(PROG_PATH_DST)/$(PROG_NAME) $(OBJS_DST) $(LINK_LIBS)

all: $(PROG_NAME)

$(PROG_NAME): $(OBJS_DST)
	$(LINK)

$(OBJ_PATH_DST)/%.o: %.c *.h
	$(COMPILE) $< -o $@

$(OBJ_PATH_DST)/%.o: stdlib/%.c stdlib/*.h
	$(COMPILE) $< -o $@

$(OBJ_PATH_DST)/%.o: unit_test/%.c unit_test/*.h
	$(COMPILE) $< -o $@

$(OBJ_PATH_DST)/%.o: net/%.c net/*.h
	$(COMPILE) $< -o $@

clean:
	rm -f $(PROG_PATH_DST)/$(PROG_NAME)
	rm -f $(OBJS_DST)

myecho:
	@(echo "STDLIB_SRC: $(STDLIB_SRC)")
	@(echo "UNIT_TEST_SRC: $(UNIT_TEST_SRC)")
	@(echo "SOURCES_SRC: $(SOURCES_SRC)")
	@(echo "OBJS_DST: $(OBJS_DST)")
	@(echo "OBJ_PATH_DST: $(OBJ_PATH_DST)")


