#!/bin/sh

valgrind --tool=memcheck --leak-check=yes -v ./get_procpath
