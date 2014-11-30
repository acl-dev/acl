#!/bin/sh

valgrind --tool=memcheck --leak-check=yes -v ./vstream_fseek
