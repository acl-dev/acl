#!/bin/sh

valgrind --tool=memcheck --leak-check=yes -v ./disque_manager -s 127.0.0.1:7711 -a addjob -n 10 -c 10
