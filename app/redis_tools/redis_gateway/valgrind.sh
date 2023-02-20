#!/bin/sh

valgrind --tool=memcheck --leak-check=yes -v ./redis_gateway alone redis_gateway.cf
