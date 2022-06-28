#!/bin/sh

os=$(echo `uname -s`)
if [ $os == "Darwin" ]; then
	./client -l "../libmbedcrypto.dylib;../libmbedx509.dylib;../libmbedtls.dylib" -c 200 -n 10000
else
	./client -l "../libmbedcrypto.so;../libmbedx509.so;../libmbedtls.so" -c 200 -n 10000
fi
