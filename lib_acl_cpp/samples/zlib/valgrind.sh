#!/bin/sh

#valgrind --show-reachable=yes --tool=memcheck --leak-check=yes -v ./zlib
valgrind --tool=memcheck --leak-check=yes -v ./zlib
