#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <set>

#include <getopt.h>
#include "acl_cpp/lib_acl.hpp"

//////////////////////////////////////////////////////////////////////////////

class mqtt_client;

class mqtt_clients {
public:
	mqtt_clients(const char* topic) : topic_(topic) {}
	~mqtt_clients(void) {}

	const char* get_topic(void) const {
		return topic_.c_str();
	}

	std::set<mqtt_client*>& get_clients(void) {
		return clients_;
	}

	bool add(mqtt_client* client);
	bool del(mqtt_client* client);
	bool empty(void) const {
		return clients_.empty();
	}

	void clear_all(void);

private:
	acl::string topic_;
	std::set<mqtt_client*> clients_;
};

class mqtt_manager {
public:
	mqtt_manager(void) {}
	~mqtt_manager(void);

	bool add(const char* topic, mqtt_client* client);
	bool del(mqtt_client* client);

	std::set<mqtt_client*>* get_clients(const char* topic) {
		std::map<acl::string, mqtt_clients*>::iterator it =
			manager_.find(topic);
		if (it == manager_.end()) {
			return NULL;
		}
		return &it->second->get_clients();
	}

private:
	std::map<acl::string, mqtt_clients*> manager_;

	void del(const char* topic, mqtt_client* client);
};

class mqtt_client : public acl::mqtt_aclient {
public:
	mqtt_client(acl::aio_handle& handle, mqtt_manager& manager);

	const std::set<acl::string>& get_topics(void) const {
		return topics_;
	}

protected:
	~mqtt_client(void);

	// @override
	void destroy(void) { delete this; }

	// @override
	bool on_open(void);

	// @override
	void on_disconnect(void) {
		printf("client disconnect\r\n");
		manager_.del(this);
	}

	// @override
	bool on_header(const acl::mqtt_header& header);

	// @override
	bool on_body(const acl::mqtt_message& body);

private:
	mqtt_manager& manager_;
	std::set<acl::string> topics_;

	bool handle_connect(const acl::mqtt_message& body);
	bool handle_subscribe(const acl::mqtt_message& body);
	bool handle_pingreq(const acl::mqtt_message& body);
	bool handle_pingresp(const acl::mqtt_message& body);
	bool handle_disconnect(const acl::mqtt_message& body);
	bool handle_publish(const acl::mqtt_message& body);
	bool handle_puback(const acl::mqtt_message& body);

	void forward_publish(const acl::mqtt_publish& publish);
	void forward_publish(mqtt_client& client, const acl::mqtt_publish& in);
};

//////////////////////////////////////////////////////////////////////////////

bool mqtt_clients::add(mqtt_client* client) {
	std::set<mqtt_client*>::const_iterator cit
		= clients_.find(client);
	if (cit == clients_.end()) {
		clients_.insert(client);
		printf("topic=%s, client=%p add ok\r\n",
				topic_.c_str(), client);
		return true;
	} else {
		printf("topic=%s, client=%p, add error, exist\r\n",
				topic_.c_str(), client);
		return false;
	}
}

bool mqtt_clients::del(mqtt_client* client) {
	std::set<mqtt_client*>::iterator it = clients_.find(client);
	if (it != clients_.end()) {
		clients_.erase(it);
		printf("topic=%s, client=%p, del ok\r\n",
				topic_.c_str(), client);
		return true;
	}
	printf("topic=%s, client=%p, del error, not exit\r\n",
			topic_.c_str(), client);
	return false;
}

void mqtt_clients::clear_all(void) {
	for (std::set<mqtt_client*>::iterator it = clients_.begin();
			it != clients_.end(); ++it) {
		(*it)->close();
	}

	clients_.clear();
}

//////////////////////////////////////////////////////////////////////////////

mqtt_manager::~mqtt_manager(void) {
	for (std::map<acl::string, mqtt_clients*>::iterator it
		 = manager_.begin(); it != manager_.end(); ++it) {
		it->second->clear_all();
		delete it->second;
	}
}

bool mqtt_manager::add(const char* topic, mqtt_client* client) {
	std::map<acl::string, mqtt_clients*>::iterator it = manager_.find(topic);
	if (it == manager_.end()) {
		mqtt_clients* clients = new mqtt_clients(topic);
		manager_[topic] = clients;
		return clients->add(client);
	} else {
		return it->second->add(client);
	}
}

bool mqtt_manager::del(mqtt_client* client) {
	const std::set<acl::string>& topics = client->get_topics();
	if (topics.empty()) {
		printf("no topics in client=%p\r\n", client);
		return false;
	}

	for (std::set<acl::string>::const_iterator cit
		 = topics.begin(); cit != topics.end(); ++cit) {
		del(*cit, client);
	}
	return true;
}


