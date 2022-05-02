#pragma once

struct time_enable {
	int from_hour;
	int from_min;
	int to_hour;
	int to_min;
};

struct filter_rule {
	std::vector<std::string> names;
	std::vector<std::string> hells;
	std::vector<time_enable> enables;
};

struct filter_rules {
	std::vector<filter_rule> rules;
};
