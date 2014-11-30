#!/bin/sh

valgrind --tool=memcheck --leak-check=yes -v ./configure -f tt2.cf 
