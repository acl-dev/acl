#!/bin/sh

valgrind --tool=memcheck --leak-check=yes -v ./string -n 102400
