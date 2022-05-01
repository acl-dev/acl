OSTYPE = $(shell uname -s)
DBFILE = libsqlite3.so

ifeq ($(findstring Darwin, $(OSTYPE)), Darwin)
	DBFILE = libsqlite3.dylib
endif

build:
	rm -rf sqlite-autoconf-3230100
	tar -zvxf sqlite-autoconf-3230100.tar.gz
	(cd sqlite-autoconf-3230100; ./configure --enable-static=no; make; cp ./.libs/$(DBFILE) ../../)

copy:
	cd sqlite-autoconf-3230100; cp ./.libs/$(DBFILE) ../../

