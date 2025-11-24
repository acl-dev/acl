#!/bin/sh

#cd acl_one
#gradle clean
#cd -
#cd http
#gradle clean
#cd -

gradle clean

rm -rf build
rm -rf .gradle
rm -rf acl_one/build
rm -rf acl_one/.cxx
rm -rf http/build
rm -rf http/.cxx
