include ../Makefile.in
ifeq ($(findstring FreeBSD, $(UNIXNAME)), FreeBSD)
	EXTLIBS += -L/usr/local/lib -liconv
endif
CFLAGS += -std=c++11
PROG = test
