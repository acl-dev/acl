#!/bin/sh
./ssl_client -d ../libmbedtls_all.dylib -s 127.0.0.1:9800 -c 10 -n 10000
