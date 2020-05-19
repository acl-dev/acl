SHELL = /bin/sh
CC      = g++
AR      = ar
ARFL    = rv
#ARFL    = cru
RANLIB  = ranlib

#-Wcast-qual
CFLAGS = -c -g -W \
-Wall \
-Wcast-align \
-Wpointer-arith \
-Werror \
-Wshadow \
-O3 \
-D_REENTRANT \
-D_POSIX_PTHREAD_SEMANTICS \
-Wno-long-long \
-fPIC \
-Wformat \
-DDEBUG
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

ifeq ($(findstring gcc, $(CC)), gcc)
	CFLAGS += -Wstrict-prototypes
endif

# For FreeBSD
ifeq ($(findstring FreeBSD, $(UNIXNAME)), FreeBSD)
	CFLAGS += -DFREEBSD
endif

# For Darwin
ifeq ($(findstring Darwin, $(UNIXNAME)), Darwin)                               
	CFLAGS += -DMACOSX -Wno-invalid-source-encoding -Wno-invalid-offsetof
	UNIXTYPE = MACOSX                                                      
endif 

# For Linux
ifeq ($(findstring Linux, $(UNIXNAME)), Linux)
	ifeq ($CC, "gcc")
		CFLAGS += -Wstrict-prototypes
	endif
	CFLAGS += -DLINUX2
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
LIB_PATH_DST = .
DEF_PATH_DST = ./Debug

CFLAGS += -I./include -I../lib_acl_cpp/include -I../lib_acl/include -I../include

#Project's objs
OBJS_DST = $(patsubst %.cpp, $(OBJ_PATH_DST)/%.o, $(notdir $(wildcard src/*.cpp)))

OBJS_DEF = $(patsubst %.cpp, $(DEF_PATH_DST)/%.inc, $(notdir $(wildcard src/*.cpp)))

###########################################################
.PHONY = static clean
VERSION = 1.0
DATE_NOW = 20`date +%y`.`date +%m`.`date +%d`

all: static

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

static: depends $(OBJS_DST)
	@echo 'creating $(LIB_PATH_DST)/lib_rpc.a'
	@$(AR) $(ARFL) $(LIB_PATH_DST)/lib_rpc.a $(OBJS_DST)
	@$(RANLIB) $(LIB_PATH_DST)/lib_rpc.a
	@echo 'create $(LIB_PATH_DST)/lib_rpc.a ok!'

depends: $(OBJS_DEF)

# src
$(OBJ_PATH_DST)/%.o: ./src/%.cpp
	$(COMPILE_OBJ)

$(DEF_PATH_DST)/%.inc: ./src/%.cpp
	$(CREATE_DEF)

samples: $(lib_rpc.a)
	@(cd samples; make)
clean:
	rm -f $(LIB_PATH_DST)/lib_rpc.a
	rm -f $(OBJS_DST)
	rm -f $(OBJS_DEF)
	@(cd samples; make clean)
