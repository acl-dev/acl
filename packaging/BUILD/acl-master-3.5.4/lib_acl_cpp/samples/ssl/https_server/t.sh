#!/bin/sh

os=$(echo `uname -s`)
if [ $os == 'Darwin' ]; then
	./https_server alone https_server2.cf
else
	./https_server alone https_server.cf
fi
