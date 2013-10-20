#!/bin/sh

valgrind --tool=memcheck --leak-check=yes -v ./memcache_pool -c 2 -n 200 -a set -k test
