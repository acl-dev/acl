#!/bin/sh

valgrind --tool=memcheck --leak-check=yes -v ./md5
