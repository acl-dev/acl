#!/bin/sh

valgrind --tool=memcheck --leak-check=yes -v ./xml -b 100

echo "Enter any key to continue ..."
read tmp
valgrind --tool=memcheck --leak-check=yes -v ./xml -d data1 -P -m -b 100

echo "Enter any key to continue ..."
read tmp
valgrind --tool=memcheck --leak-check=yes -v ./xml -d data1 -P -s -m -b 100

echo "Enter any key to continue ..."
read tmp
valgrind --tool=memcheck --leak-check=yes -v ./xml -P -d data1 -b 0 -s
