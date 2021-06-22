#pragma once

#include "../stdlib/string.hpp"
#include "mqtt_header.hpp"

namespace acl {

/**
 * the pure virtual class for creating one mqtt message object from or to mqtt
 * message data, all subclass of it should implement the pure virtual method.
 * Any subclass of it can be used to parse mqtt data to create mqtt message,
 * or serialize to string data from mqtt message.
 */
class ACL_CPP_API mqtt_message {
public:
	/**
	 * create message object to be used for sending message to peer.
	 * @param type {mqtt_type_t}
	 */
	mqtt_message(mqtt_type_t type);

	/**
	 * create message object after receiving message data from peer,
	 * because the mqtt message includes header and body, so we parse
	 * the protocol data in two steps: frist parsing the mqtt header,
	 * and then parsing the mqtt body.
	 * @param header {const mqtt_header&} created from the mqtt data and
	 * passed to the message object to be created.
	 */
	mqtt_message(const mqtt_header& header);

	/**
	 * virtual destructor.
	 */
	virtual ~mqtt_message(void);

public:
	/**
	 * the subclass should implement this method to build mqtt message data,
	 * this is used as the mqtt message serialize.
	 * @param out {string&} used to store the mqtt data.
	 * @return {bool} return true if serialize sucessfully.
	 */
	virtual bool to_string(string& out) = 0;

	/**
	 * the subclass should implement this method to parse mqtt data,
	 * this is used as the mqtt message deserialize.
	 * @param data {char*} mqtt message data received from socket.
	 * @param dlen {int} the length of data.
	 * @return {int} return the length of left data not consumed:
	 *  > 0: the current mqtt message object has been finished, the left
	 *       data is for the next message object;
	 *  -1: some error happened when parsing the mqtt message data;
	 *   0: all the data passed in has been consumed by the current mqtt
	 *      message object, the object maybe complete or not, you should
	 *      call finished() to check if the object has been completed.
	 */
	virtual int update(const char* data, int dlen) = 0;

	/**
	 * check if the current mqtt object been parsing from mqtt data has
	 * been completed.
	 * @return {bool}
	 */
	virtual bool finished(void) const {
		return false;
	}

	/**
	 * get the current mqtt message header in variable mode in order to
	 * change some information of the header.
	 * @return {mqtt_header&}
	 */
	mqtt_header& get_header(void) {
		return header_;
	}

	/**
	 * get the current mqtt message header in invariable mode, just for
	 * getting some information from it.
	 * @return {const mqtt_header&}
	 */
	const mqtt_header& get_header(void) const {
		return header_;
	}

public:
	/**
	 * create mqtt message object with the specified mqtt header after
	 * parsing the mqtt data.
	 * @param header {const mqtt_header&}
	 * @return {mqtt_message*} return no-NULL if successful, or NULL if
	 *  the mqtt object type of header is invalid.
	 */
	static mqtt_message* create_message(const mqtt_header& header);

protected:
	mqtt_header header_;

	// add one byte to string buffer when building mqtt message.
	void pack_add(unsigned char ch, string& out);

	// add two bytes to string buffer when building mqtt message.
	void pack_add(unsigned short n, string& out);

	// add string data to string buffer when building mqtt message.
	void pack_add(const string& s, string& out);

	// parse one short from mqtt data.
	bool unpack_short(const char* data, size_t len, unsigned short& out);
};

} // namespace acl
