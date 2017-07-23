include ./Makefile.in
ifeq ($(findstring FreeBSD, $(UNIXNAME)), FreeBSD)
	EXTLIBS += -L/usr/local/lib -liconv
endif
PROG = dispatch_manager
