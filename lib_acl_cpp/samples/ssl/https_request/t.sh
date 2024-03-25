#!/bin/sh

os=$(echo `uname -s`)
if [ $os == 'Darwin' ]; then
	./https_request -f "../libmbedtls_all.dylib" -s www.sina.com.cn:443 -S
	echo ""
	./https_request -f "../libmbedtls_all.dylib" -s www.baidu.com:443 -S
	echo ""
	./https_request -f "../libmbedtls_all.dylib" -s echo.websocket.org:443 -S
else
	./https_request -f "../libmbedtls_all.so" -s www.sina.com.cn:443 -S
	echo ""
	./https_request -f "../libmbedtls_all.so" -s www.baidu.com:443 -S
	echo ""
	./https_request -f "../libmbedtls_all.so" -s echo.websocket.org:443 -S
fi

 # ./https_request -f "/usr/local/lib64/libcrypto.so;/usr/local/lib64/libssl.so" -s www.baidu.com:443 -H www.baidu.com -U /  -n 1 -S
