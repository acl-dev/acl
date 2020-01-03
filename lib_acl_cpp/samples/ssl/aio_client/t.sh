#!/bin/sh

./aio_client -d ../libmbedtls_all.so -c 10 -n 100 -S -l 127.0.0.1:9800
