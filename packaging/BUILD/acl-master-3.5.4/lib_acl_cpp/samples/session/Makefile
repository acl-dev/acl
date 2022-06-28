base_path = ../..
CFLAGS += -I../

include ../Makefile.in
ifeq ($(findstring SunOS, $(UNIXNAME)), SunOS)
	EXTLIBS = -liconv
endif
ifeq ($(findstring FreeBSD, $(UNIXNAME)), FreeBSD)
	EXTLIBS += -L/usr/local/lib -liconv
endif
ifeq ($(findstring Darwin, $(UNIXNAME)), Darwin)
	EXTLIBS += -L/usr/lib -liconv
endif
PROG = session
