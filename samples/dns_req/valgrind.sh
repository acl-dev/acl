#!/bin/sh

#valgrind --tool=memcheck --leak-check=yes --show-reachable=yes -v ./dns_req -s 60.28.250.46 -p 53 -d www.sina.com.cn:www.hexun.com:www.sohu.com:www.163.com
valgrind --tool=memcheck --leak-check=yes --show-reachable=yes -v ./dns_req -s 8.8.8.8 -p 53 -d www.sina.com.cn:www.hexun.com:www.sohu.com:www.163.com
