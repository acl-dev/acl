#!/bin/sh

valgrind --tool=memcheck --leak-check=yes -v ./http_logger alone http_logger.cf
