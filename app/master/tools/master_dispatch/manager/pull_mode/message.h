#pragma once

class message
{
public:
	message(const acl::string& server);
	~message(void) {}

	const acl::string& get_server() const
	{
		return server_;
	}

	const acl::string& get_result() const
	{
		return buf_;
	}

	void add(const char* data, size_t len);

private:
	acl::string server_;
	acl::string buf_;
};

