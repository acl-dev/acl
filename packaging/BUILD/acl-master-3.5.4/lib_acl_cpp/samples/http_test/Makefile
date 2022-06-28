include ../Makefile.in
PROG = http_test
ifeq ($(findstring SunOS, $(UNIXNAME)), SunOS)
	EXTLIBS += -liconv
endif
ifeq ($(findstring FreeBSD, $(UNIXNAME)), FreeBSD)
	EXTLIBS = -L/usr/local/lib -liconv
endif
ifeq ($(findstring Darwin, $(UNIXNAME)), Darwin)
	EXTLIBS += -L/usr/lib -liconv
endif
EXTLIBS += -lz
