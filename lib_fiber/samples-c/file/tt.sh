#!/bin/sh

./file -f from.txt -o O_WRONLY -o O_CREAT -n 8291

echo "enter any key to test read..."
read n
./file -f from.txt -o O_RDONLY

echo "enter any key to test stat..."
read n
./file -f from.txt -a stat

echo "enter any key to test splice..."
read n
./file -f from.txt -a splice -p 0

echo "enter any key to test pread..."
read n
./file -f from.txt -a pread -p 512 -n 256

echo "enter any key to test rename..."
read n
./file -f from.txt -t to.txt -a rename

echo "enter any key to test unlink..."
read n
./file -f to.txt -a unlink

echo "enter any key to test pwrite..."
read n
./file -f from.txt -a pwrite -p 500 -n 256

echo "enter any key to test mkdir..."
read n
./file -f "a/b/c/d" -a mkdir
