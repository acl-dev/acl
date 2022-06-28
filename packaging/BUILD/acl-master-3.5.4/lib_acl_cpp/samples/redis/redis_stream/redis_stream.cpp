#include "stdafx.h"

static acl::string __key = "stream_key";

static void xadd(acl::redis_stream& redis, int n)
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

		if (i <= 10) {
			printf("xadd ok, key=%s, id=%s\r\n",
				__key.c_str(), result.c_str());
		}

		fields.clear();
		redis.clear();
	}

	printf("xadd ok, key=%s, n=%d\r\n", __key.c_str(), i);
}

static void xdel(acl::redis_stream& redis,
	const std::vector<acl::string>& ids)
{
	int ret = redis.xdel(__key, ids);
	printf("xdel=%d, key=%s\r\n", ret, __key.c_str());
}

static void xtrim(acl::redis_stream& redis, int n)
{
	int ret = redis.xtrim(__key, (size_t) n);
	printf("xtrim=%d, key=%s, n=%d\r\n", ret, __key.c_str(), n);
}

static void xlen(acl::redis_stream& redis)
{
	int ret = redis.xlen(__key);
	printf("xlen=%d, key=%s\r\n", ret, __key.c_str());
}

static void xgroup_help(acl::redis_stream& redis)
{
	std::vector<acl::string> infos;
	if (redis.xgroup_help(infos) == false) {
		printf("xgroup_help error=%s\r\n", redis.result_error());
	} else {
		printf("xgroup_help ok\r\n");
		for (std::vector<acl::string>::const_iterator cit =
			infos.begin(); cit != infos.end(); ++cit) {

			printf("%s\r\n", (*cit).c_str());
		}
	}
}

static void xgroup_create(acl::redis_stream& redis, const char* group)
{
	if (redis.xgroup_create(__key, group) == false) {
		printf("xgroup_create error=%s, key=%s, group=%s\r\n",
			redis.result_error(), __key.c_str(), group);
	} else {
		printf("xgroup_create ok, key=%s, group=%s\r\n",
			__key.c_str(), group);
	}
}

static void xgroup_destroy(acl::redis_stream& redis, const char* group)
{
	int ret = redis.xgroup_destroy(__key, group);
	if (ret == -1) {
		printf("xgroup_destroy error=%s, key=%s, group=%s\r\n",
			redis.result_error(), __key.c_str(), group);
	} else if (ret == 1) {
		printf("xgroup_destroy ok, key=%s, group=%s\r\n",
			__key.c_str(), group);
	} else if (ret == 0) {
		printf("xgroup_destroy, no such group=%s, key=%s\r\n",
			group, __key.c_str());
	}
}

static void xgroup_delconsumer(acl::redis_stream& redis,
	const char* group, const char* consumer)
{
	int ret = redis.xgroup_delconsumer(__key, group, consumer);
	if (ret < 0) {
		printf("xgroup_delconsumer error=%s, key=%s, group=%s, consumer=%s\r\n",
			redis.result_error(), __key.c_str(), group, consumer);
	} else if (ret == 0) {
		printf("xgroup_delconsumer, no such consumer=%s, group=%s, key=%s\r\n",
			consumer, group, __key.c_str());
	} else {
		printf("xgroup_delconsumer=%d, key=%s, group=%s, consumer=%s\r\n",
			ret, __key.c_str(), group, consumer);
	}
}

