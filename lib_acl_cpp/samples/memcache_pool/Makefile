#UTIL = $(wildcard ../*.cpp)
#LDFLAGS += -lz -liconv
#Path for SunOS
ifeq ($(findstring SunOS, $(UNIXNAME)), SunOS)
	LDFLAGS += -lz -liconv
else
	LDFLAGS += -lz
endif
ifeq ($(findstring FreeBSD, $(UNIXNAME)), FreeBSD)
	EXTLIBS += -L/usr/local/lib -liconv
endif
base_path = ../..
CFLAGS += -I../
PROG = memcache_pool
include ../Makefile.in
