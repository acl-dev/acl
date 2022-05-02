#pragma once
#include "rules.h"
#include "rules.gson.h"

class rule_match;

class rules_option {
public:
	rules_option(void);
	~rules_option(void);

	bool load(const char* filepath);

	const std::vector<std::string>* get_hells(const char* name,
		acl::string& matched, int& tm_hour, int& tm_min) const;

private:
	std::vector<rule_match*> rules_match_;

	void build(const filter_rules& rules);
	void build_add(const filter_rule& rule);
};
