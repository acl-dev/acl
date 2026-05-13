#!/bin/sh

#valgrind --tool=memcheck --leak-check=yes --show-reachable=yes -v ./http_request -f xml.txt -t xml
valgrind --tool=memcheck --leak-check=yes --show-reachable=yes -v ./http_request -f xml_utf8.txt -t xml -c utf-8
#valgrind --tool=memcheck --leak-check=yes --show-reachable=yes -v ./http_request -f json.txt -t json
#valgrind --tool=memcheck --leak-check=yes --show-reachable=yes -v ./http_request -f json_utf8.txt -t json -c utf-8
