fiber: main.o
	g++ -o fiber main.o -L../../../lib -l_fiber_cpp -l_fiber \
		-L../../../../lib_acl_cpp/lib -l_acl_cpp \
		-L../../../../lib_acl/lib -l_acl \
		-lpthread -ldl
main.o: main.cpp
	g++ -std=c++11 -O3 -Wall -c main.cpp -DLINUX2 -I.. -I../../../cpp/include \
		-I../../../../lib_acl_cpp/include

clean:
	rm -f fiber main.o
