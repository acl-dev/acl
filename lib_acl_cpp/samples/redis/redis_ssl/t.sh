#!/bin/sh

os=$(echo `uname -s`)
if [ $os == 'Darwin' ]; then
	./redis -s 127.0.0.1:6379 -L ../libpolarssl.dylib  -c -C ca.crt -S redis.crt -K redis.key  -a set -n 1000
	echo ""
	./redis -s 127.0.0.1:6379 -L ../libmbedtls_all.dylib  -c -C ca.crt -S redis.crt -K redis.key  -a get -n 1000
else
	./redis -s 127.0.0.1:6379 -L ../libpolarssl.so  -c -C ca.crt -S redis.crt -K redis.key  -a set -n 1000
	echo ""
	./redis -s 127.0.0.1:6379 -L ../libmbedtls_all.so  -c -C ca.crt -S redis.crt -K redis.key  -a get -n 1000
fi
