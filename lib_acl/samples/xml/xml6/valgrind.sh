#!/bin/sh

valgrind --tool=memcheck --leak-check=yes -v ./xml -P -d data1
