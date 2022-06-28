#!/bin/sh

valgrind --tool=memcheck --leak-check=yes -v ./http_servlet2 alone http_servlet2.cf
