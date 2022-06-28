#!/bin/sh

./mime -s -t test1 -f test1.eml
diff test1.eml var/test1.eml

echo ""
echo "Enter any key to continue ..."
read tmp
./mime -s -t test1 -f test2.eml
diff test2.eml var/test2.eml

echo ""
echo "Enter any key to continue ..."
read tmp
./mime -s -t test1 -f test3.eml
diff test3.eml var/test3.eml

echo ""
echo "Enter any key to continue ..."
read tmp
./mime -s -t test1 -f test4.eml
diff test4.eml var/test4.eml

echo ""
echo "Enter any key to continue ..."
read tmp
./mime -s -t test1 -f test5.eml
diff test5.eml var/test5.eml

echo ""
echo "Enter any key to continue ..."
read tmp
./mime -s -t test1 -f test6.eml
diff test6.eml var/test6.eml

echo ""
echo "Enter any key to continue ..."
read tmp
./mime -s -t test1 -f test7.eml
diff test7.eml var/test7.eml

echo ""
echo "Enter any key to continue ..."
read tmp
./mime -s -t test1 -f test8.eml
diff test8.eml var/test8.eml

echo ""
echo "Enter any key to continue ..."
read tmp
./mime -s -t test1 -f test9.eml
diff test8.eml var/test8.eml

echo ""
echo "Enter any key to continue ..."
read tmp
./mime -s -t test1 -f test10.eml
diff test10.eml var/test10.eml

echo ""
echo "Enter any key to continue ..."
read tmp
./mime -s -t test1 -f test11.eml
diff test11.eml var/test11.eml

echo ""
echo "Enter any key to continue ..."
read tmp
./mime -s -t test1 -f test12.eml
diff test12.eml var/test12.eml

echo ""
echo "Enter any key to continue ..."
read tmp
./mime -s -t test1 -f test13.eml
diff test13.eml var/test13.eml

echo ""
echo "Enter any key to continue ..."
read tmp
./mime -s -t test1 -f test14.eml
diff test14.eml var/test14.eml

echo ""
echo "Enter any key to continue ..."
read tmp
./mime -s -t test1 -f test15.eml
diff test15.eml var/test15.eml

echo ""
echo "Enter any key to continue ..."
read tmp
./mime -s -t test1 -f test16.eml
diff test16.eml var/test16.eml

echo ""
echo "Enter any key to continue ..."
read tmp
./mime -s -t test1 -f test17.eml
diff test17.eml var/test17.eml

echo ""
echo "Enter any key to continue ..."
read tmp
./mime -s -t test1 -f test18.eml
diff test18.eml var/test18.eml

echo ""
echo "Enter any key to continue ..."
read tmp
./mime -s -t test1 -f test19.eml
diff test19.eml var/test19.eml

echo ""
echo "Enter any key to continue ..."
read tmp
./mime -s -t test1 -f test21.eml
diff test21.eml var/test21.eml

echo ""
echo "Enter any key to continue ..."
read tmp
./mime -s -t test1 -f test22.eml
diff test22.eml var/test22.eml
