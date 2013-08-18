#!/bin/sh

#valgrind --tool=memcheck --leak-check=yes --track-origins=yes -v ./http_servlet alone
valgrind --tool=memcheck --leak-check=yes  -v ./session 127.0.0.1:11211 1
