#!/bin/sh

./fiber_connect -s 127.0.0.1:6379

echo ""
echo "Enter any key to continue..."
read n
./fiber_connect -s 127.0.0.1:6379 -S

echo ""
echo "Enter any key to continue..."
read n
./fiber_connect -s 127.0.0.1:637

echo ""
echo "Enter any key to continue..."
read n
./fiber_connect -s 127.0.0.1:637 -S
