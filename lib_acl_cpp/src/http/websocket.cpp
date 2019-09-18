#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stream/socket_stream.hpp"
#include "acl_cpp/stream/aio_socket_stream.hpp"
#include "acl_cpp/http/websocket.hpp"
#endif

#if defined(_WIN32) || defined(_WIN64)
#pragma comment(lib, "ws2_32.lib")
#endif

namespace acl
{

enum {
	WS_HEAD_2BYTES,
	WS_HEAD_LEN_2BYTES,
	WS_HEAD_LEN_8BYTES,
	WS_HEAD_MASKING_KEY,
	WS_HEAD_FINISH,
};

websocket::websocket(socket_stream& client)
: client_(client)
, header_buf_(NULL)
, header_size_(0)
, header_len_(0)
, payload_nread_(0)
, payload_nsent_(0)
, header_sent_(false)
, status_(WS_HEAD_2BYTES)
, peek_buf_(NULL)
{
	reset();
}

websocket::~websocket(void)
{
	if (header_buf_) {
		acl_myfree(header_buf_);
	}
	delete peek_buf_;
}

websocket& websocket::reset(void)
{
	header_.fin         = false;
	header_.rsv1        = false;
	header_.rsv2        = false;
	header_.rsv3        = false;
	header_.opcode      = FRAME_TEXT;
//	header_.mask        = false;
	header_.payload_len = 0;
//	header_.masking_key = 0;

	payload_nread_      = 0;
	payload_nsent_      = 0;
	header_sent_        = false;
	status_             = WS_HEAD_2BYTES;

	if (peek_buf_) {
		peek_buf_->clear();
	}

	return *this;
}

websocket& websocket::set_frame_fin(bool yes)
{
	header_.fin = yes;
	return *this;
}

websocket& websocket::set_frame_rsv1(bool yes)
{
	header_.rsv1 = yes;
	return *this;
}

websocket& websocket::set_frame_rsv2(bool yes)
{
	header_.rsv2 = yes;
	return *this;
}

websocket& websocket::set_frame_rsv3(bool yes)
{
	header_.rsv3 = yes;
	return *this;
}

websocket& websocket::set_frame_opcode(unsigned char type)
{
	header_.opcode = type;
	return *this;
}

websocket& websocket::set_frame_payload_len(unsigned long long len)
{
	header_.payload_len = len;
	return *this;
}

websocket& websocket::set_frame_masking_key(unsigned int mask)
{
	header_.masking_key = mask;
	header_.mask = mask != 0 ? true : false;
	return *this;
}

void websocket::make_frame_header(void)
{
	header_len_ = 2;
	if (header_.payload_len > 65535) {
		header_len_ += 8;
	} else if (header_.payload_len >= 126) {
		header_len_ += 2;
	}
	if (header_.mask) {
		header_len_ += 4;
	}

	if (header_len_ > header_size_) {
		header_buf_ = (char*) acl_myrealloc(header_buf_, header_len_);
		header_size_ = header_len_;
	}

	unsigned char* ptr = (unsigned char*) header_buf_;

	if (header_.fin) {
		ptr[0] = 0x80;
	} else {
		ptr[0] = 0x00;
	}

	ptr[0] |= header_.opcode;

	if (header_.payload_len && header_.mask) {
		ptr[1] = 0x80;
	} else {
		ptr[1] = 0x00;
	}

	unsigned long long offset = 1;
	unsigned long long payload_len = header_.payload_len;

	if (payload_len <= 125) {
		ptr[offset++] |= payload_len & 0xff;
	} else if (payload_len <= 65535) {
		ptr[offset++] |= 126;
		ptr[offset++] = (unsigned char) (payload_len >> 8) & 0xff;
		ptr[offset++] = (unsigned char) payload_len & 0xff;
	} else {
		ptr[offset++] |= 127;
		ptr[offset++] = (unsigned char) ((payload_len >> 56) & 0xff);
		ptr[offset++] = (unsigned char) ((payload_len >> 48) & 0xff);
		ptr[offset++] = (unsigned char) ((payload_len >> 40) & 0xff);
		ptr[offset++] = (unsigned char) ((payload_len >> 32) & 0xff);
		ptr[offset++] = (unsigned char) ((payload_len >> 24) & 0xff);
		ptr[offset++] = (unsigned char) ((payload_len >> 16) & 0xff);
		ptr[offset++] = (unsigned char) ((payload_len >> 8) & 0xff);
		ptr[offset++] = (unsigned char) (payload_len & 0xff);
	}

	if (payload_len > 0 && header_.mask) {
		unsigned int masking_key = header_.masking_key;
		ptr[offset++] = (unsigned char) ((masking_key >> 24) & 0xff);
		ptr[offset++] = (unsigned char) ((masking_key >> 16) & 0xff);
		ptr[offset++] = (unsigned char) ((masking_key >> 8) & 0xff);
		ptr[offset++] = (unsigned char) (masking_key & 0xff);

		// save result in masking_key for send_frame_data
		memcpy(&header_.masking_key, ptr + offset - 4, 4);
	}
}

//////////////////////////////////////////////////////////////////////////////

bool websocket::send_frame_data(void* data, size_t len)
{
	if (!header_sent_) {
		header_sent_ = true;
		make_frame_header();
		if (client_.write(header_buf_, header_len_) == -1) {
			logger_error("write header error %s, len: %d",
				last_serror(), (int) header_len_);
			return false;
		}
	}

	if (data == NULL || len == 0) {
		return true;
	}

	// senity check
	if (payload_nsent_ + len > header_.payload_len) {
		logger_error("data len overflow=%llu > %llu, %llu, %lu",
			payload_nsent_ + len, header_.payload_len,
			payload_nsent_, (unsigned long) len);
		return false;
	}

	if (header_.mask) {
		unsigned char* mask = (unsigned char*) &header_.masking_key;
		for (size_t i = 0; i < len; i++) {
			((char*) data)[i] ^= mask[(payload_nsent_ + i) % 4];
		}
	}

	if (client_.write(data, len) == -1) {
		logger_error("write frame data error %s", last_serror());
		return false;
	}

	payload_nsent_ += len;
	return true;
}

bool websocket::send_frame_data(const char* str)
{
	return send_frame_data(str, str ? strlen(str) : 0);
}

bool websocket::send_frame_data(const void* data, size_t len)
{
	if (data == NULL || len == 0) {
		return send_frame_data((void*) data, len);
	}

	void* buf = acl_mymemdup(data, len);
	bool  ret = send_frame_data(buf, len);
	acl_myfree(buf);
	return ret;
}

bool websocket::send_frame_data(char* str)
{
	return send_frame_data(str, str ? strlen(str) : 0);
}

bool websocket::send_frame_pong(const char* str)
{
	return send_frame_pong(str, str ? strlen(str) : 0);
}

bool websocket::send_frame_pong(const void* data, size_t len)
{
	if (data == NULL || len == 0) {
		return send_frame_pong((void*) NULL, 0);
	}

	void* buf = acl_mymemdup(data, len);
	bool  ret = send_frame_pong(buf, len);
	acl_myfree(buf);
	return ret;
}

bool websocket::send_frame_pong(char* str)
{
	return send_frame_pong(str, str ? strlen(str) : 0);
}

bool websocket::send_frame_pong(void* data, size_t len)
{
	reset();
	set_frame_fin(true);
	set_frame_opcode(FRAME_PONG);
	set_frame_payload_len(len);

	return send_frame_data(data, len);
}

bool websocket::send_frame_ping(const char* str)
{
	return send_frame_ping(str, str ? strlen(str) : 0);
}

bool websocket::send_frame_ping(const void* data, size_t len)
{
	if (data == NULL || len == 0) {
		return send_frame_ping((void*) NULL, 0);
	}

	void* buf = acl_mymemdup(data, len);
	bool ret = send_frame_ping(buf, len);
	acl_myfree(buf);
	return ret;
}

bool websocket::send_frame_ping(char* str)
{
	return send_frame_ping(str, str ? strlen(str) : 0);
}

bool websocket::send_frame_ping(void* data, size_t len)
{
	reset();
	set_frame_fin(true);
	set_frame_opcode(FRAME_PING);
	set_frame_payload_len(len);

	return send_frame_data(data, len);
}

static bool is_big_endian(void)
{
	const int n = 1;

	if (*(char*) &n) {
		return false;
	} else {
		return true;
	}
}

//////////////////////////////////////////////////////////////////////////////

bool websocket::send_frame_data(aio_socket_stream& conn, void* data, size_t len)
{
	if (!header_sent_) {
		header_sent_ = true;
		make_frame_header();
		conn.write(header_buf_, (int) header_len_);
	}

	if (data == NULL || len == 0) {
		return true;
	}

	// senity check
	if (payload_nsent_ + len > header_.payload_len) {
		logger_error("data len overflow=%llu > %llu, %llu, %lu",
			payload_nsent_ + len, header_.payload_len,
			payload_nsent_, (unsigned long) len);
		return false;
	}

	if (header_.mask) {
		unsigned char* mask = (unsigned char*) &header_.masking_key;
		for (size_t i = 0; i < len; i++) {
			((char*) data)[i] ^= mask[(payload_nsent_ + i) % 4];
		}
	}

	conn.write(data, (int) len);
	payload_nsent_ += len;
	return true;
}

bool websocket::send_frame_text(aio_socket_stream& conn, char* data, size_t len)
{
	reset();
	set_frame_fin(true);
	set_frame_opcode(FRAME_TEXT);
	set_frame_payload_len(len);
	//set_frame_masking_key(1);
	return send_frame_data(conn, data, len);
}

bool websocket::send_frame_binary(aio_socket_stream& conn, void* data, size_t len)
{
	reset();
	set_frame_fin(true);
	set_frame_opcode(FRAME_BINARY);
	set_frame_payload_len(len);
	return send_frame_data(conn, data, len);
}

bool websocket::send_frame_ping(aio_socket_stream& conn, void* data, size_t len)
{
	reset();
	set_frame_fin(true);
	set_frame_opcode(FRAME_PING);
	set_frame_payload_len(len);
	return send_frame_data(conn, data, len);
}

bool websocket::send_frame_pong(aio_socket_stream& conn, void* data, size_t len)
{
	reset();
	set_frame_fin(true);
	set_frame_opcode(FRAME_PONG);
	set_frame_payload_len(len);
	return send_frame_data(conn, data, len);
}

//////////////////////////////////////////////////////////////////////////////

#ifndef swap64
#define swap64(val) (((val) >> 56) | \
	(((val) & 0x00ff000000000000ll) >> 40) | \
	(((val) & 0x0000ff0000000000ll) >> 24) | \
	(((val) & 0x000000ff00000000ll) >> 8)  | \
	(((val) & 0x00000000ff000000ll) << 8)  | \
	(((val) & 0x0000000000ff0000ll) << 24) | \
	(((val) & 0x000000000000ff00ll) << 40) | \
	(((val) << 56)))
