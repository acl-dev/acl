#pragma once

#include "mqtt_message.hpp"

namespace acl {

/**
 * mqtt message object for the MQTT_SUBSCRIBE type.
 */
class ACL_CPP_API mqtt_subscribe : public mqtt_message {
public:
	/**
	 * constructor for creating MQTT_SUBSCRIBE mqtt message object.
	 * @see mqtt_message
	 */
	mqtt_subscribe(void);

	/**
	 * constructor for creating MQTT_SUBSCRIBE mqtt message object.
	 * @see mqtt_message
	 */
	mqtt_subscribe(const mqtt_header& header);

	~mqtt_subscribe(void);

	/**
	 * set message's id
	 * @param id {unsigned short} should > 0 && <= 65535
	 * @return {mqtt_subscribe&}
	 */
	mqtt_subscribe& set_pkt_id(unsigned short id);

	/**
	 * add one topic with its qos.
	 * @param topic {const char*} the topic of message.
	 * @param qos {mqtt_qos_t} the qos of the topic.
	 * @return {mqtt_subscribe&}
	 */
	mqtt_subscribe& add_topic(const char* topic, mqtt_qos_t qos);

	/**
	 * get the messsage's id.
	 * @return {unsigned short} should return the value that > 0 && <= 65535.
	 */
	unsigned short get_pkt_id(void) const {
		return pkt_id_;
	}

	/**
	 * get all the topics.
	 * @return {const std::vector<string>&}
	 */
	const std::vector<string>& get_topics(void) const {
		return topics_;
	}

	/**
	 * get all the qoses of all topics.
	 * @return {const std::vector<mqtt_qos_t>&}
	 */
	const std::vector<mqtt_qos_t>& get_qoses(void) const {
		return qoses_;
	}

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
	// used internal to parse mqtt message in streawming mode.

	int update_header_var(const char* data, int dlen);
	int update_topic_len(const char* data, int dlen);
	int update_topic_val(const char* data, int dlen);
	int update_topic_qos(const char* data, int dlen);

private:
	unsigned status_;
	bool finished_;
	char buff_[2];
	unsigned dlen_;

	unsigned short          pkt_id_;
	std::vector<string>     topics_;
	std::vector<mqtt_qos_t> qoses_;

	unsigned body_len_;
	unsigned nread_;

	string topic_;
};

} // namespace acl
