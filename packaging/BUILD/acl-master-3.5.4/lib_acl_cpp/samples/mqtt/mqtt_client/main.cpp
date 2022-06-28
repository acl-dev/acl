#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include "acl_cpp/lib_acl.hpp"

static std::vector<acl::string> __topics;

static bool handle_connack(acl::mqtt_client& conn,
		const acl::mqtt_message& message) {
	const acl::mqtt_connack& connack = (const acl::mqtt_connack&) message;
	printf("%s => connect code=%d\r\n", __FUNCTION__, connack.get_connack_code());

	acl::mqtt_subscribe subscribe;

	subscribe.set_pkt_id(100);

	for (std::vector<acl::string>::const_iterator cit = __topics.begin();
		  cit != __topics.end(); ++cit) {
		subscribe.add_topic(*cit, acl::MQTT_QOS1);
		printf("  %s: add topic -> %s\r\n", __FUNCTION__, (*cit).c_str());
	}

	if (conn.send(subscribe)) {
		printf("send subscribe ok\r\n");
		return true;
	}

	printf("send subscribe error\r\n");
	return false;
}

static bool handle_suback(acl::mqtt_client& conn,
		const acl::mqtt_message& message) {
	const acl::mqtt_suback& suback = (const acl::mqtt_suback&) message;
	unsigned short pkt_id = suback.get_pkt_id();

	printf("%s => pkt_id=%d\r\n", __FUNCTION__, pkt_id);

	const std::vector<acl::mqtt_qos_t>& qoses = suback.get_qoses();
	for (std::vector<acl::mqtt_qos_t>::const_iterator cit = qoses.begin();
		cit != qoses.end(); ++cit) {
		printf("  qos=%d\r\n", (int) *cit);
	}

	acl::mqtt_pingreq pingreq;
	if (!conn.send(pingreq)) {
		printf("send pingreq error\r\n");
		return false;
	}
	printf("%s => send pingreq ok\r\n", __FUNCTION__);
	return true;
}

static bool handle_publish(acl::mqtt_client& conn,
		const acl::mqtt_message& message) {
	const acl::mqtt_publish& publish = (const acl::mqtt_publish&) message;
	const acl::string& payload = publish.get_payload();
	printf("topic: %s, qos: %d, pkt_id: %d, payload len: %zd, payload: %s\r\n",
		publish.get_topic(), (int) publish.get_header().get_qos(),
		publish.get_pkt_id(), payload.size(), payload.c_str());

	if (publish.get_header().get_qos() == acl::MQTT_QOS0) {
		return true;
	}

	acl::mqtt_puback puback;
	puback.set_pkt_id(publish.get_pkt_id());
	if (!conn.send(puback)) {
		printf("%s => puback error, pkt_id=%d\r\n",
			__FUNCTION__, puback.get_pkt_id());
		return false;
	}
	printf("%s => puback ok, pkt_id=%d\r\n", __FUNCTION__, puback.get_pkt_id());
	return true;
}

static bool handle_pingreq(acl::mqtt_client& conn) {
	acl::mqtt_pingresp pingresp;
	if (conn.send(pingresp)) {
		return true;
	}
	printf("%s => send pingresp error\r\n", __FUNCTION__);
	return false;
}

static bool handle_pingresp(void) {
	printf("%s => got pingresp\r\n", __FUNCTION__);
	return true;
}

static bool handle_disconnect(acl::mqtt_message&) {
	printf("%s => disconnect\r\n", __FUNCTION__);
	return false;
}

static bool handle_message(acl::mqtt_client& conn, acl::mqtt_message& message) {
	acl::mqtt_type_t type = message.get_header().get_type();
	switch (type) {
	case acl::MQTT_CONNACK:
		return handle_connack(conn, message);
	case acl::MQTT_PINGREQ:
		return handle_pingreq(conn);
	case acl::MQTT_PINGRESP:
		return handle_pingresp();
	case acl::MQTT_DISCONNECT:
		return handle_disconnect(message);
	case acl::MQTT_SUBACK:
		return handle_suback(conn, message);
	case acl::MQTT_PUBLISH:
		return handle_publish(conn, message);
	default:
		printf("unknown type=%d\r\n", (int) type);
		return false;
	}
}

static void usage(const char* procname) {
	printf("usage: %s -h [help] -s addr\r\n", procname);
}

int main(int argc, char* argv[]) {
	char ch;
	acl::string addr("127.0.0.1|1883");

	while ((ch = getopt(argc, argv, "hs:")) > 0) {
		switch (ch) {
		case 'h':
			usage(argv[0]);
			return 0;
		case 's':
			addr = optarg;
			break;
		default:
			break;
		}
	}

	acl::log::stdout_open(true);

	printf("optind: %d, argc: %d\n", optind, argc);
	for (int i = optind; i < argc; i++) {
		printf("add one topic: %s\r\n", argv[i]);
		__topics.push_back(argv[i]);
	}

	if (__topics.empty()) {
		__topics.push_back("test/topic");
	}

	acl::mqtt_connect message;
	message.set_cid("client-id-test-xxx");
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
	printf("send message ok\r\n");

	while (true) {
		acl::mqtt_message* res = conn.get_message();
		if (res == NULL) {
			printf("read message error\r\n");
			break;
		}

		//printf("-----------------------------------------------\r\n");
		//printf("read one message\r\n");
		if (!handle_message(conn, *res)) {
			delete res;
			break;
		}
		delete res;
	}

	return 0;
}
