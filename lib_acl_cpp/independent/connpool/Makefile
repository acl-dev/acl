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
-Wformat

###########################################################
#Check system:
#       Linux, SunOS, Solaris, BSD variants, AIX, HP-UX
SYSLIB =
CHECKSYSRES = @echo "Unknow system type!";exit 1
UNIXNAME = $(shell uname -sm)

ifeq ($(CC),)
        CC = g++
endif

# For Linux
ifeq ($(findstring Linux, $(UNIXNAME)), Linux)
	ifeq ($CC, "gcc")
		CFLAGS += -Wstrict-prototypes
	endif
	CFLAGS += -DLINUX2 -D_REENTRANT
endif

#Find system type.
ifneq ($(SYSPATH),)
	CHECKSYSRES = @echo "System is $(shell uname -sm)"
endif
###########################################################

OBJ_PATH_DST = ./obj
LIB_PATH_DST = ./lib
DEF_PATH_DST = ./obj

CFLAGS += -I./include

#Project's objs
OBJS_DST = $(patsubst %.cpp, $(OBJ_PATH_DST)/%.o, $(notdir $(wildcard src/*.cpp)))

OBJS_DEF = $(patsubst %.cpp, $(DEF_PATH_DST)/%.inc, $(notdir $(wildcard src/*.cpp)))

###########################################################
.PHONY = static clean
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
	@echo 'creating $(LIB_PATH_DST)/lib_connpool.a'
	@$(AR) $(ARFL) $(LIB_PATH_DST)/lib_connpool.a $(OBJS_DST)
	@$(RANLIB) $(LIB_PATH_DST)/lib_connpool.a
	@echo 'create $(LIB_PATH_DST)/lib_connpool.a ok!'

depends: $(OBJS_DEF)

# src
$(OBJ_PATH_DST)/%.o: src/%.cpp
	$(COMPILE_OBJ)
$(DEF_PATH_DST)/%.inc: src/%.cpp
	$(CREATE_DEF)
clean:
	rm -f $(LIB_PATH_DST)/lib_connpool.a
	rm -f $(OBJS_DST)
	rm -f $(OBJS_DEF)
