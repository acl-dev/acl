#include "stdafx.h"

static acl::string __key = "stream_key";

static void test_xadd(acl::redis_stream& redis, int n)
{
	acl::string name, value, result;
	std::map<acl::string, acl::string> fields;

	int i;
	for (i = 0; i < n; i++) {
		name.format("name-%d-1", i);
		value.format("value-%d-1", i);
		fields[name] = value;

		name.format("name-%d-2", i);
		value.format("value-%d-2", i);
		fields[name] = value;

		name.format("name-%d-3", i);
		value.format("value-%d-3", i);
		fields[name] = value;

		if (redis.xadd(__key, fields, result) == false) {
			printf("xadd error=%s\r\n", redis.result_error());
			break;
		}

		printf("xadd ok, key=%s, id=%s\r\n",
			__key.c_str(), result.c_str());

		fields.clear();
	}

	printf("xadd ok, key=%s, n=%d\r\n", __key.c_str(), i);
}

static void test_xdel(acl::redis_stream& redis, const char* id)
{
	std::vector<acl::string> ids;
	ids.push_back(id);
	int ret = redis.xdel(__key, ids);
	printf("xdel=%d, key=%s\r\n", ret, __key.c_str());
}

static void test_xtrim(acl::redis_stream& redis, int n)
{
	int ret = redis.xtrim(__key, (size_t) n);
	printf("xtrim=%d, key=%s, n=%d\r\n", ret, __key.c_str(), n);
}

static void test_xlen(acl::redis_stream& redis)
{
	int ret = redis.xlen(__key);
	printf("xlen=%d, key=%s\r\n", ret, __key.c_str());
}

static void test_xgroup_create(acl::redis_stream& redis, const char* group)
{
	if (redis.xgroup_create(__key, group) == false) {
		printf("xgroup_create error=%s, key=%s, group=%s\r\n",
			redis.result_error(), __key.c_str(), group);
	} else {
		printf("xgroup_create ok, key=%s, group=%s\r\n",
			__key.c_str(), group);
	}
}

static void show_message(const acl::redis_stream_message& message)
{
	printf("\tid=%s\r\n", message.id.c_str());
	for (std::vector<acl::redis_stream_field>::const_iterator cit
		= message.fields.begin(); cit != message.fields.end(); ++cit) {
		printf("\t\tname=%s, value=%s\r\n", (*cit).name.c_str(),
			(*cit).value.c_str());
	}
}

static void show_messages(const acl::redis_stream_messages& messages)
{
	printf("key=%s\r\n", messages.key.c_str());
	for (std::vector<acl::redis_stream_message>::const_iterator
		it = messages.messages.begin(); it != messages.messages.end();
		++it) {

		show_message(*it);
	}
	printf("total messages count=%lu\r\n", (unsigned long) messages.size());
}

static void test_xreadgroup(acl::redis_stream& redis, const char* group,
	const char* consumer)
{
	acl::redis_stream_messages messages;
	std::map<acl::string, acl::string> streams;
	streams[__key] = ">";
	if (redis.xreadgroup(messages, group, consumer, streams) == false) {
		printf("xreadgroup error=%s, key=%s, group=%s, consumer=%s\r\n",
			redis.result_error(), __key.c_str(), group, consumer);
		const acl::string* req = redis.request_buf();
		printf("request=[%s]\r\n", req ? req->c_str() : "NULL");
		return;
	}

	printf("xreadgroup ok, key=%s, group=%s, consumer=%s\r\n",
		__key.c_str(), group, consumer);

	if (messages.empty()) {
		printf("no messages\r\n");
	} else {
		show_messages(messages);
	}
}

static void test_xread(acl::redis_stream& redis, size_t count)
{
	acl::redis_stream_messages messages;
	std::map<acl::string, acl::string> streams;
	streams[__key] = "0-0";

	if (redis.xread(messages, streams, count) == false) {
		printf("xread error=%s, key=%s\r\n",
			redis.result_error(), __key.c_str());
		const acl::string* req = redis.request_buf();
		printf("request=[%s]\r\n", req ? req->c_str() : "NULL");
		return;
	}

	printf("xread ok, key=%s\r\n", __key.c_str());

	if (messages.empty()) {
		printf("no messages\r\n");
	} else {
		show_messages(messages);
	}
}

static void test_xack(acl::redis_stream& redis, const char* group,
	const char* id, bool detail = true)
{
	int ret = redis.xack(__key, group, id);
	if (detail) {
		const acl::string* req = redis.request_buf();
		printf("xack=%d, key=%s, group=%s, id=%s, req=[%s]\r\n", ret,
			__key.c_str(), group, id, req ? req->c_str() : "NULL");
	} else {
		printf("xack=%d, key=%s, group=%s, id=%s\r\n", ret,
			__key.c_str(), group, id);
	}
}

