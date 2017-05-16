#!/bin/sh

valgrind --tool=memcheck --leak-check=yes -v ./qrserver alone qrserver.cf
