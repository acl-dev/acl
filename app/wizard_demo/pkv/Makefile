
# make BUILD_WITH_C11=YES; or make
#
ifeq ($(BUILD_WITH_C11), YES)
    BUILD_ARGS = -DBUILD_WITH_C11=YES
else
    BUILD_ARGS = -DBUILD_WITH_C11=NO
endif

BUILD_ARGS += -DCMAKE_VERBOSE_MAKEFILE=ON
#BUILD_ARGS += -DHAS_ROCKSDB=YES
#BUILD_ARGS += -DHAS_WT=YES
BUILD_ARGS += -DHAS_JEMALLOC=YES

all:
	@(mkdir -p build; cd build; cmake ${BUILD_ARGS} ..; make -j 4)

clean cl:
	@(rm -rf build pkv)

rebuild rb: cl all
