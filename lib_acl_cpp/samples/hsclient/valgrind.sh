#!/bin/sh

#valgrind --tool=memcheck --leak-check=yes -v ./hsclient -s 124.238.255.29:19999 -Q
valgrind --tool=memcheck --leak-check=yes -v ./hsclient -s 192.168.1.232:9999 -n 2 -q