static void xgroup_setid(acl::redis_stream& redis, const char* group)
{
	if (redis.xgroup_setid(__key, group)) {
		printf("%s: ok, key=%s, group=%s\r\n",
			__FUNCTION__, __key.c_str(), group);
	} else {
		printf("%s: error=%s, key=%s, group=%s\r\n",
			__FUNCTION__, redis.result_error(), __key.c_str(), group);
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
	size_t i = 0;
	printf("key=%s\r\n", messages.key.c_str());
	for (std::vector<acl::redis_stream_message>::const_iterator
		it = messages.messages.begin(); it != messages.messages.end();
		++it) {

		if (++i <= 10) {
			show_message(*it);
		}
	}
	printf("total messages count=%lu\r\n", (unsigned long) messages.size());
}

static void xreadgroup(acl::redis_stream& redis, const char* group,
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

static void xread(acl::redis_stream& redis, size_t count)
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

static void xack(acl::redis_stream& redis, const char* group,
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

static void xack(acl::redis_stream& redis, const char* group,
	const std::vector<acl::string>& ids, bool detail = true)
{
	int ret = redis.xack(__key, group, ids);
	if (detail) {
		const acl::string* req = redis.request_buf();
		printf("xack=%d, key=%s, group=%s, ids=%ld, req=[%s]\r\n",
			ret, __key.c_str(), group, ids.size(),
			req ? req->c_str() : "NULL");
	} else {
		printf("xack=%d, key=%s, group=%s, id=%ld\r\n", ret,
			__key.c_str(), group, ids.size());
	}
}

static void xreadgroup_with_noack(acl::redis_stream& redis,
	const char* group, const char* consumer)
{
	printf("\r\n");
	acl::redis_stream_messages messages;
	std::map<acl::string, acl::string> streams;
	streams[__key] = ">";

	if (!redis.xreadgroup_with_noack(messages, group, consumer, streams)) {
		const acl::string* req = redis.request_buf();
		printf("xreadgroup_with_noack error=%s, key=%s, group=%s, "
			"conserumer=%s, req=[%s]\r\n", redis.result_error(),
			__key.c_str(), group, consumer, req ? req->c_str() : "null");
	}

	const acl::string* req = redis.request_buf();
	printf("xreadgroup_with_noack ok, key=%s, group=%s, consumer=%s, req=%s\r\n",
		__key.c_str(), group, consumer, req ? req->c_str() : "null");

	if (messages.empty()) {
		printf("no messages\r\n");
		return;
	}

	size_t i = 0;
	for (std::vector<acl::redis_stream_message>::const_iterator cit =
		messages.messages.begin(); cit != messages.messages.end();
		++cit) {

		if (++i <= 10) {
			printf("read one message-id=%s\r\n",  (*cit).id.c_str());
		}
	}
}

static void xreadgroup_with_xack(acl::redis_stream& redis,
	const char* group, const char* consumer)
{
	printf("\r\n");
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
		printf("no messages, ");
		const acl::string* req = redis.request_buf();
		printf("request=[%s]\r\n", req ? req->c_str() : "NULL");
		return;
	}

	size_t i = 0;

	for (std::vector<acl::redis_stream_message>::const_iterator cit =
		messages.messages.begin(); cit != messages.messages.end();
		++cit) {

		if (++i <= 10) {
			printf("read one message-id=%s\r\n",  (*cit).id.c_str());
		}
	}

	printf("\r\n");
	printf("key=%s, begin xack all messages received\r\n", messages.key.c_str());
	for (std::vector<acl::redis_stream_message>::const_iterator cit =
		messages.messages.begin(); cit != messages.messages.end();
		++cit) {

		xack(redis, group, (*cit).id, false);
	}
}

static void xreadgroup_loop(acl::redis_stream& redis, const char* group,
	const char* consumer, size_t count)
{
	std::map<acl::string, acl::string> streams;
	streams[__key] = ">";
	size_t n = 0;
	while (true) {
		acl::redis_stream_messages messages;
		if (!redis.xreadgroup(messages, group, consumer, streams,
			count, 1000, true)) {

			printf("%s: error=%s, key=%s, group=%s, consumer=%s\r\n",
				__FUNCTION__, redis.result_error(), __key.c_str(),
				group, consumer);
			break;
		}

		n += messages.size();
		if (n > 0 && n % 1000 == 0) {
			printf("xreadgroup ok, key=%s, group=%s, consumer=%s, "
				"count=%lu, total=%lu\r\n", __key.c_str(),
				group, consumer, messages.size(), n);
		}

		redis.clear();
	}
}

static void xrange(acl::redis_stream& redis, size_t count)
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

static void xrevrange(acl::redis_stream& redis, size_t count)
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
	printf("greaid=%s\r\n", summary.greatest_id.c_str());

	for (std::vector<acl::redis_pending_consumer>::const_iterator cit =
		summary.consumers.begin(); cit != summary.consumers.end(); ++cit) {
		show_pending_consumer(*cit);
	}
}

static void xpending_summary(acl::redis_stream& redis, const char* group)
{
	acl::redis_pending_summary summary;

	if (redis.xpending_summary(__key, group, summary) == false) {
		printf("xpending_summary error=%s, key=%s, group=%s\r\n",
			redis.result_error(), __key.c_str(), group);
		return;
	}

	if (summary.empty()) {
		printf("xpending_summaray, no consumers, key=%s, group=%s\r\n",
			__key.c_str(), group);
	} else {
		show_xpending_summary(summary);
	}
}

static void show_xpending_detail(const acl::redis_pending_detail& detail)
{
	for (std::map<acl::string, acl::redis_pending_message>::const_iterator
		cit = detail.messages.begin(); cit != detail.messages.end();
		++cit) {

		printf("id=%s, %s, consumer=%s, eplapsed=%llu, delivered=%lu\r\n",
			cit->first.c_str(), cit->second.id.c_str(),
			cit->second.consumer.c_str(), cit->second.elapsed,
			cit->second.delivered);
	}
}

static void xpending_detail(acl::redis_stream& redis, const char* group,
	size_t count)
{
	acl::redis_pending_detail detail;

	if (!redis.xpending_detail(detail, __key, group, "-", "+", count)) {
		printf("xpending_detail error, key=%s, group=%s\r\n",
			__key.c_str(), group);
	} else if (detail.empty()) {
		printf("xpending_detail empty, key=%s, group=%s\r\n",
			__key.c_str(), group);
	} else {
		show_xpending_detail(detail);
	}
}

static void xinfo_help(acl::redis_stream& redis)
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

static void xinfo_consumers(acl::redis_stream& redis, const char* group)
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

static void xinfo_groups(acl::redis_stream& redis)
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

static void xinfo_stream(acl::redis_stream& redis)
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
		"  -S [in single mode, default: no]\r\n"
		"  -s redis_addr[default: 127.0.0.1:6379]\r\n"
		"  -a cmd[default: xadd, xadd|xlen|xdel|xtrim|xgroup_help"
		"xgroup_create|xgroup_destroy|xgroup_delconsumer|xgroup_setid"
		"|xreadgroup|xreadgorup_with_noack|xreadgroup_with_xack"
		"|xreadgroup_loop|xack|xrange|xrevrange|xpending_summary"
		"|xinfo_help|xinfo_consumers|xinfo_groups|xinfo_stream]\r\n"
		"  -g group[default: mygroup]\r\n"
		"  -u consumer[default: consumer]\r\n"
		"  -i message_id\r\n"
		, procname);
}

