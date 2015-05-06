#!/bin/sh

valgrind --tool=memcheck --leak-check=yes -v ./disque_client -s 127.0.0.1:7711 -a all -n 10
