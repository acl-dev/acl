#!/bin/sh
os=$(echo `uname -s`)
if [ $os == "Darwin" ]; then
	./client -s "0.0.0.0|2443" -L "../libmbedcrypto.dylib;../libmbedx509.dylib;../libmbedtls.dylib" -c 10 -n 100
elif [ $os == "Linux" ]; then
	./client -s "0.0.0.0|2443" -L "../libmbedcrypto.so;../libmbedx509.so;../libmbedtls.so" -c 10 -n 100
else
	echo "unknown os=$os"
fi
