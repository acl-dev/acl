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
-DHAS_MYSQL_DLL \
-Winvalid-pch \
-DACL_PREPARE_COMPILE
#-DHAS_POLARSSL
#-DUSE_DYNAMIC
# -Wcast-align
#-Wcast-qual
#just for freebsd's iconv.h
CFLAGS += -I/usr/local/include

shared_ldflags = -l_protocol -l_acl -lz -lpthread

###########################################################
#Check system:
#       Linux, SunOS, Solaris, BSD variants, AIX, HP-UX
SYSLIB =
CHECKSYSRES = @echo "Unknow system type!";exit 1
UNIXNAME = $(shell uname -sm)
UNIXTYPE = LINUX
polarssl =

ifeq ($(CC),)
        CC = g++
endif

ifeq ($(AR),)
	AR = ar
endif

ifeq ($(RANLIB),)
	RANLIB = ranlib
endif

ifeq ($(findstring HAS_POLARSSL, $(FLAGS)), HAS_POLARSSL)
	CFLAGS += -DHAS_POLARSSL
	polarssl = on
endif

ifeq ($(findstring on, $(polarssl)), on)
	CFLAGS += -DHAS_POLARSSL
endif

ifeq ($(findstring clang++, $(CC)), clang++)
	CFLAGS += -Wno-invalid-source-encoding \
		  -Wno-extended-offsetof
endif

# For FreeBSD
ifeq ($(findstring FreeBSD, $(UNIXNAME)), FreeBSD)
	CFLAGS += -DFREEBSD -fPIC -Werror
	UNIXTYPE = FREEBSD
	shared_ldflags = -l_protocol -l_acl -L/usr/local/lib -liconv -lz -lpthread
endif

# For Darwin
ifeq ($(findstring Darwin, $(UNIXNAME)), Darwin)
	CFLAGS += -DMACOSX -Wno-invalid-source-encoding \
		  -Wno-extended-offsetof -fPIC -Werror \
		  -Wno-unused-private-field
	UNIXTYPE = MACOSX
	shared_ldflags = -l_protocol -l_acl -L/usr/lib -liconv -lz -lpthread
endif

# For Linux
ifeq ($(findstring Linux, $(UNIXNAME)), Linux)
	CFLAGS += -DLINUX2 -fPIC -Werror
	UNIXTYPE = LINUX
	shared_ldflags = -l_protocol -l_acl -lrt -lpthread -lz -ldl
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

OBJ_PATH_DST = ./Debug
LIB_PATH_DST = ./lib
DEF_PATH_DST = ./Debug

CFLAGS += -I./src -I./include -I../lib_acl/include -I../lib_protocol/include \
	  -I../include/sqlite -I../include/mysql -I../include/zlib -I../include

