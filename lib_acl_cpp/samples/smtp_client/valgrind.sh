#!/bin/sh

valgrind --tool=memcheck --leak-check=yes -v ./smtp_client -s smtp.126.com:465 -u zsxxsz@126.com -p 111111 -t zsxxsz@126.com -e
