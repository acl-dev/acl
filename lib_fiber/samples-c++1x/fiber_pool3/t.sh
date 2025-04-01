#!/bin/sh

./fibers  -t 10 -n 20 -d 100 -s 500 -C -A

echo ""
echo "Enter any key to continue..."
read n
./fibers  -t 10 -n 20 -d 100 -s 500 -A

echo ""
echo "Enter any key to continue..."
read n
./fibers  -t 10 -i 100 -n 20 -d 200 -s 100 -A

echo ""
echo "Enter any key to continue..."
read n
./fibers  -t 10 -n 20 -d 0 -s 0 -A
