#!/bin/sh

valgrind --tool=memcheck --leak-check=yes -v ./master_udp_threads alone master_udp_threads.cf
