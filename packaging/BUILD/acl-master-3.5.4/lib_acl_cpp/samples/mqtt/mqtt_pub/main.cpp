#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include "acl_cpp/lib_acl.hpp"

static bool handle_connack(const acl::mqtt_message& message) {
	const acl::mqtt_connack& connack = (const acl::mqtt_connack&) message;
	printf("%s: connect code=%d\r\n", __FUNCTION__, connack.get_connack_code());

	return true;
}

static bool handle_puback(acl::mqtt_client&, const acl::mqtt_message&) {
	return true;
}

static bool test_publish(acl::mqtt_client& conn, unsigned short id) {
	acl::mqtt_publish publish;

	publish.get_header().set_qos(acl::MQTT_QOS1);
	publish.set_pkt_id(id);

	const char* topic = "test/topic";
	publish.set_topic(topic);

	acl::string payload;
	payload.format("payload-%ddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd", id);
	publish.set_payload((unsigned) payload.size(), payload);

	if (!conn.send(publish)) {
		printf("send publish error\r\n");
		return false;
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
	printf("%s => puback ok, pkt id=%d\r\n",
		__FUNCTION__, suback->get_pkt_id());

	delete res;
	return true;
}

static void usage(const char* procname) {
	printf("usage: %s -h [help] -s addr -n max\r\n", procname);
}

int main(int argc, char* argv[]) {
	char ch;
	int  max = 1;
	acl::string addr("127.0.0.1|1883");

	while ((ch = getopt(argc, argv, "hs:n:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			addr = optarg;
			break;
		case 'n':
			max = atoi(optarg);
			break;
		default:
			break;
		}
	}

	acl::log::stdout_open(true);

	acl::mqtt_connect message;
	message.set_cid("client-id-zsx-pub");
	message.set_username("user-zsx");
	//message.set_passwd("pass");
#if 0
	message.set_will_qos(acl::MQTT_QOS0);
	message.set_will_topic("test/topic");
	message.set_will_msg("msg-hello");
#endif

	printf("-----------------------------------------------\r\n");
	acl::mqtt_client conn(addr, 10, 0);
	if (!conn.send(message)) {
		printf("send message error\r\n");
		return 1;
	}
	printf("send connect message ok\r\n");

	acl::mqtt_message* res = conn.get_message();
	if (res == NULL) {
		printf("read CONNACK error\r\n");
		return 1;
	}
	acl::mqtt_type_t type = res->get_header().get_type();
	if (type != acl::MQTT_CONNACK) {
		printf("invalid message type=%d\r\n", (int) type);
		delete res;
		return 1;
	}
	if (!handle_connack(*res)) {
		delete res;
		return 1;
	}

	unsigned short id = 1;
	for (int i = 1; i <= max; i++) {
		if (!test_publish(conn, id++)) {
			break;
		}
		// id must be more than 0
		if (id == 0) {
			id = 1;
		}
	}
	return 0;
}
