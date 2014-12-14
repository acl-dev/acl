#!/bin/sh

valgrind --tool=memcheck --leak-check=yes -v ./master_dispatch alone 1080 master_dispatch.cf