#endif

#define	hton64(val) is_big_endian() ? val : swap64(val)
#define	ntoh64(val) hton64(val)

bool websocket::read_frame_head(void)
{
	reset();

	unsigned char buf[8];

	if (client_.read(buf, 2) == -1) {
		if (last_error() != ACL_ETIMEDOUT) {
			logger_error("read first two char error: %d, %s",
				last_error(), last_serror());
		}
		return false;
	}

	update_head_2bytes(buf[0], buf[1]);

	size_t count;

	// payload_len: <= 125 | 126 | 127

	if (header_.payload_len == 126) {
		count = 2;
	} else if (header_.payload_len > 126) {
		count = 8;
	} else {
		count = 0;
	}

	if (count > 0) {
		int  ret;

		if ((ret = client_.read(buf, count)) == -1) {
			if (last_error() != ACL_ETIMEDOUT) {
				logger_error("read ext_payload_len error:"
					" %d, %s", last_error(), last_serror());
			}
			return false;
		} else if (ret == 2) {
			unsigned short n;
			memcpy(&n, buf, ret);
			header_.payload_len = ntohs(n);
		} else {
			// ret == 8
			memcpy(&header_.payload_len, buf, ret);
			header_.payload_len = ntoh64(header_.payload_len);
		}
	}

	if (!header_.mask) {
		return true;
	}

	if (client_.read(&header_.masking_key, sizeof(unsigned int)) == -1) {
		if (last_error() != ACL_ETIMEDOUT) {
			logger_error("read masking_key error: %d, %s",
				last_error(), last_serror());
		}
		return false;
	}

	return true;
}

