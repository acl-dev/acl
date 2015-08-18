#!/bin/sh

valgrind --tool=memcheck --leak-check=yes -v ./http_request -s 127.0.0.1:8888 -n 200 -z
