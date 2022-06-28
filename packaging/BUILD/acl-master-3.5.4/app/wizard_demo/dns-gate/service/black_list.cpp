#include "stdafx.h"
#include "black_list.h"

black_list::black_list(void) : block_all_(false) {}
black_list::~black_list(void) {}

void black_list::add_list(const char* names) {
	acl::string buf(names);
	buf.lower();
	const std::vector<acl::string>& tokens = buf.split2(";, \t");
	for (auto cit = tokens.begin(); cit != tokens.end(); ++cit) {
		names_.push_back(*cit);
		if (strcasecmp((*cit), "all") == 0) {
			block_all_ = true;
		}
	}
}

bool black_list::is_blocked(const char* name) const {
	if (block_all_) {
		logger_warn("All names were blocked, name=%s", name);
		return true;
	}

	acl::string buf(name);
	buf.lower();
	for (auto cit = names_.begin(); cit != names_.end(); ++cit) {
		if (strstr(name, (*cit).c_str()) != NULL) {
			return true;
		}
	}
	return false;
}
