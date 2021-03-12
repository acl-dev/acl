#pragma once

#include "mqtt_message.hpp"

namespace acl {

/**
 * the base class for acking some mqtt message, used by some mqtt message type.
 */
class ACL_CPP_API mqtt_ack : public mqtt_message {
public:
	/**
	 * usually used when build a ack mqtt message.
	 * @param type {mqtt_type_t}
	 */
	mqtt_ack(mqtt_type_t type);

	/**
	 * usually used when parsing a ack mqtt messsage.
	 * @param header {const mqtt_header&} will be copied internal.
	 */
	mqtt_ack(const mqtt_header& header);

	virtual ~mqtt_ack(void);

	/**
	 * set the mqtt message's id.
	 * @param id {unsigned short} should  > 0 && <= 65535.
	 */
	void set_pkt_id(unsigned short id);

	/**
	 * get the mqtt message's id
	 * @return {unsigned short} if some error happened, 0 will be returned.
	 */
	unsigned short get_pkt_id(void) const {
		return pkt_id_;
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
	// for parsing message header in streaming mode.
	int update_header_var(const char* data, int dlen);

private:
	unsigned status_;
	bool finished_;
	char hbuf_[2];
	unsigned hlen_;

	unsigned short pkt_id_;
};

} // namespace acl
