#!/bin/sh

valgrind --tool=memcheck --leak-check=yes -v ./udp_server -s 127.0.0.1:8888 -c
