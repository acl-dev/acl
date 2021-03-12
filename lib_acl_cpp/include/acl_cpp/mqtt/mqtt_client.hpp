#pragma once

#include "../stdlib/string.hpp"
#include "../connpool/connect_client.hpp"
#include "../stream/socket_stream.hpp"

namespace acl {

class mqtt_header;
class mqtt_message;

/**
 * mqtt communication class in sync mode, used by mqtt client or mqtt server.
 */
class ACL_CPP_API mqtt_client : public connect_client {
public:
	/**
	 * used to construct one mqtt client for connecting one mqtt server.
	 * @param addr {const char*} the mqtt server's addr with the format
	 *  like ip|port or domain|port.
	 * @param conn_timeout {int} the timeout for connecting mqtt server.
	 * @param rw_timeout {int} the timeout for reading from mqtt connection.
	 */
	mqtt_client(const char* addr, int conn_timeout = 10, int rw_timeout = 10);

	/**
	 * used to construct one mqtt client for connecting mqtt server, or
	 * receiving from mqtt client.
	 * @param conn {acl::socket_stream&}
	 */
	mqtt_client(acl::socket_stream& conn);

	~mqtt_client(void);

	/**
	 * send ont mqtt message to the peer mqtt.
	 * @param message {mqtt_message&} the mqtt message to be sent, where
	 *  some viriable method of the mesage will be called, so we can't add
	 *  the const limit on it.
	 * @return {bool} return true if sending successfully.
	 */
	bool send(mqtt_message& message);

	/**
	 * read mqtt data from mqtt connection and create the correspondingly
	 * mqtt message object with the mqtt type from mqtt header.
	 * @return {mqtt_message*} return NULL if reading error or data invalid.
	 */
	mqtt_message* get_message(void);

public:
	/**
	 * read mqtt header information from mqtt connection.
	 * @param header {const mqtt_header&} will store the mqtt headeer info.
	 * @return {bool} return true if reading successfully.
	 */
	bool read_header(mqtt_header& header);

	/**
	 * read mqtt body information from mqtt connection.
	 * @param header {const mqtt_header&} used to parse mqtt body.
	 * @param body {mqtt_message&} will store mqtt body info.
	 * @return {bool} return true if reading successfully.
	 */
	bool read_message(const mqtt_header& header, mqtt_message& body);

protected:
	// @override
	bool open(void);

private:
	string addr_;
	int conn_timeout_;
	int rw_timeout_;

	socket_stream* conn_;
	socket_stream* conn_internal_;
};

} // namespace acl
