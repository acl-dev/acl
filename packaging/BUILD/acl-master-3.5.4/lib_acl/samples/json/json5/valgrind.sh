#!/bin/sh

valgrind --tool=memcheck --leak-check=yes -v ./json -b -m 10
