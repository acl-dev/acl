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
	mqtt_unsubscribe();

	/**
	 * constructor for creating MQTT_PUBACK mqtt message object.
	 * @see mqtt_message
	 */
	mqtt_unsubscribe(const mqtt_header& header);

	~mqtt_unsubscribe();

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

	/**
	 * get all the topics.
	 * @return {const std::vector<std::string>&}
	 */
	const std::vector<std::string>& get_topics() const {
		return topics_;
	}

protected:
	// @override
	bool to_string(string& out);

	// @override
	int update(const char* data, int dlen);

	// @override
	bool finished() const {
		return finished_;
	}

public:
	// used internal to parse unsubscribe message in streaming mode.

	int update_header_var(const char* data, int dlen);
	int update_topic_len(const char* data, int dlen);
	int update_topic_val(const char* data, int dlen);

private:
	unsigned short  pkt_id_;
	char buff_[2];
	unsigned dlen_;
	unsigned status_;

	std::vector<std::string> topics_;

	unsigned body_len_;
	unsigned nread_;

	std::string topic_;
	bool finished_;
};

} // namespace acl

