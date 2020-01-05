#!/bin/sh

os=$(echo `uname -s`)
if [ $os == 'Darwin' ]; then
	./aio_server -d ../libmbedtls_all.dylib -K ../ssl_key.pem -C ../ssl_crt.pem
else
	./aio_server -d ../libmbedtls_all.so -K ../ssl_key.pem -C ../ssl_crt.pem
fi
