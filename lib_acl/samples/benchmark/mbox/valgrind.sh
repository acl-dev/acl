#!/bin/sh

valgrind --show-reachable=yes --tool=memcheck --leak-check=yes -v ./mbox -p 5 -n 10
