
# make BUILD_WITH_C11=YES; or make
#
ifeq ($(BUILD_WITH_C11), YES)
    BUILD_ARGS = -DBUILD_WITH_C11=YES
else
    BUILD_ARGS = -DBUILD_WITH_C11=NO
endif

BUILD_ARGS += -DCMAKE_VERBOSE_MAKEFILE=ON

all:
	@(mkdir -p build; cd build; cmake ${BUILD_ARGS} ..; make -j 4)

clean cl:
	@(rm -rf build pkv)

rebuild rb: cl all
