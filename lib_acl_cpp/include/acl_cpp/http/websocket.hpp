/**
 * Copyright (C) 2015-2018
 * All rights reserved.
 *
 * AUTHOR(S)
 *   niukey@qq.com
 *   shuxin.zheng@qq.com
 * 
 * VERSION
 *   Sun 18 Sep 2016 05:15:52 PM CST
 */

#pragma once
#include "../acl_cpp_define.hpp"
#include "../stdlib/noncopyable.hpp"

namespace acl {

class socket_stream;
class aio_socket_stream;

enum {
	FRAME_CONTINUATION = 0x00,
	FRAME_TEXT         = 0x01,
	FRAME_BINARY       = 0x02,
	FRAME_RSV3         = 0x03,
	FRAME_RSV4         = 0x04,
	FRAME_RSV5         = 0x05,
	FRAME_RSV6         = 0x06,
	FRAME_RSV7         = 0x07,
	FRAME_CLOSE        = 0x08,
	FRAME_PING         = 0x09,
	FRAME_PONG         = 0x0A,
	FRAME_CTL_RSVB     = 0x0B,
	FRAME_CTL_RSVC     = 0x0C,
	FRAME_CTL_RSVD     = 0x0D,
	FRAME_CTL_RSVE     = 0x0E,
	FRAME_CTL_RSVF     = 0x0F,
};

struct frame_header {
	bool fin;
	bool rsv1;
	bool rsv2;
	bool rsv3;
	unsigned char opcode:4;
	bool mask;
	unsigned long long payload_len;
	unsigned int masking_key;

	frame_header() {
		fin         = false;
		rsv1        = false;
		rsv2        = false;
		rsv3        = false;
		opcode      = FRAME_TEXT;
		mask        = false;
		payload_len = 0;
		masking_key = 0;
	}
};

class string;

/**
 * websocket base class
 */
class ACL_CPP_API websocket : public noncopyable {
public:
	/**
	 * Constructor
	 * @param client {socket_stream&}
	 */
	explicit websocket(socket_stream& client);
	~websocket();

	/**
	 * When class object is reused, need to reset state through this method
	 */
	websocket& reset();

	/**
	 * Get socket_stream object bound to this class object
	 * @return {socket_stream&}
	 */
	socket_stream& get_stream() const {
		return client_;
	}

	/**
	 * Set finish flag bit
	 * @param yes {bool}
	 * @return {websocket&}
	 */
	websocket& set_frame_fin(bool yes);

	/**
	 * Set reserved flag bit
	 * @param yes {bool}
	 * @return {websocket&}
	 */
	websocket& set_frame_rsv1(bool yes);

	/**
	 * Set reserved flag bit
	 * @param yes {bool}
	 * @return {websocket&}
	 */
	websocket& set_frame_rsv2(bool yes);

	/**
	 * Set reserved flag bit
	 * @param yes {bool}
	 * @return {websocket&}
	 */
	websocket& set_frame_rsv3(bool yes);

	/**
	 * Set data frame type. See definitions above: FRAME_XXX
	 * @param type {unsigned char}
	 * @return {websocket&}
	 */
	websocket& set_frame_opcode(unsigned char type);

	/**
	 * Set total length of this data frame's data payload
	 * @param len {unsigned long long}
	 * @return {websocket&}
	 */
	websocket& set_frame_payload_len(unsigned long long len);

	/**
	 * Set masking key value of data frame data. Must set this item in client mode
	 * @param mask {unsigned int}
	 * @return {websocket&}
	 */
	websocket& set_frame_masking_key(unsigned int mask);

	/**
	 * Send data body in data frame. Can call this method in a loop to send data of
	 * this frame. Total length of sent data
	 * (i.e., sum of data lengths from multiple calls to this method) should be
	 * same as value set by set_frame_payload_len
	 * method
	 * @param data {const void*}
	 * @param len {size_t}
	 * @return {bool} Whether sending was successful
	 */
	bool send_frame_data(const void* data, size_t len);
	bool send_frame_data(void* data, size_t len);
	bool send_frame_data(const char* str);
	bool send_frame_data(char* str);

	/**
	 * Send PONG data frame
	 * @param data {const void*} Data payload of PONG data frame, can be NULL
	 * @param len {size_t} data data length. When data is NULL or len is 0,
	 *  indicates no data payload
	 * @return {bool} Whether sending was successful
	 */
	bool send_frame_pong(const void* data, size_t len);
	bool send_frame_pong(void* data, size_t len);
	bool send_frame_pong(const char* str);
	bool send_frame_pong(char* str);

	/**
	 * Send PING data frame
	 * @param data {const void*} Data payload of PING data frame, can be NULL
	 * @param len {size_t} data data length. When data is NULL or len is 0,
	 *  indicates no data payload
	 * @return {bool} Whether sending was successful
	 */
	bool send_frame_ping(const void* data, size_t len);
	bool send_frame_ping(void* data, size_t len);
	bool send_frame_ping(const char* str);
	bool send_frame_ping(char* str);

	/**
	 * Call non-blocking send interface to asynchronously send data. After sending
	 * data, application layer should call
	 * reset() method to reset state. Before sending a data packet, application
	 * layer needs to call above
	 * set_frame_xxx methods to set frame header information for each data packet
	 * @param conn {aio_socket_stream&}
	 * @param data {void*} Data to be sent, will be modified internally
	 * @param len {size_t} data data length
	 * @return {bool} Whether error occurred
	 */
	bool send_frame_data(aio_socket_stream& conn, void* data, size_t len);

