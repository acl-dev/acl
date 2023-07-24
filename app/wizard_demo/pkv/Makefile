all:
	@(mkdir -p build; cd build; cmake ..; make -j 4)

clean cl:
	@(rm -rf build pkv)

rebuild rb: cl all
