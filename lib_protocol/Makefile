SHELL = /bin/sh
CC      = gcc
#CC      = g++
CC	= ${MY_ENV_CC}
AR      = ar
ARFL    = rv
#ARFL    = cru
RANLIB  = ranlib

CFLAGS = -c -g -W -Wall -Wcast-qual -Wcast-align \
-Waggregate-return -Wmissing-prototypes \
-Wpointer-arith -Werror -Wshadow -O3 \
-D_REENTRANT -D_POSIX_PTHREAD_SEMANTICS -D_USE_FAST_MACRO \
-Wno-long-long \
-fPIC

#CFLAGS = -c -g -W -Wall -Wcast-qual -Wcast-align \
#-Waggregate-return -Wmissing-prototypes \
#-Wpointer-arith -Werror -Wshadow -O2 \
#-D_POSIX_PTHREAD_SEMANTICS -D_USE_FAST_MACRO
###########################################################
#Check system:
#       Linux, SunOS, Solaris, BSD variants, AIX, HP-UX
SYSLIB =
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
	CFLAGS += -DFREEBSD -D_REENTRANT -pedantic
endif

# For Darwin
ifeq ($(findstring Darwin, $(UNIXNAME)), Darwin)
	CFLAGS += -DMACOSX -D_REENTRANT -pedantic
endif

# For Linux
ifeq ($(findstring Linux, $(UNIXNAME)), Linux)
	CFLAGS += -DLINUX2 -D_REENTRANT -pedantic
endif

# For SunOS
ifeq ($(findstring SunOS, $(UNIXNAME)), SunOS)
	ifeq ($(findstring 86, $(UNIXNAME)), 86)
		SYSLIB = -lsocket -lnsl -lrt
	endif
	ifeq ($(findstring sun4u, $(UNIXNAME)), sun4u)
		SYSLIB = -lsocket -lnsl -lrt
	endif
	CFLAGS += -DSUNOS5 -D_REENTRANT -pedantic
endif

# For HP-UX
ifeq ($(findstring HP-UX, $(UNIXNAME)), HP-UX)
	CFLAGS += -DHP_UX -DHPUX11
	PLAT_NAME=hp-ux
endif

#Find system type.
ifneq ($(SYSPATH),)
	CHECKSYSRES = @echo "System is $(shell uname -sm)"
endif
###########################################################

OUT_PATH = .
OBJ_PATH_DST = $(OUT_PATH)/debug
LIB_PATH_DST = $(OUT_PATH)/lib

BASE_PATH  = .
INC_PATH_SRC = $(BASE_PATH)/include
SRC_PATH_SRC = $(BASE_PATH)/src

INC_COMPILE  = -I$(INC_PATH_SRC) -I./src
CFLAGS += $(INC_COMPILE)

#Project's objs
BASE_SRC   = $(wildcard $(SRC_PATH_SRC)/*.c)
HTTP_SRC   = $(wildcard $(SRC_PATH_SRC)/http/*.c)
ICMP_SRC   = $(wildcard $(SRC_PATH_SRC)/icmp/*.c)
SMTP_SRC   = $(wildcard $(SRC_PATH_SRC)/smtp/*.c)
SOURCES_SRC  = $(HTTP_SRC) $(ICMP_SRC) $(SMTP_SRC)

BASE_OBJ = $(patsubst %.c, $(OBJ_PATH_DST)/%.o, $(notdir $(BASE_SRC)))
HTTP_OBJ = $(patsubst %.c, $(OBJ_PATH_DST)/%.o, $(notdir $(HTTP_SRC)))
ICMP_OBJ = $(patsubst %.c, $(OBJ_PATH_DST)/%.o, $(notdir $(ICMP_SRC)))
SMTP_OBJ = $(patsubst %.c, $(OBJ_PATH_DST)/%.o, $(notdir $(SMTP_SRC)))
OBJS_DST = $(BASE_OBJ) $(HTTP_OBJ) $(ICMP_OBJ) $(SMTP_OBJ)
###########################################################

LIB_ACL_PATH = ../lib_acl
CFLAGS += -I $(LIB_ACL_PATH)/include

###########################################################

.PHONY = static shared clean
COMPILE = $(CC) $(CFLAGS)

all: static shared

$(shell mkdir -p $(OBJ_PATH_DST))

static: $(OBJS_DST)
	$(AR) $(ARFL) $(LIB_PATH_DST)/lib_protocol.a $(OBJS_DST)
	$(RANLIB) $(LIB_PATH_DST)/lib_protocol.a

SHARED_FDFLAGS = -l_acl -lrt -lpthread
shared: $(OBJS_DST)
	@echo 'creating $(LIB_PATH_DST)/lib_protocol.so'
	@if test -n "$(rpath)"; then \
		$(CC) -shared -o $(LIB_PATH_DST)/lib_protocol.so $(OBJS_DST) $(SHARED_FDFLAGS) \
			-L$(rpath) -Wl,-rpath,$(rpath); \
		echo 'build $(LIB_PATH_DST)/lib_protocol.so ok!'; \
	else \
		echo 'skip build $(LIB_PATH_DST)/lib_protocol.so; usage: make shared rpath=xxx'; \
	fi

$(OBJ_PATH_DST)/%.o: $(SRC_PATH_SRC)/%.c
	$(COMPILE) $< -o $@

# http
$(OBJ_PATH_DST)/%.o: $(SRC_PATH_SRC)/http/%.c
	$(COMPILE) $< -o $@

# icmp
$(OBJ_PATH_DST)/%.o: $(SRC_PATH_SRC)/icmp/%.c
	$(COMPILE) $< -o $@

# smtp
$(OBJ_PATH_DST)/%.o: $(SRC_PATH_SRC)/smtp/%.c
	$(COMPILE) $< -o $@

clean:
	rm -f $(LIB_PATH_DST)/lib_protocol.a
	rm -f $(LIB_PATH_DST)/lib_protocol.so
	rm -f $(OBJS_DST)

all_lib: $(OBJS_DST)
	@(cd debug; ar x ../../lib_acl/lib/lib_acl.a)
	$(AR) $(ARFL) $(LIB_PATH_DST)/lib_protocol.a debug/*.o
	$(RANLIB) $(LIB_PATH_DST)/lib_protocol.a
	@(cd debug; rm -f `ar -t ../../lib_acl/lib/lib_acl.a`)
