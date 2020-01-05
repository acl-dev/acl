#!/bin/sh

os=$(echo `uname -s`)
if [ $os == 'Darwin' ]; then
	./aio_client -d ../libmbedtls_all.dylib -c 10 -n 10000 -S -l 127.0.0.1:9800
else
	./aio_client -d ../libmbedtls_all.so -c 10 -n 10000 -S -l 127.0.0.1:9800
fi
