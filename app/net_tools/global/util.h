#pragma once
#include <vector>

class util
{
public:
	util() {}
	~util() {}

	static size_t get_dns(std::vector<acl::string>& dns_list);
	static double stamp_sub(const struct timeval *from,
		const struct timeval *sub_by);
protected:
private:
};