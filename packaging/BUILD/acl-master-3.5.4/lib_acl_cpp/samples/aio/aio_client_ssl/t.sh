#!/bin/sh

os=$(echo `uname -s`)
if [ $os == "Darwin" ]; then
	./aio_client_ssl -c 100 -n 10000 -k -S "../libmbedcrypto.dylib;../libmbedx509.dylib;../libmbedtls.dylib"
else
	./aio_client_ssl -c 100 -n 10000 -k -S "../libmbedcrypto.so;../libmbedx509.so;../libmbedtls.so"
fi
