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

# ./https_client -f "/usr/local/lib64/libcrypto.so;/usr/local/lib64/libssl.so" -s www.163.com:443 -H www.163.com -S -n 1
