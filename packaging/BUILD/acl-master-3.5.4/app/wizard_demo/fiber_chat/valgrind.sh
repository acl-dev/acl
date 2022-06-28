#!/bin/sh

valgrind --tool=memcheck --leak-check=yes -v ./fiber_chat alone fiber_chat.cf