int websocket::read_frame_data(void* buf, size_t size)
{
	if (payload_nread_ >= header_.payload_len) {
		return 0;
	}

	if (header_.payload_len < payload_nread_ + size) {
		size = (size_t) (header_.payload_len - payload_nread_);
	}

	int ret = client_.read(buf, size, false);
	if (ret == -1) {
		if (last_error() != ACL_ETIMEDOUT) {
			logger_error("read frame data error: %d, %s",
				last_error(), last_serror());
		}
		return -1;
	}

	if (header_.mask) {
		unsigned char* mask = (unsigned char*) &header_.masking_key;
		for (int i = 0; i < ret; i++) {
			((char*) buf)[i] ^= mask[(payload_nread_ + i) % 4];
		}
	}

	payload_nread_ += ret;
	return ret;
}

void websocket::update_head_2bytes(unsigned char ch1, unsigned ch2)
{
	header_.fin         = (ch1 >> 7) & 0x01;
	header_.rsv1        = (ch1 >> 6) & 0x01;
	header_.rsv2        = (ch1 >> 5) & 0x01;
	header_.rsv3        = (ch1 >> 4) & 0x01;
	header_.opcode      = ch1 & 0x0f;
	header_.mask        = (ch2 >> 7) & 0x01;

	header_.payload_len = ch2 & 0x7f;
}

