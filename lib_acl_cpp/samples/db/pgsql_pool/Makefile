base_path = ../../..
include ../../Makefile.in
PROG = pgsql_pool
ifneq ($(findstring FreeBSD, $(UNIXNAME)), FreeBSD)
	EXTLIBS += -ldl
endif