#Project's objs
OBJS_DST = $(patsubst %.cpp, $(OBJ_PATH_DST)/%.o, $(notdir $(wildcard src/*.cpp))) \
	   $(patsubst %.cpp, $(OBJ_PATH_DST)/%.o, $(notdir $(wildcard src/stdlib/*.cpp))) \
	   $(patsubst %.cpp, $(OBJ_PATH_DST)/%.o, $(notdir $(wildcard src/stream/*.cpp))) \
	   $(patsubst %.cpp, $(OBJ_PATH_DST)/%.o, $(notdir $(wildcard src/master/*.cpp))) \
	   $(patsubst %.cpp, $(OBJ_PATH_DST)/%.o, $(notdir $(wildcard src/ipc/*.cpp))) \
	   $(patsubst %.cpp, $(OBJ_PATH_DST)/%.o, $(notdir $(wildcard src/db/*.cpp))) \
	   $(patsubst %.cpp, $(OBJ_PATH_DST)/%.o, $(notdir $(wildcard src/event/*.cpp))) \
	   $(patsubst %.cpp, $(OBJ_PATH_DST)/%.o, $(notdir $(wildcard src/queue/*.cpp))) \
	   $(patsubst %.cpp, $(OBJ_PATH_DST)/%.o, $(notdir $(wildcard src/http/*.cpp))) \
	   $(patsubst %.cpp, $(OBJ_PATH_DST)/%.o, $(notdir $(wildcard src/hsocket/*.cpp))) \
	   $(patsubst %.cpp, $(OBJ_PATH_DST)/%.o, $(notdir $(wildcard src/beanstalk/*.cpp))) \
	   $(patsubst %.cpp, $(OBJ_PATH_DST)/%.o, $(notdir $(wildcard src/memcache/*.cpp))) \
	   $(patsubst %.cpp, $(OBJ_PATH_DST)/%.o, $(notdir $(wildcard src/session/*.cpp))) \
	   $(patsubst %.cpp, $(OBJ_PATH_DST)/%.o, $(notdir $(wildcard src/smtp/*.cpp))) \
	   $(patsubst %.cpp, $(OBJ_PATH_DST)/%.o, $(notdir $(wildcard src/mime/*.cpp))) \
	   $(patsubst %.cpp, $(OBJ_PATH_DST)/%.o, $(notdir $(wildcard src/mime/internal/*.cpp))) \
	   $(patsubst %.cpp, $(OBJ_PATH_DST)/%.o, $(notdir $(wildcard src/connpool/*.cpp))) \
	   $(patsubst %.cpp, $(OBJ_PATH_DST)/%.o, $(notdir $(wildcard src/redis/*.cpp))) \
	   $(patsubst %.cpp, $(OBJ_PATH_DST)/%.o, $(notdir $(wildcard src/disque/*.cpp)))

OBJS_DEF = $(patsubst %.cpp, $(DEF_PATH_DST)/%.inc, $(notdir $(wildcard src/*.cpp))) \
	   $(patsubst %.cpp, $(DEF_PATH_DST)/%.inc, $(notdir $(wildcard src/stdlib/*.cpp))) \
	   $(patsubst %.cpp, $(DEF_PATH_DST)/%.inc, $(notdir $(wildcard src/stream/*.cpp))) \
	   $(patsubst %.cpp, $(DEF_PATH_DST)/%.inc, $(notdir $(wildcard src/master/*.cpp))) \
	   $(patsubst %.cpp, $(DEF_PATH_DST)/%.inc, $(notdir $(wildcard src/ipc/*.cpp))) \
	   $(patsubst %.cpp, $(DEF_PATH_DST)/%.inc, $(notdir $(wildcard src/db/*.cpp))) \
	   $(patsubst %.cpp, $(DEF_PATH_DST)/%.inc, $(notdir $(wildcard src/event/*.cpp))) \
	   $(patsubst %.cpp, $(DEF_PATH_DST)/%.inc, $(notdir $(wildcard src/queue/*.cpp))) \
	   $(patsubst %.cpp, $(DEF_PATH_DST)/%.inc, $(notdir $(wildcard src/http/*.cpp))) \
	   $(patsubst %.cpp, $(DEF_PATH_DST)/%.inc, $(notdir $(wildcard src/hsocket/*.cpp))) \
	   $(patsubst %.cpp, $(DEF_PATH_DST)/%.inc, $(notdir $(wildcard src/beanstalk/*.cpp))) \
	   $(patsubst %.cpp, $(DEF_PATH_DST)/%.inc, $(notdir $(wildcard src/memcache/*.cpp))) \
	   $(patsubst %.cpp, $(DEF_PATH_DST)/%.inc, $(notdir $(wildcard src/session/*.cpp))) \
	   $(patsubst %.cpp, $(DEF_PATH_DST)/%.inc, $(notdir $(wildcard src/smtp/*.cpp))) \
	   $(patsubst %.cpp, $(DEF_PATH_DST)/%.inc, $(notdir $(wildcard src/mime/*.cpp))) \
	   $(patsubst %.cpp, $(DEF_PATH_DST)/%.inc, $(notdir $(wildcard src/mime/internal/*.cpp))) \
	   $(patsubst %.cpp, $(DEF_PATH_DST)/%.inc, $(notdir $(wildcard src/connpool/*.cpp))) \
	   $(patsubst %.cpp, $(DEF_PATH_DST)/%.inc, $(notdir $(wildcard src/redis/*.cpp))) \
	   $(patsubst %.cpp, $(DEF_PATH_DST)/%.inc, $(notdir $(wildcard src/disque/*.cpp)))

###########################################################
.PHONY = static shared clean
VERSION = 1.1.1.2
DATE_NOW = 20`date +%y`.`date +%m`.`date +%d`

STATIC_LIBNAME = lib_acl_cpp.a
SHARED_LIBNAME = lib_acl_cpp.so
PCH = src/acl_stdafx.hpp.gch

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
check:
	@if test -n $(polarssl) && test "$(polarssl)" = "on"; then \
		rm -f Debug/polarssl_io.o; \
		rm -f Debug/polarssl_conf.o; \
		rm -f Debug/polarssl_io.inc; \
		rm -f Debug/polarssl_conf.inc; \
	fi

pch: src/acl_stdafx.hpp
	rm -f $(PCH)
	@if test "$(UNIXTYPE)" = "LINUX" || test "$(UNIXTYPE)" = "FREEBSD" \
		|| test "$(UNIXTYPE)" = "MACOSX"; then \
		$(CC) -o $(PCH) -x c++-header $(CFLAGS) src/acl_stdafx.hpp; \
	fi

# src
$(OBJ_PATH_DST)/%.o: ./src/%.cpp
	$(COMPILE_OBJ)
$(OBJ_PATH_DST)/%.o: ./src/stdlib/%.cpp
	$(COMPILE_OBJ)
$(OBJ_PATH_DST)/%.o: ./src/stream/%.cpp
	$(COMPILE_OBJ)
$(OBJ_PATH_DST)/%.o: ./src/master/%.cpp
	$(COMPILE_OBJ)
$(OBJ_PATH_DST)/%.o: ./src/ipc/%.cpp
	$(COMPILE_OBJ)
$(OBJ_PATH_DST)/%.o: ./src/db/%.cpp
	$(COMPILE_OBJ)
$(OBJ_PATH_DST)/%.o: ./src/event/%.cpp
	$(COMPILE_OBJ)
$(OBJ_PATH_DST)/%.o: ./src/queue/%.cpp
	$(COMPILE_OBJ)
$(OBJ_PATH_DST)/%.o: ./src/http/%.cpp
	$(COMPILE_OBJ)
$(OBJ_PATH_DST)/%.o: ./src/hsocket/%.cpp
	$(COMPILE_OBJ)
$(OBJ_PATH_DST)/%.o: ./src/beanstalk/%.cpp
	$(COMPILE_OBJ)
$(OBJ_PATH_DST)/%.o: ./src/memcache/%.cpp
	$(COMPILE_OBJ)
$(OBJ_PATH_DST)/%.o: ./src/session/%.cpp
	$(COMPILE_OBJ)
$(OBJ_PATH_DST)/%.o: ./src/connpool/%.cpp
	$(COMPILE_OBJ)
$(OBJ_PATH_DST)/%.o: ./src/smtp/%.cpp
	$(COMPILE_OBJ)
$(OBJ_PATH_DST)/%.o: ./src/mime/%.cpp
	$(COMPILE_OBJ)
$(OBJ_PATH_DST)/%.o: ./src/mime/internal/%.cpp
	$(COMPILE_OBJ)
$(OBJ_PATH_DST)/%.o: ./src/redis/%.cpp
	$(COMPILE_OBJ)
$(OBJ_PATH_DST)/%.o: ./src/disque/%.cpp
	$(COMPILE_OBJ)

$(DEF_PATH_DST)/%.inc: ./src/%.cpp
	$(CREATE_DEF)
$(DEF_PATH_DST)/%.inc: ./src/stdlib/%.cpp
	$(CREATE_DEF)
$(DEF_PATH_DST)/%.inc: ./src/stream/%.cpp
	$(CREATE_DEF)
$(DEF_PATH_DST)/%.inc: ./src/master/%.cpp
	$(CREATE_DEF)
$(DEF_PATH_DST)/%.inc: ./src/ipc/%.cpp
	$(CREATE_DEF)
$(DEF_PATH_DST)/%.inc: ./src/db/%.cpp
	$(CREATE_DEF)
$(DEF_PATH_DST)/%.inc: ./src/event/%.cpp
	$(CREATE_DEF)
$(DEF_PATH_DST)/%.inc: ./src/queue/%.cpp
	$(CREATE_DEF)
$(DEF_PATH_DST)/%.inc: ./src/http/%.cpp
	$(CREATE_DEF)
$(DEF_PATH_DST)/%.inc: ./src/hsocket/%.cpp
	$(CREATE_DEF)
$(DEF_PATH_DST)/%.inc: ./src/beanstalk/%.cpp
	$(CREATE_DEF)
$(DEF_PATH_DST)/%.inc: ./src/memcache/%.cpp
	$(CREATE_DEF)
$(DEF_PATH_DST)/%.inc: ./src/session/%.cpp
	$(CREATE_DEF)
$(DEF_PATH_DST)/%.inc: ./src/connpool/%.cpp
	$(CREATE_DEF)
$(DEF_PATH_DST)/%.inc: ./src/smtp/%.cpp
	$(CREATE_DEF)
$(DEF_PATH_DST)/%.inc: ./src/mime/%.cpp
	$(CREATE_DEF)
$(DEF_PATH_DST)/%.inc: ./src/mime/internal/%.cpp
	$(CREATE_DEF)
$(DEF_PATH_DST)/%.inc: ./src/redis/%.cpp
	$(CREATE_DEF)
$(DEF_PATH_DST)/%.inc: ./src/disque/%.cpp
	$(CREATE_DEF)

clean:
	rm -f $(LIB_PATH_DST)/lib_acl_cpp.a
	rm -f $(LIB_PATH_DST)/lib_acl_cpp.so
	rm -f $(OBJS_DST)
	rm -f $(OBJS_DEF)
	rm -f $(PCH)
	@(cd samples; make clean)

rebuild rb: clean static

build_src: clean
	@(echo "begin building src release...")
	@(rm -rf acl_cpp)
	@(echo "copy files ...")
	@(mkdir acl_cpp)
	@(cp -R src acl_cpp/)
	@(cp -R mime acl_cpp/)
	@(cp -R lib acl_cpp/)
	@(cp -R include acl_cpp/)
	@(cp -R samples acl_cpp/)
	@(cp -R Debug acl_cpp/)
	@(cp -R DebugDll acl_cpp/)
	@(cp -R Release acl_cpp/)
	@(cp -R ReleaseDll acl_cpp/)
	@(cp -R bin acl_cpp/)
	@(cp -R doc acl_cpp/)
	@(cp lib_acl_cpp_vc2003.vcproj acl_cpp/)
	@(cp acl_wrap_vc2003.sln acl_cpp/)
	@(cp lib_acl_cpp_vc2010.vcxproj acl_cpp/)
	@(cp acl_wrap_vc2010.sln acl_cpp/)
	@(cp Makefile acl_cpp/)
	@(cp changes.txt acl_cpp/)
	@(cp ReadMe.txt acl_cpp/)
	@(cp Doxyfile acl_cpp/)
	@(echo "make tar package ...")
	@(tar -cf acl_cpp.src.tar acl_cpp)
	@(echo "make gzip package ...")
	@(gzip -c acl_cpp.src.tar > acl_cpp.src.tgz)
	@(rm acl_cpp.src.tar)
	@(rm -rf acl_cpp)
	@(echo "move acl_cpp.src.tgz to ../acl_cpp$(VERSION).src.$(DATE_NOW).tgz")
	(mv acl_cpp.src.tgz ../acl_cpp$(VERSION).src.$(DATE_NOW).tgz)
