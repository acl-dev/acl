#!/bin/sh
valgrind --tool=memcheck --error-limit=no --leak-check=yes --show-reachable=yes ./tlssvr -c tlssvr.cf
