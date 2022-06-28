#!/bin/sh

valgrind --tool=memcheck --leak-check=yes -v ./httpd_fiber_upload alone httpd_fiber_upload.cf
