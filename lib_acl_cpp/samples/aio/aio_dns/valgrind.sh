#!/bin/sh

valgrind --show-reachable=yes --tool=memcheck --leak-check=yes -v ./aio_dns -t 10
