#!/bin/sh

valgrind --tool=memcheck --leak-check=yes -v ./https_client -s 127.0.0.1:1443  -c 100 -n 100 -S -k
