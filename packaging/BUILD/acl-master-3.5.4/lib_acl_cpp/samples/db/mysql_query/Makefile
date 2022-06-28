base_path = ../../..
include ../../Makefile.in
PROG = mysql_query
ifneq ($(findstring FreeBSD, $(UNIXNAME)), FreeBSD)
	EXTLIBS += -ldl
endif
