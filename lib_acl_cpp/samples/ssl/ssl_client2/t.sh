#!/bin/sh
os=$(echo `uname -s`)
if [ $os == 'Darwin' ]; then
	./ssl_client -d ../libmbedtls_all.dylib -s 127.0.0.1:9800 -c 10 -n 10000
else
	./ssl_client -d ../libmbedtls_all.so -s 127.0.0.1:9800 -c 10 -n 10000
fi
