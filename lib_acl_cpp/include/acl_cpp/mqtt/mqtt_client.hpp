#pragma once

#include "../stdlib/string.hpp"
#include "../connpool/connect_client.hpp"
#include "../stream/socket_stream.hpp"

namespace acl {

class mqtt_message;

class mqtt_client : public connect_client {
public:
	mqtt_client(const char* addr, int conn_timeout = 10, int rw_timeout = 10);
	~mqtt_client(void);

	bool send(mqtt_message& message);

	mqtt_message* get_message(void);

public:
	bool read_header(mqtt_message& header);
	bool read_body(const mqtt_message& header, mqtt_message& body);
	mqtt_message* create_body(const mqtt_message& header);

protected:
	// @override
	bool open(void);

private:
	string addr_;
	int conn_timeout_;
	int rw_timeout_;

	socket_stream conn_;
};

} // namespace acl
