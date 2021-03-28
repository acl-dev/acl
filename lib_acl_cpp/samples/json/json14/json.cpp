#include "stdafx.h"
#include <getopt.h>

static void usage(const char* procname) {
	printf("usage: %s -h [help[ -f json_file\r\n", procname);
}

int main(int argc, char* argv[]) {
	acl::string file;
	int ch;

	while ((ch = getopt(argc, argv, "hf:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'f':
			file = optarg;
			break;
		default:
			break;
		}
	}

	if (file.empty()) {
		usage(argv[0]);
		return 1;
	}

	acl::string buf;
	if (!acl::ifstream::load(file, &buf)) {
		printf("load %s error %s\r\n", file.c_str(), acl::last_serror());
		return 1;
	}

	acl::json json(buf);
	if (!json.finish()) {
		printf("invalid json: %s\r\n", buf.c_str());
		return 1;
	}

	printf("parse json ok: %s\r\n", json.to_string().c_str());

	json.reset();
	json.get_root().add_array(true)
		.add_array_text("name1")
		.add_array_text("name2")
		.add_array_text("name3");
	printf("build json: %s\r\n", json.to_string().c_str());

	json.reset();
	json.get_root().add_text("name1", "value1")
		.add_text("name2", "value2")
		.add_text("name3", "value3")
		.add_number("name4", 100);
	printf("build json: %s\r\n", json.to_string().c_str());

	json.reset();
	json.get_root().add_text("name1", "value1");
	printf("build json: %s\r\n", json.to_string().c_str());

	json.reset();
	json.get_root().add_child(false, true)
		.add_text("name1", "value1")
		.add_text("name2", "value2")
		.add_text("name3", "value3");
	printf("build json: %s\r\n", json.to_string().c_str());

	json.reset();
	json.get_root().add_child(false, true)
		.add_text("name1", "value1");
	acl::json_node& node = json.create_node("name2", "value2");
	json.get_root().add_child(node);
	node.set_tag("name3");
	node.set_text("value3");
	printf("build json: %s\r\n", json.to_string().c_str());

	return 0;
}
