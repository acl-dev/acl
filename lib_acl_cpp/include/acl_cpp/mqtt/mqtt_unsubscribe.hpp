#pragma once

#include "mqtt_message.hpp"

namespace acl {

/**
 * mqtt message object for MQTT_UNSUBSCRIBE type.
 */
class ACL_CPP_API mqtt_unsubscribe : public mqtt_message {
public:
	/**
	 * constructor for creating MQTT_PUBACK mqtt message object.
	 * @see mqtt_message
	 */
	mqtt_unsubscribe(void);

	/**
	 * constructor for creating MQTT_PUBACK mqtt message object.
	 * @see mqtt_message
	 */
	mqtt_unsubscribe(const mqtt_header& header);

	~mqtt_unsubscribe(void);

	/**
	 * set the message id.
	 * @param id {unsigned short} should > 0 && <= 65535.
	 * @return {mqtt_unsubscribe&}
	 */
	mqtt_unsubscribe& set_pkt_id(unsigned short id);

	/**
	 * set the message's topic.
	 * @param topic {const char*}
	 * @return {mqtt_unsubscribe&}
	 */
	mqtt_unsubscribe& add_topic(const char* topic);

protected:
	// @override
	bool to_string(string& out);

	// @override
	int update(const char* data, int dlen);

	// @override
	bool finished(void) const {
		return finished_;
	}

public:
	// used internal to parse unsubscribe message in streaming mode.

	int update_header_var(const char* data, int dlen);
	int update_topic_len(const char* data, int dlen);
	int update_topic_val(const char* data, int dlen);

private:
	unsigned status_;
	bool finished_;
	char buff_[2];
	unsigned dlen_;

	unsigned short  pkt_id_;
	std::vector<string> topics_;

	unsigned body_len_;
	unsigned nread_;

	string topic_;
};

} // namespace acl

