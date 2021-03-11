#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include "acl_cpp/lib_acl.hpp"

static std::vector<acl::string> __topics;

class mqtt_client : public acl::mqtt_aclient {
public:
	mqtt_client(acl::aio_handle& handle);

protected:
	~mqtt_client(void);

	// @override
	void destroy(void) { delete this; }

	// @override
	bool on_open(void);

	// @override
	void on_ns_failed(void) {}

	// @override
	void on_connect_timeout(void) {}

	// @override
	void on_connect_failed(void) {}

	// @override
	void on_disconnect(void) {}

	// @override
	bool on_header(const acl::mqtt_header& header);

	// @override
	bool on_body(const acl::mqtt_message& body);

private:
	bool handle_connack(const acl::mqtt_message& body);
	bool handle_suback(const acl::mqtt_message& body);
	bool handle_pingreq(const acl::mqtt_message& body);
	bool handle_pingresp(const acl::mqtt_message& body);
	bool handle_disconnect(const acl::mqtt_message& body);
	bool handle_publish(const acl::mqtt_message& body);
};

mqtt_client::mqtt_client(acl::aio_handle& handle)
: acl::mqtt_aclient(handle) {}

mqtt_client::~mqtt_client(void) {}

bool mqtt_client::on_open(void) {
	acl::mqtt_connect message;
	message.set_cid("aclient-id-test-xxx");
	message.set_username("user-zsx");
	//message.set_passwd("pass");
#if 0
	message.set_will_qos(acl::MQTT_QOS0);
	message.set_will_topic("test/topic");
	message.set_will_msg("msg-hello");
#endif
	if (this->send(message)) {
		printf("send connect message ok\r\n");
		return true;
	}

	printf("send connect message error\r\n");
	return false;
}

bool mqtt_client::on_header(const acl::mqtt_header& header) {
	printf("got new mqtt header, type=%s\r\n",
		acl::mqtt_type_desc(header.get_type()));
	return true;
}

bool mqtt_client::on_body(const acl::mqtt_message& body) {
	acl::mqtt_type_t type = body.get_header().get_type();
	switch (type) {
	case acl::MQTT_CONNACK:
		return handle_connack(body);
	case acl::MQTT_SUBACK:
		return handle_suback(body);
	case acl::MQTT_PINGREQ:
		return handle_pingreq(body);
	case acl::MQTT_PINGRESP:
		return handle_pingresp(body);
	case acl::MQTT_DISCONNECT:
		return handle_disconnect(body);
	case acl::MQTT_PUBLISH:
		return handle_publish(body);
	default:
		printf("unknown type=%d, %s\r\n",
			(int) type, acl::mqtt_type_desc(type));
		return true;
	}
}

bool mqtt_client::handle_connack(const acl::mqtt_message& body) {
	(void) body;

	acl::mqtt_subscribe subscribe;

	subscribe.set_pkt_id(100);

	for (std::vector<acl::string>::const_iterator cit = __topics.begin();
		  cit != __topics.end(); ++cit) {
		subscribe.add_topic(*cit, acl::MQTT_QOS1);
		printf("  %s: add sub topic -> %s\r\n",
			__FUNCTION__, (*cit).c_str());
	}

	return this->send(subscribe);
}

bool mqtt_client::handle_suback(const acl::mqtt_message& body) {
	const acl::mqtt_suback& suback = (const acl::mqtt_suback&) body;
	unsigned short pkt_id = suback.get_pkt_id();

	printf("%s => pkt_id=%d\r\n", __FUNCTION__, pkt_id);

	const std::vector<acl::mqtt_qos_t>& qoses = suback.get_qoses();
	for (std::vector<acl::mqtt_qos_t>::const_iterator cit = qoses.begin();
		cit != qoses.end(); ++cit) {
		printf("  qos=%d\r\n", (int) *cit);
	}

	acl::mqtt_pingreq pingreq;
	printf("%s => send pingreq\r\n", __FUNCTION__);
	return this->send(pingreq);
}

bool mqtt_client::handle_pingreq(const acl::mqtt_message&) {
	acl::mqtt_pingresp pingresp;
	if (this->send(pingresp)) {
		return true;
	}
	printf("%s => send pingresp error\r\n", __FUNCTION__);
	return false;
}

bool mqtt_client::handle_pingresp(const acl::mqtt_message&) {
	printf("%s => got pingresp\r\n", __FUNCTION__);
	return true;
}

bool mqtt_client::handle_disconnect(const acl::mqtt_message&) {
	printf("%s => disconnect\r\n", __FUNCTION__);
	return false;
}

bool mqtt_client::handle_publish(const acl::mqtt_message& body) {
	const acl::mqtt_publish& publish = (const acl::mqtt_publish&) body;
	const acl::string& payload = publish.get_payload();
	printf("topic: %s, qos: %d, pkt_id: %d, payload: %s\r\n",
		publish.get_topic(), (int) publish.get_header().get_qos(),
		publish.get_pkt_id(), payload.c_str());

	if (publish.get_header().get_qos() == acl::MQTT_QOS0) {
		return true;
	}

	acl::mqtt_puback puback;
	puback.set_pkt_id(publish.get_pkt_id());
	if (!this->send(puback)) {
		printf("%s => puback error, pkt_id=%d\r\n",
			__FUNCTION__, puback.get_pkt_id());
		return false;
	}
	printf("%s => puback ok, pkt_id=%d\r\n",
		__FUNCTION__, puback.get_pkt_id());
	return true;
}

//////////////////////////////////////////////////////////////////////////////

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

	acl::aio_handle handle(acl::ENGINE_KERNEL);
	acl::mqtt_aclient* conn = new mqtt_client(handle);
	if (conn->open(addr, 10, 0)) {
		printf("begin connect server continue\r\n");
	} else {
		printf("open %s error %s\r\n", addr.c_str(), acl::last_serror());
		return 1;
	}

	while (handle.check()) {}
	handle.check();

	return 0;
}
