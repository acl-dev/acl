#!/bin/sh

valgrind --tool=memcheck --leak-check=yes  -v ./dbuf

echo "Enter any key to continue ..."
read tmp

valgrind --tool=memcheck --leak-check=yes  -v ./dbuf -r
