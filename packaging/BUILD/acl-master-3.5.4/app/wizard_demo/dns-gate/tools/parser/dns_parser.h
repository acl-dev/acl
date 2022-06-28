/**
 * Copyright (C) 2015-2018 IQIYI
 * All rights reserved.
 *
 * AUTHOR(S)
 *   Zheng Shuxin
 *   E-mail: zhengshuxin@qiyi.com
 * 
 * VERSION
 *   Sat Apr 30 21:48:45 2022
 */

#pragma once

class dns_item {
public:
	dns_item(void) {}
	~dns_item(void) {}

	acl::string name;
	std::map<acl::string, double> stamps;
};

class dns_parser {
public:
	dns_parser(acl::redis_client& conn, int count);
	~dns_parser(void);

	bool start(void);

private:
	acl::redis_client& conn_;
	int count_;
	std::map<acl::string, dns_item> items_;
	std::set<acl::string> root_names_;

	bool load_all(void);
	void load(const char* name);
	void show_all(void);
	void show_one(const char* name, const dns_item& item);

	void add_root_name(const char* name);
	void show_root_names(void);
};
