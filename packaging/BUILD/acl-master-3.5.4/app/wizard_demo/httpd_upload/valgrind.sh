#!/bin/sh

valgrind --tool=memcheck --leak-check=yes -v ./httpd_upload alone httpd_upload.cf
