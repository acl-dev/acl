#!/bin/sh

valgrind --tool=memcheck --leak-check=yes -v ./webapp alone webapp.cf
