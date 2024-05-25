#!/bin/sh
os=$(echo `uname -s`)
if [ $os == "Darwin" ]; then
	./client -s "127.0.0.1|2443" -l "/usr/local/lib/libcrypto.dylib;/usr/local/lib/libssl.dylib" -c 1 -n 1
elif [ $os == "Linux" ]; then
	./client -s "127.0.0.1|2443" -l "/usr/local/lib64/libcrypto.so;/usr/local/lib64/libssl.so" -c 1 -n 1
else
	echo "unknown os=$os"
fi
