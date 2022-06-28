#include "stdafx.h"
#include "acl_cpp/serialize/serialize.hpp"
#include "rules_option.h"

class rule_match {
public:
	rule_match(void) : enables_(NULL) {}

	~rule_match(void) {
		if (enables_) {
			acl_dlink_free(enables_);
		}
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
		// Reset the enables.
		if (enables_) {
			acl_dlink_free(enables_);
		}

		enables_ = acl_dlink_create(10);
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
			acl::string& matched, int tm_hour, int tm_min) const {
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

	bool has(const char* name) const {
		for (auto cit = names_.begin(); cit != names_.end(); ++cit) {
			if (acl_strcasestr(name, (*cit).c_str()) != NULL) {
				return true;
			}
		}

		return false;
	}

	bool empty(void) const {
		return names_.empty();
	}

	bool update(const char* name, const std::vector<std::string>& hells,
			const std::vector<time_enable>& enables) {
		bool matched = false;
		for (auto it = names_.begin(); it != names_.end(); ++it) {
			if (acl_strcasestr(name, (*it).c_str()) != NULL) {
				break;
			}
		}
		if (!matched) {
			return false;
		}
		hells_ = hells;

		set_enables(enables);
		return true;
	}

	bool remove(const char* name) {
		for (auto it = names_.begin(); it != names_.end(); ++it) {
			if (acl_strcasestr(name, (*it).c_str()) != NULL) {
				names_.erase(it);
				return true;
			}
		}
		return false;
	}

	void add(int from_hour, int from_min, int to_hour, int to_min) {
		if (enables_ == NULL) {
			enables_ = acl_dlink_create(10);
		}
		long long begin = from_hour * 60 + from_min;
		long long end   = to_hour * 60 + to_min;
		acl_dlink_insert(enables_, begin, end);
	}

	void get_rules(filter_rules& rules) const {
		filter_rule rule;
		rule.names = names_;
		rule.hells = hells_;

		ACL_ITER iter;
		acl_foreach(iter, enables_) {
			ACL_DITEM* ditem = (ACL_DITEM*) iter.data;
			time_enable enable;

			parse_ditem(ditem, enable);
			rule.enables.push_back(enable);

		}

		rules.rules.push_back(rule);
	}

private:
	std::vector<std::string> names_;
	std::vector<std::string> hells_;
	ACL_DLINK* enables_;

	void parse_ditem(const ACL_DITEM* ditem, time_enable& enable) const {
		int from = (int) ditem->begin;
		int to   = (int) ditem->end;
		enable.from_hour = from / 60;
		enable.from_min  = from % 60;
		enable.to_hour   = to / 60;
		enable.to_min    = to % 60;
	}
};

//////////////////////////////////////////////////////////////////////////////

rules_option::rules_option(void) {}

rules_option::~rules_option(void) {
	free_rules();
}

void rules_option::free_rules(void) {
	for (std::vector<rule_match*>::const_iterator cit = rules_match_.begin();
		cit != rules_match_.end(); ++cit) {
		delete *cit;
	}
	rules_match_.clear();
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

bool rules_option::reload(const char* filepath) {
	free_rules();
	return load(filepath);
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

int rules_option::update(const char* name, const std::vector<std::string>& hells,
		const std::vector<time_enable>& enables) {
	int n = 0;
	for (auto it = rules_match_.begin(); it != rules_match_.end(); ++it) {
		if ((*it)->update(name, hells, enables)) {
			n++;
		}
	}

	return n;
}

int rules_option::remove(const char* name) {
	int n = 0;
	auto it = rules_match_.begin();
	for (; it != rules_match_.end();) {
		if ((*it)->remove(name)) {
			n++;
		}
		if ((*it)->empty()) {
			delete *it;
			it = rules_match_.erase(it);
		} else {
			++it;
		}
	}

	return n;
}

void rules_option::add(const char* name, int from_hour, int from_min,
		int to_hour, int to_min) {
	int added = 0;
	for (auto it = rules_match_.begin(); it != rules_match_.end(); ++it) {
		if ((*it)->has(name)) {
			(*it)->add(from_hour, from_min, to_hour, to_min);
			++added;
		}
	}

	if (added == 0) {
		rule_match* match = new rule_match;
		(*match).set_names({ name }).set_hells({"127.0.0.1"});
		time_enable enable;
		enable.from_hour = from_hour;
		enable.from_min  = from_min;
		enable.to_hour   = to_hour;
		enable.to_min    = to_min;
		(*match).set_enables({enable});
		rules_match_.push_back(match);
	}
}

void rules_option::get_rules(filter_rules& rules) const {
	for (auto it = rules_match_.begin(); it != rules_match_.end(); ++it) {
		(*it)->get_rules(rules);
	}
}
