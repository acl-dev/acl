#!/bin/sh

valgrind --show-reachable=yes --tool=memcheck --leak-check=yes -v ./http_client -s 127.0.0.1:8888 -D -U "/" -c 1
