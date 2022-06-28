#!/bin/sh

valgrind --show-reachable=yes --tool=memcheck --leak-check=yes -v ./http_aclient_ssl -s www.iqiyi.com:443 -H www.iqiyi.com  -S ./libpolarssl.so
echo ""
echo ""

sleep 2
valgrind --show-reachable=yes --tool=memcheck --leak-check=yes -v ./http_aclient_ssl -s www.baidu.com:443 -H www.baidu.com  -S ./libpolarssl.so
echo ""
echo ""

sleep 2
valgrind --show-reachable=yes --tool=memcheck --leak-check=yes -v ./http_aclient_ssl -s www.iqiyi.com:80 -H www.iqiyi.com
echo ""
echo ""

sleep 2
valgrind --show-reachable=yes --tool=memcheck --leak-check=yes -v ./http_aclient_ssl -s www.baidu.com:80 -H www.baidu.com
echo ""
echo ""

sleep 2
valgrind --show-reachable=yes --tool=memcheck --leak-check=yes -v ./http_aclient_ssl -s www.baidu.com:80 -H www.baidu.com -D -Z -U
echo ""
echo ""

sleep 2
valgrind --show-reachable=yes --tool=memcheck --leak-check=yes -v ./http_aclient_ssl -s www.baidu.com:443 -H www.baidu.com  -D -Z -U -S ./libpolarssl.so
echo ""
echo ""

valgrind --show-reachable=yes --tool=memcheck --leak-check=yes -v ./http_aclient_ssl -s www.iqiyi.com:443 -H www.iqiyi.com  -D -Z -U -S ./libpolarssl.so
echo ""
echo ""

sleep 2
valgrind --show-reachable=yes --tool=memcheck --leak-check=yes -v ./http_aclient_ssl  -s 127.0.0.1:8885 -D -W
echo ""
echo ""
