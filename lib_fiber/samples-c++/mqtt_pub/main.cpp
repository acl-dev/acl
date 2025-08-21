#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include "lib_acl.h"
#include "acl_cpp/lib_acl.hpp"
#include "fiber/libfiber.hpp"

static bool handle_connack(const acl::mqtt_message& message) {
	const acl::mqtt_connack& connack = (const acl::mqtt_connack&) message;
	printf("%s: connect code=%d\r\n", __FUNCTION__, connack.get_connack_code());

	return true;
}

static bool handle_puback(acl::mqtt_client&, const acl::mqtt_message&) {
	return true;
}

static int __count = 0;
static bool __debug = false;

static bool test_publish(acl::mqtt_client& conn, const char* topic,
		unsigned short id, const acl::string& payload) {
	acl::mqtt_publish publish;

	publish.get_header().set_qos(acl::MQTT_QOS1);
	publish.set_pkt_id(id);

	publish.set_topic(topic);

	publish.set_payload((unsigned) payload.size(), payload.c_str());

	if (!conn.send(publish)) {
		printf("send publish error\r\n");
		return false;
	}

	if (__debug) {
		ACL_VSTREAM* vs = conn.sock_stream()->get_vstream();
		assert(vs);

		printf("payload->size=%zd, total sent size=%lld, send count=%d\n",
			payload.size(), vs->total_write_cnt, ++__count);
	}

	if (publish.get_header().get_qos() == acl::MQTT_QOS0) {
		return true;
	}

	acl::mqtt_message* res = conn.get_message();
	if (res == NULL) {
		printf("get puback error\r\n");
		return false;
	}

	if (res->get_header().get_type() != acl::MQTT_PUBACK) {
		printf("not puback type, type=%d\r\n",
			(int) res->get_header().get_type());
		delete res;
		return false;
	}

	if (!handle_puback(conn, *res)) {
		printf("handle_puback error\r\n");
		delete res;
		return false;
	}

	acl::mqtt_suback* suback = (acl::mqtt_suback*) res;
	if (__debug) {
		printf("%s => puback ok, pkt id=%d\r\n",
			__FUNCTION__, suback->get_pkt_id());
	}

	delete res;
	return true;
}

class mqtt_pub : public acl::fiber {
public:
	mqtt_pub(const char* addr, const char* topic, const char* payload,
		int max, int uid)
	: addr_(addr), topic_(topic), payload_(payload), max_(max), uid_(uid)
	{}


protected:
	~mqtt_pub() {}

	void run() {
		doit();
		delete this;
	}

private:
	void doit() {
		acl::mqtt_connect message;
		acl::string cid;
		cid.format("pub-%d", uid_);
		message.set_cid(cid);
		message.set_username("user-zsx");
		message.set_passwd("pass");
#if 0
		message.set_will_qos(acl::MQTT_QOS0);
		message.set_will_topic("test/topic");
		message.set_will_msg("msg-hello");
#endif

		printf("-----------------------------------------------\r\n");
		acl::mqtt_client conn(addr_, 10, 0);
		if (!conn.send(message)) {
			printf("send message error\r\n");
			return;
		}
		printf("send connect message ok\r\n");

		acl::mqtt_message* res = conn.get_message();
		if (res == NULL) {
			printf("read CONNACK error\r\n");
			return;
		}
		acl::mqtt_type_t type = res->get_header().get_type();
		if (type != acl::MQTT_CONNACK) {
			printf("invalid message type=%d\r\n", (int) type);
			delete res;
			return;
		}
		if (!handle_connack(*res)) {
			delete res;
			return;
		}

		delete res;

		unsigned short id = 1;
		for (int i = 1; i <= max_; i++) {
			if (!test_publish(conn, topic_, id++, payload_)) {
				break;
			}

			// id must be more than 0
			if (id == 0) {
				id = 1;
			}
		}
	}

private:
	acl::string addr_, topic_, payload_;
	int max_, uid_;
};

static void usage(const char* procname) {
	printf("usage: %s -h [help] -s addrs -t topic -n max -c cocurrent -D [if in debug mode]\r\n", procname);
}

int main(int argc, char* argv[]) {
	char ch;
	int  max = 1, cocurrent = 1;
	acl::string addresses("127.0.0.1|1883");
	acl::string topic("test/topic");
	acl::string payload = "{\"topic\":\"test1\",\"from\":\"test2\",\"to\":\"test2\",\"msg\":\"hello world\"}";

	while ((ch = getopt(argc, argv, "hs:n:t:c:D")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			addresses = optarg;
			break;
		case 'n':
			max = atoi(optarg);
			break;
		case 't':
			topic = optarg;
			break;
		case 'c':
			cocurrent = atoi(optarg);
			break;
		case 'D':
			__debug = true;
			break;
		default:
			break;
		}
	}

	acl::log::stdout_open(true);

	std::vector<acl::string>& addrs = addresses.split2(",;");
	for (int i = 0; i < cocurrent; i++) {
		acl::fiber* fb = new mqtt_pub(addrs[i%addrs.size()],
			topic, payload, max, i);
		fb->start();
	}

	acl::fiber::schedule();
	return 0;
}
