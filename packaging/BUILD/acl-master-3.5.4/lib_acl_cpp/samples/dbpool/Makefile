SHELL = /bin/sh
CC      = gcc
CC      = g++
AR      = ar
ARFL    = rv
#ARFL    = cru
RANLIB  = ranlib

CFLAGS = -c -g -W -Wall -Wcast-qual -Wcast-align \
-Wno-long-long \
-Wpointer-arith -Werror -Wshadow -O2 \
-D_REENTRANT -D_POSIX_PTHREAD_SEMANTICS -D_USE_FAST_MACRO
#-Waggregate-return 
###########################################################
#Check system:
#       Linux, SunOS, Solaris, BSD variants, AIX, HP-UX
SYSLIB = -lpthread -lcrypt -lz
CHECKSYSRES = @echo "Unknow system type!";exit 1
UNIXNAME = $(shell uname -sm)

ifeq ($(CC),)
        CC = gcc
endif

# For FreeBSD
ifeq ($(findstring FreeBSD, $(UNIXNAME)), FreeBSD)
	ifeq ($(findstring gcc, $(CC)), gcc)
		CFLAGS += -Wstrict-prototypes
	endif
	CFLAGS += -DFREEBSD -D_REENTRANT
	SYSLIB = -lcrypt -lpthread
endif

#Path for Linux
ifeq ($(findstring Linux, $(UNIXNAME)), Linux)
	ifeq ($CC, "gcc")
		CFLAGS += -Wstrict-prototypes
	endif
	CFLAGS += -DLINUX2 -D_REENTRANT
endif

#Path for SunOS
ifeq ($(findstring SunOS, $(UNIXNAME)), SunOS)
	ifeq ($(findstring 86, $(UNIXNAME)), 86)
		SYSLIB += -lsocket -lnsl -lrt
	endif
	ifeq ($(findstring sun4u, $(UNIXNAME)), sun4u)
		SYSLIB += -lsocket -lnsl -lrt
	endif
	ifeq ($CC, "gcc")
		CFLAGS += -Wstrict-prototypes
	endif
	CFLAGS += -DSUNOS5 -D_REENTRANT
endif

#Path for HP-UX
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

CFLAGS += $(INC_COMPILE)

INC_COMPILE = -I../../include -I../../../lib_acl/include -I../../../include/mysql
LIB_COMPILE = -L../../lib -l_acl_cpp -L/opt/import_lib/lib -l_acl \
	      -lmysqlclient_r -ldl

###########################################################
OUT_PATH = .
OBJ_PATH_DST = $(OUT_PATH)

#Project's objs
SRC = $(wildcard *.cpp)
OBJ = $(patsubst %.cpp, $(OBJ_PATH_DST)/%.o, $(notdir $(SRC)))
###########################################################

PROG   = dbpool

.PHONY = all clean
COMPILE = $(CC) $(CFLAGS)

$(PROG): $(OBJ)
	$(CC) $(OBJ) $(LIB_COMPILE) $(SYSLIB) -o $(OBJ_PATH_DST)/$(PROG)
	@echo ""
	@echo "All ok! Output:$(PROG)"
	@echo ""
# fifo
$(OBJ_PATH_DST)/%.o: %.cpp
	$(COMPILE) $< -o $@

clean:
	rm -f $(PROG)
	rm -f $(OBJ)

