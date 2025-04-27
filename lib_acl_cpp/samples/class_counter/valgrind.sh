#!/bin/sh

valgrind --tool=memcheck --leak-check=yes -v ./class_counter 100
