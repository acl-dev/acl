#!/bin/sh

#valgrind --tool=memcheck --leak-check=yes --leak-check=full --show-reachable=yes --max-stackframe=3426305034400000 -v ./fiber -n 10 -m 20
valgrind --tool=memcheck --leak-check=yes --leak-check=full --show-reachable=yes -v ./tcp_keeper  -s 127.0.0.1:8001 -n 10 -m 10 -i 10000 -l 1
