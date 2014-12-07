base_path = ../../..
include ../../Makefile.in
#EXTLIBS = -lz -liconv
#Path for SunOS
ifeq ($(findstring SunOS, $(UNIXNAME)), SunOS)
	EXTLIBS = -lz -liconv
else
	EXTLIBS = -lz
endif
PROG = master_http_threads2
