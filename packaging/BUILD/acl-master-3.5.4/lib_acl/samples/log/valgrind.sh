#!/bin/sh

valgrind --show-below-main=yes --show-reachable=yes --tool=memcheck --leak-check=yes -v ./log "UDP:192.168.1.229:12345|TCP:127.0.0.1:12345|log.txt"
