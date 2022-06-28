base_path = ../../..
include ../../Makefile.in
PROG = mysql_manager
ifneq ($(findstring FreeBSD, $(UNIXNAME)), FreeBSD)
	EXTLIBS += -ldl
endif
