#!/bin/sh

valgrind --tool=memcheck --leak-check=full --show-reachable=yes -v ./mysql_test
