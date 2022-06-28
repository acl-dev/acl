#!/bin/sh

valgrind --tool=memcheck --leak-check=yes -v ./gid_server test -f gid_server.cf
