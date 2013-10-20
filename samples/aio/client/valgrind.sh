#!/bin/sh

valgrind --tool=memcheck --leak-check=yes -v ./aclient -s 127.0.0.1:8888 -m select -n 10 -l 10 -d 1000
