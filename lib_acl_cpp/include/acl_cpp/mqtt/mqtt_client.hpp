#pragma once

#include "../stdlib/string.hpp"
#include "../connpool/connect_client.hpp"
#include "../stream/socket_stream.hpp"

namespace acl {

class mqtt_header;
class mqtt_message;

class ACL_CPP_API mqtt_client : public connect_client {
public:
	mqtt_client(const char* addr, int conn_timeout = 10, int rw_timeout = 10);
	~mqtt_client(void);

	bool send(mqtt_message& message);

	mqtt_message* get_message(void);

public:
	bool read_header(mqtt_header& header);
	bool read_message(const mqtt_header& header, mqtt_message& body);

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
