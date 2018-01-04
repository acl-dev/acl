#!/bin/sh

valgrind --tool=memcheck --leak-check=yes -v ./service_guard alone service_guard.cf
