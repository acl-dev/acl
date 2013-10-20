#!/bin/sh

valgrind --tool=memcheck --leak-check=yes -v ./dns -h 8.8.8.8 -p 53 -d www.sina.com
