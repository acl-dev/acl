#!/bin/sh

echo "======================================================================="
echo ""
./diff -n var/dat0/cur.txt -o var/dat0/old.txt -N 0 -U 0 -D 0 -E 3
if [ $? -gt 0 ]; then
	echo "Should all equal 3"
	exit 1
fi
echo "======================================================================="
echo ""

echo "Enter any key to continue ..."
read tmp
./diff -n var/dat1/cur.txt -o var/dat1/old.txt -N 0 -U 1 -D 0 -E 2
if [ $? -gt 0 ]; then
	echo "Should equal 2, update 1"
	exit 1
fi
echo "======================================================================="
echo ""

echo "Enter any key to continue ..."
read tmp
./diff -n var/dat2/cur.txt -o var/dat2/old.txt -N 3 -U 0 -D 3 -E 0
if [ $? -gt 0 ]; then
	echo "Should delete 3 and add 3"
	exit 1
fi
echo "======================================================================="
echo ""

echo "Enter any key to continue ..."
read tmp
./diff -n var/dat3/cur.txt -o var/dat3/old.txt -N 1 -U 1 -D 1 -E 1
if [ $? -gt 0 ]; then
	echo "Should update 1, delete 1, add 1, equal 1"
	exit 1
fi

echo "======================================================================="
echo ""

echo "Enter any key to continue ..."
read tmp
./diff -n var/dat4/cur.txt -o var/dat4/old.txt -N 0 -U 0 -D 3 -E 0
if [ $? -gt 0 ]; then
	echo "Should delete 3"
	exit 1
fi
echo "======================================================================="
echo ""

echo "Enter any key to continue ..."
read tmp
./diff -n var/dat5/cur.txt -o var/dat5/old.txt -N 3 -U 0 -D 0 -E 0
if [ $? -gt 0 ]; then
	echo "Should add 3"
	exit 1
fi
echo "======================================================================="
echo ""
echo "++++++++++++++++++++++++++++++All OK+++++++++++++++++++++++++++++++++++"
echo ""
