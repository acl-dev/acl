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

public:
	int update(const char* name, const std::vector<std::string>& hells,
		const std::vector<time_enable>& enables);

	int remove(const char* name);

	void add(const char* name, int from_hour, int from_min,
		int to_hour, int to_min);

	void get_rules(filter_rules& rules) const;

	bool reload(const char* filepath);

private:
	std::vector<rule_match*> rules_match_;

	void build(const filter_rules& rules);
	void build_add(const filter_rule& rule);

	void free_rules(void);
};
