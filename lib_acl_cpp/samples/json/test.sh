#!/bin/sh

cd json0
chmod 755 valgrind.sh
./valgrind.sh
cd ..

echo "Enter any key to test json1 ..."
read tmp
cd json1
chmod 755 valgrind.sh
./valgrind.sh
cd ..

echo "Enter any key to test json2 ..."
read tmp
cd json2
chmod 755 valgrind.sh
./valgrind.sh
cd ..

echo "Enter any key to test json3 ..."
read tmp
cd json3
chmod 755 valgrind.sh
./valgrind.sh
cd ..

echo "Enter any key to test json4 ..."
read tmp
cd json4
chmod 755 valgrind.sh
./valgrind.sh
cd ..

echo "Enter any key to test json5 ..."
read tmp
cd json5
chmod 755 valgrind.sh
./valgrind.sh
cd ..

echo "Enter any key to test json6 ..."
read tmp
cd json6
chmod 755 valgrind.sh
./valgrind.sh
cd ..

echo "Enter any key to test json7 ..."
read tmp
cd json7
chmod 755 valgrind.sh
./valgrind.sh
cd ..

echo "Enter any key to test json8 ..."
read tmp
cd json8
chmod 755 valgrind.sh
./valgrind.sh
cd ..

echo "Enter any key to test json9 ..."
read tmp
cd json9
chmod 755 valgrind.sh
./valgrind.sh
cd ..

echo "Enter any key to test json10 ..."
read tmp
cd json10
chmod 755 valgrind.sh
./valgrind.sh
cd ..

echo "Enter any key to test json11 ..."
read tmp
cd json11
chmod 755 valgrind.sh
./valgrind.sh
cd ..
