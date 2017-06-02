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

namespace acl
{

class socket_stream;

enum
{
	FRAME_CONTINUATION = 0x00,
	FRAME_TEXT = 0x01,
	FRAME_BINARY = 0x02,
	FRAME_RSV3 = 0x03,
	FRAME_RSV4 = 0x04,
	FRAME_RSV5 = 0x05,
	FRAME_RSV6 = 0x06,
	FRAME_RSV7 = 0x07,
	FRAME_CLOSE = 0x08,
	FRAME_PING = 0x09,
	FRAME_PONG = 0x0A,
	FRAME_CTL_RSVB = 0x0B,
	FRAME_CTL_RSVC = 0x0C,
	FRAME_CTL_RSVD = 0x0D,
	FRAME_CTL_RSVE = 0x0E,
	FRAME_CTL_RSVF = 0x0F,
};

struct frame_header
{
	bool fin;
	bool rsv1;
	bool rsv2;
	bool rsv3;
	unsigned char opcode:4;
	bool mask;
	unsigned long long payload_len;
	unsigned int masking_key;
};

class ACL_CPP_API websocket
{
public:
	websocket(socket_stream& client);
	~websocket(void);

	websocket& reset(void);
	socket_stream& get_stream(void) const
	{
		return client_;
	}

	bool read_frame_head(void);
	int read_frame_data(char* buf, size_t size);
	const frame_header& get_frame_header(void) const
	{
		return header_;
	}

	websocket& set_frame_fin(bool yes);
	websocket& set_frame_rsv1(bool yes);
	websocket& set_frame_rsv2(bool yes);
	websocket& set_frame_rsv3(bool yes);
	websocket& set_frame_opcode(unsigned char type);
	websocket& set_frame_payload_len(unsigned long long len);
	websocket& set_frame_masking_key(unsigned int mask);

	bool send_frame_data(const void* data, size_t len);
	bool send_frame_pong(const void* data, size_t len);
	bool send_frame_ping(const void* data, size_t len);

	bool send_frame_data(void* data, size_t len);
	bool send_frame_pong(void* data, size_t len);
	bool send_frame_ping(void* data, size_t len);

	bool send_frame_data(const char* str);
	bool send_frame_pong(const char* str);
	bool send_frame_ping(const char* str);

	bool send_frame_data(char* str);
	bool send_frame_pong(char* str);
	bool send_frame_ping(char* str);

	bool get_frame_fin(void) const
	{
		return header_.fin;
	}

	bool get_frame_rsv1(void) const
	{
		return header_.rsv1;
	}

	bool get_frame_rsv2(void) const
	{
		return header_.rsv2;
	}

	bool get_frame_rsv3(void) const
	{
		return header_.rsv3;
	}

	unsigned char get_frame_opcode(void) const
	{
		return header_.opcode;
	}

	bool get_frame_mask(void) const
	{
		return header_.mask;
	}

	unsigned long long get_frame_payload_len(void) const
	{
		return header_.payload_len;
	}

	unsigned int get_frame_masking_key(void) const
	{
		return header_.masking_key;
	}

	unsigned long long get_frame_payload_nread(void) const
	{
		return payload_nread_;
	}

private:
	socket_stream& client_;
	struct frame_header header_;
	char*  header_buf_;
	size_t header_size_;
	size_t header_len_;
	unsigned long long payload_nread_;
	bool header_sent_;

	void make_frame_header(void);
};

} // namespace acl
