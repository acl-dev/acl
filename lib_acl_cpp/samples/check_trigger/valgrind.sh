#!/bin/sh

valgrind --tool=memcheck --leak-check=yes -v ./check_trigger alone check_trigger.cf
