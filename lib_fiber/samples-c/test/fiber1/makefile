fiber: main.o
	gcc -o fiber main.o -L../../../lib -l_fiber -L../../../../lib_acl/lib -l_acl -lpthread -ldl
main.o: main.c
	gcc -O3 -c main.c -DLINUX2 -I.. -I../../../c/include -I../../../../lib_acl/include

clean:
	rm -f fiber main.o
