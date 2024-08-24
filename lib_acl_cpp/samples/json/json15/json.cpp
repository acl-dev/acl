#include "stdafx.h"
#include <getopt.h>

static bool matched(const std::vector<std::string>& tokens, const char* name) {
	for (std::vector<std::string>::const_iterator cit = tokens.begin();
	    cit != tokens.end(); ++cit) {
		if (*cit == name) {
			return true;
		}
	}

	return false;
}

static void usage(const char* procname) {
	printf("usage: %s -h [help[ -f json_file -n names -D [remove if matched]\r\n", procname);
}

int main(int argc, char* argv[]) {
	acl::string file;
	acl::string names;
	bool use_remove = false;
	int ch;

	while ((ch = getopt(argc, argv, "hf:n:D")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'f':
			file = optarg;
			break;
		case 'n':
			names = optarg;
			break;
		case 'D':
			use_remove = true;
			break;
		default:
			break;
		}
	}

	if (file.empty() || names.empty()) {
		usage(argv[0]);
		return 1;
	}

	acl::string buf;
	if (!acl::ifstream::load(file, &buf)) {
		printf("load %s error %s\r\n", file.c_str(), acl::last_serror());
		return 1;
	}

	std::vector<std::string> tokens;
	acl::split(names.c_str(), ",; \t", tokens);

	acl::json json(buf);
	if (!json.finish()) {
		printf("invalid json: %s\r\n", buf.c_str());
		return 1;
	}

	acl::json_node* node = json.first_node();
	while (node) {
		const char* tag = node->tag_name();
		if (tag == NULL || *tag == 0) {
			node = json.next_node();
			continue;
		}

		if (matched(tokens, tag)) {
			if (use_remove) {
				node = json.free_node(node);
			} else {
				node->disable(true);
				node = json.next_node();
			}
		} else {
			node = json.next_node();
		}
	}

	printf("%s\r\n", json.to_string(NULL, true).c_str());
	return 0;
}
