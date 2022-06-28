
all:
	@(../gson -d benchmark; cd benchmark; make; ./test1)
	@(../gson -d test; cd test; make; ./test)
	@(../gson -d test0; cd test0; make; ./test)
	@(../gson -d test1; cd test1; make; ./test)
	@(../gson -d test2; cd test2; make; ./test)
	@(../gson -d test3; cd test3; make; ./test)
	@(../gson -d test4; cd test4; make; ./test)
	@(../gson -d test5; cd test5; make; ./test)
clean:
	@(cd benchmark; make clean)
	@(cd test; make clean)
	@(cd test0; make clean)
	@(cd test1; make clean)
	@(cd test2; make clean)
	@(cd test3; make clean)
	@(cd test4; make clean)
	@(cd test5; make clean)
