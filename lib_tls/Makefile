SHELL = /bin/sh
CC      = gcc
#CC      = g++
CC	= ${MY_ENV_CC}
AR      = ar
ARFL    = rv
#ARFL    = cru
RANLIB  = ranlib

CFLAGS = -c -g -W -Wall -Wcast-align \
-Waggregate-return -Wmissing-prototypes \
-Wpointer-arith -Werror -Wshadow -O2 \
-D_REENTRANT -D_POSIX_PTHREAD_SEMANTICS -D_USE_FAST_MACRO \
-Wno-long-long \
-DUSE_TLS \
-fPIC
#-Wcast-qual 
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

# For FreeBSD
ifeq ($(findstring FreeBSD, $(UNIXNAME)), FreeBSD)
	ifeq ($(findstring gcc, $(CC)), gcc)
		CFLAGS += -Wstrict-prototypes
	endif
	CFLAGS += -DFREEBSD -D_REENTRANT -pedantic
endif

# For Darwin
ifeq ($(findstring Darwin, $(UNIXNAME)), Darwin)                               
	CFLAGS += -DMACOSX                                                     
	UNIXTYPE = MACOSX                                                      
endif 

# For Linux
ifeq ($(findstring Linux, $(UNIXNAME)), Linux)
	ifeq ($CC, "gcc")
		CFLAGS += -Wstrict-prototypes
	endif
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
	ifeq ($CC, "gcc")
		CFLAGS += -Wstrict-prototypes
	endif
	CFLAGS += -DSUNOS5 -D_REENTRANT -pedantic
endif

# For HP-UX
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

OUT_PATH = .
OBJ_PATH_DST = $(OUT_PATH)/debug
LIB_PATH_DST = $(OUT_PATH)/lib

#Project's objs
TLS_SRC   = $(wildcard ./*.c) \
		$(wildcard ./util/*.c) \
		$(wildcard ./attr/*.c) \
		$(wildcard ./tls/*.c)

TLS_OBJ = $(patsubst %.c, $(OBJ_PATH_DST)/%.o, $(notdir $(TLS_SRC)))

#CFLAGS += -I./attr -I./util
###########################################################

ACL_PATH = ../lib_acl
ACL_INC = $(ACL_PATH)/include
DICT_PATH = ../lib_dict
DICT_INC  = $(DICT_PATH)/include

CFLAGS += -I. -I./include -I$(ACL_INC) -I$(DICT_INC)

###########################################################
.PHONY = clean LIB_NAME

LIB_NAME  = lib_tls.a

COMPILE = $(CC) $(CFLAGS)

all: TLSMGR

$(shell mkdir -p $(OBJ_PATH_DST))

TLSMGR: $(LIB_NAME)
	@(cd tlsmgr; make)

$(LIB_NAME): $(TLS_OBJ)
	$(AR) $(ARFL) $(LIB_PATH_DST)/$(LIB_NAME) $(TLS_OBJ)
	$(RANLIB) $(LIB_PATH_DST)/$(LIB_NAME)

$(OBJ_PATH_DST)/%.o: ./%.c
	$(COMPILE) $< -o $@
$(OBJ_PATH_DST)/%.o: ./tls/%.c
	$(COMPILE) $< -o $@
$(OBJ_PATH_DST)/%.o: ./util/%.c
	$(COMPILE) $< -o $@
$(OBJ_PATH_DST)/%.o: ./attr/%.c
	$(COMPILE) $< -o $@

clean:
	rm -f $(LIB_PATH_DST)/$(LIB_NAME)
	rm -f $(TLS_OBJ)
	@(cd tlsmgr; make clean)
