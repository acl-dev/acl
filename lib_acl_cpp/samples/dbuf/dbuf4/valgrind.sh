#!/bin/sh

valgrind --tool=memcheck --leak-check=yes  -v ./dbuf -n 1000000 -p 12345
