include ../Makefile.in
#LDFLAGS += -lz -liconv
#Path for SunOS
ifeq ($(findstring SunOS, $(UNIXNAME)), SunOS)
	LDFLAGS += -lz -liconv
else
	LDFLAGS += -lz
endif
ifeq ($(findstring Darwin, $(UNIXNAME)), Darwin)
	EXTLIBS += -L/usr/lib -liconv
endif
PROG = connect_manager
