UTIL = $(wildcard ../*.cpp)
include ../Makefile.in
#LDFLAGS += -lz -liconv
#Path for SunOS
ifeq ($(findstring SunOS, $(UNIXNAME)), SunOS)
	LDFLAGS += -lz -liconv
else
	LDFLAGS += -lz
endif
CFLAGS += -I../
PROG = memcache_pool
