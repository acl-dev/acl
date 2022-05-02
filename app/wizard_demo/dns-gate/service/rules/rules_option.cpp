#include "stdafx.h"
#include "acl_cpp/serialize/serialize.hpp"
#include "rules_option.h"

class rule_match {
public:
	rule_match(void) {
		enables_ = acl_dlink_create(10);
	}

	~rule_match(void) {
		acl_dlink_free(enables_);
	}

	rule_match& set_names(const std::vector<std::string>& names) {
		names_ = names;
		return *this;
	}

	rule_match& set_hells(const std::vector<std::string>& hells) {
		hells_ = hells;
		return *this;
	}

	rule_match& set_enables(const std::vector<time_enable>& enables) {
		long long begin, end;
		for (auto cit = enables.begin(); cit != enables.end(); ++cit) {
			begin = (*cit).from_hour * 60 + (*cit).from_min;
			end   = (*cit).to_hour * 60 + (*cit).to_min;
			acl_dlink_insert(enables_, begin, end);
		}
		return *this;
	}

public:
	const std::vector<std::string>* get_hells(const char* name,
			acl::string& matched, int tm_hour, int tm_min) {
		long long n = tm_hour * 60 + tm_min;
		if (acl_dlink_lookup(enables_, n) != NULL) {
			return NULL;
		}

		for (auto cit = names_.begin(); cit != names_.end(); ++cit) {
			if (acl_strcasestr(name, (*cit).c_str()) != NULL) {
				matched = (*cit).c_str();
				break;
			}
		}

		if (matched.empty()) {
			return NULL;
		}

		return &hells_;
	}

private:
	std::vector<std::string> names_;
	std::vector<std::string> hells_;
	ACL_DLINK* enables_;
};

//////////////////////////////////////////////////////////////////////////////

rules_option::rules_option(void) {}

rules_option::~rules_option(void) {
	for (std::vector<rule_match*>::const_iterator cit = rules_match_.begin();
		cit != rules_match_.end(); ++cit) {
		delete *cit;
	}
}

void rules_option::build_add(const filter_rule& rule) {
	rule_match* match = new rule_match;
	(*match).set_names(rule.names).set_hells(rule.hells)
		.set_enables(rule.enables);
	rules_match_.push_back(match);
}

void rules_option::build(const filter_rules& rules) {
	for (auto cit = rules.rules.begin(); cit != rules.rules.end(); ++cit) {
		build_add(*cit);
	}
}

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

	filter_rules rules;
	buf.clear();
	if (!acl::deserialize<filter_rules>(json, rules, &buf)) {
		logger_error("deserialize error %s", buf.c_str());
		return false;
	}

	build(rules);

	logger("load %s ok", filepath);
	return true;
}

const std::vector<std::string>* rules_option::get_hells(const char* name,
		acl::string& matched, int& tm_hour, int& tm_min) const {
	time_t now = time(NULL);
	struct tm *local_time = localtime(&now);
	tm_hour = local_time->tm_hour;
	tm_min  = local_time->tm_min;

	const std::vector<std::string>* hells = NULL;

	for (auto it = rules_match_.begin(); it != rules_match_.end(); ++it) {
		hells = (*it)->get_hells(name, matched, tm_hour, tm_min);
		if (hells) {
			return hells;
		}
	}

	return NULL;
}
