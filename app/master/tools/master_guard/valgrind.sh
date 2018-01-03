#!/bin/sh

valgrind --tool=memcheck --leak-check=yes -v ./master_guard alone master_guard.cf
