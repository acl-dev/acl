#!/bin/sh

./dns -n "www.qiyi.com"

./dns  -n "m.mo.mm,aaa.aaa.aaa,abc.abc.a,www.ccc.ccc,www.263.net,www.baidu.com"  -a 8.8.8.8 -p 53

echo ""
echo ""
echo ""
sleep 1

./dns  -n "m.mo.mm,aaa.aaa.aaa,abc.abc.a,www.ccc.ccc,www.263.net,www.baidu.com"
