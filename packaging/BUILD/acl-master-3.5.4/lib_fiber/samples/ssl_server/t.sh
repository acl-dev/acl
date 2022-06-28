#!/bin/sh

os=$(echo `uname -s`)
if [ $os == "Darwin" ]; then
	./server -l "../libmbedcrypto.dylib;../libmbedx509.dylib;../libmbedtls.dylib" -c ./ssl_crt.pem -k ./ssl_key.pem
else
	./server -l "../libmbedcrypto.so;../libmbedx509.so;../libmbedtls.so" -c ./ssl_crt.pem -k ./ssl_key.pem
fi
