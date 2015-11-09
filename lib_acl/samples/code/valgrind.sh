#!/bin/sh

valgrind --show-below-main=yes --show-reachable=yes --tool=memcheck --leak-check=yes -v ./code
