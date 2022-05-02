#pragma once

class black_list {
public:
	black_list(void);
	~black_list(void);

	void add_list(const char* names);
	bool is_blocked(const char* name) const;

private:
	bool block_all_;
	std::vector<acl::string> names_;
};
