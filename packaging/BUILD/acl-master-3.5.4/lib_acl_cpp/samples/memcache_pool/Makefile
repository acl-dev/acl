#UTIL = $(wildcard ../*.cpp)
#LDFLAGS += -lz -liconv
base_path = ../..
include ../Makefile.in
#Path for SunOS
ifeq ($(findstring SunOS, $(UNIXNAME)), SunOS)
	LDFLAGS += -lz -liconv
else
	LDFLAGS += -lz
endif
ifeq ($(findstring FreeBSD, $(UNIXNAME)), FreeBSD)
	EXTLIBS += -L/usr/local/lib -liconv
endif
ifeq ($(findstring Darwin, $(UNIXNAME)), Darwin)
	EXTLIBS += -L/usr/lib -liconv
endif
CFLAGS += -I../
PROG = memcache_pool
