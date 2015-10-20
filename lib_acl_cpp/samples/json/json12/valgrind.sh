#!/bin/sh

valgrind --tool=memcheck --leak-check=yes -v ./json -f json1.txt
