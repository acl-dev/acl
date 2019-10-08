SHELL = /bin/sh
#CC      = gcc
#CC      = g++
CC	= ${ENV_CC}
AR      = ${ENV_AR}
ARFL    = rv
#ARFL    = cru
RANLIB  = ${ENV_RANLIB}

FLAGS   = ${ENV_FLAGS}

# default lib name
LIB_NAME  = libacl.a

#-fno-strict-aliasing \
#CFLAGS = -c -g -W -Wall -Wcast-qual -Wcast-align

CFLAGS = -c -g -W \
-fPIC \
-Wall \
-Werror \
-Wshadow \
-Wpointer-arith \
-Waggregate-return \
-Wmissing-prototypes \
-D_REENTRANT \
-D_USE_FAST_MACRO \
-DACL_WRITEABLE_CHECK \
-Wno-long-long \
-Wuninitialized \
-D_POSIX_PTHREAD_SEMANTICS \
-Winvalid-pch \
-DACL_PREPARE_COMPILE \
-DUSE_REUSEPORT \
#-DACL_CLIENT_ONLY \
#-Wno-implicit-fallthrough \
#-fstack-protector-all \
#-DDEBUG_MEM
#-DUSE_EPOLL \
#-Wno-tautological-compare \
#-Wno-invalid-source-encoding \
#-Wno-extended-offsetof
#-Wcast-align
#-Winvalid-pch -DACL_PREPARE_COMPILE

###########################################################
#Check system:
#       Linux, SunOS, Solaris, BSD variants, AIX, HP-UX
SYSLIB = -lpthread
CHECKSYSRES = @echo "Unknow system type!";exit 1
#UNIXNAME = $(shell uname -sm)
UNIXNAME = $(shell uname -s)
UNIXTYPE = LINUX
#OSTYPE = $(shell uname -p)
OSTYPE = $(shell uname -m)
RPATH =

ifeq ($(CC),)
        CC = gcc
endif

ifeq ($(AR),)
	AR = ar
endif

ifeq ($(RANLIB),)
	RANLIB = ranlib
endif

#using "gcc -dumpversion" can get the gcc's version more easily
ifeq ($(findstring gcc, $(CC)), gcc)
	CFLAGS += -Wstrict-prototypes
	GCC_VERSION=$(shell gcc --version | grep ^gcc | sed 's/^.* //g')
	GCC_MAJOR:=$(shell echo "$(GCC_VERSION)" | cut -d'.' -f1)
	GCC_MINOR:=$(shell echo "$(GCC_VERSION)" | cut -d'.' -f2)
	GCC_SUB:=$(shell echo "$(GCC_VERSION)" | cut -d'.' -f3)
	GCC_VER:=$(shell [ $(GCC_MAJOR) -gt 4 -o \( $(GCC_MAJOR) -eq 4 -a $(GCC_MINOR) -gt 4 \) ] && echo true)
	ifeq ($(GCC_VER), true)
		CFLAGS += -Wno-implicit-fallthrough
	endif
endif

ifeq ($(findstring clang, $(CC)), clang)
	CFLAGS += -Wstrict-prototypes \
		  -Wno-invalid-source-encoding \
		  -Wno-invalid-offsetof
endif

ifeq ($(findstring clang++, $(CC)), clang)
	CFLAGS += -Wno-invalid-source-encoding \
		  -Wno-invalid-offsetof
endif

# For FreeBSD
ifeq ($(findstring FreeBSD, $(UNIXNAME)), FreeBSD)
	UNIXTYPE = FREEBSD
	RPATH = freebsd
	CFLAGS += -O3
endif

# For Darwin
ifeq ($(findstring Darwin, $(UNIXNAME)), Darwin)
	CFLAGS += -Wno-invalid-source-encoding \
		  -Wno-invalid-offsetof \
		  -ffunction-sections \
		  -flto \
		  -Os
	UNIXTYPE = MACOSX
	RPATH = macos
endif

# For Linux
ifeq ($(findstring Linux, $(UNIXNAME)), Linux)
	UNIXTYPE = LINUX
	ifeq ($(findstring i686, $(OSTYPE)), i686)
#		CFLAGS += -march=i686
		RPATH = linux32
	endif
	ifeq ($(findstring x86_64, $(OSTYPE)), x86_64)
#		CFLAGS += -m64
		RPATH = linux64
	endif
	CFLAGS += -O3
