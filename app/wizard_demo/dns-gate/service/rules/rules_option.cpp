#include "stdafx.h"
#include "acl_cpp/serialize/serialize.hpp"
#include "rules_option.h"

rules_option::rules_option(void) {}
rules_option::~rules_option(void) {}

bool rules_option::load(const char* filepath) {
	acl::string buf;
	if (!acl::ifstream::load(filepath, &buf)) {
		logger_error("load %s error %s", filepath, acl::last_serror());
		return false;
	}

	acl::json json(buf);
	if (!json.finish()) {
		logger_error("invalid json data in %s", filepath);
		return false;
	}

	buf.clear();
	if (!acl::deserialize<filter_rules>(json, rules_, &buf)) {
		logger_error("deserialize error %s", buf.c_str());
		return false;
	}
	return true;
}

bool rules_option::is_blocked(const char* name, int* hour, int* min,
		std::vector<std::string>& hells) const {
	const filter_rule* rule = NULL;

	// This can be optimized when having many objects.
	for (std::vector<filter_rule>::const_iterator cit = rules_.rules.begin();
		cit != rules_.rules.end(); ++cit) {
		for (std::vector<std::string>::const_iterator cit2 = (*cit).names.begin();
			cit2 != (*cit).names.end(); ++cit2) {
			if (acl_strcasestr((*cit2).c_str(), name) != NULL) {
				rule = &(*cit);
				break;
			}
		}
		if (rule != NULL) {
			break;
		}
	}

	if (rule == NULL) {
		return false;
	}

	time_t now = time(NULL);
	struct tm *local_time = localtime(&now);
	*hour = local_time->tm_hour;
	*min  = local_time->tm_min;

	for (std::vector<time_enable>::const_iterator cit = rule->enables.begin();
		cit != rule->enables.end(); ++cit) {
		if (local_time->tm_hour >= (*cit).from_hour
			&& local_time->tm_min >= (*cit).from_min
			&& local_time->tm_hour <= (*cit).to_hour
			&& local_time->tm_min <= (*cit).to_min) {
			return false;
		}
	}
	hells = rule->hells;
	return true;
}
