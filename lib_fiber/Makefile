.PHONEY   = all lib_fiber clean cl rebuild rb
DESTDIR   =
PREFIX    = /usr
BIN_PATH  = $(DESTDIR)/$(PREFIX)/bin/
LIB_ACL   = $(DESTDIR)/$(PREFIX)/lib
INC_ACL   = $(DESTDIR)/$(PREFIX)/include/acl-lib

lib_fiber:
	@(cd c; make)
	@(cd cpp; make)

all: lib_fiber
	@(cd samples; make)

clean cl:
	@(cd c; make clean)
	@(cd cpp; make clean)
	@(cd samples; make clean)


packinstall:
	@(echo "")
	@(echo "begin copy file...")
	$(shell mkdir -p $(INC_ACL)/fiber)
	$(shell mkdir -p $(LIB_ACL)/)
	cp -f c/include/fiber/lib_fiber.h $(INC_ACL)/fiber/
	cp -f cpp/include/fiber/*.hpp $(INC_ACL)/fiber/
	cp -f lib/libfiber.a lib/libfiber_cpp.a $(LIB_ACL)/

rebuild rb: cl all