static void test_xreadgroup_with_xack(acl::redis_stream& redis,
	const char* group, const char* consumer)
{
	acl::redis_stream_messages messages;
	std::map<acl::string, acl::string> streams;
	streams[__key] = ">";

	if (redis.xreadgroup(messages, group, consumer, streams) == false) {
		printf("xreadgroup error=%s, key=%s, group=%s, consumer=%s\r\n",
			redis.result_error(), __key.c_str(), group, consumer);
		const acl::string* req = redis.request_buf();
		printf("request=[%s]\r\n", req ? req->c_str() : "NULL");
		return;
	}

	printf("xreadgroup ok, key=%s, group=%s, consumer=%s\r\n",
		__key.c_str(), group, consumer);

	if (messages.empty()) {
		printf("no messages\r\n");
		return;
	}

	printf("key=%s, begin xack all messages received\r\n", messages.key.c_str());
	for (std::vector<acl::redis_stream_message>::const_iterator cit =
		messages.messages.begin(); cit != messages.messages.end();
		++cit) {

		test_xack(redis, group, (*cit).id, false);
	}
}

static void test_xrange(acl::redis_stream& redis, size_t count)
{
	acl::redis_stream_messages messages;

	if (redis.xrange(messages, __key, "-", "+", count) == false) {
		printf("xrange error=%s, key=%lu\r\n",
			redis.result_error(), (unsigned long) count);
		return;
	}

	printf("xrange ok, key=%s\r\n", __key.c_str());

	if (messages.empty()) {
		printf("no messages\r\n");
		const acl::string* req = redis.request_buf();
		printf("request=[%s]\r\n", req ? req->c_str() : "NULL");
	} else {
		show_messages(messages);
	}
}

static void test_xrevrange(acl::redis_stream& redis, size_t count)
{
	acl::redis_stream_messages messages;

	if (redis.xrevrange(messages, __key, "+", "-", count) == false) {
		printf("xrevrange error=%s, key=%lu\r\n",
			redis.result_error(), (unsigned long) count);
		return;
	}

	printf("xrevrange ok, key=%s\r\n", __key.c_str());

	const acl::string* req = redis.request_buf();
	printf("request=[%s]\r\n", req ? req->c_str() : "NULL");

	if (messages.empty()) {
		printf("no messages\r\n");
	} else {
		show_messages(messages);
	}
}

static void show_pending_consumer(const acl::redis_pending_consumer& consumer)
{
	printf("\tconsumer=%s\r\n", consumer.name.c_str());
	printf("\tpending_number=%lu\r\n", (unsigned long) consumer.pending_number);
}

static void show_xpending_summary(const acl::redis_pending_summary& summary)
{
	printf("smallest_id=%s\r\n", summary.smallest_id.c_str());
	printf("greatest_id=%s\r\n", summary.greatest_id.c_str());

	for (std::vector<acl::redis_pending_consumer>::const_iterator cit =
		summary.consumers.begin(); cit != summary.consumers.end(); ++cit) {
		show_pending_consumer(*cit);
	}
}

static void test_xpending_summary(acl::redis_stream& redis, const char* group)
{
	acl::redis_pending_summary summary;

	if (redis.xpending_summary(__key, group, summary) == false) {
		printf("xpending_summary error=%s, key=%s, group=%s\r\n",
			redis.result_error(), __key.c_str(), group);
		return;
	}

	show_xpending_summary(summary);
}

static void test_xinfo_help(acl::redis_stream& redis)
{
	std::vector<acl::string> infos;
	if (redis.xinfo_help(infos)) {
		for (std::vector<acl::string>::const_iterator cit = infos.begin();
			cit != infos.end(); ++cit) {

			printf("%s\r\n", (*cit).c_str());
		}
	} else {
		printf("xinfo_help error=%s\r\n", redis.result_error());
	}
}

static void show_xinfo_consumer(const acl::redis_xinfo_consumer& info)
{
	printf("\tconsumer=%s\r\n", info.name.c_str());
	printf("\tpending=%lu\r\n", (unsigned long) info.pending);
	printf("\tidle=%lu\r\n", (unsigned long) info.idle);
}

static void test_xinfo_consumers(acl::redis_stream& redis, const char* group)
{
	std::map<acl::string, acl::redis_xinfo_consumer> result;

	if (redis.xinfo_consumers(__key, group, result) == false) {
		printf("xinfo_consumers error=%s, key=%s, group=%s\r\n",
			redis.result_error(), __key.c_str(), group);
		return;
	}

	printf("key=%s, group=%s\r\n", __key.c_str(), group);

	for (std::map<acl::string, acl::redis_xinfo_consumer>::const_iterator
		cit = result.begin(); cit != result.end(); ++cit) {

		show_xinfo_consumer(cit->second);
	}
}

