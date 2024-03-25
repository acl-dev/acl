#!/bin/sh

os=$(echo `uname -s`)
if [ $os == "Darwin" ]; then
	./server -l "../libmbedcrypto.dylib;../libmbedx509.dylib;../libmbedtls.dylib" -c ./ssl_crt.pem -k ./ssl_key.pem
else
	./server -l "/usr/local/lib64/libcrypto.so; /usr/local/lib64/libssl.so" -c ./ssl_crt.pem -k ./ssl_key.pem
fi
