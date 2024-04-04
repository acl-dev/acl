#!/bin/sh
mkdir -p build
cd build
#cmake -DCMAKE_INSTALL_PREFIX=/opt/soft/acl -DACL_BUILD_SHARED=YES -DACL_BUILD_SHARED_ONE=YES ..
cmake -DCMAKE_INSTALL_PREFIX=/opt/soft/acl -DACL_BUILD_SHARED=YES ..
make -j 4
make install
