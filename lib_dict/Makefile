SHELL = /bin/sh
CC      = gcc
#CC      = g++
CC	= ${MY_ENV_CC}
AR      = ar
ARFL    = rv
#ARFL    = cru
RANLIB  = ranlib

#CFLAGS = -c -g -W -Wall -Wcast-qual -Wcast-align \
#-Waggregate-return -Wmissing-prototypes \
#-Wpointer-arith -Werror -Wshadow -O2 \
#-D_REENTRANT -D_POSIX_PTHREAD_SEMANTICS -D_USE_FAST_MACRO \
#-Wno-long-long -DHAS_DB

CFLAGS = -c -g -W -Wall -Wcast-align \
-Waggregate-return \
-Wmissing-prototypes \
-Wpointer-arith \
-Werror \
-Wshadow \
-O2 \
-D_REENTRANT \
-D_POSIX_PTHREAD_SEMANTICS \
-D_USE_FAST_MACRO \
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
LIB_PATH_DST = $(OUT_PATH)

BASE_PATH  = .
INC_PATH_SRC = $(BASE_PATH)/include
SRC_PATH_SRC = $(BASE_PATH)/src

INC_COMPILE  = -I$(INC_PATH_SRC) -I./bdb/include -I./cdb/include -I./tc/include
#CFLAGS += $(INC_COMPILE) -DHAS_BDB
#CFLAGS += $(INC_COMPILE)
#CFLAGS += $(INC_COMPILE) -DHAS_DB -DHAS_TC -DHAS_CDB
CFLAGS += $(INC_COMPILE) -DHAS_CDB

#Project's objs
DICT_SRC   = $(wildcard $(SRC_PATH_SRC)/*.c)
SOURCES_SRC  = $(DICT_SRC)

DICT_OBJ = $(patsubst %.c, $(OBJ_PATH_DST)/%.o, $(notdir $(DICT_SRC)))
OBJS_DST = $(DICT_OBJ)

###########################################################
ACL_PATH = ../lib_acl
ACL_INC = $(ACL_PATH)/include

CFLAGS += -I $(ACL_INC)

###########################################################

LIB_NAME  = lib_dict.a

.PHONY = clean
COMPILE = $(CC) $(CFLAGS)

all: $(LIB_NAME)

$(shell mkdir -p $(OBJ_PATH_DST))

$(LIB_NAME): $(OBJS_DST)
	$(AR) $(ARFL) $(LIB_PATH_DST)/$(LIB_NAME) $(OBJS_DST)
	$(RANLIB) $(LIB_PATH_DST)/$(LIB_NAME)

# dict
$(OBJ_PATH_DST)/%.o: $(SRC_PATH_SRC)/%.c
	$(COMPILE) $< -o $@

# icmp
$(OBJ_PATH_DST)/%.o: $(SRC_PATH_SRC)/icmp/%.c
	$(COMPILE) $< -o $@

clean:
	rm -f $(LIB_PATH_DST)/$(LIB_NAME)
	rm -f $(OBJS_DST)

