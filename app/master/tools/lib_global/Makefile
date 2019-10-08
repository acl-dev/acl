SHELL = /bin/sh
CC      = clang++
CC	= g++
CC	= ${ENV_CC}
AR      = ar
ARFL    = rv
#ARFL    = cru
RANLIB  = ranlib

CFLAGS = -c -g -W \
-Wall \
-Wpointer-arith \
-Wshadow \
-D_REENTRANT \
-D_POSIX_PTHREAD_SEMANTICS \
-Wno-long-long \
-Wformat \
-DHAS_POLARSSL \
-DHAS_MYSQL_DLL \
-O3 \
#-march=i686
#-DUSE_DYNAMIC
# -Wcast-align
#-Wcast-qual
#just for freebsd's iconv.h
CFLAGS += -I/usr/local/include -I../contrib/zmq/include
CFLAGS += -Wno-invalid-offsetof -g


###########################################################
#Check system:
#       Linux, SunOS, Solaris, BSD variants, AIX, HP-UX
SYSLIB =
CHECKSYSRES = @echo "Unknow system type!";exit 1
UNIXNAME = $(shell uname -sm)
UNIXTYPE = LINUX
RPATH =

ifeq ($(CC),)
        CC = g++
endif

ifeq ($(findstring clang++, $(CC)), clang++)
	CFLAGS += -Wno-invalid-source-encoding \
		  -Wno-invalid-offsetof
endif

# For FreeBSD
ifeq ($(findstring FreeBSD, $(UNIXNAME)), FreeBSD)
	CFLAGS += -DFREEBSD -fPIC -Werror
	UNIXTYPE = FREEBSD
endif

# For Darwin
ifeq ($(findstring Darwin, $(UNIXNAME)), Darwin)
	CFLAGS += -DMACOSX -Wno-invalid-source-encoding \
		  -Wno-invalid-offsetof -fPIC -Werror \
		  -Wno-unused-private-field
	UNIXTYPE = MACOSX
endif

# For Linux
ifeq ($(findstring Linux, $(UNIXNAME)), Linux)
	ifeq ($(findstring i686, $(UNIXNAME)), i686)
		RPATH = linux32
	endif

	ifeq ($(findstring x86_64, $(UNIXNAME)), x86_64)
		RPATH = linux64
	endif

	CFLAGS += -DLINUX2 -fPIC -Werror
	UNIXTYPE = LINUX
endif

# For MINGW
ifeq ($(findstring MINGW, $(UNIXNAME)), MINGW)
	CFLAGS += -DLINUX2 -DMINGW
	UNIXTYPE = LINUX
endif

# For SunOS
ifeq ($(findstring SunOS, $(UNIXNAME)), SunOS)
	ifeq ($(findstring 86, $(UNIXNAME)), 86)
		SYSLIB = -lsocket -lnsl -lrt
	endif
	ifeq ($(findstring sun4u, $(UNIXNAME)), sun4u)
		SYSLIB = -lsocket -lnsl -lrt
	endif
	CFLAGS += -DSUNOS5 -fPIC -Werror
	UNIXTYPE = SUNOS5
endif

# For HP-UX
ifeq ($(findstring HP-UX, $(UNIXNAME)), HP-UX)
	CFLAGS += -DHP_UX -DHPUX11 -fPIC -Werror
	UNIXTYPE = HPUX
endif

#Find system type.
ifneq ($(SYSPATH),)
	CHECKSYSRES = @echo "System is $(shell uname -sm)"
endif
###########################################################

OBJ_PATH_DST = ./debug
DEF_PATH_DST = ./debug
LIB_PATH_DST = .

ACL_PATH = ../../../..
CFLAGS += -I. -I./include -I./src -I${ACL_PATH}/lib_acl_cpp/include \
	  -I${ACL_PATH}/lib_acl/include

#Project's objs
OBJS_DST = $(patsubst %.cpp, $(OBJ_PATH_DST)/%.o, $(notdir $(wildcard ./*/*.cpp)))

###########################################################
.PHONY = static clean

STATIC_LIBNAME = libglobal.a

lib: static

$(shell mkdir -p $(DEF_PATH_DST))
COMPILE = $(CC) $(CFLAGS)
COMPILE_OBJ = @(echo 'building $<'; $(COMPILE) $< -o $@)

###########################################################

static: $(OBJS_DST)
	@echo 'creating $(LIB_PATH_DST)/$(STATIC_LIBNAME)'
	@$(AR) $(ARFL) $(LIB_PATH_DST)/$(STATIC_LIBNAME) $(OBJS_DST)
	@$(RANLIB) $(LIB_PATH_DST)/$(STATIC_LIBNAME)
	@echo 'create $(LIB_PATH_DST)/$(STATIC_LIBNAME) ok!'

# src
$(OBJ_PATH_DST)/%.o: ./src/%.cpp
	$(COMPILE_OBJ)
$(OBJ_PATH_DST)/%.o: ./json/%.cpp
	$(COMPILE_OBJ)
clean cl:
	rm -f $(LIB_PATH_DST)/$(STATIC_LIBNAME)
	rm -f $(OBJS_DST)

rebuild rb: clean lib
