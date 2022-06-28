#!/bin/sh

cd lib_acl/
./ndk-build-r9d.sh
cd ..

cd lib_protocol/
./ndk-build-r9d.sh
cd ..

cd lib_acl_cpp/
./ndk-build-r9d.sh
cd ..
