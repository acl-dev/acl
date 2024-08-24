#!/bin/sh

./json -f json1.txt -n name113|jq
echo "------------------------name113-----------------------------------------"

read ch
./json -f json1.txt -n name1131|jq
echo "------------------------name1131----------------------------------------"

read ch
./json -f json1.txt -n name1132|jq
echo "------------------------name1132----------------------------------------"

read ch
./json -f json1.txt -n name1133|jq
echo "------------------------name1133----------------------------------------"

read ch
./json -f json1.txt -n name117|jq
echo "------------------------name117-----------------------------------------"

read ch
./json -f json1.txt -n name1171|jq
echo "------------------------name1171----------------------------------------"

read ch
./json -f json1.txt -n name11711|jq
echo "------------------------name11711---------------------------------------"

read ch
./json -f json1.txt -n name117111|jq
echo "------------------------name117111--------------------------------------"

read ch
./json -f json1.txt -n "name1131,name117111"|jq
echo "------------------------name1131,name117111-----------------------------"

read ch
./json -f json1.txt -n "name1131,name1132"|jq
echo "------------------------name1131,name1132-------------------------------"

read ch
./json -f json1.txt -n "name118"|jq
echo "------------------------name118-----------------------------------------"

read ch
./json -f json1.txt -n "name1181"|jq
echo "------------------------name1181----------------------------------------"

read ch
./json -f json1.txt -n "name1182"|jq
echo "------------------------name1183----------------------------------------"

read ch
./json -f json1.txt -n "name1183"|jq
echo "------------------------name1183----------------------------------------"

read ch
./json -f json1.txt -n "name1181,name1182"|jq
echo "------------------------name1181,name1182-------------------------------"

read ch
./json -f json1.txt -n "name1181,name1183"|jq
echo "------------------------name1181,name1183-------------------------------"

read ch
./json -f json1.txt -n "name1183,name1182"|jq
echo "------------------------name1183,name1182-------------------------------"

read ch
./json -f json1.txt -n "name1181,name1182,name1183"|jq
echo "------------------------name1181,name1182,name1183----------------------"