endif

ifeq ($(findstring HAS_TRACE, $(FLAGS)), HAS_TRACE)
	CFLAGS += -DHAS_TRACE
endif

EXECINFO_H = /usr/include/execinfo.h
EXECINFO_H_EXIST = $(shell if [ -f $(EXECINFO_H) ]; \
		   then echo "exist"; else echo "notexist"; fi;)

ifeq ($(findstring exist, $(EXECINFO_H_EXIST)), exist)
	CFLAGS += -DHAS_EXECINFO_HEAD
endif

# For MINGW
ifeq ($(findstring MINGW, $(UNIXNAME)), MINGW)
	CFLAGS += -DLINUX2 -DMINGW
	CFLAGS += -O3
	UNIXTYPE = LINUX
	SYSLIB = -lpthread-2
	RPATH = mingw
endif

# For MSYS
ifeq ($(findstring MSYS, $(UNIXNAME)), MSYS)
	CFLAGS += -DLINUX2 -DMINGW
	CFLAGS += -O3
	UNIXTYPE = LINUX
	SYSLIB = -lpthread-2
	RPATH = mingw
endif

# For SunOS
ifeq ($(findstring SunOS, $(UNIXNAME)), SunOS)
	ifeq ($(findstring 86, $(OSTYPE)), 86)
		SYSLIB += -lsocket -lnsl -lrt
	endif
	ifeq ($(findstring sun4u, $(OSTYPE)), sun4u)
		SYSLIB += -lsocket -lnsl -lrt
	endif
	CFLAGS += -DSUNOS5 -fPIC -Werror
	CFLAGS += -O3
	UNIXTYPE = SUNOS5
	RPATH = sunos5_x86
endif

# For HP-UX
ifeq ($(findstring HP-UX, $(UNIXNAME)), HP-UX)
	CFLAGS += -DHP_UX -DHPUX11 -fPIC -Werror
	CFLAGS += -O3
	UNIXTYPE = HPUX
	RPATH = hpux
endif

#CCARGS =
#ifeq ($(findstring HAS_MYSQL, $(CCARGS)), HAS_MYSQL)
#        CFLAGS += -DHAS_MYSQL
#	LIB_NAME = lib_acl.a
#endif

#CFLAGS += -DHAS_MYSQL

#Find system type.
ifneq ($(SYSPATH),)
	CHECKSYSRES = @echo "System is $(shell uname -sm)"
endif
###########################################################

OUT_PATH = .
OBJ_PATH_DST = $(OUT_PATH)/debug
LIB_PATH_DST = $(OUT_PATH)/lib
DEF_PATH_DST = $(OUT_PATH)/debug

BASE_PATH  = .
INC_PATH_SRC = $(BASE_PATH)/include
SRC_PATH_SRC = $(BASE_PATH)/src

CFLAGS += -I. -I$(INC_PATH_SRC) -I../include/mysql

###########################################################

