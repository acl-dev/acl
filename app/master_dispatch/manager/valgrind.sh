#!/bin/sh

valgrind --tool=memcheck --leak-check=yes -v ./dispatch_manager alone dispatch_manager.cf
