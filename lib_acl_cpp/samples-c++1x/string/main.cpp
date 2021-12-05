#include "stdafx.h"
#include <iostream>
#include <unordered_map>

struct key_hash {
	std::size_t operator() (const acl::string& key) const {
		return (std::size_t) key.hash();
	}
};

struct key_equal {
	bool operator()(const acl::string& lhs, const acl::string& rhs) const {
		return lhs == rhs;
	}
};

static void test1(void) {
	std::unordered_map<acl::string, acl::string> dummy;
	dummy["name1"] = "value1";
	dummy["name2"] = "value2";
	dummy["name3"] = "value3";

	for (const auto& cit : dummy) {
		std::cout << cit.first.c_str() << "="
			<< cit.second.c_str() << std::endl;
	}
	std::cout << std::endl;
}

static void test2(void) {
	std::unordered_map<acl::string, acl::string, key_hash, key_equal> dummy;
	dummy["name1"] = "value1";
	dummy["name2"] = "value2";
	dummy["name3"] = "value3";

	for (auto cit = dummy.cbegin(); cit != dummy.cend(); ++cit) {
		std::cout << cit->first.c_str() << "="
			<< cit->second.c_str() << std::endl;
	}
}

int main(void) {
	test1();

	std::cout << std::endl;

	test2();
	return 0;
}
