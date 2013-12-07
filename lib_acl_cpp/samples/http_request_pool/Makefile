include ../Makefile.in
#LDFLAGS += -lz -liconv
#Path for SunOS
ifeq ($(findstring SunOS, $(UNIXNAME)), SunOS)
	LDFLAGS += -lz -liconv
else
	LDFLAGS += -lz
endif
PROG = http_request_pool
