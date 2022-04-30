#!/bin/sh

valgrind --tool=memcheck --leak-check=yes -v ./dns-gate alone dns-gate.cf
