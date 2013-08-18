#!/bin/sh

valgrind --tool=memcheck --leak-check=yes --show-reachable=yes -v ./http_response alone
