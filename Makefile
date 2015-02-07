SHELL = /bin/sh
#OSNAME = $(shell uname -sm)
#OSTYPE = $(shell uname -p)
OSNAME = $(shell uname -a)
OSTYPE = $(shell uname -a)

LIB_PATH = ./dist/lib
ACL_LIB = $(LIB_PATH)
PROTO_LIB = $(LIB_PATH)
DICT_LIB = $(LIB_PATH)
TLS_LIB = $(LIB_PATH)

INC_PATH = ./dist/include
ACL_INC = $(INC_PATH)/acl
PROTO_INC = $(INC_PATH)/protocol
DICT_INC = $(INC_PATH)/dict
TLS_INC = $(INC_PATH)/tls

RPATH =
DATE_NOW = 20`date +%y`.`date +%m`.`date +%d`
MAKE_ARGS =

ifeq ($(findstring FreeBSD, $(OSNAME)), FreeBSD)
	RPATH = freebsd
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
endif
ifeq ($(findstring SunOS, $(OSNAME)), SunOS)
	ifeq ($(findstring i386, $(OSTYPE)), i386)
		RPATH = sunos_x86
	endif
endif
##############################################################################

.PHONY = check help all clean install uninstall uninstall_all build_bin build_src
VERSION = 3.1.0

help:
	@(echo "usage: make help|all|clean|install|uninstall|uninstall_all|build_bin|build_src")
all:
	@(cd lib_acl; make $(MAKE_ARGS))
	@(cd lib_protocol; make $(MAKE_ARGS))
	@(cd lib_acl_cpp; make $(MAKE_ARGS))
	@(cd lib_rpc; make $(MAKE_ARGS))
	@(cd unit_test; make $(MAKE_ARGS))
	@(cd lib_acl/samples; make)
	@(cd lib_protocol/samples; make)
	@(cd lib_acl_cpp/samples; make)
#	@(cd lib_dict; make $(MAKE_ARGS))
#	@(cd lib_tls; make $(MAKE_ARGS))
clean:
	@(cd lib_acl; make clean)
	@(cd lib_protocol; make clean)
	@(cd lib_acl_cpp; make clean)
	@(cd lib_rpc; make clean)
	@(cd unit_test; make clean)
	@(cd lib_acl/samples; make clean)
	@(cd lib_protocol/samples; make clean)
#	@(cd lib_dict; make clean)
#	@(cd lib_tls; make clean)

