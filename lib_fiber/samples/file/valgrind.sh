#!/bin/sh

valgrind --tool=memcheck --leak-check=yes --leak-check=full --show-reachable=yes -v ./file -f xxx.txt -n 4096 -o O_WRONLY -o O_CREAT
valgrind --tool=memcheck --leak-check=yes --leak-check=full --show-reachable=yes -v ./file -f xxx.txt -o O_RDONLY
