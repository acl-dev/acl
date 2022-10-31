#!/bin/sh

valgrind --tool=memcheck --leak-check=yes --leak-check=full --show-reachable=yes -v ./file_copy -e io_uring -s file_copy -d d1 -t 2 -c 10 -a splice

echo "Enter any key to test pread_pwrite..."
read n

valgrind --tool=memcheck --leak-check=yes --leak-check=full --show-reachable=yes -v ./file_copy -e io_uring -s file_copy -d d1 -t 2 -c 10 -a pread_pwrite
