#!/bin/sh

valgrind --tool=memcheck --leak-check=yes -v ./thread_pool -t 10 -n 10000
