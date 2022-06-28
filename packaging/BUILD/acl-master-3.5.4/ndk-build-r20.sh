#!/bin/sh

cd lib_acl/
./ndk-build-r20.sh
cd ..

cd lib_protocol/
./ndk-build-r20.sh
cd ..

cd lib_acl_cpp/
./ndk-build-r20.sh
cd ..
