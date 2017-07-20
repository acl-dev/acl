#!/bin/sh

valgrind --tool=memcheck --leak-check=yes -v ./master_ctld alone master_ctld.cf
