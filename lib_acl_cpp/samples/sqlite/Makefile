include ../Makefile.in
PROG = sqlite
ifneq ($(findstring FreeBSD, $(UNIXNAME)), FreeBSD)
	EXTLIBS += -ldl
endif
#EXTLIBS += -lsqlite3 -lmysqlclient
