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
