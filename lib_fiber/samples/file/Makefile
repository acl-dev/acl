include ../Makefile.in
ifeq ($(findstring Linux, $(UNIXNAME)), Linux)
	has_io_uring=$(HAS_IO_URING)
	ifeq ($(has_io_uring), yes)
		SYSLIB += -luring
	endif
endif
PROG = file
