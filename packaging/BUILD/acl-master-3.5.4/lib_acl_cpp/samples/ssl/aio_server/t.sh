#!/bin/sh

os=$(echo `uname -s`)
if [ $os == 'Darwin' ]; then
	./aio_server -d "../libmbedcrypto.dylib;../libmbedx509.dylib;../libmbedtls.dylib" -K ../ssl_key.pem -C ../ssl_crt.pem
else
	./aio_server -d "../libmbedcrypto.so;../libmbedx509.so;../libmbedtls.so" -K ../ssl_key.pem -C ../ssl_crt.pem
fi
