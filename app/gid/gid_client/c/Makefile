SHELL = /bin/sh
CC      = gcc
AR      = ar
ARFL    = rv
#ARFL    = cru
RANLIB  = ranlib

CFLAGS = -c -g -W -Wall -Wcast-qual -Wcast-align \
-Wpointer-arith -Werror -Wshadow -O3 \
-D_REENTRANT -D_POSIX_PTHREAD_SEMANTICS \
-Wno-long-long \
-Wno-unused-parameter \
-fPIC -Wno-unknown-pragmas \
-DUSE_SERVICE_CLIENT
###########################################################
#Check system:
#       Linux, SunOS, Solaris, BSD variants, AIX, HP-UX
SYSLIB =
CHECKSYSRES = @echo "Unknow system type!";exit 1
UNIXNAME = $(shell uname -sm)

ifeq ($(CC),)
        CC = g++
endif

# For FreeBSD
ifeq ($(findstring FreeBSD, $(UNIXNAME)), FreeBSD)
	ifeq ($(findstring gcc, $(CC)), gcc)
		CFLAGS += -Wstrict-prototypes
	endif
	CFLAGS += -DFREEBSD -D_REENTRANT
endif

# For Linux
ifeq ($(findstring Linux, $(UNIXNAME)), Linux)
	ifeq ($CC, "gcc")
		CFLAGS += -Wstrict-prototypes
	endif
	CFLAGS += -DLINUX2 -D_REENTRANT
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
	CFLAGS += -DSUNOS5 -D_REENTRANT
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

OBJ_PATH_DST = ./debug
LIB_PATH_DST = ./lib

INC_COMPILE = -I./include \
	      -I../../../../lib_protocol/include \
	      -I../../../../lib_acl/include
CFLAGS += $(INC_COMPILE)

#Project's objs
OBJS_DST = $(patsubst %.c, $(OBJ_PATH_DST)/%.o, $(notdir $(wildcard src/*.c)))
###########################################################

LIB_NAME  = lib_gid.a

.PHONY = clean
COMPILE = $(CC) $(CFLAGS)

$(LIB_NAME): $(OBJS_DST)
	$(AR) $(ARFL) $(LIB_PATH_DST)/$(LIB_NAME) $(OBJS_DST)
	$(RANLIB) $(LIB_PATH_DST)/$(LIB_NAME)

# src
$(OBJ_PATH_DST)/%.o: src/%.c
	$(COMPILE) $< -o $@
clean:
	rm -f $(LIB_PATH_DST)/$(LIB_NAME)
	rm -f $(OBJS_DST)
