#!/bin/sh

#valgrind --tool=memcheck --leak-check=yes --track-origins=yes -v ./http_server
valgrind --tool=memcheck --leak-check=yes  -v ./http_server -k
