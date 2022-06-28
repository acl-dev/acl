#!/bin/sh

valgrind --tool=memcheck --leak-check=yes -v ./master_monitor alone master_monitor.cf
