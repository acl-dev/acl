#!/bin/sh

valgrind --tool=memcheck --leak-check=yes -v ./pkv alone pkv.cf
