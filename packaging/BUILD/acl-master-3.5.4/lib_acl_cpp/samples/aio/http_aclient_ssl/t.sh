#!/bin/sh

./http_aclient_ssl -s www.iqiyi.com:443 -H www.iqiyi.com  -S ./libpolarssl.so -N 8.8.8.8:53
echo ""
echo ""

sleep 2
./http_aclient_ssl -s www.baidu.com:443 -H www.baidu.com  -S ./libpolarssl.so -N 8.8.8.8:53
echo ""
echo ""

sleep 2
./http_aclient_ssl -s www.iqiyi.com:80 -H www.iqiyi.com -N 8.8.8.8:53
echo ""
echo ""

sleep 2
./http_aclient_ssl -s www.baidu.com:80 -H www.baidu.com -N 8.8.8.8:53
echo ""
echo ""

sleep 2
./http_aclient_ssl -s www.baidu.com:80 -H www.baidu.com -D -Z -U -N 8.8.8.8:53
echo ""
echo ""

sleep 2
./http_aclient_ssl -s www.baidu.com:443 -H www.baidu.com  -D -Z -U -S ./libpolarssl.so -N 8.8.8.8:53
echo ""
echo ""

./http_aclient_ssl -s www.iqiyi.com:443 -H www.iqiyi.com  -D -Z -U -S ./libpolarssl.so -N 8.8.8.8:53
echo ""
echo ""

sleep 2
./http_aclient_ssl  -s 127.0.0.1:8885 -D -W
echo ""
echo ""

os=$(echo `uname -s`)
if [ $os == "Darwin" ]; then
	./http_aclient_ssl -s echo.websocket.org:443 -H echo.websocket.org -D -Z -U \
		-S "../libmbedcrypto.dylib;../libmbedx509.dylib;../libmbedtls.dylib" -N 8.8.8.8:53
else
	./http_aclient_ssl -s echo.websocket.org:443 -H echo.websocket.org -D -Z -U \
		-S "../libmbedcrypto.so;../libmbedx509.so;../libmbedtls.so" -N 8.8.8.8:53
fi
echo ""
echo ""
