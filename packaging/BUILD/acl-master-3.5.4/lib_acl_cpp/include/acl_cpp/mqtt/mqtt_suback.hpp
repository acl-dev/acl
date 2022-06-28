#pragma once

#include <vector>
#include "mqtt_message.hpp"

namespace acl {

/**
 * mqtt message object for MQTT_SUBACK type.
 */
class ACL_CPP_API mqtt_suback : public mqtt_message {
public:
	/**
	 * constructor for creating MQTT_SUBACK mqtt message object.
	 * @see mqtt_ack
	 */
	mqtt_suback(void);

	/**
	 * constructor for creating MQTT_SUBACK mqtt message object.
	 * @see mqtt_ack
	 */
	mqtt_suback(const mqtt_header& header);

	~mqtt_suback(void);

	/**
	 * set the messsage's id.
	 * @param id {unsigned short} should > 0 && <= 65535.
	 * @return {mqtt_suback&}
	 */
	mqtt_suback& set_pkt_id(unsigned short id);

	/**
	 * add the topic's qos.
	 * @param qos {mqtt_qos_t}
	 * @return {mqtt_suback&}
	 */
	mqtt_suback& add_topic_qos(mqtt_qos_t qos);

	/**
	 * add some qoses of topics.
	 * @param qoses {const std::vector<mqtt_qos_t>&}
	 * @return {mqtt_suback&}
	 */
	mqtt_suback& add_topic_qos(const std::vector<mqtt_qos_t>& qoses);

	/**
	 * get the messsage's id.
	 * @return {unsigned short} some error happened if return 0.
	 */
	unsigned short get_pkt_id(void) const {
		return pkt_id_;
	}

	/**
	 * get all qoses of the topics.
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
	// use internal to parse suback message in streaming mode.

	int update_header_var(const char* data, int dlen);
	int update_topic_qos(const char* data, int dlen);

private:
	unsigned status_;
	bool finished_;
	char buff_[2];
	unsigned dlen_;

	unsigned short pkt_id_;
	std::vector<mqtt_qos_t> qoses_;

	unsigned body_len_;
	unsigned nread_;
};

} // namespace acl