int main(int argc, char* argv[])
{
	acl::string addr("127.0.0.1:7001"), cmd("xadd"), group("mygroup");
	acl::string consumer("myconsumer"), id;
	std::vector<acl::string> ids;
	bool single_mode = false;
	int ch, n = 10;

	while ((ch = getopt(argc, argv, "hSs:a:n:g:u:i:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 'S':
			single_mode = true;
			break;
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
			ids.push_back(optarg);
			break;
		default:
			break;
		}
	}

	acl::acl_cpp_init();
	acl::log::stdout_open(true);

	acl::redis_client_cluster cluster;
	cluster.set(addr, 0, 5, 5);

	acl::redis_client client(addr, 5, 5);

	acl::redis_stream redis;

	if (single_mode) {
		redis.set_client(&client);
	} else {
		redis.set_cluster(&cluster);
	}

	cmd.lower();

	if (cmd == "xadd") {
		xadd(redis, n);
	} else if (cmd == "xlen") {
		xlen(redis);
	} else if (cmd == "xdel") {
		if (ids.empty()) {
			printf("ids empty\r\n");
		} else {
			xdel(redis, ids);
		}
	} else if (cmd == "xtrim") {
		xtrim(redis, n);
	} else if (cmd == "xgroup_help") {
		xgroup_help(redis);
	} else if (cmd == "xgroup_create") {
		xgroup_create(redis, group);
	} else if (cmd == "xgroup_destroy") {
		xgroup_destroy(redis, group);
	} else if (cmd == "xgroup_delconsumer") {
		xgroup_delconsumer(redis, group, consumer);
	} else if (cmd == "xgroup_setid") {
		xgroup_setid(redis, group);
	} else if (cmd == "xreadgroup") {
		xreadgroup(redis, group, consumer);
	} else if (cmd == "xreadgroup_with_noack") {
		xreadgroup_with_noack(redis, group, consumer);
	} else if (cmd == "xreadgroup_with_xack") {
		xreadgroup_with_xack(redis, group, consumer);
	} else if (cmd == "xreadgroup_loop") {
		xreadgroup_loop(redis, group, consumer, n);
	} else if (cmd == "xread") {
		xread(redis, n);
	} else if (cmd == "xack") {
		if (ids.empty()) {
			printf("message id null\r\n");
		} else {
			xack(redis, group, ids);
		}
	} else if (cmd == "xrange") {
		xrange(redis, n);
	} else if (cmd == "xrevrange") {
		xrevrange(redis, n);
	} else if (cmd == "xpending_summary") {
		xpending_summary(redis, group);
	} else if (cmd == "xpending_detail") {
		xpending_detail(redis, group, (size_t) n);
	} else if (cmd == "xinfo_help") {
		xinfo_help(redis);
	} else if (cmd == "xinfo_consumers") {
		xinfo_consumers(redis, group);
	} else if (cmd == "xinfo_groups") {
		xinfo_groups(redis);
	} else if (cmd == "xinfo_stream") {
		xinfo_stream(redis);
	} else {
		usage(argv[0]);
		return 1;
	}

	return 0;
}
