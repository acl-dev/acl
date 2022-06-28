#!/bin/sh

valgrind --tool=memcheck --leak-check=yes --leak-check=full --show-reachable=yes -v ./http_request
