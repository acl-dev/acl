#pragma once
#include "rules.h"
#include "rules.gson.h"

class rules_option {
public:
	rules_option(void);
	~rules_option(void);

	bool load(const char* filepath);

	bool is_blocked(const char* name, int* hour, int* min,
		std::vector<std::string>& hells) const;

private:
	filter_rules rules_;
};