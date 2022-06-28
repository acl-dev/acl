#!/bin/sh

valgrind --show-reachable=yes --tool=memcheck --leak-check=yes -v ./aio_server_ssl -k -S
