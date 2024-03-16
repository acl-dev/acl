#!/bin/sh
export ENV_CC=cosmocc
export ENV_CPP=cosmoc++

cd lib_acl; make -j 4;
cd samples; make;
cd ../..

cd lib_protocol; make -j 4;
cd samples; make;
cd ../..

cd lib_acl_cpp; make -j 10
cd samples; make;
cd ../..

