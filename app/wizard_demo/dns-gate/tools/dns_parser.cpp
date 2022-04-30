#include "stdafx.h"
#include "dns_parser.h"

dns_parser::dns_parser(acl::redis_client& conn, int count)
: conn_(conn)
, count_(count)
{
}

dns_parser::~dns_parser(void) {}

bool dns_parser::start(void) {
	if (!load_all()) {
		return false;
	}

	show_all();
	show_root_names();

	return true;
}

void dns_parser::add_root_name(const char* name) {
	acl::string buf(name);
	char* ptr = buf.buf_end();

	int dots = 0;
	while (ptr > buf.c_str()) {
		if (*ptr == '.' && ++dots == 2) {
			root_names_.insert(ptr);
			break;
		}
		ptr--;
	}
}

void dns_parser::show_root_names(void) {
	for (std::set<acl::string>::const_iterator cit = root_names_.begin();
		cit != root_names_.end(); ++cit) {
		printf("%s\r\n", (*cit).c_str());
	}
	printf("---show root names over, count=%zd---\r\n", root_names_.size());
}

bool dns_parser::load_all(void) {
	acl::redis cmd(&conn_);
	std::vector<acl::string> res;
	const char* pattern = "*";

	if (cmd.keys_pattern(pattern, &res) <= 0) {
		logger("keys_pattern error=%s", cmd.result_error());
		return false;
	}

	for (std::vector<acl::string>::const_iterator cit = res.begin();
		cit != res.end(); ++cit) {
		load(*cit);
		add_root_name(*cit);
	}
	return true;
}

void dns_parser::load(const char* name) {
	acl::redis cmd(&conn_);
	std::vector<std::pair<acl::string, double> > res;
	int n = cmd.zrevrange_with_scores(name, 0, count_, res);
	if (n <= 0) {
		logger_error("zrevrange_with_scores return=%d, %s",
			n, cmd.result_error());
		return;
	}

	dns_item item;
	item.name = name;
	for (std::vector<std::pair<acl::string, double> >::const_iterator
		cit = res.begin(); cit != res.end(); ++cit) {
		item.stamps[(*cit).first] = (*cit).second;
	}

	items_[name] = item;
}

void dns_parser::show_all(void) {
	for (std::map<acl::string, dns_item>::const_iterator cit = items_.begin();
		cit != items_.end(); ++cit) {
		show_one(cit->first, cit->second);
	}
}

void dns_parser::show_one(const char* name, const dns_item& item) {
	printf("name: %s\r\n", name);
	for (std::map<acl::string, double>::const_iterator
		cit = item.stamps.begin(); cit != item.stamps.end(); ++cit) {
		printf("  %s: %.2f\r\n", cit->first.c_str(), cit->second);
	}
}
