SHELL     = /bin/sh
#CC       = g++
CC        = ${ENV_CC}
#AR       = ar
AR        = ${ENV_AR}
ARFL      = rv
#RANLIB   = ranlib
RANLIB    = ${ENV_RANLIB}

#OSNAME   = $(shell uname -sm)
OSNAME    = $(shell uname -s)
OSTYPE    = $(shell uname -m)

DESTDIR   =
PREFIX    = usr
BIN_PATH  = $(DESTDIR)/$(PREFIX)/bin/
LIB_ACL   = $(DESTDIR)/$(PREFIX)/lib
INC_ACL   = $(DESTDIR)/$(PREFIX)/include/acl-lib

LIB_DIST  = ./dist/lib
INC_PATH  = ./dist/include

RPATH =
DATE_NOW = 20`date +%y`.`date +%m`.`date +%d`
MAKE_ARGS =

SYSLIB = -lpthread -lz
LDFLAGS = -shared
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

ifeq ($(findstring on, $(polarssl)), on)
	CFLAGS += -DHAS_POLARSSL
endif

ifeq ($(findstring Linux, $(OSNAME)), Linux)
	ifeq ($(findstring i686, $(OSTYPE)), i686)
		RPATH = linux32
	endif
	ifeq ($(findstring x86_64, $(OSTYPE)), x86_64)
		RPATH = linux64
	endif
	n = `cat /proc/cpuinfo | grep processor | wc -l`
	MAKE_ARGS = -j $(n)
	SYSLIB += -lrt -ldl
endif

# For Darwin
ifeq ($(findstring Darwin, $(OSNAME)), Darwin)
	RPATH = macos
	SYSLIB +=  -rdynamic -L/usr/lib -liconv
	LDFLAGS = -dynamiclib -shared
endif

ifeq ($(findstring FreeBSD, $(OSNAME)), FreeBSD)
	RPATH = freebsd
	SYSLIB += -L/usr/local/lib -liconv
endif

ifeq ($(findstring SunOS, $(OSNAME)), SunOS)
	ifeq ($(findstring i386, $(OSTYPE)), i386)
		RPATH = sunos_x86
	endif
	SYSLIB += -liconv
endif

##############################################################################

.PHONY = check help all_lib all samples all clean install uninstall uninstall_all build_one
VERSION = 3.5.1-4

default: build_one acl_master
help h:
	@(echo "usage: make help|all|all_lib|all_samples|clean|install|uninstall|uninstall_all|build_one")
all_lib:
	@if test "$(polarssl)" = "on"; then \
		ENV_FLAGS = $(ENV_FLAGS):HAS_POLARSSL \
		export ENV_FLAGS; \
	else \
		export ENV_FLAGS; \
	fi
#	@(cd lib_acl; make pch)
	@(cd lib_acl; make $(MAKE_ARGS))
	@(cd lib_protocol; make $(MAKE_ARGS))
	@(cd lib_acl_cpp; make check)
#	@(cd lib_acl_cpp; make pch)
	@(cd lib_acl_cpp; make $(MAKE_ARGS))
	@(cd lib_rpc; make $(MAKE_ARGS))
	@if test "$(OSNAME)" = "Linux" -o "$(OSNAME)" = "FreeBSD" -o "$(OSNAME)" = "Darwin"; then cd lib_fiber; make; fi
all_samples: all_lib
	@(cd unit_test; make $(MAKE_ARGS))
	@(cd lib_acl/samples; make)
	@(cd lib_protocol/samples; make)
	@(cd lib_acl_cpp/samples; make)
#	@(cd lib_dict; make $(MAKE_ARGS))
#	@(cd lib_tls; make $(MAKE_ARGS))
all: all_lib acl_master all_samples
clean cl:
	@(cd lib_acl; make clean)
	@(cd lib_protocol; make clean)
	@(cd lib_acl_cpp; make clean)
	@(cd lib_fiber; make clean)
	@(cd lib_rpc; make clean)
	@(cd unit_test; make clean)
	@(cd lib_acl/samples; make clean)
	@(cd lib_protocol/samples; make clean)
	@(cd app; make clean)
	@(rm -f libacl_all.a libacl.a)
	@(rm -f libacl_all.so libacl.so)
