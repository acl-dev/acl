#!/bin/sh

valgrind --tool=memcheck --leak-check=yes -v ./httpd_download alone httpd_download.cf