install:
	@(echo "")
	@(echo "begin copy file...")
	cp -f lib_acl/master/acl_master ./dist/master/libexec/$(RPATH)/
	cp -f lib_acl/lib/lib_acl.a $(ACL_LIB)/$(RPATH)/
	cp -Rf lib_acl/include/* $(ACL_INC)/
	cp -f lib_protocol/lib/lib_protocol.a $(PROTO_LIB)/$(RPATH)/
	cp -Rf lib_protocol/include/* $(PROTO_INC)/
	cp -f lib_acl_cpp/lib/lib_acl_cpp.a $(ACL_LIB)/$(RPATH)/
	cp -Rf lib_acl_cpp/include/acl_cpp/* $(INC_PATH)/acl_cpp/
#	cp -f lib_dict/lib/lib_dict.a $(DICT_LIB)/$(RPATH)/
#	cp -Rf lib_dict/include/* $(DICT_INC)/
#	cp -f lib_tls/lib/lib_tls.a $(TLS_LIB)/$(RPATH)/
#	cp -Rf lib_tls/include/* $(TLS_INC)/
uninstall:
	@(echo "")
	@(echo "begin remove file...")
	rm -f ./dist/master/libexec/$(RPATH)/*
	rm -f $(ACL_LIB)/$(RPATH)/lib_acl.a
	rm -Rf $(ACL_INC)/*
	rm -f $(PROTO_LIB)/$(RPATH)/lib_protocol.a
	rm -Rf $(PROTO_INC)/*
	rm -f $(ACL_LIB)/$(RPATH)/lib_acl_cpp.a
	rm -Rf $(INC_PATH)/acl_cpp/*
	rm -f $(DICT_LIB)/$(RPATH)/lib_dict.a
	rm -Rf $(DICT_INC)/*
	rm -f $(TLS_LIB)/$(RPATH)/lib_tls.a
	rm -Rf $(TLS_INC)/*
uninstall_all:
	@(echo "")
	@(echo "begin remove all dist files ...")
	rm -Rf $(ACL_INC)/*
	rm -Rf $(PROTO_INC)/*
	rm -Rf $(DICT_INC)/*
	rm -Rf $(TLS_INC)/*
	rm -f ./dist/master/libexec/linux32/*
	rm -f ./dist/master/libexec/linux64/*
	rm -f ./dist/master/libexec/sunos_x86/*
	rm -f ./dist/master/libexec/freebsd/*
	rm -f $(ACL_LIB)/linux32/*.a
	rm -f $(ACL_LIB)/linux64/*.a
	rm -f $(ACL_LIB)/sunos_x86/*.a
	rm -f $(ACL_LIB)/freebsd/*.a
	rm -f $(ACL_LIB)/win32/*.lib
	rm -f $(ACL_LIB)/win32/*.dll
	rm -f $(PROTO_LIB)/linux32/*.a
	rm -f $(PROTO_LIB)/linux64/*.a
	rm -f $(PROTO_LIB)/sunos_x86/*.a
	rm -f $(PROTO_LIB)/freebsd/*.a
	rm -f $(PROTO_LIB)/win32/*.lib
	rm -f $(PROTO_LIB)/win32/*.dll
	rm -f $(DICT_LIB)/linux32/*.a
	rm -f $(DICT_LIB)/linux64/*.a
	rm -f $(DICT_LIB)/sunos_x86/*.a
	rm -f $(DICT_LIB)/freebsd/*.a
	rm -f $(DICT_LIB)/win32/*.lib
	rm -f $(DICT_LIB)/win32/*.dll
	rm -f $(TLS_LIB)/linux32/*.a
	rm -f $(TLS_LIB)/linux64/*.a
	rm -f $(TLS_LIB)/sunos_x86/*.a
	rm -f $(TLS_LIB)/freebsd/*.a
	rm -f $(TLS_LIB)/win32/*.lib
	rm -f $(TLS_LIB)/win32/*.dll
	rm -f win32_build/vc/lib_acl/*.map
	rm -f win32_build/vc/lib_acl/*.ilk
	rm -f win32_build/vc/lib_protocol/*.map
	rm -f win32_build/vc/lib_protocol/*.ilk
build_bin:
	@(echo "please waiting ...")
	@(echo "begin building bin release...")
	@(rm -rf acl.bin)
	@(echo "copy files ...")
	@(cp -R dist acl.bin)
	@(cp lib_acl/changes.txt acl.bin/include/acl/)
	@(cp lib_protocol/changes.txt acl.bin/include/protocol/)
	@(cp lib_dict/changes.txt acl.bin/include/dict/)
	@(cp lib_tls/changes.txt acl.bin/include/tls/)
	@(cp changes.txt acl.bin/)
	@(echo "make tar package ...")
	@(tar -cf acl.bin.tar acl.bin)
	@(echo "make gzip package ...")
	@(gzip -c acl.bin.tar > acl.bin.tgz)
	@(rm acl.bin.tar)
	@(rm -rf acl.bin)
	@(echo "move acl.bin.tgz to ../acl$(VERSION).bin.$(DATE_NOW).tgz")
	@(mv acl.bin.tgz ../acl$(VERSION).bin.$(DATE_NOW).tgz)
#	@(echo "move acl.bin.tgz to ../acl$(VERSION).bin.tgz")
#	@(mv acl.bin.tgz ../acl$(VERSION).bin.tgz)
build_src: clean uninstall_all
	@(echo "begin building src release...")
	@(rm -rf acl)
	@(echo "copy files ...")
	@(mkdir acl)
	@(cp -R dist acl/)
	@(cp -R lib_acl acl/)
	@(cp -R lib_protocol acl/)
	@(cp -R lib_dict acl/)
	@(cp -R lib_tls acl/)
	@(cp -R samples acl/)
	@(cp -R unit_test acl/)
	@(cp -R win32_build acl/)
	@(cp -R lib acl/)
	@(cp -R doc acl/)
	@(cp Makefile acl/)
	@(cp changes.txt acl/)
	@(cp readme.txt acl/)
	@(cp Doxyfile acl/)
	@(echo "make tar package ...")
	@(tar -cf acl.src.tar acl)
	@(echo "make gzip package ...")
	@(gzip -c acl.src.tar > acl.src.tgz)
	@(rm acl.src.tar)
	@(rm -rf acl)
	@(echo "move acl.src.tgz to ../acl$(VERSION).src.$(DATE_NOW).tgz")
	@(mv acl.src.tgz ../acl$(VERSION).src.$(DATE_NOW).tgz)
#	@(echo "move acl.src.tgz to ../acl$(VERSION).src.tgz")
#	@(mv acl.src.tgz ../acl$(VERSION).src.tgz)
check:
	echo "TYPE=$(OSTYPE), OSNAME=$(OSNAME), RPATH=$(RPATH)"
