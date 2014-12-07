#!/bin/sh

valgrind --tool=memcheck --leak-check=yes --show-reachable=yes -v ./ssl_client
