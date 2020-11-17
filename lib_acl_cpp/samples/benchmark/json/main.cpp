#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <yyjson.h>
#include "simdjson.h"
#include <cjson/cJSON.h>
#include <rapidjson/reader.h>

#include "lib_acl.h"
#include "acl_cpp/lib_acl.hpp"

static int yyjson_test(const acl::string& data, int count) {
	size_t dlen = data.size();
	int i;

	for (i = 0; i < count; i++) {
		yyjson_doc* json = yyjson_read(data, dlen, 0);
		if (json) {
			yyjson_doc_free(json);
		} else {
			printf("parse json error\r\n");
			break;
		}
	}

	return i;
}

static int simdjson_test(const acl::string& in, int count) {
	int i;

	std::string data =in.c_str();
	for (i = 0; i < count; i++) {
		simdjson::dom::parser parser;
		parser.parse(data);
	}

	return i;
}

static int cjson_test(const acl::string& in, int count) {
	int i;

	for (i = 0; i < count; i++) {
		cJSON* json = cJSON_Parse(in.c_str());
		if (json) {
			cJSON_Delete(json);
		} else {
			printf("cJSON_Parse parse error\r\n");
			break;
		}
	}

	return i;
}

using namespace rapidjson;
using namespace std;

struct MyHandler {
	bool Null() { return true; }
	bool Bool(bool) { return true; }
	bool Int(int) { return true; }
	bool Uint(unsigned) { return true; }
	bool Int64(int64_t) { return true; }
	bool Uint64(uint64_t) { return true; }
	bool Double(double) { return true; }
	bool RawNumber(const char*, SizeType, bool) { return true; }
	bool String(const char*, SizeType, bool) { return true; }
	bool StartObject() { return true; }
	bool Key(const char*, SizeType, bool) { return true; }
	bool EndObject(SizeType) { return true; }
	bool StartArray() { return true; }
	bool EndArray(SizeType) { return true; }
};

static int rapidjson_test(const acl::string& in, int count) {
	int i;

	for (i = 0; i < count; i++) {
		MyHandler handler;
		Reader reader;
		StringStream ss(in.c_str());
		reader.Parse(ss, handler);
	}

	return i;
}

static int acl_cppjson_test(const acl::string& data, int count) {
	int i;

	for (i = 0; i < count; i++) {
		acl::json json(data);
		//json.update(data);
		if (!json.finish()) {
			printf("acl cppjson: invalid json\r\n");
			break;
		}
		//json.reset();
	}

	return i;
}

static int acl_cjson_test(const acl::string& data, int count) {
	int i;

	for (i = 0; i < count; i++) {
		ACL_JSON* json = acl_json_alloc();
		acl_json_update(json, data.c_str());
		if (!acl_json_finish(json)) {
			printf("acl cjson: invalid json\r\n");
			break;
		}
		acl_json_free(json);
		//acl_json_reset(json);
	}

	return i;
}

static void usage(const char* procname) {
	printf("usage: %s -h [help] -f filename, -n max_loop\r\n", procname);
}

int main(int argc, char* argv[]) {
	char ch;
	int max = 100;
	acl::string filename = "./test.json";

	while ((ch = getopt(argc, argv, "hf:n:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'f':
			filename = optarg;
			break;
		case 'n':
			max = atoi(optarg);
			break;
		default:
			break;
		}
	}

	acl::string data;
	if (!acl::ifstream::load(filename, data)) {
		printf("load data from %s error %s\r\n",
			filename.c_str(), acl::last_serror());
		return 1;
	}

	size_t dlen = data.size();

	struct timeval begin;
	gettimeofday(&begin, NULL);

	int count = yyjson_test(data, max);

	struct timeval end;
	gettimeofday(&end, NULL);
	double cost = acl::stamp_sub(end, begin);
	double speed = (count * 1000) / (cost > 0 ? cost : 0.1);
	double size = (dlen * speed) / (1024 * 1024);

	printf("yyjson: count=%d, cost=%.2f ms, speed=%.2f, size=%.2f MB\r\n",
		max, cost, speed, size);

	/////////////////////////////////////////////////////////////////////

	gettimeofday(&begin, NULL);
	count = simdjson_test(data, max);
	gettimeofday(&end, NULL);

	cost = acl::stamp_sub(end, begin);
	speed = (count * 1000) / (cost > 0 ? cost : 0.1);
	size = (dlen * speed) / (1024 * 1024);

	printf("simdjson: count=%d, cost=%.2f ms, speed=%.2f, size=%.2f MB\r\n",
		max, cost, speed, size);

	/////////////////////////////////////////////////////////////////////

	gettimeofday(&begin, NULL);
	count = cjson_test(data, max);
	gettimeofday(&end, NULL);

	cost = acl::stamp_sub(end, begin);
	speed = (count * 1000) / (cost > 0 ? cost : 0.1);
	size = (dlen * speed) / (1024 * 1024);

	printf("cjson: count=%d, cost=%.2f ms, speed=%.2f, size=%.2f MB\r\n",
		max, cost, speed, size);


	/////////////////////////////////////////////////////////////////////

	gettimeofday(&begin, NULL);
	count = rapidjson_test(data, max);
	gettimeofday(&end, NULL);

	cost = acl::stamp_sub(end, begin);
	speed = (count * 1000) / (cost > 0 ? cost : 0.1);
	size = (dlen * speed) / (1024 * 1024);

	printf("rapidjson: count=%d, cost=%.2f ms, speed=%.2f, size=%.2f MB\r\n",
		max, cost, speed, size);


	/////////////////////////////////////////////////////////////////////

	gettimeofday(&begin, NULL);
	count = acl_cppjson_test(data, max);
	gettimeofday(&end, NULL);

	cost = acl::stamp_sub(end, begin);
	speed = (count * 1000) / (cost > 0 ? cost : 0.1);
	size = (dlen * speed) / (1024 * 1024);

	printf("acl_cppjson: count=%d, cost=%.2f ms, speed=%.2f, size=%.2f MB\r\n",
		max, cost, speed, size);

	/////////////////////////////////////////////////////////////////////

	gettimeofday(&begin, NULL);
	count = acl_cjson_test(data, max);
	gettimeofday(&end, NULL);

	cost = acl::stamp_sub(end, begin);
	speed = (count * 1000) / (cost > 0 ? cost : 0.1);
	size = (dlen * speed) / (1024 * 1024);

	printf("acl_cjson: count=%d, cost=%.2f ms, speed=%.2f, size=%.2f MB\r\n",
		max, cost, speed, size);

	return 0;
}
