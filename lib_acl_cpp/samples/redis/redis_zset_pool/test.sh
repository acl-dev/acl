#!/bin/sh
time ./redis_zset_pool -s 127.0.0.1:6379 -n 20000 -c 10 -a zadd -l 10240 -b 1024
time ./redis_zset_pool -s 127.0.0.1:6379 -n 20000 -c 10 -a zrange -l 10240 -b 1024 -S
time ./redis_zset_pool -s 127.0.0.1:6379 -n 20000 -c 10 -a zrange -l 10240 -b 1024
time ./redis_zset_pool -s 127.0.0.1:6379 -n 20000 -c 10 -a del
