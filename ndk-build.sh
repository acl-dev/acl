#!/bin/sh

cd lib_acl/
./ndk-build.sh
cd ..

cd lib_protocol/
./ndk-build.sh
cd ..

cd lib_acl_cpp/
./ndk-build.sh
cd ..
