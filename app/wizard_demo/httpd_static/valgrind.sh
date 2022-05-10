#!/bin/sh

valgrind --tool=memcheck --leak-check=yes -v ./httpd_static alone httpd_static.cf
