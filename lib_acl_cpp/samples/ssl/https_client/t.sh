#!/bin/sh

os=$(echo `uname -s`)
if [ $os == 'Darwin' ]; then
	./https_client -f ../libmbedtls_all.dylib -s www.sina.com.cn:443 -S
	./https_client -f ../libmbedtls_all.dylib -s www.baidu.com:443 -S
	./https_client -f ../libmbedtls_all.dylib -s echo.websocket.org:443 -S
else
	./https_client -f ../libmbedtls_all.so -s www.sina.com.cn:443 -S
	./https_client -f ../libmbedtls_all.so -s www.baidu.com:443 -S
	./https_client -f ../libmbedtls_all.so -s echo.websocket.org:443 -S
fi
