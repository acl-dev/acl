SHELL = /bin/sh
CC      = g++
AR      = ar
ARFL    = rv
#ARFL    = cru
RANLIB  = ranlib

#-Wcast-qual
CFLAGS = -c -g -W -Wall -Wcast-align \
-Wpointer-arith -Werror -Wshadow -O3 \
-D_REENTRANT -D_POSIX_PTHREAD_SEMANTICS \
-Wno-long-long \
-fPIC \
-DHAS_MYSQL \
-DHAS_SQLITE \
-Wformat
#-DHAS_POLARSSL
#-DUSE_DYNAMIC

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
	   $(patsubst %.cpp, $(OBJ_PATH_DST)/%.o, $(notdir $(wildcard src/queue/*.cpp))) \
	   $(patsubst %.cpp, $(OBJ_PATH_DST)/%.o, $(notdir $(wildcard src/http/*.cpp))) \
	   $(patsubst %.cpp, $(OBJ_PATH_DST)/%.o, $(notdir $(wildcard src/hsocket/*.cpp))) \
	   $(patsubst %.cpp, $(OBJ_PATH_DST)/%.o, $(notdir $(wildcard src/beanstalk/*.cpp))) \
	   $(patsubst %.cpp, $(OBJ_PATH_DST)/%.o, $(notdir $(wildcard src/memcache/*.cpp))) \
	   $(patsubst %.cpp, $(OBJ_PATH_DST)/%.o, $(notdir $(wildcard src/session/*.cpp))) \
	   $(patsubst %.cpp, $(OBJ_PATH_DST)/%.o, $(notdir $(wildcard src/mime/*.cpp))) \
	   $(patsubst %.cpp, $(OBJ_PATH_DST)/%.o, $(notdir $(wildcard src/mime/internal/*.cpp))) \
	   $(patsubst %.cpp, $(OBJ_PATH_DST)/%.o, $(notdir $(wildcard src/connpool/*.cpp))) \

OBJS_DEF = $(patsubst %.cpp, $(DEF_PATH_DST)/%.inc, $(notdir $(wildcard src/*.cpp))) \
	   $(patsubst %.cpp, $(DEF_PATH_DST)/%.inc, $(notdir $(wildcard src/stdlib/*.cpp))) \
	   $(patsubst %.cpp, $(DEF_PATH_DST)/%.inc, $(notdir $(wildcard src/stream/*.cpp))) \
	   $(patsubst %.cpp, $(DEF_PATH_DST)/%.inc, $(notdir $(wildcard src/master/*.cpp))) \
	   $(patsubst %.cpp, $(DEF_PATH_DST)/%.inc, $(notdir $(wildcard src/ipc/*.cpp))) \
	   $(patsubst %.cpp, $(DEF_PATH_DST)/%.inc, $(notdir $(wildcard src/db/*.cpp))) \
	   $(patsubst %.cpp, $(DEF_PATH_DST)/%.inc, $(notdir $(wildcard src/queue/*.cpp))) \
	   $(patsubst %.cpp, $(DEF_PATH_DST)/%.inc, $(notdir $(wildcard src/http/*.cpp))) \
	   $(patsubst %.cpp, $(DEF_PATH_DST)/%.inc, $(notdir $(wildcard src/hsocket/*.cpp))) \
	   $(patsubst %.cpp, $(DEF_PATH_DST)/%.inc, $(notdir $(wildcard src/beanstalk/*.cpp))) \
	   $(patsubst %.cpp, $(DEF_PATH_DST)/%.inc, $(notdir $(wildcard src/memcache/*.cpp))) \
	   $(patsubst %.cpp, $(DEF_PATH_DST)/%.inc, $(notdir $(wildcard src/session/*.cpp))) \
	   $(patsubst %.cpp, $(DEF_PATH_DST)/%.inc, $(notdir $(wildcard src/mime/*.cpp))) \
	   $(patsubst %.cpp, $(DEF_PATH_DST)/%.inc, $(notdir $(wildcard src/mime/internal/*.cpp))) \
	   $(patsubst %.cpp, $(DEF_PATH_DST)/%.inc, $(notdir $(wildcard src/connpool/*.cpp))) \

###########################################################
.PHONY = static shared clean
VERSION = 1.1.1.2
DATE_NOW = 20`date +%y`.`date +%m`.`date +%d`

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
	@echo 'creating $(LIB_PATH_DST)/lib_acl_cpp.a'
	@$(AR) $(ARFL) $(LIB_PATH_DST)/lib_acl_cpp.a $(OBJS_DST)
	@$(RANLIB) $(LIB_PATH_DST)/lib_acl_cpp.a
	@echo 'create $(LIB_PATH_DST)/lib_acl_cpp.a ok!'

shared_ldflags = -l_protocol -l_acl \
	-lmysqlclient -lsqlite3 -lrt -lpthread -lz -ldl
shared: depends $(OBJS_DST)
	@echo 'creating $(LIB_PATH_DST)/lib_acl_cpp.so'
	@if test -n "$(rpath)"; then \
		$(CC) -shared -o $(LIB_PATH_DST)/lib_acl_cpp.so $(OBJS_DST) \
			-L$(rpath) $(shared_ldflags) -Wl,-rpath,$(rpath); \
		echo 'build $(LIB_PATH_DST)/lib_acl_cpp.so ok!'; \
	else \
		echo 'skip build $(LIB_PATH_DST)/lib_acl_cpp.so; usage: make rpath=xxx'; \
	fi

depends: $(OBJS_DEF)

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
$(OBJ_PATH_DST)/%.o: ./src/mime/%.cpp
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
$(DEF_PATH_DST)/%.inc: ./src/mime/%.cpp
	$(CREATE_DEF)

# mime
$(OBJ_PATH_DST)/%.o: ./src/mime/internal/%.cpp
	$(COMPILE_OBJ)

$(DEF_PATH_DST)/%.inc: ./src/mime/internal/%.cpp
	$(CREATE_DEF)

clean:
	rm -f $(LIB_PATH_DST)/lib_acl_cpp.a
	rm -f $(OBJS_DST)
	rm -f $(OBJS_DEF)
	@(cd samples; make clean)

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
