SHELL = /bin/sh
#CC      = g++
CC	= ${ENV_CPP}
AR      = ${ENV_AR}
ARFL    = rv
#ARFL    = cru
RANLIB  = ${ENV_RANLIB}

FLAGS   = ${ENV_FLAGS}

CFLAGS = -c -g -W \
-O \
-fPIC \
-Wall \
-Werror \
-Wpointer-arith \
-Wshadow \
-D_REENTRANT \
-D_POSIX_PTHREAD_SEMANTICS \
-Wno-long-long \
-Wformat \
-Winvalid-pch \
-DACL_PREPARE_COMPILE \
#-DACL_USE_CPP11 -std=c++11
#-DUSE_DYNAMIC
# -Wcast-align
#-Wcast-qual
#just for freebsd's iconv.h
CFLAGS += -I/usr/local/include

shared_ldflags = -lacl_cpp -lprotocol -lacl -lz -lpthread

###########################################################
#Check system:
#       Linux, SunOS, Solaris, BSD variants, AIX, HP-UX
SYSLIB =
CHECKSYSRES = @echo "Unknow system type!";exit 1
UNIXNAME = $(shell uname -sm)
UNIXTYPE = LINUX

ifeq ($(CC),)
        CC = g++
endif

ifeq ($(AR),)
	AR = ar
endif

ifeq ($(RANLIB),)
	RANLIB = ranlib
endif

ifeq ($(findstring clang++, $(CC)), clang++)
	CFLAGS += -Wno-invalid-source-encoding \
		  -Wno-extended-offsetof
endif

ifeq ($(findstring g++, $(CC)), g++)
	GCC_VERSION:=$(shell echo `gcc --version|grep ^gcc|cut -d' ' -f3`)
	GCC_MAJOR:=$(shell echo "$(GCC_VERSION)" | cut -d'.' -f1)
	GCC_MINOR:=$(shell echo "$(GCC_VERSION)" | cut -d'.' -f2)
	GCC_VER:=$(shell [ $(GCC_MAJOR) -gt 4 -o \( $(GCC_MAJOR) -eq 4 -a $(GCC_MINOR) -ge 7 \) ] && echo true)
	ifeq ($(GCC_VER), true)
		CFLAGS += -std=c++11 -DACL_USE_CPP11
	endif
endif

# For FreeBSD
ifeq ($(findstring FreeBSD, $(UNIXNAME)), FreeBSD)
	UNIXTYPE = FREEBSD
	shared_ldflags = -lacl_cpp -l_protocol -lacl -L/usr/local/lib -liconv -lz -lpthread
endif

# For Darwin
ifeq ($(findstring Darwin, $(UNIXNAME)), Darwin)
	CFLAGS += -DMACOSX -Wno-invalid-source-encoding \
		  -Wno-extended-offsetof \
		  -Wno-unused-private-field
	UNIXTYPE = MACOSX
	shared_ldflags = -lacl_cpp -l_protocol -lacl -L/usr/lib -liconv -lz -lpthread
endif

# For Linux
ifeq ($(findstring Linux, $(UNIXNAME)), Linux)
	UNIXTYPE = LINUX
	shared_ldflags = -lacl_cpp -l_protocol -lacl -lrt -lpthread -lz -ldl
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
	CFLAGS += -DSUNOS5
	UNIXTYPE = SUNOS5
endif

# For HP-UX
ifeq ($(findstring HP-UX, $(UNIXNAME)), HP-UX)
	CFLAGS += -DHP_UX -DHPUX11
	UNIXTYPE = HPUX
endif

#Find system type.
ifneq ($(SYSPATH),)
	CHECKSYSRES = @echo "System is $(shell uname -sm)"
endif
###########################################################

OBJ_PATH_DST = ./debug
LIB_PATH_DST = ../lib
DEF_PATH_DST = ./debug

CFLAGS += -I./src -I../c/include  -I./include -I../../lib_acl/include -I../../lib_acl_cpp/include

#Project's objs

