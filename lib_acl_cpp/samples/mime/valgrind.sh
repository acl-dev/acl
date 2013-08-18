#!/bin/sh

valgrind --tool=memcheck --leak-check=yes --show-reachable=yes -v ./mime -s -f test2.eml
