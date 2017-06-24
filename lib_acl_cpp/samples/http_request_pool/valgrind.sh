#!/bin/sh

valgrind --tool=memcheck --leak-check=yes -v ./http_request_pool -c 2 -n 2
