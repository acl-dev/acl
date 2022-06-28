#!/bin/sh

os=$(echo `uname -s`)
if [ $os == 'Darwin' ]; then
	./https_client -f "../libmbedcrypto.dylib;../libmbedx509.dylib;../libmbedtls.dylib" -s www.sina.com.cn:443 -S
	echo ""
	./https_client -f "../libmbedcrypto.dylib;../libmbedx509.dylib;../libmbedtls.dylib" -s www.baidu.com:443 -S
	echo ""
	./https_client -f "../libmbedcrypto.dylib;../libmbedx509.dylib;../libmbedtls.dylib" -s echo.websocket.org:443 -S
else
	./https_client -f "../libmbedcrypto.so;../libmbedx509.so;../libmbedtls.so" -s www.sina.com.cn:443 -S
	echo ""
	./https_client -f "../libmbedcrypto.so;../libmbedx509.so;../libmbedtls.so" -s www.baidu.com:443 -S
	echo ""
	./https_client -f "../libmbedcrypto.so;../libmbedx509.so;../libmbedtls.so" -s echo.websocket.org:443 -S
fi
