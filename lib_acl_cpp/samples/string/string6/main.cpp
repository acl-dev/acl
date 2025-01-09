#include <string>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include "acl_cpp/lib_acl.hpp"

int main()
{
	acl::string s = "hello world!";

	std::cout << s << std::endl;

	std::string s1 = "Hello, you're welcome!";
	s = s1;
	std::cout << s << std::endl;

	if (s == s1) {
		std::cout << "s == s1\r\n";
	} else {
		std::cout << "s != s1\r\n";
	}

	if (s1 == s) {
		std::cout << "s1 == s\r\n";
	} else {
		std::cout << "s1 != s\r\n";
	}

#if 1
	std::string s2 = s;
#else
	// This'll be failed.
	std::string s2;
	s2 = s;
#endif
	std::cout << "after std::string=acl::string: " << s2 << std::endl;

	std::cout << "Before pop_back, s: " << s << ", size: " << s.size() << std::endl;

	char ch = s.back();
	if (ch == (char) -1) {
		std::cout << "back nil!\r\n";
	} else {
		s.pop_back();

		std::cout << "pop_back one char: " << (char) (ch)
			<< ", left: " << s << ", left size: " << s.size()
			<< std::endl;
	}

	s.clear();
	ch = s.back();
	if (ch == (char) -1) {
		std::cout << "no char\r\n";
	} else {
		std::cout << "back char: " << ch << std::endl;
	}

	return 0;
}
