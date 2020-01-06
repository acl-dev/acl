#!/bin/sh
os=$(echo `uname -s`)
if [ $os == 'Darwin' ]; then
	./ssl_client -d "../libmbedcrypto.dylib;../libmbedx509.dylib;../libmbedtls.dylib" -s 127.0.0.1:9800 -c 10 -n 10000
else
	./ssl_client -d "../libmbedcrypto.so;../libmbedx509.so;../libmbedtls.so" -s 127.0.0.1:9800 -c 10 -n 10000
fi
