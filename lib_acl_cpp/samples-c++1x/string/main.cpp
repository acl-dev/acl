#include "stdafx.h"
#include <string>
#include <iostream>
#include <unordered_map>

struct key_hash {
	std::size_t operator() (const acl::string& key) const {
		return (std::size_t) acl_hash_crc32(key, key.size());
	}
};
struct key_equal {
	bool operator()(const acl::string& lhs, const acl::string& rhs) const {
		return lhs == rhs;
	}
};

#if 0
template <>
struct std::hash<acl::string> {
	std::size_t operator()(const acl::string& key) const {
		return (std::size_t) key.hash();
	}
};
#endif

int main(void)
{
#if 0
	std::unordered_map<acl::string, acl::string, key_hash, key_equal> dummy;
#else
	std::unordered_map<acl::string, acl::string> dummy;
#endif
	dummy["name1"] = "value1";
	dummy["name2"] = "value2";
	dummy["name3"] = "value3";

	for (auto cit = dummy.cbegin(); cit != dummy.cend(); ++cit) {
		std::cout << cit->first.c_str() << "=" << cit->second.c_str() << std::endl;
	}

	std::cout << std::endl;

	for (const auto& cit : dummy) {
		std::cout << cit.first.c_str() << "=" << cit.second.c_str() << std::endl;
	}
	return 0;
}
