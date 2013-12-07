include ../Makefile.in
#EXTLIBS = -lpolarssl
#EXTLIBS = -lz -liconv
#Path for SunOS
ifeq ($(findstring SunOS, $(UNIXNAME)), SunOS)
	EXTLIBS = -lz -liconv
else
	EXTLIBS = -lz
endif
PROG = http_servlet
