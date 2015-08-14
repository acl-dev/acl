#pragma once

class connect_manager : public acl::connect_manager
{
public:
	connect_manager();

	virtual ~connect_manager();

protected:
	// 基类纯虚函数的实现
	acl::connect_pool* create_pool(const char* addr,
		size_t count, size_t idx);
};
