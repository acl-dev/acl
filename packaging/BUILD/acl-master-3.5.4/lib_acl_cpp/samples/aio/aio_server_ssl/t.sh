#!/bin/sh

os=$(echo `uname -s`)
if [ $os == "Darwin" ]; then
	./aio_server_ssl -k -S "../libmbedcrypto.dylib;../libmbedx509.dylib;../libmbedtls.dylib" -K ../../ssl/ssl_key.pem -C ../../ssl/ssl_crt.pem 
else
	./aio_server_ssl -k -S "../libmbedcrypto.so;../libmbedx509.so;../libmbedtls.so" -K ../../ssl/ssl_key.pem -C ../../ssl/ssl_crt.pem 
fi
