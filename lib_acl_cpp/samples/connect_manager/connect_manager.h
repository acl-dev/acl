#pragma once

class connect_manager : public acl::connect_manager
{
public:
	connect_manager();

	virtual ~connect_manager();

protected:
	virtual acl::connect_pool* create_pool(const char* addr,
		int count, size_t idx);
};
