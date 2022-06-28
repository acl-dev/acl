#!/bin/sh

valgrind --show-reachable=yes --tool=memcheck --leak-check=yes -v ./aio_client_ssl -k -S -c 1 -n 10
