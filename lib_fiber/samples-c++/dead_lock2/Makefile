# Don't build the sample on MacOS because libunwind don't support it.
include ./Makefile.in
ifeq ($(findstring Linux, $(UNIXNAME)), Linux)
	SYSLIB += -lunwind -lunwind-x86_64
endif
PROG = dead_lock
