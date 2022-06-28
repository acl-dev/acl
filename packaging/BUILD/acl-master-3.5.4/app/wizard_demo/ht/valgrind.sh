#!/bin/sh

valgrind --tool=memcheck --leak-check=yes -v ./ht alone ht.cf
