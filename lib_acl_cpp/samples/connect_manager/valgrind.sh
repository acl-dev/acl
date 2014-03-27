#!/bin/sh

#valgrind --tool=memcheck --leak-check=yes -v ././connect_manager -s "www.qq.com:81"
#valgrind --tool=memcheck --leak-check=yes -v ./connect_manager -s "202.108.33.60:80;211.150.74.6:80;61.135.167.36:81"
#valgrind --tool=memcheck --leak-check=yes -v ./connect_manager -s "61.135.167.36:81;61.135.167.81:80;202.108.33.60:80;202.108.33.60:81"
valgrind --tool=memcheck --leak-check=yes -v ./connect_manager -s "123.125.119.147:80;123.125.119.147:81;202.108.33.60:80"
