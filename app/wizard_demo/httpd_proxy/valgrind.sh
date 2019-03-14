#!/bin/sh

valgrind --tool=memcheck --leak-check=yes -v ./httpd_proxy alone httpd_proxy.cf
