#!/bin/sh

valgrind --tool=memcheck --leak-check=yes -v ./scan_dir -d ../../../lib_acl/src -r -t all -a