bool websocket::peek_head_2bytes(void)
{
	size_t len = peek_buf_->size();

	if (len >= 2) {
		logger_fatal("overflow, len=%ld", (long) len);
	}

	if (!client_.readn_peek(peek_buf_, 2 - len, false)) {
		return false;
	}

	assert(peek_buf_->size() == 2);

	unsigned char* s = (unsigned char*) peek_buf_->c_str();
	update_head_2bytes(s[0], s[1]);

	if (header_.payload_len == 126) {
		status_ = WS_HEAD_LEN_2BYTES;
	} else if (header_.payload_len > 126) {
		status_ = WS_HEAD_LEN_8BYTES;
	}
	// header_.payload_len <= 125
	else if (header_.mask) {
		status_ = WS_HEAD_MASKING_KEY;
	} else {
		status_ = WS_HEAD_FINISH;
	}

	peek_buf_->clear();
	return true;
}

bool websocket::peek_head_len_2bytes(void)
{
	size_t len = peek_buf_->size();

	if (len >= 2) {
		logger_fatal("overflow, len=%ld", (long) len);
	}

	if (!client_.readn_peek(peek_buf_, 2 - len, false)) {
		return false;
	}

	assert(peek_buf_->size() == 2);

	unsigned short n;
	memcpy(&n, peek_buf_->c_str(), 2);
	header_.payload_len = ntohs(n);

	status_ = header_.mask ? WS_HEAD_MASKING_KEY : WS_HEAD_FINISH;
	peek_buf_->clear();
	return true;
}

