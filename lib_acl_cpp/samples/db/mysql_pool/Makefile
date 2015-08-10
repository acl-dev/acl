base_path = ../../..
include ../../Makefile.in
PROG = mysql_pool
ifneq ($(findstring FreeBSD, $(UNIXNAME)), FreeBSD)
	EXTLIBS += -ldl
endif
