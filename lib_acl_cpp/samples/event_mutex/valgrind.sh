#!/bin/sh

valgrind --tool=memcheck --leak-check=yes  -v ./event_mutex
