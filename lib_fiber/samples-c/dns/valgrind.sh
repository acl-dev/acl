#!/bin/sh

#valgrind --tool=memcheck --leak-check=yes --leak-check=full --show-reachable=yes --max-stackframe=3426305034400000 -v ./fiber -n 10 -m 20
valgrind --tool=memcheck --leak-check=yes --leak-check=full --show-reachable=yes -v ./dns  -n "m.mo.mm,aaa.aaa.aaa,abc.abc.a,www.ccc.ccc,www.263.net,www.baidu.com"

sleep 2

valgrind --tool=memcheck --leak-check=yes --leak-check=full --show-reachable=yes -v ./dns  -n "m.mo.mm,aaa.aaa.aaa,abc.abc.a,www.ccc.ccc,www.263.net,www.baidu.com"
