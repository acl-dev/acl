#!/bin/sh
os=$(echo `uname -s`)
if [ $os == "Darwin" ]; then
	./server -s "0.0.0.0|2443" -r 2 -o 2 -t openssl -L "/usr/local/lib/libcrypto.dylib; /usr/local/lib/libssl.dylib" -c ../ssl_crt.pem -k ../ssl_key.pem
elif [ $os == "Linux" ]; then
	./server -s "0.0.0.0|2443" -r 2 -o 2 -t openssl -L "/usr/local/lib64/libcrypto.so; /usr/local/lib64/libssl.so" -c ../ssl_crt.pem -k ../ssl_key.pem
else
	echo "unknown os=$os"
fi
