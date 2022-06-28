#!/bin/sh

valgrind --tool=memcheck --leak-check=yes --show-reachable=yes -v ./ssl_server alone mm.crt mm.key req