#	@(cd lib_dict; make clean)
#	@(cd lib_tls; make clean)

acl_master: all_lib
	@(cd app/master/daemon; make $(MAKE_ARGS); make install)
	@(cd app/master/tools/lib_global; make $(MAKE_ARGS);)
	@(cd app/master/tools/master_ctl; make $(MAKE_ARGS); make install)

packinstall:
	@(echo "")
	@(echo "begin copy file...")
	$(shell mkdir -p $(INC_ACL)/acl)
	$(shell mkdir -p $(INC_ACL)/acl_cpp)
	$(shell mkdir -p $(INC_ACL)/protocol)
	$(shell mkdir -p $(LIB_ACL))
	@echo "copying lib_acl/include/* $(INC_ACL)/acl/"
	@cp -Rf lib_acl/include/* $(INC_ACL)/acl/
	@echo "copying lib_acl_cpp/include/acl_cpp/* $(INC_ACL)/acl_cpp/"
	@cp -Rf lib_acl_cpp/include/acl_cpp/* $(INC_ACL)/acl_cpp/
	@echo "copying lib_protocol/include/* $(INC_ACL)/protocol/"
	@cp -Rf lib_protocol/include/* $(INC_ACL)/protocol/
	@echo "copying libacl_all.a $(LIB_ACL)/libacl_all.a";
	@cp -f libacl_all.a $(LIB_ACL)/libacl_all.a;
	@if test "$(OSNAME)" = "Linux"; then \
		$(shell mkdir -p $(INC_ACL)/fiber) \
		echo "copying lib_fiber/c/include/fiber/* $(INC_ACL)/fiber/"; \
		cp -f lib_fiber/c/include/fiber/* $(INC_ACL)/fiber/; \
		echo "copying lib_fiber/cpp/include/fiber/* $(INC_ACL)/fiber/"; \
		cp -f lib_fiber/cpp/include/fiber/* $(INC_ACL)/fiber/; \
		echo "copying lib_fiber/lib/libfiber.a $(LIB_ACL)/libfiber.a"; \
		cp -f lib_fiber/lib/libfiber.a $(LIB_ACL)/libfiber.a; \
		echo "copying lib_fiber/lib/libfiber_cpp.a $(LIB_ACL)/libfiber_cpp.a"; \
		cp -f lib_fiber/lib/libfiber_cpp.a $(LIB_ACL)/libfiber_cpp.a; \
	fi

install_master:
	$(shell mkdir -p $(BIN_PATH))
	$(shell mkdir -p $(DESTDIR)/opt/soft/services/)
	$(shell mkdir -p $(DESTDIR)/opt/soft/acl-master/conf/service)
	$(shell mkdir -p ./dist/master/libexec/$(RPATH))
	$(shell mkdir -p ./dist/master/bin/$(RPATH))
	@(cd app/master/daemon; make install)
	@(cd app/master/tools/master_ctl; make install)
	@(cd lib_fiber; make)
	#@echo "copying app/master/daemon/acl_master $(BIN_PATH)"
	#@cp -f app/master/daemon/acl_master $(BIN_PATH)
	@(cd dist/master && ./setup.sh $(DESTDIR) /opt/soft/acl-master)

install:
	@(echo "")
	@(echo "begin copy file...")
	$(shell mkdir -p $(INC_PATH)/acl)
	$(shell mkdir -p $(INC_PATH)/protocol)
	$(shell mkdir -p $(INC_PATH)/acl_cpp)
	$(shell mkdir -p $(LIB_DIST)/$(RPATH))
	$(shell mkdir -p ./dist/master/libexec/$(RPATH))
	cp -f app/master/daemon/acl_master ./dist/master/libexec/$(RPATH)/
	cp -f libacl_all.a $(LIB_DIST)/$(RPATH)/
	cp -f lib_acl/lib/libacl.a $(LIB_DIST)/$(RPATH)/
	cp -f lib_acl_cpp/lib/libacl_cpp.a $(LIB_DIST)/$(RPATH)/
	cp -f lib_protocol/lib/libprotocol.a $(LIB_DIST)/$(RPATH)/
	cp -Rf lib_acl/include/* $(INC_PATH)/acl/
	cp -Rf lib_protocol/include/* $(INC_PATH)/protocol/
	cp -Rf lib_acl_cpp/include/acl_cpp/* $(INC_PATH)/acl_cpp/

uninstall:
	@(echo "")
	@(echo "begin remove file...")
	rm -f ./dist/master/libexec/$(RPATH)/*
	rm -f $(LIB_DIST)/$(RPATH)/libacl_all.a
	rm -f $(LIB_DIST)/$(RPATH)/libacl.a
	rm -f $(LIB_DIST)/$(RPATH)/libprotocol.a
	rm -f $(LIB_DIST)/$(RPATH)/libacl_cpp.a
	rm -f $(LIB_DIST)/$(RPATH)/lib_dict.a
	rm -f $(LIB_DIST)/$(RPATH)/lib_tls.a
	rm -Rf $(INC_PATH)/protocol/*
	rm -Rf $(INC_PATH)/acl_cpp/*
	rm -Rf $(INC_PATH)/acl/*
	rm -Rf $(INC_PATH)/dict/*
	rm -Rf $(INC_PATH)/tls/*

uninstall_all:
	@(echo "")
	@(echo "begin remove all dist files ...")
	rm -Rf $(INC_PATH)/acl/*
	rm -Rf $(INC_PATH)/protocol/*
	rm -Rf $(INC_PATH)/acl_cpp/*
	rm -Rf $(INC_PATH)/dist/*
	rm -Rf $(INC_PATH)/tls/*
	rm -f ./dist/master/libexec/linux32/*
	rm -f ./dist/master/libexec/linux64/*
	rm -f ./dist/master/libexec/sunos_x86/*
	rm -f ./dist/master/libexec/freebsd/*
	rm -f $(LIB_DIST)/linux32/*.a
	rm -f $(LIB_DIST)/linux64/*.a
	rm -f $(LIB_DIST)/sunos_x86/*.a
	rm -f $(LIB_DIST)/freebsd/*.a
	rm -f $(LIB_DIST)/win32/*.lib
	rm -f $(LIB_DIST)/win32/*.dll
	rm -f win32_build/vc/lib_acl/*.map
	rm -f win32_build/vc/lib_acl/*.ilk
	rm -f win32_build/vc/lib_protocol/*.map
	rm -f win32_build/vc/lib_protocol/*.ilk

RELEASE_PATH = release
build_one: all_lib
	@(mkdir -p $(RELEASE_PATH); mkdir -p $(RELEASE_PATH)/acl; \
		mkdir -p $(RELEASE_PATH)/protocol; \
		mkdir -p $(RELEASE_PATH)/acl_cpp)
	@(cp lib_acl/lib/libacl.a $(RELEASE_PATH)/acl/)
	@(cp lib_protocol/lib/libprotocol.a $(RELEASE_PATH)/protocol/)
	@(cp lib_acl_cpp/lib/libacl_cpp.a $(RELEASE_PATH)/acl_cpp/)
	@(cd $(RELEASE_PATH)/acl; ar -x libacl.a)
	@(cd $(RELEASE_PATH)/protocol; ar -x libprotocol.a)
	@(cd $(RELEASE_PATH)/acl_cpp; ar -x libacl_cpp.a)
	$(AR) $(ARFL) ./libacl_all.a $(RELEASE_PATH)/acl/*.o \
		$(RELEASE_PATH)/protocol/*.o $(RELEASE_PATH)/acl_cpp/*.o
	$(RANLIB) ./libacl_all.a
	rm -f libacl.a
	ln -s libacl_all.a libacl.a
	$(CC) $(LDFLAGS) -o ./libacl_all.so $(RELEASE_PATH)/acl_cpp/*.o \
		$(RELEASE_PATH)/protocol/*.o $(RELEASE_PATH)/acl/*.o \
		$(SYSLIB)
	rm -f libacl.so
	ln -s libacl_all.so libacl.so
	@(rm -rf $(RELEASE_PATH))
	@echo ""
	@echo "Over, libacl_all.a and libacl_all.so were built ok!"
	@echo ""

check:
	@(echo "TYPE:	$(OSTYPE)")
	@(echo "OSNAME:	$(OSNAME)")
	@(echo "RPATH:	$(RPATH)")
