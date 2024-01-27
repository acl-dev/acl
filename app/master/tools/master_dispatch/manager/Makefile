include ./Makefile.in
ifeq ($(findstring FreeBSD, $(UNIXNAME)), FreeBSD)
	EXTLIBS += -L/usr/local/lib -liconv
endif
CFLAGS += -std=c++98
PROG = dispatch_manager
