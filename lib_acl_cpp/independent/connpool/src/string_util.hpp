#pragma once
#include <vector>
#include <string>

namespace acl_min
{

	std::vector<std::string>& split3(const char *str, const char *delim,
		std::vector<std::string>);
	char* lowercase(const char* src, char* buf, size_t size);
	char *strtrim(char *str);

}; // name space acl_min
