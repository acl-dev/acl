#!/bin/sh
mkdir -p build
cd build
cmake -DCMAKE_INSTALL_PREFIX=/opt/soft/acl -DACL_BUILD_SHARED=YES ..
#cmake -DACL_BUILD_SHARED=YES ..
make -j 4
make install
