#!/bin/sh

valgrind --tool=memcheck --leak-check=yes -v ./master_dispatch alone master_dispatch.cf