bool websocket::peek_head_len_8bytes(void)
{
	size_t len = peek_buf_->size();

	if (len >= 8) {
		logger_fatal("overflow, len=%ld", (long) len);
	}

	if (!client_.readn_peek(peek_buf_, 8 - len, false)) {
		return false;
	}

	assert(peek_buf_->size() == 8);

	memcpy(&header_.payload_len, peek_buf_->c_str(), 8);
	header_.payload_len = ntoh64(header_.payload_len);

	status_ = header_.mask ? WS_HEAD_MASKING_KEY : WS_HEAD_FINISH;
	peek_buf_->clear();
	return true;
}

bool websocket::peek_head_masking_key(void)
{
	size_t len = peek_buf_->size();

	if (len >= sizeof(unsigned)) {
		logger_fatal("overflow, len=%ld", (long) len);
	}

	if (!client_.readn_peek(peek_buf_, sizeof(unsigned) - len, false)) {
		return false;
	}

	assert(peek_buf_->size() == sizeof(unsigned));

	memcpy(&header_.masking_key, peek_buf_->c_str(), sizeof(unsigned));
	status_ = WS_HEAD_FINISH;
	peek_buf_->clear();
	return true;
}

bool websocket::peek_frame_head(void)
{
	if (peek_buf_ == NULL) {
		peek_buf_ = NEW string(8);
	}

	while (true) {
		switch (status_) {
		case WS_HEAD_2BYTES:
			if (!peek_head_2bytes()) {
				return false;
			}
			break;
		case WS_HEAD_LEN_2BYTES:
			if (!peek_head_len_2bytes()) {
				return false;
			}
			break;
		case WS_HEAD_LEN_8BYTES:
			if (!peek_head_len_8bytes()) {
				return false;
			}
			break;
		case WS_HEAD_MASKING_KEY:
			if (!peek_head_masking_key()) {
				return false;
			}
			break;
		case WS_HEAD_FINISH:
			return true;
		default:
			logger_fatal("invalid status=%d", status_);
			break;
		}
	}
}

bool websocket::is_head_finish(void) const
{
	return status_ == WS_HEAD_FINISH;
}

int websocket::peek_frame_data(char* buf, size_t size)
{
	if (payload_nread_ >= header_.payload_len) {
		reset();
		return 0;
	}

	if (header_.payload_len < payload_nread_ + size) {
		size = (size_t) (header_.payload_len - payload_nread_);
	}

	// 如果未读满所要求的数据且读到的数据为空，则返回-1
	// readn_peek 第三个参数为 true 要求内部自动清空缓冲区
	if (!client_.readn_peek(peek_buf_, size, true) && peek_buf_->empty()) {
		return -1;
	}

	acl_assert(!peek_buf_->empty());

	size_t len = peek_buf_->size();
	memcpy(buf, peek_buf_->c_str(), len);

	if (header_.mask) {
		unsigned char* mask = (unsigned char*) &header_.masking_key;
		for (size_t i = 0; i < len; i++) {
			buf[i] ^= mask[(payload_nread_ + i) % 4];
		}
	}

	payload_nread_ += len;
	return (int) len;
}

int websocket::peek_frame_data(string& buf, size_t size)
{
	if (payload_nread_ >= header_.payload_len) {
		reset();
		return 0;
	}

	if (header_.payload_len < payload_nread_ + size) {
		size = (size_t) (header_.payload_len - payload_nread_);
	}

	size_t nbefore = buf.size();
	if (!client_.readn_peek(buf, size, false)) {
		if (buf.size() == nbefore) {
			return -1;
		}
	}

	size_t nafter = buf.size();

	acl_assert(nafter > nbefore);
	size_t len = nafter - nbefore;

	if (header_.mask) {
		unsigned char* mask = (unsigned char*) &header_.masking_key;
		char* ptr = buf.c_str() + nbefore;

		for (size_t i = 0; i < len; i++) {
			ptr[i] ^= mask[(payload_nread_ + i) % 4];
		}
	}

	payload_nread_ += len;
	return (int) len;
}


bool websocket::eof(void)
{
	return client_.eof();
}

} // namespace acl
