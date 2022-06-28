#!/bin/sh

#valgrind --tool=memcheck --leak-check=yes -v ./redis_builder  -a create -f cluster.xml
#valgrind --tool=memcheck --leak-check=yes -v ./redis_builder  -a create -r 2 -f nodes2.xml
#valgrind --tool=memcheck --leak-check=yes -v ./redis_builder  -a create -r 2 -f nodes3.xml
valgrind --tool=memcheck --leak-check=yes -v ./redis_builder  -a create -r 2 -f nodes.xml
valgrind --tool=memcheck --leak-check=yes -v ./redis_builder  -s 192.168.136.176:16386 -N 192.168.136.176:16387 -S -a add_node
valgrind --tool=memcheck --leak-check=yes -v ./redis_builder  -s 192.168.136.176:16386 -N 192.168.136.176:16388 -S -a add_node
valgrind --tool=memcheck --leak-check=yes -v ./redis_builder  -s 192.168.136.176:16380 -N 192.168.136.176:16386 -a add_node
valgrind --tool=memcheck --leak-check=yes -v ./redis_builder  -s 192.168.136.176:16380 -a reshard
