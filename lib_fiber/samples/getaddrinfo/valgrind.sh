#!/bin/sh

valgrind --tool=memcheck --leak-check=yes --leak-check=full --show-reachable=yes -v ./getaddrinfo -n "192.168.188.143,mmm.mmm.xxx,www.baidu.com"
