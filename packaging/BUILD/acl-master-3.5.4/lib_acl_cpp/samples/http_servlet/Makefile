include ../Makefile.in
#EXTLIBS = -lpolarssl
#EXTLIBS = -lz -liconv
#EXTLIBS = -lz

ifeq ($(findstring FreeBSD, $(UNIXNAME)), FreeBSD)
	EXTLIBS += -L/usr/local/lib -liconv
endif
ifeq ($(findstring Darwin, $(UNIXNAME)), Darwin)
	EXTLIBS += -L/usr/lib -liconv
endif
ifeq ($(findstring SunOS, $(UNIXNAME)), SunOS)
	EXTLIBS += -lz -liconv
else
	EXTLIBS += -lz
endif
PROG = http_servlet
