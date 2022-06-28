#!/bin/sh

#valgrind --tool=memcheck --leak-check=yes --track-origins=yes -v ./upload alone
valgrind --tool=memcheck --leak-check=yes  -v ./upload alone