static void show_xinfo_group(const acl::redis_xinfo_group& info)
{
	printf("\tgroup=%s\r\n", info.name.c_str());
	printf("\tlast_delivered_id=%s\r\n", info.last_delivered_id.c_str());
	printf("\tconsumers=%lu\r\n", (unsigned long) info.consumers);
	printf("\tpending=%lu\r\n", (unsigned long) info.pending);
}

static void test_xinfo_groups(acl::redis_stream& redis)
{
	std::map<acl::string, acl::redis_xinfo_group> result;

	if (redis.xinfo_groups(__key, result) == false) {
		printf("xinfo_groups error=%s, key=%s\r\n",
			redis.result_error(), __key.c_str());
		return;
	}

	printf("key=%s\r\n", __key.c_str());
	for (std::map<acl::string, acl::redis_xinfo_group>::const_iterator
		cit = result.begin(); cit != result.end(); ++cit) {

		show_xinfo_group(cit->second);
	}
}

static void test_xinfo_stream(acl::redis_stream& redis)
{
	acl::redis_stream_info info;

	if (redis.xinfo_stream(__key, info) == false) {
		printf("xinfo_stream error=%s, key=%s\r\n",
			redis.result_error(), __key.c_str());
		return;
	}

	printf("key=%s\r\n", __key.c_str());
	printf("\tlength=%lu\r\n", (unsigned long) info.length);
	printf("\tradix_tree_keys=%lu\r\n", (unsigned long) info.radix_tree_keys);
	printf("\tradix_tree_nodes=%lu\r\n", (unsigned long) info.radix_tree_nodes);
	printf("\tgroups=%lu\r\n", (unsigned long) info.groups);
	printf("\tlast_generated_id=%s\r\n", info.last_generated_id.c_str());
	printf("\tfirst_entry:\r\n");
	show_message(info.first_entry);
	printf("\tlast_entry:\r\n");
	show_message(info.last_entry);
}

static void usage(const char* procname)
{
	printf("usage: %s -h [help]\r\n"
		"  -s redis_addr[default: 127.0.0.1:6379]\r\n"
		"  -a cmd[default: xadd, xadd|xlen|xdel|xgroup_create"
		"|xreadgroup|xreadgroup_with_xack|xack|xrange|xrevrange"
		"|xpending_summary|xinfo_help|xinfo_consumers|xinfo_groups"
		"|xinfo_stream]\r\n"
		"  -g group[default: mygroup]\r\n"
		"  -u consumer[default: consumer]\r\n"
		"  -i message_id\r\n"
		, procname);
}

int main(int argc, char* argv[])
{
	acl::string addr("127.0.0.1:16379"), cmd("xadd"), group("mygroup");
	acl::string consumer("myconsumer"), id;
	int ch, n = 10;

	while ((ch = getopt(argc, argv, "hs:a:n:g:u:i:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			addr = optarg;
			break;
		case 'a':
			cmd = optarg;
			break;
		case 'n':
			n = atoi(optarg);
			break;
		case 'g':
			group = optarg;
			break;
		case 'u':
			consumer = optarg;
			break;
		case 'i':
			id = optarg;
			break;
		default:
			break;
		}
	}

	acl::acl_cpp_init();
	acl::log::stdout_open(true);

	acl::redis_client_cluster cluster;
	cluster.set(addr, 0, 5, 5);

	acl::redis_stream redis;
	redis.set_cluster(&cluster, 0);

	cmd.lower();

	if (cmd == "xadd") {
		test_xadd(redis, n);
	} else if (cmd == "xlen") {
		test_xlen(redis);
	} else if (cmd == "xdel") {
		test_xdel(redis, id);
	} else if (cmd == "xtrim") {
		test_xtrim(redis, n);
	} else if (cmd == "xgroup_create") {
		test_xgroup_create(redis, group);
	} else if (cmd == "xreadgroup") {
		test_xreadgroup(redis, group, consumer);
	} else if (cmd == "xreadgroup_with_xack") {
		test_xreadgroup_with_xack(redis, group, consumer);
	} else if (cmd == "xread") {
		test_xread(redis, n);
	} else if (cmd == "xack") {
		if (id.empty()) {
			printf("message id null\r\n");
		} else {
			test_xack(redis, group, id);
		}
	} else if (cmd == "xrange") {
		test_xrange(redis, n);
	} else if (cmd == "xrevrange") {
		test_xrevrange(redis, n);
	} else if (cmd == "xpending_summary") {
		test_xpending_summary(redis, group);
	} else if (cmd == "xinfo_help") {
		test_xinfo_help(redis);
	} else if (cmd == "xinfo_consumers") {
		test_xinfo_consumers(redis, group);
	} else if (cmd == "xinfo_groups") {
		test_xinfo_groups(redis);
	} else if (cmd == "xinfo_stream") {
		test_xinfo_stream(redis);
	} else {
		usage(argv[0]);
		return 1;
	}

	return 0;
}
