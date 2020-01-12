#!/bin/sh
os=$(echo `uname -s`)
if [ $os == "Darwin" ]; then
	./server -s "0.0.0.0|2443" -L "../libmbedcrypto.dylib;../libmbedx509.dylib;../libmbedtls.dylib" -c ../ssl_crt.pem -k ../ssl_key.pem
elif [ $os == "Linux" ]; then
	./server -s "0.0.0.0|2443" -L "../libmbedcrypto.so;../libmbedx509.so;../libmbedtls.so" -c ../ssl_crt.pem -k ../ssl_key.pem
else
	echo "unknown os=$os"
fi