#Project's objs
INIT_SRC     = $(wildcard $(SRC_PATH_SRC)/init/*.c)
PRIV_SRC     = $(wildcard $(SRC_PATH_SRC)/private/*.c)
STDLIB_SRC   = $(wildcard $(SRC_PATH_SRC)/stdlib/*.c) \
		$(wildcard $(SRC_PATH_SRC)/stdlib/common/*.c) \
		$(wildcard $(SRC_PATH_SRC)/stdlib/iostuff/*.c) \
		$(wildcard $(SRC_PATH_SRC)/stdlib/configure/*.c) \
		$(wildcard $(SRC_PATH_SRC)/stdlib/filedir/*.c) \
		$(wildcard $(SRC_PATH_SRC)/stdlib/string/*.c) \
		$(wildcard $(SRC_PATH_SRC)/stdlib/memory/*.c) \
		$(wildcard $(SRC_PATH_SRC)/stdlib/debug/*.c) \
		$(wildcard $(SRC_PATH_SRC)/stdlib/sys/*.c) \
		$(wildcard $(SRC_PATH_SRC)/stdlib/sys/unix/*.c)
NET_SRC      = $(wildcard $(SRC_PATH_SRC)/net/*.c) \
		$(wildcard $(SRC_PATH_SRC)/net/listen/*.c) \
		$(wildcard $(SRC_PATH_SRC)/net/connect/*.c) \
		$(wildcard $(SRC_PATH_SRC)/net/dns/*.c)
ENGINE_SRC   = $(wildcard $(SRC_PATH_SRC)/event/*.c)
IOCTL_SRC    = $(wildcard $(SRC_PATH_SRC)/ioctl/*.c)
AIO_SRC      = $(wildcard $(SRC_PATH_SRC)/aio/*.c)
MSG_SRC      = $(wildcard $(SRC_PATH_SRC)/msg/*.c)
THREAD_SRC   = $(wildcard $(SRC_PATH_SRC)/thread/*.c)
SVR_SRC      = $(wildcard $(SRC_PATH_SRC)/svr/*.c)
DB_SRC       = $(wildcard $(SRC_PATH_SRC)/db/*.c) \
		$(wildcard $(SRC_PATH_SRC)/db/mysql/*.c) \
		$(wildcard $(SRC_PATH_SRC)/db/null/*.c) \
		$(wildcard $(SRC_PATH_SRC)/db/memdb/*.c) \
		$(wildcard $(SRC_PATH_SRC)/db/zdb/*.c)
CODE_SRC     = $(wildcard $(SRC_PATH_SRC)/code/*.c) 
MASTER_SRC   = $(wildcard $(SRC_PATH_SRC)/master/*.c) \
		$(wildcard $(SRC_PATH_SRC)/master/template/*.c)
PROCTL_SRC   = $(wildcard $(SRC_PATH_SRC)/proctl/*.c)
XML_SRC      = $(wildcard $(SRC_PATH_SRC)/xml/*.c)
JSON_SRC      = $(wildcard $(SRC_PATH_SRC)/json/*.c)
UTEST_SRC    = $(wildcard $(SRC_PATH_SRC)/unit_test/*.c)
#EXP_SRC      = $(wildcard $(SRC_PATH_SRC)/experiment/avl/*.c)
#$(EXP_SRC)

###########################################################
INIT_OBJ     = $(patsubst %.c, $(OBJ_PATH_DST)/%.o, $(notdir $(INIT_SRC)))
PRIV_OBJ     = $(patsubst %.c, $(OBJ_PATH_DST)/%.o, $(notdir $(PRIV_SRC)))
STDLIB_OBJ   = $(patsubst %.c, $(OBJ_PATH_DST)/%.o, $(notdir $(STDLIB_SRC)))
NET_OBJ      = $(patsubst %.c, $(OBJ_PATH_DST)/%.o, $(notdir $(NET_SRC)))
ENGINE_OBJ   = $(patsubst %.c, $(OBJ_PATH_DST)/%.o, $(notdir $(ENGINE_SRC)))
IOCTL_OBJ    = $(patsubst %.c, $(OBJ_PATH_DST)/%.o, $(notdir $(IOCTL_SRC)))
AIO_OBJ      = $(patsubst %.c, $(OBJ_PATH_DST)/%.o, $(notdir $(AIO_SRC)))
MSG_OBJ      = $(patsubst %.c, $(OBJ_PATH_DST)/%.o, $(notdir $(MSG_SRC)))
THREAD_OBJ   = $(patsubst %.c, $(OBJ_PATH_DST)/%.o, $(notdir $(THREAD_SRC)))
SVR_OBJ      = $(patsubst %.c, $(OBJ_PATH_DST)/%.o, $(notdir $(SVR_SRC)))
DB_OBJ       = $(patsubst %.c, $(OBJ_PATH_DST)/%.o, $(notdir $(DB_SRC)))
CODE_OBJ     = $(patsubst %.c, $(OBJ_PATH_DST)/%.o, $(notdir $(CODE_SRC)))
MASTER_OBJ   = $(patsubst %.c, $(OBJ_PATH_DST)/%.o, $(notdir $(MASTER_SRC)))
PROCTL_OBJ   = $(patsubst %.c, $(OBJ_PATH_DST)/%.o, $(notdir $(PROCTL_SRC)))
XML_OBJ      = $(patsubst %.c, $(OBJ_PATH_DST)/%.o, $(notdir $(XML_SRC)))
JSON_OBJ     = $(patsubst %.c, $(OBJ_PATH_DST)/%.o, $(notdir $(JSON_SRC)))
UTEST_OBJ    = $(patsubst %.c, $(OBJ_PATH_DST)/%.o, $(notdir $(UTEST_SRC)))
#EXP_OBJ      = $(patsubst %.c, $(OBJ_PATH_DST)/%.o, $(notdir $(EXP_SRC)))

OBJS_DST     = $(INIT_OBJ) $(PRIV_OBJ) $(STDLIB_OBJ) \
	        $(NET_OBJ) $(ENGINE_OBJ) $(IOCTL_OBJ) \
		$(AIO_OBJ) $(MSG_OBJ) $(THREAD_OBJ) \
		$(SVR_OBJ) $(DB_OBJ) $(CODE_OBJ) \
		$(MASTER_OBJ) $(PROCTL_OBJ) $(XML_OBJ) \
		$(JSON_OBJ) $(UTEST_OBJ)
#$(EXP_OBJ)

###########################################################
INIT_DEF     = $(patsubst %.c, $(DEF_PATH_DST)/%.inc, $(notdir $(INIT_SRC)))
PRIV_DEF     = $(patsubst %.c, $(DEF_PATH_DST)/%.inc, $(notdir $(PRIV_SRC)))
STDLIB_DEF   = $(patsubst %.c, $(DEF_PATH_DST)/%.inc, $(notdir $(STDLIB_SRC)))
NET_DEF      = $(patsubst %.c, $(DEF_PATH_DST)/%.inc, $(notdir $(NET_SRC)))
ENGINE_DEF   = $(patsubst %.c, $(DEF_PATH_DST)/%.inc, $(notdir $(ENGINE_SRC)))
IOCTL_DEF    = $(patsubst %.c, $(DEF_PATH_DST)/%.inc, $(notdir $(IOCTL_SRC)))
AIO_DEF      = $(patsubst %.c, $(DEF_PATH_DST)/%.inc, $(notdir $(AIO_SRC)))
MSG_DEF      = $(patsubst %.c, $(DEF_PATH_DST)/%.inc, $(notdir $(MSG_SRC)))
THREAD_DEF   = $(patsubst %.c, $(DEF_PATH_DST)/%.inc, $(notdir $(THREAD_SRC)))
SVR_DEF      = $(patsubst %.c, $(DEF_PATH_DST)/%.inc, $(notdir $(SVR_SRC)))
DB_DEF       = $(patsubst %.c, $(DEF_PATH_DST)/%.inc, $(notdir $(DB_SRC)))
CODE_DEF     = $(patsubst %.c, $(DEF_PATH_DST)/%.inc, $(notdir $(CODE_SRC)))
MASTER_DEF   = $(patsubst %.c, $(DEF_PATH_DST)/%.inc, $(notdir $(MASTER_SRC)))
PROCTL_DEF   = $(patsubst %.c, $(DEF_PATH_DST)/%.inc, $(notdir $(PROCTL_SRC)))
XML_DEF      = $(patsubst %.c, $(DEF_PATH_DST)/%.inc, $(notdir $(XML_SRC)))
JSON_DEF     = $(patsubst %.c, $(DEF_PATH_DST)/%.inc, $(notdir $(JSON_SRC)))
UTEST_DEF    = $(patsubst %.c, $(DEF_PATH_DST)/%.inc, $(notdir $(UTEST_SRC)))
#EXP_DEF      = $(patsubst %.c, $(DEF_PATH_DST)/%.inc, $(notdir $(EXP_SRC)))

OBJS_DEF     = $(INIT_DEF) $(PRIV_DEF) $(STDLIB_DEF) \
	        $(NET_DEF) $(ENGINE_DEF) $(IOCTL_DEF) \
		$(AIO_DEF) $(MSG_DEF) $(THREAD_DEF) \
		$(SVR_DEF) $(DB_DEF) $(CODE_DEF) \
		$(MASTER_DEF) $(PROCTL_DEF) $(XML_DEF) \
		$(JSON_DEF) $(UTEST_DEF)

###########################################################
.PHONY = all static shared clean cl clean_lib rebuild rb

STATIC_OLD_LIBNAME = lib_acl.a
SHARED_OLD_LIBNAME = lib_acl.so
STATIC_LIBNAME = libacl.a
SHARED_LIBNAME = libacl.so

all: static shared

$(shell mkdir -p $(DEF_PATH_DST))
#ifneq ($(MAKECMDGOALS),clean)
#-include $(OBJS_DEF)
#endif

clean cl: clean_lib
rebuild rb: clean all

PCH = StdAfx.h.gch

###########################################################

COMPILE = $(CC) $(CFLAGS)
COMPILE_OBJ = @(echo 'building $<'; $(COMPILE) $< -o $@)
CREATE_DEF = @(echo 'creating $@'; rm -f $@; \
	$(COMPILE) -MM $< > $@.$$$$; \
	sed 's,.*.o\( \)*:,$(patsubst %.inc,%.o,$@) $@ :,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$)

#static: depends $(OBJS_DST)
static: $(OBJS_DST)
	@echo 'creating $(LIB_PATH_DST)/$(STATIC_LIBNAME)'
	@$(AR) $(ARFL) $(LIB_PATH_DST)/$(STATIC_LIBNAME) $(OBJS_DST)
	@$(RANLIB) $(LIB_PATH_DST)/$(STATIC_LIBNAME)
	@(cd $(LIB_PATH_DST); rm -f $(STATIC_OLD_LIBNAME); \
		ln -s $(STATIC_LIBNAME) $(STATIC_OLD_LIBNAME))
	@echo 'build $(LIB_PATH_DST)/$(STATIC_LIBNAME) ok!'

#shared_ldflags = -lmysqlclient -lrt -lpthread -ldl
shared_ldflags = -lrt -lpthread -ldl
#shared: depends $(OBJS_DST)
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
			-L$(rpath) $(shared_ldflags); \
		echo 'build $(rpath)/$(SHARED_LIBNAME) ok!'; \
	elif test -n "$(rpath)" && test "$(UNIXTYPE)" = "MACOSX"; then \
		echo "building for Darwin"; \
		$(CC) -shared -o $(rpath)/$(SHARED_LIBNAME) $(OBJS_DST) \
			-L$(rpath) -lpthread; \
		echo 'build $(rpath)/$(SHARED_LIBNAME) ok!'; \
	elif test -n "$(rpath)" && test "$(UNIXTYPE)" = "FREEBSD"; then \
		echo "building for FreeBSD"; \
		$(CC) -shared -o $(rpath)/$(SHARED_LIBNAME) $(OBJS_DST) \
			-L$(rpath) -lpthread; \
		echo 'build $(rpath)/$(SHARED_LIBNAME) ok!'; \
	else \
		echo 'skip build $(SHARED_LIBNAME); usage: make shared rpath=xxx'; \
	fi

depends: $(OBJS_DEF)

pch: StdAfx.h
	rm -f $(PCH)
	$(CC) -o $(PCH) -x c-header $(CFLAGS) StdAfx.h

###########################################################

# init
$(OBJ_PATH_DST)/%.o: $(SRC_PATH_SRC)/init/%.c
	$(COMPILE_OBJ)
$(DEF_PATH_DST)/%.inc: $(SRC_PATH_SRC)/init/%.c
	$(CREATE_DEF)
# private 
$(OBJ_PATH_DST)/%.o: $(SRC_PATH_SRC)/private/%.c
	$(COMPILE_OBJ)
$(DEF_PATH_DST)/%.inc: $(SRC_PATH_SRC)/private/%.c
	$(CREATE_DEF)
# stdlib
$(OBJ_PATH_DST)/%.o: $(SRC_PATH_SRC)/stdlib/%.c
	$(COMPILE_OBJ)
$(DEF_PATH_DST)/%.inc: $(SRC_PATH_SRC)/stdlib/%.c
	$(CREATE_DEF)
$(OBJ_PATH_DST)/%.o: $(SRC_PATH_SRC)/stdlib/common/%.c
	$(COMPILE_OBJ)
$(DEF_PATH_DST)/%.inc: $(SRC_PATH_SRC)/stdlib/common/%.c
	$(CREATE_DEF)
$(OBJ_PATH_DST)/%.o: $(SRC_PATH_SRC)/stdlib/iostuff/%.c
	$(COMPILE_OBJ)
$(DEF_PATH_DST)/%.inc: $(SRC_PATH_SRC)/stdlib/iostuff/%.c
	$(CREATE_DEF)
$(OBJ_PATH_DST)/%.o: $(SRC_PATH_SRC)/stdlib/configure/%.c
	$(COMPILE_OBJ)
$(DEF_PATH_DST)/%.inc: $(SRC_PATH_SRC)/stdlib/configure/%.c
	$(CREATE_DEF)
$(OBJ_PATH_DST)/%.o: $(SRC_PATH_SRC)/stdlib/filedir/%.c
	$(COMPILE_OBJ)
$(DEF_PATH_DST)/%.inc: $(SRC_PATH_SRC)/stdlib/filedir/%.c
	$(CREATE_DEF)
$(OBJ_PATH_DST)/%.o: $(SRC_PATH_SRC)/stdlib/string/%.c
	$(COMPILE_OBJ)
$(DEF_PATH_DST)/%.inc: $(SRC_PATH_SRC)/stdlib/string/%.c
	$(CREATE_DEF)
$(OBJ_PATH_DST)/%.o: $(SRC_PATH_SRC)/stdlib/memory/%.c
	$(COMPILE_OBJ)
$(DEF_PATH_DST)/%.inc: $(SRC_PATH_SRC)/stdlib/memory/%.c
	$(CREATE_DEF)
$(OBJ_PATH_DST)/%.o: $(SRC_PATH_SRC)/stdlib/debug/%.c
	$(COMPILE_OBJ)
$(DEF_PATH_DST)/%.inc: $(SRC_PATH_SRC)/stdlib/debug/%.c
	$(CREATE_DEF)
$(OBJ_PATH_DST)/%.o: $(SRC_PATH_SRC)/stdlib/sys/%.c
	$(COMPILE_OBJ)
$(DEF_PATH_DST)/%.inc: $(SRC_PATH_SRC)/stdlib/sys/%.c
	$(CREATE_DEF)
$(OBJ_PATH_DST)/%.o: $(SRC_PATH_SRC)/stdlib/sys/unix/%.c
	$(COMPILE_OBJ)
$(DEF_PATH_DST)/%.inc: $(SRC_PATH_SRC)/stdlib/sys/unix/%.c
	$(CREATE_DEF)
# net
$(OBJ_PATH_DST)/%.o: $(SRC_PATH_SRC)/net/%.c
	$(COMPILE_OBJ)
$(DEF_PATH_DST)/%.inc: $(SRC_PATH_SRC)/net/%.c
	$(CREATE_DEF)
$(OBJ_PATH_DST)/%.o: $(SRC_PATH_SRC)/net/listen/%.c
	$(COMPILE_OBJ)
$(DEF_PATH_DST)/%.inc: $(SRC_PATH_SRC)/net/listen/%.c
	$(CREATE_DEF)
$(OBJ_PATH_DST)/%.o: $(SRC_PATH_SRC)/net/connect/%.c
	$(COMPILE_OBJ)
$(DEF_PATH_DST)/%.inc: $(SRC_PATH_SRC)/net/connect/%.c
	$(CREATE_DEF)
$(OBJ_PATH_DST)/%.o: $(SRC_PATH_SRC)/net/dns/%.c
	$(COMPILE_OBJ)
$(DEF_PATH_DST)/%.inc: $(SRC_PATH_SRC)/net/dns/%.c
	$(CREATE_DEF)
# event
$(OBJ_PATH_DST)/%.o: $(SRC_PATH_SRC)/event/%.c
	$(COMPILE_OBJ)
$(DEF_PATH_DST)/%.inc: $(SRC_PATH_SRC)/event/%.c
	$(CREATE_DEF)

# ioctl
$(OBJ_PATH_DST)/%.o: $(SRC_PATH_SRC)/ioctl/%.c
	$(COMPILE_OBJ)
$(DEF_PATH_DST)/%.inc: $(SRC_PATH_SRC)/ioctl/%.c
	$(CREATE_DEF)

# aio
$(OBJ_PATH_DST)/%.o: $(SRC_PATH_SRC)/aio/%.c
	$(COMPILE_OBJ)
$(DEF_PATH_DST)/%.inc: $(SRC_PATH_SRC)/aio/%.c
	$(CREATE_DEF)

# msg
$(OBJ_PATH_DST)/%.o: $(SRC_PATH_SRC)/msg/%.c
	$(COMPILE_OBJ)
$(DEF_PATH_DST)/%.inc: $(SRC_PATH_SRC)/msg/%.c
	$(CREATE_DEF)

# thread
$(OBJ_PATH_DST)/%.o: $(SRC_PATH_SRC)/thread/%.c
	$(COMPILE_OBJ)
$(DEF_PATH_DST)/%.inc: $(SRC_PATH_SRC)/thread/%.c
	$(CREATE_DEF)

# svr
$(OBJ_PATH_DST)/%.o: $(SRC_PATH_SRC)/svr/%.c
	$(COMPILE_OBJ)
$(DEF_PATH_DST)/%.inc: $(SRC_PATH_SRC)/svr/%.c
	$(CREATE_DEF)

# db
$(OBJ_PATH_DST)/%.o: $(SRC_PATH_SRC)/db/%.c
	$(COMPILE_OBJ)
$(DEF_PATH_DST)/%.inc: $(SRC_PATH_SRC)/db/%.c
	$(CREATE_DEF)
$(OBJ_PATH_DST)/%.o: $(SRC_PATH_SRC)/db/mysql/%.c
	$(COMPILE_OBJ)
$(DEF_PATH_DST)/%.inc: $(SRC_PATH_SRC)/db/mysql/%.c
	$(CREATE_DEF)
$(OBJ_PATH_DST)/%.o: $(SRC_PATH_SRC)/db/null/%.c
	$(COMPILE_OBJ)
$(DEF_PATH_DST)/%.inc: $(SRC_PATH_SRC)/db/null/%.c
	$(CREATE_DEF)
$(OBJ_PATH_DST)/%.o: $(SRC_PATH_SRC)/db/memdb/%.c
	$(COMPILE_OBJ)
$(DEF_PATH_DST)/%.inc: $(SRC_PATH_SRC)/db/memdb/%.c
	$(CREATE_DEF)
$(OBJ_PATH_DST)/%.o: $(SRC_PATH_SRC)/db/zdb/%.c
	$(COMPILE_OBJ)
$(DEF_PATH_DST)/%.inc: $(SRC_PATH_SRC)/db/zdb/%.c
	$(CREATE_DEF)

# code
$(OBJ_PATH_DST)/%.o: $(SRC_PATH_SRC)/code/%.c
	$(COMPILE_OBJ)
$(DEF_PATH_DST)/%.inc: $(SRC_PATH_SRC)/code/%.c
	$(CREATE_DEF)

# master
$(OBJ_PATH_DST)/%.o: $(SRC_PATH_SRC)/master/%.c
	$(COMPILE_OBJ)
$(DEF_PATH_DST)/%.inc: $(SRC_PATH_SRC)/master/%.c
	$(CREATE_DEF)
$(OBJ_PATH_DST)/%.o: $(SRC_PATH_SRC)/master/template/%.c
	$(COMPILE_OBJ)
$(DEF_PATH_DST)/%.inc: $(SRC_PATH_SRC)/master/template/%.c
	$(CREATE_DEF)

# proctl
$(OBJ_PATH_DST)/%.o: $(SRC_PATH_SRC)/proctl/%.c
	$(COMPILE_OBJ)
$(DEF_PATH_DST)/%.inc: $(SRC_PATH_SRC)/proctl/%.c
	$(CREATE_DEF)

# xml
$(OBJ_PATH_DST)/%.o: $(SRC_PATH_SRC)/xml/%.c
	$(COMPILE_OBJ)
$(DEF_PATH_DST)/%.inc: $(SRC_PATH_SRC)/xml/%.c
	$(CREATE_DEF)

# json
$(OBJ_PATH_DST)/%.o: $(SRC_PATH_SRC)/json/%.c
	$(COMPILE_OBJ)
$(DEF_PATH_DST)/%.inc: $(SRC_PATH_SRC)/json/%.c
	$(CREATE_DEF)

# unit_test
$(OBJ_PATH_DST)/%.o: $(SRC_PATH_SRC)/unit_test/%.c
	$(COMPILE_OBJ)
$(DEF_PATH_DST)/%.inc: $(SRC_PATH_SRC)/unit_test/%.c
	$(CREATE_DEF)

# experiment
#$(OBJ_PATH_DST)/%.o: $(SRC_PATH_SRC)/experiment/%.c
#	$(COMPILE_OBJ)
#$(DEF_PATH_DST)/%.inc: $(SRC_PATH_SRC)/experiment/%.c
#	$(CREATE_DEF)

clean_lib:
	rm -f $(LIB_PATH_DST)/$(STATIC_LIBNAME)
	rm -f $(LIB_PATH_DST)/$(STATIC_OLD_LIBNAME)
	rm -f $(LIB_PATH_DST)/libacl.so
	rm -f $(OBJS_DST)
	rm -f $(OBJS_DEF)
	rm -f $(PCH)

###########################################################
