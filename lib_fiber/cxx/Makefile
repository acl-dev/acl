SHELL = /bin/sh
#CC      = g++
CC	= ${ENV_CPP}
AR      = ${ENV_AR}
ARFL    = rv
#ARFL    = cru
RANLIB  = ${ENV_RANLIB}

FLAGS   = ${ENV_FLAGS}

CFLAGS = -c -g -W \
-O3 \
-Wall \
-Wpointer-arith \
-Wshadow \
-D_REENTRANT \
-D_POSIX_PTHREAD_SEMANTICS \
-Wno-long-long \
-Wformat \
-Winvalid-pch \
-DACL_PREPARE_COMPILE
#-DUSE_DYNAMIC
# -Wcast-align
#-Wcast-qual
#just for freebsd's iconv.h
CFLAGS += -I/usr/local/include

shared_ldflags = -l_acl_cpp -l_protocol -l_acl -lz -lpthread

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

# For FreeBSD
ifeq ($(findstring FreeBSD, $(UNIXNAME)), FreeBSD)
	CFLAGS += -DFREEBSD -fPIC -Werror
	UNIXTYPE = FREEBSD
	shared_ldflags = -l_acl_cpp -l_protocol -l_acl -L/usr/local/lib -liconv -lz -lpthread
endif

# For Darwin
ifeq ($(findstring Darwin, $(UNIXNAME)), Darwin)
	CFLAGS += -DMACOSX -Wno-invalid-source-encoding \
		  -Wno-extended-offsetof -fPIC -Werror \
		  -Wno-unused-private-field
	UNIXTYPE = MACOSX
	shared_ldflags = -l_acl_cpp -l_protocol -l_acl -L/usr/lib -liconv -lz -lpthread
endif

# For Linux
ifeq ($(findstring Linux, $(UNIXNAME)), Linux)
	CFLAGS += -DLINUX2 -fPIC -Werror
	UNIXTYPE = LINUX
	shared_ldflags = -l_acl_cpp -l_protocol -l_acl -lrt -lpthread -lz -ldl
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

STATIC_LIBNAME = lib_fiber_cpp.a
SHARED_LIBNAME = lib_fiber_cpp.so
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

clean:
	rm -f $(LIB_PATH_DST)/lib_fiber_cpp.a
	rm -f $(LIB_PATH_DST)/lib_fiber_cpp.so
	rm -f $(OBJS_DST)
	rm -f $(OBJS_DEF)
	rm -f $(PCH)

rebuild rb: clean static
