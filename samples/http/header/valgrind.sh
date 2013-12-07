#!/bin/sh

valgrind --tool=memcheck --leak-check=yes -v ./header -n 10 -f header.txt -a parse
