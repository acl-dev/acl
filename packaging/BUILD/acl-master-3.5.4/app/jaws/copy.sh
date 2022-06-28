#!/bin/sh
make clean
make
cp master_main/jaws /opt/jaws/libexec/
cp module/mod_http/mod_http.so /opt/jaws/module/
cp plugin/cgi/cgi.so /opt/jaws/plugin/
cp plugin/gbfilter/gbfilter.so /opt/jaws/plugin/
