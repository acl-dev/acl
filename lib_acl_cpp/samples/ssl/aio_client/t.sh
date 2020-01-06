#!/bin/sh

os=$(echo `uname -s`)
if [ $os == 'Darwin' ]; then
	./aio_client -d "../libmbedcrypto.dylib;../libmbedx509.dylib;../libmbedtls.dylib" -c 10 -n 10000 -S -l 127.0.0.1:9800
else
	./aio_client -d "../libmbedcrypto.so;../libmbedx509.so;../libmbedtls.so" -c 10 -n 10000 -S -l 127.0.0.1:9800
fi
