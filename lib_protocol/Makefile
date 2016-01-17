SHELL = /bin/sh
CC      = gcc
#CC      = g++
CC	= ${ENV_CC}
AR      = ${ENV_AR}
ARFL    = rv
#ARFL    = cru
RANLIB  = ${ENV_RANLIB}

CFLAGS = -c -g -W \
-O3 \
-Wall \
-Wcast-qual \
-Waggregate-return \
-Wmissing-prototypes \
-Wpointer-arith \
-Wshadow \
-D_REENTRANT \
-D_POSIX_PTHREAD_SEMANTICS \
-D_USE_FAST_MACRO \
-Wno-long-long

#-pedantic
# -Wcast-align
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
UNIXTYPE = LINUX

ifeq ($(CC),)
        CC = gcc
endif

ifeq ($(AR),)
	AR = ar
endif

ifeq ($(RANLIB),)
	RANLIB = ranlib
endif

ifeq ($(findstring gcc, $(CC)), gcc)
	CFLAGS += -Wstrict-prototypes
endif

ifeq ($(findstring clang, $(CC)), clang)
	CFLAGS += -Wstrict-prototypes \
		  -Wno-invalid-source-encoding \
		  -Wno-extended-offsetof
endif

ifeq ($(findstring clang++, $(CC)), clang++)
	CFLAGS += -Wno-invalid-source-encoding \
		  -Wno-extended-offsetof
endif

# For FreeBSD
ifeq ($(findstring FreeBSD, $(UNIXNAME)), FreeBSD)
	CFLAGS += -DFREEBSD -fPIC -Werror
	UNIXTYPE = FREEBSD
endif

# For Darwin
ifeq ($(findstring Darwin, $(UNIXNAME)), Darwin)
	CFLAGS += -DMACOSX -Wno-invalid-source-encoding \
		  -Wno-extended-offsetof -fPIC -Werror
	UNIXTYPE = MACOSX
endif

# For Linux
ifeq ($(findstring Linux, $(UNIXNAME)), Linux)
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

STATIC_LIBNAME = lib_protocol.a
SHARED_LIBNAME = lib_protocol.so

###########################################################

.PHONY = static shared clean
COMPILE = $(CC) $(CFLAGS)

all: static shared

$(shell mkdir -p $(OBJ_PATH_DST))

static: $(OBJS_DST)
	@echo 'creating $(LIB_PATH_DST)/$(STATIC_LIBNAME)'
	$(AR) $(ARFL) $(LIB_PATH_DST)/$(STATIC_LIBNAME) $(OBJS_DST)
	$(RANLIB) $(LIB_PATH_DST)/$(STATIC_LIBNAME)
	@echo 'build $(LIB_PATH_DST)/$(STATIC_LIBNAME) ok!'

shared_ldflags = -l_acl -lrt -lpthread
shared: $(OBJS_DST)
	@echo ''
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
			-R$(rpath) -L$(rpath) -l_acl -lpthread; \
		echo 'build $(rpath)/$(SHARED_LIBNAME) ok!'; \
	elif test -n "$(rpath)" && test "$(UNIXTYPE)" = "FREEBSD"; then \
		echo "building for FreeBSD"; \
		$(CC) -shared -o $(rpath)/$(SHARED_LIBNAME) $(OBJS_DST) \
			-R$(rpath) -L$(rpath) -l_acl -lpthread; \
		echo 'build $(rpath)/$(SHARED_LIBNAME) ok!'; \
	else \
		echo 'skip build $(SHARED_LIBNAME); usage: make shared rpath=xxx'; \
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
