#!/bin/sh

os=$(echo `uname -s`)
if [ $os == "Darwin" ]; then
	./httpc -l "../libmbedcrypto.dylib;../libmbedx509.dylib;../libmbedtls.dylib" -c 200 -n 10000
else
	./httpc -l "../libmbedcrypto.so;../libmbedx509.so;../libmbedtls.so" -c 200 -n 10000
fi
