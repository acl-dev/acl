#!/bin/sh

valgrind --tool=memcheck --leak-check=yes -v ./mqtt_pub -c 10000 -B
