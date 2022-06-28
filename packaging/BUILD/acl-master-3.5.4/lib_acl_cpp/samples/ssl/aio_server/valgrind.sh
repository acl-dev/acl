#!/bin/sh

valgrind --show-reachable=yes --tool=memcheck --leak-check=yes -v ./aio_server -K ssl_key.pem -C ssl_crt.pem
