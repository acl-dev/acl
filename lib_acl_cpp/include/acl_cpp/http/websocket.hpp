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

/**
 * websocket 基础类
 */
class ACL_CPP_API websocket
{
public:
	/**
	 * 构造方法
	 * @param client {socket_stream&}
	 */
	websocket(socket_stream& client);
	~websocket(void);

	/**
	 * 当类对象被重复使用时，需要通过本方法将状态重重
	 */
	websocket& reset(void);

	/**
	 * 获得本类对象所绑定的 socket_stream 对象
	 * @return {socket_stream&}
	 */
	socket_stream& get_stream(void) const
	{
		return client_;
	}

	/**
	 * 设置是否结束的标志位
	 * @param yes {bool}
	 * @return {websocket&}
	 */
	websocket& set_frame_fin(bool yes);

	/**
	 * 设置保留标志位
	 * @param yes {bool}
	 * @return {websocket&}
	 */
	websocket& set_frame_rsv1(bool yes);

	/**
	 * 设置保留标志位
	 * @param yes {bool}
	 * @return {websocket&}
	 */
	websocket& set_frame_rsv2(bool yes);

	/**
	 * 设置保留标志位
	 * @param yes {bool}
	 * @return {websocket&}
	 */
	websocket& set_frame_rsv3(bool yes);

	/**
	 * 设置数据帧类型，参见上面定义：FRAME_XXX
	 * @param type {unsigned char}
	 * @return {websocket&}
	 */
	websocket& set_frame_opcode(unsigned char type);

	/**
	 * 设置本数据帧数据载体的总长度
	 * @param len {unsigned long long}
	 * @return {websocket&}
	 */
	websocket& set_frame_payload_len(unsigned long long len);

	/**
	 * 设置数据帧数据的掩码值
	 * @param mask {unsigned int}
	 * @return {websocket&}
	 */
	websocket& set_frame_masking_key(unsigned int mask);

	/**
	 * 发送数制帧中的数据体，可以循环调用本方法发送本帧的数据，发送数据
	 * 总长度(即多次调用本方法的数据长度之和)应与 set_frame_payload_len
	 * 方法设置的值相同
	 * @param data {const void*}
	 * @param len {size_t}
	 * @return {bool} 发送是否成功
	 */
	bool send_frame_data(const void* data, size_t len);
	bool send_frame_data(void* data, size_t len);
	bool send_frame_data(const char* str);
	bool send_frame_data(char* str);

	/**
	 * 发送 PONG 数据帧
	 * @param data {const void*} PONG 数据帧的数据载体，可以为 NULL
	 * @param len {size_t} data 数据长度，当 data 为 NULL 或 len 为 0 时，
	 *  表示没有数据载荷
	 * @return {bool} 发送是否成功
	 */
	bool send_frame_pong(const void* data, size_t len);
	bool send_frame_pong(void* data, size_t len);
	bool send_frame_pong(const char* str);
	bool send_frame_pong(char* str);

	/**
	 * 发送 PING 数据帧
	 * @param data {const void*} PING 数据帧的数据载体，可以为 NULL
	 * @param len {size_t} data 数据长度，当 data 为 NULL 或 len 为 0 时，
	 *  表示没有数据载荷
	 * @return {bool} 发送是否成功
	 */
	bool send_frame_ping(const void* data, size_t len);
	bool send_frame_ping(void* data, size_t len);
	bool send_frame_ping(const char* str);
	bool send_frame_ping(char* str);

	/**
	 * 读取数据帧帧头
	 * @return {bool}
	 */
	bool read_frame_head(void);

	/**
	 * 读取数据帧数据体，需要循环调用本方法直至正常结束或出错
	 * @param buf {void*} 存放数据的缓冲区
	 * @param size {size_t} buf 数据缓冲区大小
	 * @return {int} 返回值 > 0 表示读到的数据长度需再次读，== 0 表示读结束，
	 *  < 0 表示读出错
	 */
	int read_frame_data(void* buf, size_t size);

	/**
	 * 获得读到的数据帧的帧头
	 * @return {const frame_header&}
	 */
	const frame_header& get_frame_header(void) const
	{
		return header_;
	}

	/**
	 * 判断本帧是否为结束帧
	 * @return {bool}
	 */
	bool frame_is_fin(void) const
	{
		return header_.fin;
	}

	/**
	 * 判断本帧是否设置了保留标志位
	 * @return {bool}
	 */
	bool frame_is_rsv1(void) const
	{
		return header_.rsv1;
	}

	/**
	 * 判断本帧是否设置了保留标志位
	 * @return {bool}
	 */
	bool frame_is_rsv2(void) const
	{
		return header_.rsv2;
	}

	/**
	 * 判断本帧是否设置了保留标志位
	 * @return {bool}
	 */
	bool frame_is_rsv3(void) const
	{
		return header_.rsv3;
	}

	/**
	 * 获得本数据帧的状态码，参见上面：FRAME_XXX
	 * @return {unsigned char}
	 */
	unsigned char get_frame_opcode(void) const
	{
		return header_.opcode;
	}

	/**
	 * 获得本数据帧是否设置了掩码
	 * @return {bool}
	 */
	bool frame_has_mask(void) const
	{
		return header_.mask;
	}

	/**
	 * 获得本数据帧的数据体长度
	 * @return {unsigned long long}
	 */
	unsigned long long get_frame_payload_len(void) const
	{
		return header_.payload_len;
	}

	/**
	 * 获得本数据帧的掩码值
	 * @return {unsigned int}
	 */
	unsigned int get_frame_masking_key(void) const
	{
		return header_.masking_key;
	}

	/**
	 * 获得本数据帧已读到的数据长度
	 * @return {unsigned long long}
	 */
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
	unsigned long long payload_nsent_;
	bool header_sent_;

	void make_frame_header(void);
};

} // namespace acl
