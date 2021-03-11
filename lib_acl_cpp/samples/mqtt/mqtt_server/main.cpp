#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include "acl_cpp/lib_acl.hpp"

static bool handle_connect(acl::mqtt_client& conn, const acl::mqtt_message&) {
	acl::mqtt_connack connack;
	connack.set_connack_code(acl::MQTT_CONNACK_OK);
	printf("%s => send connack\r\n", __FUNCTION__);

	if (conn.send(connack)) {
		printf("%s => send connack ok\r\n", __FUNCTION__);
		return true;
	}

	printf("%s => send connack error\r\n", __FUNCTION__);
	return false;
}

static bool handle_subscribe(acl::mqtt_client& conn,
		const acl::mqtt_message& message) {
	const acl::mqtt_subscribe& subscribe = (const acl::mqtt_subscribe&) message;

	acl::mqtt_suback suback;
	suback.set_pkt_id(subscribe.get_pkt_id());
	const std::vector<acl::mqtt_qos_t>& qoses = subscribe.get_qoses();
	suback.add_topic_qos(qoses);                                           
	if (conn.send(suback)) {
		printf("%s => send suback ok\r\n", __FUNCTION__);
		return true;
	}

	printf("%s => send suback error\r\n", __FUNCTION__);
	return false;
}

static bool handle_publish(acl::mqtt_client& conn,
		const acl::mqtt_message& message) {
	const acl::mqtt_publish& publish = (const acl::mqtt_publish&) message;
	const acl::string& payload = publish.get_payload();
	printf("topic: %s, qos: %d, pkt_id: %d, payload: %s\r\n",
		publish.get_topic(), (int) publish.get_header().get_qos(),
		publish.get_pkt_id(), payload.c_str());

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
	case acl::MQTT_CONNECT:
		return handle_connect(conn, message);
	case acl::MQTT_PINGREQ:
		return handle_pingreq(conn);
	case acl::MQTT_PINGRESP:
		return handle_pingresp();
	case acl::MQTT_DISCONNECT:
		return handle_disconnect(message);
	case acl::MQTT_SUBSCRIBE:
		return handle_subscribe(conn, message);
	case acl::MQTT_PUBLISH:
		return handle_publish(conn, message);
	default:
		printf("unknown type=%d\r\n", (int) type);
		return false;
	}
}

class mqtt_client : public acl::thread {
public:
	mqtt_client(acl::socket_stream* conn) : conn_(conn) {}

protected:
	~mqtt_client(void) { delete conn_; }

	// @override
	void* run(void) {
		acl::mqtt_client client(*conn_);
		while (true) {
			acl::mqtt_message* res = client.get_message();
			if (res == NULL) {
				printf("read message error\r\n");
				break;
			}

			//printf("read one message\r\n");
			if (!handle_message(client, *res)) {
				delete res;
				break;
			}
			delete res;
		}

		printf("mqtt_client thread exit\r\n");
		delete this;
		return NULL;
	}

private:
	acl::socket_stream* conn_;
};

static void usage(const char* procname) {
	printf("usage: %s -h [help] -s addr\r\n", procname);
}

int main(int argc, char* argv[]) {
	char ch;
	acl::string addr("127.0.0.1|1883");;

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

	acl::server_socket ss;
	if (!ss.open(addr)) {
		printf("listen on %s error %s\r\n",
			addr.c_str(), acl::last_serror());
		return 1;
	}

	while (true) {
		acl::socket_stream* conn = ss.accept();
		if (conn == NULL) {
			printf("accept error %s\r\n", acl::last_serror());
			break;
		}
		mqtt_client* client = new mqtt_client(conn);
		client->start();
	}

	return 0;
}