OBJS_DST = $(patsubst %.cpp, $(OBJ_PATH_DST)/%.o, $(notdir $(wildcard src/*.cpp)))
OBJS_DEF = $(patsubst %.cpp, $(DEF_PATH_DST)/%.inc, $(notdir $(wildcard src/*.cpp)))

###########################################################
.PHONY = static shared clean
VERSION = 0.9
DATE_NOW = 20`date +%y`.`date +%m`.`date +%d`

STATIC_LIBNAME = libfiber_cpp.a
SHARED_LIBNAME = libfiber_cpp.so
STATIC_OLD_LIBNAME = lib_fiber_cpp.a
SHARED_OLD_LIBNAME = lib_fiber_cpp.so
PCH = ./acl_stdafx.hpp.gch

lib: static
all: static shared sample

$(shell mkdir -p $(DEF_PATH_DST))
ifneq ($(MAKECMDGOALS),clean)
-include $(OBJS_DEF)
endif

COMPILE = $(CC) $(CFLAGS)
COMPILE_OBJ = @(echo 'building $<'; $(COMPILE) $< -o $@)
CREATE_DEF = @(echo 'creating $@'; rm -f $@; \
	$(COMPILE) -MM $< > $@.$$$$; \
	sed 's,.*.o\( \)*:,$(patsubst %.inc,%.o,$@) $@ :,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$)

###########################################################

sample: static
	@(cd samples; make)

static: depends $(OBJS_DST)
	@echo 'creating $(LIB_PATH_DST)/$(STATIC_LIBNAME)'
	@$(AR) $(ARFL) $(LIB_PATH_DST)/$(STATIC_LIBNAME) $(OBJS_DST)
	@$(RANLIB) $(LIB_PATH_DST)/$(STATIC_LIBNAME)
	@(cd ${LIB_PATH_DST}; rm -f ${STATIC_OLD_LIBNAME}; \
		ln -s ${STATIC_LIBNAME} ${STATIC_OLD_LIBNAME})
	@echo 'create $(LIB_PATH_DST)/$(STATIC_LIBNAME) ok!'

shared: depends pch $(OBJS_DST)
	@echo 'creating $(SHARED_LIBNAME)'
	@if test -n "$(rpath)" && test "$(UNIXTYPE)" = "LINUX"; then \
		echo "building for linux"; \
		$(CC) -shared -o $(rpath)/$(SHARED_LIBNAME) $(OBJS_DST) \
			-L$(rpath) $(shared_ldflags) -Wl,-rpath,$(rpath); \
		echo 'build $(rpath)/$(SHARED_LIBNAME) ok!'; \
	elif test -n "$(rpath)" && test "$(UNIXTYPE)" = "SUNOS5"; then \
		echo "building for sunos5"; \
		$(CC) -shared -o $(rpath)/$(SHARED_LIBNAME) $(OBJS_DST) \
			-R$(rpath) -L$(rpath) $(shared_ldflags); \
		echo 'build $(rpath)/$(SHARED_LIBNAME) ok!'; \
	elif test -n "$(rpath)" && test "$(UNIXTYPE)" = "MACOSX"; then \
		echo "building for Darwin"; \
		$(CC) -shared -o $(rpath)/$(SHARED_LIBNAME) $(OBJS_DST) \
			-R$(rpath) -L$(rpath) $(shared_ldflags); \
		echo 'build $(rpath)/$(SHARED_LIBNAME) ok!'; \
	elif test -n "$(rpath)" && test "$(UNIXTYPE)" = "FREEBSD"; then \
		echo "building for Darwin"; \
		$(CC) -shared -o $(rpath)/$(SHARED_LIBNAME) $(OBJS_DST) \
			-R$(rpath) -L$(rpath) $(shared_ldflags); \
		echo 'build $(rpath)/$(SHARED_LIBNAME) ok!'; \
	else \
		echo 'skip build $(SHARED_LIBNAME); usage: make shared rpath=xxx'; \
	fi

depends: $(OBJS_DEF)
pch: ./acl_stdafx.hpp
	rm -f $(PCH)
	@if test "$(UNIXTYPE)" = "LINUX" || test "$(UNIXTYPE)" = "FREEBSD" \
		|| test "$(UNIXTYPE)" = "MACOSX"; then \
		$(CC) -o $(PCH) -x c++-header $(CFLAGS) src/acl_stdafx.hpp; \
	fi

# src
$(OBJ_PATH_DST)/%.o: ./src/%.cpp
	$(COMPILE_OBJ)

$(DEF_PATH_DST)/%.inc: ./src/%.cpp
	$(CREATE_DEF)

clean cl:
	rm -f $(LIB_PATH_DST)/${STATIC_LIBNAME}
	rm -f $(LIB_PATH_DST)/${SHARED_LIBNAME}
	rm -f $(LIB_PATH_DST)/${STATIC_OLD_LIBNAME}
	rm -f $(LIB_PATH_DST)/${SHARED_OLD_LIBNAME}
	rm -f $(OBJS_DST)
	rm -f $(OBJS_DEF)
	rm -f $(PCH)

rebuild rb: clean static
