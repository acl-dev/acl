#!/bin/sh

valgrind --show-reachable=yes --tool=memcheck --leak-check=yes -v ./websocket -s 127.0.0.1:8885 -D -W
