base_path = ../../..
include ../../Makefile.in
#EXTLIBS = -lz -liconv
#Path for SunOS
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
PROG = master_http_threads2