	/**
	 * Asynchronously send a FRAME_TEXT type data frame
	 * @param conn {aio_socket_stream&}
	 * @param data {char*}
	 * @param len {size_t}
	 * @return {bool}
	 */
	bool send_frame_text(aio_socket_stream& conn, char* data, size_t len);

	/**
	 * Asynchronously send a FRAME_BINARY type data frame
	 * @param conn {aio_socket_stream&}
	 * @param data {char*}
	 * @param len {size_t}
	 * @return {bool}
	 */
	bool send_frame_binary(aio_socket_stream& conn, void* data, size_t len);

	/**
	 * Asynchronously send a FRAME_PING type data frame
	 * @param conn {aio_socket_stream&}
	 * @param data {char*}
	 * @param len {size_t}
	 * @return {bool}
	 */
	bool send_frame_ping(aio_socket_stream& conn, void* data, size_t len);

	/**
	 * Asynchronously send a FRAME_PONG type data frame
	 * @param conn {aio_socket_stream&}
	 * @param data {char*}
	 * @param len {size_t}
	 * @return {bool}
	 */
	bool send_frame_pong(aio_socket_stream& conn, void* data, size_t len);

	/**
	 * Read data frame header
	 * @return {bool}
	 */
	bool read_frame_head();

	/**
	 * Read data frame body. Need to call this method in a loop until normally ends
	 * or error occurs
	 * @param buf {void*} Buffer for storing data
	 * @param size {size_t} buf data buffer size
	 * @return {int} Return value > 0 indicates length of data read, need to read
	 * again. == 0 indicates read complete,
	 *  < 0 indicates read error
	 */
	int read_frame_data(void* buf, size_t size);

	/**
	 * Used in non-blocking network communication, try to read websocket header.
	 * Can call this method in a loop
	 * until this method returns true indicating complete websocket header was
	 * read. If returns false,
	 * need to determine whether network connection has been disconnected through
	 * eof() method. If eof() returns true,
	 * should release this object
	 * @return {bool} Returns true indicates complete websocket header was read,
	 * can read body by calling
	 *  read_frame_data()
	 */
	bool peek_frame_head();

	/**
	 * Used in non-blocking network communication, try to read websocket body. Can
	 * call this method in a loop
	 * @param buf {char*} Store read data
	 * @param size {size_t} buf space size
	 * @return {int} Length of data read. When return value is:
	 *   0: Indicates body data of this frame is read complete
	 * -1: Indicates read error, need to determine whether connection has been
	 * closed by calling eof()
	 *  >0: Indicates length of data read this time
	 */
	int peek_frame_data(char* buf, size_t size);
	int peek_frame_data(string& buf, size_t size);

	/**
	 * Determine whether websocket frame header data has been read completely
	 * @return {bool}
	 */
	bool is_head_finish() const;

	/**
	 * Determine whether current network connection has been disconnected
	 * @return {bool}
	 */
	bool eof();

	/**
	 * Get read data frame's frame header
	 * @return {const frame_header&}
	 */
	const frame_header& get_frame_header() const {
		return header_;
	}

	/**
	 * Determine whether this frame is an end frame
	 * @return {bool}
	 */
	bool frame_is_fin() const {
		return header_.fin;
	}

	/**
	 * Determine whether this frame has reserved flag bit set
	 * @return {bool}
	 */
	bool frame_is_rsv1() const {
		return header_.rsv1;
	}

	/**
	 * Determine whether this frame has reserved flag bit set
	 * @return {bool}
	 */
	bool frame_is_rsv2() const {
		return header_.rsv2;
	}

	/**
	 * Determine whether this frame has reserved flag bit set
	 * @return {bool}
	 */
	bool frame_is_rsv3() const {
		return header_.rsv3;
	}

	/**
	 * Get opcode of this data frame. See above: FRAME_XXX
	 * @return {unsigned char}
	 */
	unsigned char get_frame_opcode() const {
		return header_.opcode;
	}

	/**
	 * Get whether this data frame has mask set
	 * @return {bool}
	 */
	bool frame_has_mask() const {
		return header_.mask;
	}

	/**
	 * Get body length of this data frame
	 * @return {unsigned long long}
	 */
	unsigned long long get_frame_payload_len() const {
		return header_.payload_len;
	}

	/**
	 * Get masking key value of this data frame
	 * @return {unsigned int}
	 */
	unsigned int get_frame_masking_key() const {
		return header_.masking_key;
	}

	/**
	 * Get length of data already read for this data frame
	 * @return {unsigned long long}
	 */
	unsigned long long get_frame_payload_nread() const {
		return payload_nread_;
	}

private:
	socket_stream& client_;
	struct frame_header header_;
	char*  header_buf_;
	size_t header_size_;
	size_t header_len_;
	unsigned long long payload_nread_;
	unsigned long long payload_nsent_;
	bool header_sent_;

	unsigned status_;
	string*  peek_buf_;

	void make_frame_header();

	void update_head_2bytes(unsigned char ch1, unsigned ch2);
	bool peek_head_2bytes();
	bool peek_head_len_2bytes();
	bool peek_head_len_8bytes();
	bool peek_head_masking_key();

};

} // namespace acl

