#!/bin/sh

./dns  -n "m.mo.mm,aaa.aaa.aaa,abc.abc.a,www.ccc.ccc,www.263.net,www.baidu.com"  -a 10.202.72.118 -p 53

echo ""
echo ""
echo ""
sleep 1

./dns  -n "m.mo.mm,aaa.aaa.aaa,abc.abc.a,www.ccc.ccc,www.263.net,www.baidu.com"
