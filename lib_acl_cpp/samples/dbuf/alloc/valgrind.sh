#!/bin/sh

valgrind --tool=memcheck --leak-check=yes  -v ./dbuf_alloc -n 10
