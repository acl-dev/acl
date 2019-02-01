#!/bin/sh

valgrind --tool=memcheck --leak-check=yes -v ./scan_rmdir -d ./t1 -a -r -R -C