void mqtt_manager::del(const char* topic, mqtt_client* client) {
	std::map<acl::string, mqtt_clients*>::iterator it = manager_.find(topic);
	if (it != manager_.end()) {
		it->second->del(client);
		if (it->second->empty()) {
			printf("topic=%s, delete mqtt_clients\r\n",
				it->second->get_topic());
			delete it->second;
			manager_.erase(it);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////

mqtt_client::mqtt_client(acl::aio_handle& handle, mqtt_manager& manager)
: acl::mqtt_aclient(handle), manager_(manager) {}

mqtt_client::~mqtt_client(void) {}

bool mqtt_client::on_open(void) {
	printf("one client connected\r\n");
	return true;
}

bool mqtt_client::on_header(const acl::mqtt_header& header) {
	printf("got new mqtt header, type=%s\r\n",
		acl::mqtt_type_desc(header.get_type()));
	return true;
}

bool mqtt_client::on_body(const acl::mqtt_message& body) {
	acl::mqtt_type_t type = body.get_header().get_type();
	switch (type) {
	case acl::MQTT_CONNECT:
		return handle_connect(body);
	case acl::MQTT_SUBSCRIBE:
		return handle_subscribe(body);
	case acl::MQTT_PINGREQ:
		return handle_pingreq(body);
	case acl::MQTT_PINGRESP:
		return handle_pingresp(body);
	case acl::MQTT_DISCONNECT:
		return handle_disconnect(body);
	case acl::MQTT_PUBLISH:
		return handle_publish(body);
	case acl::MQTT_PUBACK:
		return handle_puback(body);
	default:
		printf("unknown type=%d, %s\r\n",
			(int) type, acl::mqtt_type_desc(type));
		return true;
	}
}

bool mqtt_client::handle_connect(const acl::mqtt_message&) {
	acl::mqtt_connack connack;
	connack.set_connack_code(acl::MQTT_CONNACK_OK);
	printf("%s => send connack\r\n", __FUNCTION__);
	return this->send(connack);
}

bool mqtt_client::handle_subscribe(const acl::mqtt_message& body) {
	const acl::mqtt_subscribe& subscribe = (const acl::mqtt_subscribe&) body;
	const std::vector<acl::string>& topics = subscribe.get_topics();
	const std::vector<acl::mqtt_qos_t>& qoses = subscribe.get_qoses();

	if (topics.empty()) {
		printf("not topic got\r\n");
		return false;
	}
	if (qoses.size() != topics.size()) {
		printf("qos count(%zd) != topic count(%zd)\r\n",
			qoses.size(), topics.size());
		return true;
	}

	printf("%s => pkt_id=%d\r\n", __FUNCTION__, subscribe.get_pkt_id());
	size_t n = topics.size();

	for (size_t i = 0; i < n; i++) {
		printf("%s => topic=%s, qos=%s\r\n", __FUNCTION__,
			topics[i].c_str(), acl::mqtt_qos_desc(qoses[i]));
		topics_.insert(topics[i]);

		if (!manager_.add(topics[i], this)) {
			printf("add to manager error, this=%p\r\n", this);
			return false;
		}
	}

	acl::mqtt_suback suback;
	suback.set_pkt_id(subscribe.get_pkt_id());
	suback.add_topic_qos(qoses);

	printf("%s => send suback\r\n", __FUNCTION__);
	return this->send(suback);
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

void mqtt_client::forward_publish(const acl::mqtt_publish& publish) {
	std::set<mqtt_client*>* clients =
		manager_.get_clients(publish.get_topic());
	if (clients == NULL || clients->empty()) {
		return;
	}

	for (std::set<mqtt_client*>::iterator it = clients->begin();
		it != clients->end(); ++it) {
		forward_publish(**it, publish);
	}
}

void mqtt_client::forward_publish(mqtt_client& client,
	const acl::mqtt_publish& in) {

	acl::mqtt_publish publish;
	publish.get_header().set_qos(in.get_header().get_qos());
	publish.set_pkt_id(in.get_pkt_id());
	publish.set_topic(in.get_topic());
	const acl::string& payload = in.get_payload();
	publish.set_payload((unsigned) payload.size(), payload);
	if (client.send(publish)) {
		printf("%s => send publish ok\r\n", __FUNCTION__);
	} else {
		printf("%s => send publish error\r\n", __FUNCTION__);
	}
}

bool mqtt_client::handle_publish(const acl::mqtt_message& body) {
	const acl::mqtt_publish& publish = (const acl::mqtt_publish&) body;
	const acl::string& payload = publish.get_payload();
	const char* topic = publish.get_topic();
	printf("topic: %s, qos: %d, pkt_id: %d, payload: %s\r\n",
		topic, (int) publish.get_header().get_qos(),
		publish.get_pkt_id(), payload.c_str());

	forward_publish(publish);

	if (publish.get_header().get_qos() == acl::MQTT_QOS0) {
		return true;
	}

	acl::mqtt_qos_t qos = body.get_header().get_qos();
	if (qos != acl::MQTT_QOS1) {
		printf("topic=%s, qos=%s, needn't ack\r\n",
			publish.get_topic(), acl::mqtt_qos_desc(qos));
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

bool mqtt_client::handle_puback(const acl::mqtt_message&) {
	return true;
}

//////////////////////////////////////////////////////////////////////////////

class server_callback : public acl::aio_accept_callback {
public:
	server_callback(acl::aio_handle& handle) : handle_(handle) {}
	~server_callback(void) {}

protected:
	// @override
	bool accept_callback(acl::aio_socket_stream* conn) {
		printf("connect from %s, fd=%d\r\n", conn->get_peer(true),
			conn->sock_handle());

		acl::mqtt_aclient* client = new mqtt_client(handle_, manager_);
		if (client->open(conn)) {
			return true;
		}

		printf("open one mqtt client failed\r\n");
		client->destroy();
		conn->close();
		return true;
	}

private:
	acl::aio_handle& handle_;
	mqtt_manager manager_;
};

//////////////////////////////////////////////////////////////////////////////

static void usage(const char* procname) {
	printf("usage: %s -h [help] -s addr\r\n", procname);
}

int main(int argc, char* argv[]) {
	char ch;
	acl::string addr("0.0.0.0|1883");

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

	acl::aio_handle handle(acl::ENGINE_KERNEL);

	acl::aio_listen_stream* listener = new acl::aio_listen_stream(&handle);
	if (!listener->open(addr)) {
		printf("listen %s error %s\r\n", addr.c_str(), acl::last_serror());
		return 1;
	}
	printf("listen %s ok\r\n", addr.c_str());

	server_callback callback(handle);
	listener->add_accept_callback(&callback);

	while (handle.check()) {}

	handle.check();
	listener->destroy();

	return 0;
}
