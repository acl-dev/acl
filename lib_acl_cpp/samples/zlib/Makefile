include ../Makefile.in
PROG = zlib
#EXTLIBS += -lz -liconv
#Path for SunOS
ifeq ($(findstring SunOS, $(UNIXNAME)), SunOS)
	EXTLIBS = -lz -liconv
endif
EXTLIBS += -lz
