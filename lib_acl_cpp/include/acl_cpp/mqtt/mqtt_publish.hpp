#pragma once

#include "mqtt_message.hpp"

namespace acl {

/**
 * mqtt message object for MQTT_PUBLISH type.
 */
class ACL_CPP_API mqtt_publish : public mqtt_message {
public:
	/**
	 * constructor for creating MQTT_PUBLISH mqtt message object.
	 * @see mqtt_message
	 */
	mqtt_publish(void);

	/**
	 * constructor for creating MQTT_PUBLISH mqtt message object.
	 * @see mqtt_message
	 */
	mqtt_publish(const mqtt_header& header);

	~mqtt_publish(void);

	/**
	 * set the message topic.
	 * @param topic {const char*}
	 * @return {mqtt_publish&}
	 */
	mqtt_publish& set_topic(const char* topic);

	/**
	 * set the message id.
	 * @param id {unsigned short} the value must > 0 && <= 65535.
	 * @return {mqtt_publish&}
	 */
	mqtt_publish& set_pkt_id(unsigned short id);

	/**
	 * set the message payload.
	 * @param len {unsigned} the length of the payload.
	 * @param data {const char*} the payload data.
	 * @return {mqtt_publish&}
	 */
	mqtt_publish& set_payload(unsigned len, const char* data = NULL);

	/**
	 * get the message's topic.
	 * @return {const char*}
	 */
	const char* get_topic(void) const {
		return topic_.c_str();
	}

	/**
	 * get the message's id.
	 * @return {unsigned short} the message will be invalid if return 0.
	 */
	unsigned short get_pkt_id(void) const {
		return pkt_id_;
	}

	/**
	 * get the length of the payload.
	 * @return {unsigned}
	 */
	unsigned get_payload_len(void) const {
		return payload_len_;
	}

	/**
	 * get the palyload.
	 * @return {const string&}
	 */
	const string& get_payload(void) const {
		return payload_;
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
	// the below methods were used internal to parse mqtt message
	// in streaming mode.

	int update_header_var(const char* data, int dlen);
	int update_topic_len(const char* data, int dlen);
	int update_topic_val(const char* data, int dlen);
	int update_pktid(const char* data, int dlen);
	int update_payload(const char* data, int dlen);

private:
	unsigned status_;
	bool finished_;
	char buff_[2];
	int  dlen_;
	unsigned hlen_var_;

	string topic_;
	unsigned short pkt_id_;
	unsigned payload_len_;
	string payload_;
};

} // namespace acl
