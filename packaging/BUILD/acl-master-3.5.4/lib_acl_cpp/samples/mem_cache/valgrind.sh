#!/bin/sh

valgrind --track-origins=yes --tool=memcheck --leak-check=yes -v ./mem_cache -g -n 10
