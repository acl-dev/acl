#include "acl_stdafx.hpp"

#if defined(_WIN32) || defined(_WIN64)
# include "zlib-1.2.11/zlib.h"
#else
# include <zlib.h>
#endif

#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/snprintf.hpp"
#include "acl_cpp/stdlib/zlib_stream.hpp"
#include "acl_cpp/stream/ostream.hpp"
#include "acl_cpp/stream/socket_stream.hpp"
#include "acl_cpp/http/http_header.hpp"
#include "acl_cpp/http/http_client.hpp"
#endif

namespace acl
{

http_client::http_client(void)
: stream_(NULL)
, stream_fixed_(false)
, hdr_res_(NULL)
, res_(NULL)
, hdr_req_(NULL)
, req_(NULL)
, unzip_(true)
, zstream_(NULL)
, is_request_(true)
, head_sent_(false)
, body_finish_(false)
, disconnected_(true)
, chunked_transfer_(false)
, gzip_crc32_(0)
, gzip_total_in_(0)
, buf_(NULL)
{
}

http_client::http_client(socket_stream* client, bool is_request /* = false */,
	bool unzip /* = true */, bool stream_fixed /* = true */)
: stream_(client)
, stream_fixed_(stream_fixed)
, hdr_res_(NULL)
, res_(NULL)
, hdr_req_(NULL)
, req_(NULL)
, unzip_(unzip)
, zstream_(NULL)
, is_request_(is_request)
, head_sent_(false)
, body_finish_(false)
, disconnected_(false)
, chunked_transfer_(false)
, gzip_crc32_(0)
, gzip_total_in_(0)
, buf_(NULL)
{
}

http_client::~http_client(void)
{
	reset();
	if (!stream_fixed_) {
		delete stream_;
	}
	delete buf_;
}

void http_client::reset(void)
{
	if (buf_) {
		buf_->clear();
	}

	if (res_) {
		// 说明是长连接的第二次请求，所以需要把上次请求的
		// 响应头对象及响应体对象释放

		acl_assert(hdr_res_);
		http_res_free(res_);
		hdr_res_ = NULL;
		res_     = NULL;
	} else if (hdr_res_) {
		// 说明是长连接的第二次请求，因为有可能第一次请求
		// 只有响应头，所以仅需要释放上次的响应头对象

		http_hdr_res_free(hdr_res_);
		hdr_res_ = NULL;
	}

	if (req_) {
		acl_assert(hdr_req_);
		http_req_free(req_);
		hdr_req_ = NULL;
		req_     = NULL;
	} else if (hdr_req_) {
		http_hdr_req_free(hdr_req_);
		hdr_req_ = NULL;
	}

	delete zstream_;
	zstream_ = NULL;

	last_ret_         = -1;
	head_sent_        = false;
	body_finish_      = false;
	chunked_transfer_ = false;
	gzip_crc32_       = 0;
	gzip_total_in_    = 0;
}

bool http_client::open(const char* addr, int conn_timeout /* = 60 */,
	int rw_timeout /* = 60 */, bool unzip /* = true */)
{
	is_request_ = true;

	if (stream_ && !stream_fixed_) {
		delete stream_;
		stream_       = NULL;
		disconnected_ = true;
	}

	socket_stream* stream = NEW socket_stream();
	unzip_ = unzip;

	if (!stream->open(addr, conn_timeout, rw_timeout)) {
		delete stream;
		disconnected_ = true;
		return false;
	}

	disconnected_ = false;
	stream_       = stream;
	return true;
}

//////////////////////////////////////////////////////////////////////////////

bool http_client::write_chunk(ostream& out, const void* data, size_t len)
{
#ifndef HAS_IOV
# define HAS_IOV
#endif

#ifdef HAS_IOV
	struct iovec iov[3];

	char hdr[32];
	safe_snprintf(hdr, sizeof(hdr), "%x\r\n", (int) len);

#ifdef MINGW
	iov[0].iov_base = hdr;
#else
	iov[0].iov_base = (void*) hdr;
#endif
	iov[0].iov_len  = strlen(hdr);

#ifdef MINGW
	iov[1].iov_base = (char*) data;
#else
	iov[1].iov_base = (void*) data;
#endif
	iov[1].iov_len  = (int) len;

#ifdef MINGW
	iov[2].iov_base = (char*) "\r\n";
#else
	iov[2].iov_base = (void*) "\r\n";
#endif
	iov[2].iov_len  = 2;

	if (out.writev(iov, 3) == -1) {
		disconnected_ = true;
		return false;
	} else {
		return true;
	}
#else
	if (out.format("%x\r\n", (int) len) == -1
		|| out.write(data, len) == -1
		|| out.write("\r\n", 2) == -1) {

		disconnected_ = true;
		return false;
	} else {
		return true;
	}
#endif
}

bool http_client::write_chunk_trailer(ostream& out)
{
	static const char trailer[] = "0\r\n\r\n";

	if (out.write(trailer, sizeof(trailer) - 1) == -1) {
		disconnected_ = true;
		return false;
	} else {
		return true;
	}
}

bool http_client::write_gzip(ostream& out, const void* data, size_t len)
{
	acl_assert(zstream_);

	if (buf_ == NULL) {
		buf_ = NEW string(4096);
	} else {
		buf_->clear();
	}

	// 边压缩边输出数据
	if (data && len > 0) {
		// 增加非压缩数据总长度
		gzip_total_in_ += (unsigned int) len;

		// 计算 crc32 数据校验和
		gzip_crc32_ = zstream_->crc32_update(gzip_crc32_, data, len);

		// 对该段数据进行压缩处理
		if (!zstream_->zip_update((const char*) data, (int) len, buf_)) {
			logger_error("zip_update error!");
			return false;
		}

		// 如果为空，则直接返回，等待下次的写操作
		if (buf_->empty()) {
			return true;
		}

		data = buf_->c_str();
		len  = buf_->size();
	}

	// 写入 zstream 流对象中最后可能缓存的数据，同时结束压缩过程
	else {
		// 检查数据长度有效
		unsigned total_in = (unsigned) zstream_->get_zstream()->total_in;
		if (total_in != gzip_total_in_) {
			logger_warn("total_in: %d != gzip_total_in_: %d",
				total_in, gzip_total_in_);
		}

		if (!zstream_->zip_finish(buf_)) {
			logger_error("zip_finish error!");
			return false;
		}

		if (buf_->empty()) {
			return true;
		}

		data = buf_->c_str();
		len  = buf_->size();
	}

	// 块传输方式输出压缩数据
	if (chunked_transfer_) {
		return write_chunk(out, data, len);
	}

	// 普通流式方式输出压缩数据
	if (out.write(data, len, true, true) < 0 || !out.fflush()) {
		disconnected_ = true;
		return false;
	}

	return true;
}

// 输出 gzip 尾部结束数据

bool http_client::write_gzip_trailer(ostream& out)
{
#ifdef HAVE_BIG_ENDIAN
	struct gztrailer {
		unsigned char crc32_[4];
		unsigned char zlen_[4];
	};
	struct gztrailer trailer;

	trailer.crc32_[0] = (u_char) (gzip_crc32_ & 0xff);
	trailer.crc32_[1] = (u_char) ((gzip_crc32_ >> 8) & 0xff);
	trailer.crc32_[2] = (u_char) ((gzip_crc32_ >> 16) & 0xff);
	trailer.crc32_[3] = (u_char) ((gzip_crc32_ >> 24) & 0xff);

	trailer.zlen_[0]  = (u_char) (gzip_total_in_ & 0xff);
	trailer.zlen_[1]  = (u_char) ((gzip_total_in_ >> 8) & 0xff);
	trailer.zlen_[2]  = (u_char) ((gzip_total_in_ >> 16) & 0xff);
	trailer.zlen_[3]  = (u_char) ((gzip_total_in_ >> 24) & 0xff);
#else
	struct gztrailer {
		unsigned int crc32_;
		unsigned int zlen_;
	};

	struct gztrailer trailer;
	trailer.crc32_ = gzip_crc32_;
	trailer.zlen_  = gzip_total_in_;

#endif // HAVE_BIG_ENDIAN

	// 块传输方式输出 gzip 尾
	if (chunked_transfer_) {
		return write_chunk(out, &trailer, sizeof(trailer))
			&& write_chunk_trailer(out);
	}

	// 普通方式输出 gzip 尾
	if (out.write(&trailer, sizeof(trailer)) < 0) {
		disconnected_ = true;
		return false;
	} else {
		return true;
	}
}

bool http_client::write_head(const http_header& header)
{
	if (head_sent_) {
		return true;
	}
	head_sent_ = true;

	// 先保留是否为块传输的状态
	chunked_transfer_ = header.chunked_transfer();

	// 如果响应数据时设置了 gzip 传输方式，则需要先初始化 zlib 流对象
	if (header.is_transfer_gzip()) {
		delete zstream_;
		zstream_ = NEW zlib_stream;
		if (zstream_->zip_begin(zlib_default, -zlib_wbits_15,
			zlib_mlevel_9)) {
			// 初始化 crc32 校验和
			gzip_crc32_ = zstream_->crc32_update(0, Z_NULL, 0);
		} else {
			logger_error("zip_begin error!");
			delete zstream_;
			zstream_ = NULL;

			// 如果初始化 zip 失败，则强制转换成非 zip 模式
			const_cast<http_header*>
				(&header)->set_transfer_gzip(false);
		}

		// 初始化非压缩数据总长度
		gzip_total_in_ = 0;
	}

	// 创建 HTTP 请求/响应头
	string buf;
	if (header.is_request()) {
		header.build_request(buf);
	} else {
		header.build_response(buf);
	}

	ostream& out = get_ostream();

	// 先写 HTTP 头
	if (out.write(buf.c_str(), buf.length(), true,
		header.get_content_length() > 0) < 0) {

		disconnected_ = true;
		return false;
	} else if (zstream_ == NULL) {
		return true;
	}

	// 如果是采用 gzip 数据压缩方式，则需要先输出 gzip 头

	/**
	 * RFC 1952 Section 2.3 defines the gzip header:
	 * +---+---+---+---+---+---+---+---+---+---+
	 * |ID1|ID2|CM |FLG|     MTIME     |XFL|OS |
	 * +---+---+---+---+---+---+---+---+---+---+
	 * Unix OS_CODE: 3
	 */
	static const unsigned char gzheader[10] =
		{ 0x1f, 0x8b, Z_DEFLATED, 0, 0, 0, 0, 0, 0, 3 };

	if (chunked_transfer_) {
		// 块传输方式写数据
		if (!write_chunk(out, gzheader, sizeof(gzheader))) {
			return false;
		} else {
			return true;
		}
	}

	// 普通流式写数据
	else if (out.write(gzheader, sizeof(gzheader), true, true) < 0) {
		disconnected_ = true;
		return false;
	} else {
		return true;
	}
}

bool http_client::write_body(const void* data, size_t len)
{
	ostream& out = get_ostream();

	// 如果是 gzip 传输，则边压缩边写数据
	if (zstream_ != NULL) {
		// 输出压缩数据体
		if (!write_gzip(out, data, len)) {
			return false;
		}

		// 如果数据输出完毕，则还需输出 gzip 尾部字段
		if (data == NULL || len == 0) {
			return write_gzip_trailer(out);
		} else {
			return true;
		}
	}

	// 非压缩方式传输数据

	// 如果参数为 NULL，则说明数据写完毕
	if (data == NULL || len == 0) {
		if (chunked_transfer_) {
			return write_chunk_trailer(out);
		} else {
			return true;
		}
	}

	// 块方式写入数据体
	else if (chunked_transfer_) {
		return write_chunk(out, data, len);
	}

	// 普通流式写入数据体
	else if (out.write(data, len, true, true) == -1 || !out.fflush()) {
		disconnected_ = true;
		return false;
	} else {
		return true;
	}
}

//////////////////////////////////////////////////////////////////////////////

ostream& http_client::get_ostream(void) const
{
	acl_assert(stream_);
	return *stream_;
}

istream& http_client::get_istream(void) const
{
	acl_assert(stream_);
	return *stream_;
}

socket_stream& http_client::get_stream(void) const
{
	acl_assert(stream_);
	return *stream_;
}

bool http_client::read_head(void)
{
	if (is_request_) {
		return read_response_head();
	} else {
		return read_request_head();
	}
}

bool http_client::read_response_head(void)
{
	// 以防万一，先清除可能的上次请求的残留的中间数据对象
	reset();

	if (stream_ == NULL) {
		logger_error("connect stream not open yet");
		disconnected_ = true;
		return false;
	}
	ACL_VSTREAM* vstream = stream_->get_vstream();
	if (vstream == NULL) {
		logger_error("connect stream null");
		disconnected_ = true;
		return false;
	}

	hdr_res_ = http_hdr_res_new();
	int ret = http_hdr_res_get_sync(hdr_res_, vstream, vstream->rw_timeout);
	if (ret == -1) {
		http_hdr_res_free(hdr_res_);
		hdr_res_      = NULL;
		disconnected_ = true;
		return false;
	}

	if (http_hdr_res_parse(hdr_res_) < 0) {
		logger_error("parse response header error");
		http_hdr_res_free(hdr_res_);
		hdr_res_      = NULL;
		disconnected_ = true;
		return false;
	}

	// 块传输的优先级最高
	if (!hdr_res_->hdr.chunked) {
		// 如果服务器响应时明确指明了长度为 0 则表示不没有数据体
		if (hdr_res_->hdr.content_length == 0) {
			body_finish_ = true;
			return true;
		}
	}

	if (!unzip_) {
		return true;
	}

#define	EQ(x, y) !strcasecmp((x), (y))

	bool gzipped = false;
	const char* ptr = http_hdr_entry_value(&hdr_res_->hdr, "Content-Encoding");
	if (ptr) {
		if (EQ(ptr, "gzip") || EQ(ptr, "x-gzip")) {
			gzipped = true;
		} else {
			logger_warn("unknown compress format: %s", ptr);
		}
	} else if ((ptr = http_hdr_entry_value(&hdr_res_->hdr, "Content-Type"))) {
		if (EQ(ptr, "application/x-gzip")) {
			gzipped = true;
		}
	}

	// 目前仅支持 gzip 数据的解压，如果服务器返回 gzip 数据且初始化 zlib 成功，
	// 则创建 zlib_stream 解压对象并初始化
	if (gzipped && zlib_stream::zlib_load_once()) {
		zstream_ = NEW zlib_stream();
		if (!zstream_->unzip_begin(false)) {
			logger_error("unzip_begin error");
			delete zstream_;
			zstream_ = NULL;
		} else {
			// gzip 响应数据体前会有 10 字节的头部字段
			gzip_header_left_ = 10;
		}
	}

	return true;
}

bool http_client::read_request_head(void)
{
	// 以防万一，先清除可能的上次请求的残留的中间数据对象
	reset();

	if (stream_ == NULL) {
		logger_error("client stream not open yet");
		disconnected_ = true;
		return false;
	}
	ACL_VSTREAM* vstream = stream_->get_vstream();
	if (vstream == NULL) {
		logger_error("client stream null");
		disconnected_ = true;
		return false;
	}

	hdr_req_ = http_hdr_req_new();
	int ret = http_hdr_req_get_sync(hdr_req_, vstream, vstream->rw_timeout);
	if (ret == -1) {
		http_hdr_req_free(hdr_req_);
		hdr_req_      = NULL;
		disconnected_ = true;
		return false;
	}

	if (http_hdr_req_parse(hdr_req_) < 0) {
		logger_error("parse request header error");
		http_hdr_req_free(hdr_req_);
		hdr_req_      = NULL;
		disconnected_ = true;
		return false;
	}

	if (hdr_req_->hdr.content_length <= 0) {
		body_finish_ = true;
	}
	return true;
}

acl_int64 http_client::body_length(void) const
{
	if (is_request_) {
		if (hdr_res_) {
			return hdr_res_->hdr.content_length;
		}
	} else if (hdr_req_) {
		return hdr_req_->hdr.content_length;
	}
	return -1;
}

bool http_client::request_range(acl_int64& range_from, acl_int64& range_to)
{
	if (hdr_req_ == NULL) {
		return false;
	}
	return http_hdr_req_range(hdr_req_, &range_from, &range_to)
			< 0 ? false : true;
}

bool http_client::response_range(acl_int64& range_from,
	acl_int64& range_to, acl_int64& total)
{
	if (hdr_res_ == NULL) {
		return false;
	}
	return http_hdr_res_range(hdr_res_, &range_from, &range_to, &total)
			< 0 ? false : true;
}

bool http_client::keep_alive(void) const
{
	return is_keep_alive();
}

bool http_client::is_keep_alive(void) const
{
	if (is_request_) {
		return is_server_keep_alive();
	} else {
		return is_client_keep_alive();
	}
}

bool http_client::is_server_keep_alive(void) const
{
	// 表示该对象为 HTTP 请求客户端对象，所以需要根据服务端响应
	// 头中的字段判断是否需要保持长连接

	if (hdr_res_ == NULL) {
		return false;
	}

	if (hdr_res_->hdr.keep_alive == 0) {
		return false;
	} else if (hdr_res_->hdr.keep_alive > 0) {
		return true;
	}

	// 如果未置 Connection: keep-alive 字段，当版本为1.1时
	// 则按支持长连接对待

	unsigned major, minor;
	if (!get_version(major, minor)) {
		return false;
	}

	if ((major == 1 && minor >= 1) || major > 1) {
		return true;
	}
	return false;
}

bool http_client::is_client_keep_alive(void) const
{
	if (hdr_req_ == NULL) {
		return false;
	}

	// 表示该对象为 HTTP 响应客户端对象，即本对象处于服务端位置，
	// 需要根据 HTTP 请求客户端对象中的字段是否需要保持长连接

	if (hdr_req_->hdr.keep_alive == 0) {
		return false;
	} else if (hdr_req_->hdr.keep_alive > 0) {
		return true;
	}

	// 如果未置 Connection: keep-alive 字段，当版本为1.1时
	// 则按支持长连接对待

	unsigned major, minor;
	if (!get_version(major, minor)) {
		return false;
	}

	if ((major == 1 && minor >= 1) || major > 1) {
		return true;
	}
	return false;
}

bool http_client::get_version(unsigned& major, unsigned& minor) const
{
	major = 0;
	minor = 0;

	if (hdr_req_) {
		major = hdr_req_->hdr.version.major;
		minor = hdr_req_->hdr.version.minor;
	} else if (hdr_res_) {
		major = hdr_res_->hdr.version.major;
		minor = hdr_res_->hdr.version.minor;
	} else {
		return false;
	}
	return true;
}

HTTP_HDR* http_client::get_http_hdr(void) const
{
	if (is_request_) {
		if (hdr_res_ == NULL) {
			return NULL;
		}
		return &hdr_res_->hdr;
	} else if (hdr_req_ != NULL) {
		return &hdr_req_->hdr;
	} else {
		return NULL;
	}
}

const char* http_client::header_value(const char* name) const
{
	HTTP_HDR* hdr = get_http_hdr();

	return hdr != NULL ? http_hdr_entry_value(hdr, name) : NULL;
}

void http_client::header_disable(const char* name)
{
	HTTP_HDR* hdr = get_http_hdr();

	if (hdr != NULL) {
		http_hdr_entry_off(hdr, name);
	}
}

bool http_client::header_update(const char* name, const char* value,
	bool force_add /* = true */)
{
	HTTP_HDR* hdr = get_http_hdr();

	if (hdr == NULL) {
		return false;
	}

	return http_hdr_entry_replace(hdr, name, value, force_add ? 1 : 0)
			== 0 ? true : false;
}

int http_client::header_update(const char* name, const char* match,
	const char* to, bool case_sensitive /* = false */)
{
	HTTP_HDR* hdr = get_http_hdr();

	if (hdr == NULL) {
		return -1;
	}
	return http_hdr_entry_replace2(hdr, name, match, to,
			case_sensitive ? 0 : 1);
}

int http_client::response_status(void) const
{
	if (is_request_ && hdr_res_) {
		return hdr_res_->reply_status;
	}
	return -1;
}

const char* http_client::request_host(void) const
{
	if (!is_request_ && hdr_req_) {
		return hdr_req_->host;
	}
	return NULL;
}

int http_client::request_port(void) const
{
	if (!is_request_ && hdr_req_) {
		return hdr_req_->port;
	}
	return -1;
}

const char* http_client::request_method(void) const
{
	if (!is_request_ && hdr_req_) {
		return hdr_req_->method;
	}
	return NULL;
}

const char* http_client::request_url(void) const
{
	if (!is_request_ && hdr_req_ && hdr_req_->url_part) {
		return acl_vstring_str(hdr_req_->url_part);
	}
	return NULL;
}

const char* http_client::request_path(void) const
{
	if (!is_request_ && hdr_req_ && hdr_req_->url_path) {
		return acl_vstring_str(hdr_req_->url_path);
	}
	return NULL;
}

const char* http_client::request_params(void) const
{
	if (!is_request_ && hdr_req_ && hdr_req_->url_params) {
		return acl_vstring_str(hdr_req_->url_params);
	}
	return NULL;
}

const char* http_client::request_param(const char* name) const
{
	if (!is_request_ && hdr_req_) {
		return http_hdr_req_param(hdr_req_, name);
	}
	return NULL;
}

const char* http_client::request_cookie(const char* name) const
{
	if (!is_request_ && hdr_req_) {
		return http_hdr_req_cookie_get(hdr_req_, name);
	}
	return NULL;
}

int http_client::read_body(char* buf, size_t size)
{
	if (is_request_) {
		return read_response_body(buf, size);
	} else {
		return read_request_body(buf, size);
	}
}

int http_client::read_response_body(char* buf, size_t size)
{
	if (hdr_res_ == NULL) {
		logger_error("response header not get yet");
		disconnected_ = true;
		return -1;
	}

	if (stream_ == NULL) {
		logger_error("not connected yet");
		disconnected_ = true;
		return -1;
	}
	ACL_VSTREAM* vstream = stream_->get_vstream();
	if (vstream == NULL) {
		logger_error("connect stream null");
		disconnected_ = true;
		return -1;
	}

	if (res_ == NULL) {
		res_ = http_res_new(hdr_res_);
	}

	// 缓冲区太大了没有任何意义
	if (size >= 1024000) {
		size = 1024000;
	}
	http_off_t ret = http_res_body_get_sync(res_, vstream, buf, (int) size);

	if (ret <= 0) {
		// 如果在读响应头时调用了 unzip_begin，则必须保证读完数据
		// 后再调用 unzip_finish，否则会因为 zlib 本身的设计而导致
		// 内存泄露
		if (zstream_ != NULL) {
			string dummy(64);
			zstream_->unzip_finish(&dummy);
		}
		body_finish_ = true;
		if (ret < 0) {
			disconnected_ = true;
		}
	}

	return (int) ret;
}

int http_client::read_request_body(char* buf, size_t size)
{
	if (hdr_req_ == NULL) {
		logger_error("request header not get yet");
		disconnected_ = true;
		return -1;
	}

	if (stream_ == NULL) {
		logger_error("not connected yet");
		disconnected_ = true;
		return -1;
	}
	ACL_VSTREAM* vstream = stream_->get_vstream();
	if (vstream == NULL) {
		logger_error("client stream null");
		disconnected_ = true;
		return -1;
	}

	if (req_ == NULL) {
		req_ = http_req_new(hdr_req_);
	}

	// 缓冲区太大了没有任何意义
	if (size >= 1024000) {
		size = 1024000;
	}

	http_off_t ret = http_req_body_get_sync(req_, vstream, buf, (int) size);

	if (ret < 0) {
		disconnected_ = true;
		body_finish_  = true;
	} else if (ret == 0) {
		body_finish_  = true;
	}

	return ((int) ret);
}

bool http_client::body_finish(void) const
{
	return body_finish_;
}

bool http_client::disconnected(void) const
{
	return disconnected_;
}

int http_client::read_body(string& out, bool clean /* = true */,
	int* real_size /* = NULL */)
{
	if (is_request_) {
		return read_response_body(out, clean, real_size);
	} else {
		return read_request_body(out, clean, real_size);
	}
}

int http_client::read_response_body(string& out, bool clean, int* real_size)
{
	if (real_size) {
		*real_size = 0;
	}

	if (body_finish_) {
		return last_ret_;
	}

	if (stream_ == NULL) {
		logger_error("connect null");
		disconnected_ = true;
		return -1;
	}

	ACL_VSTREAM* vs = stream_->get_vstream();
	if (vs == NULL) {
		logger_error("connect stream null");
		disconnected_ = true;
		return -1;
	}

	if (hdr_res_ == NULL) {
		logger_error("response header not get yet");
		disconnected_ = true;
		return -1;
	}

	if (res_ == NULL) {
		res_ = http_res_new(hdr_res_);
	}

	if (clean) {
		out.clear();
	}

	int   saved_count = (int) out.length();
	char  buf[8192];

READ_AGAIN:  // 对于有 GZIP 头数据，可能需要重复读

	int ret = (int) http_res_body_get_sync(res_, vs, buf, sizeof(buf));

	if (zstream_ == NULL) {
		if (ret > 0) {
			out.append(buf, ret);
			if (real_size) {
				*real_size = ret;
			}
		} else {
			body_finish_ = true; // 表示数据已经读完
			if (ret < 0) {
				disconnected_ = true;
			}
			last_ret_ = ret;
		}
		return ret;
	}

	if (ret <= 0) {
		if (!zstream_->unzip_finish(&out)) {
			logger_error("unzip_finish error");
			return -1;
		}

		last_ret_    = ret; // 记录返回值
		body_finish_ = true; // 表示数据已经读完且解压缩完毕
		if (ret < 0) {
			disconnected_ = true;
		}
		return (int) out.length() - saved_count;
	}

	if (real_size) {
		(*real_size) += ret;
	}

	// 需要先跳过 gzip 头

	if (gzip_header_left_ >= ret) {
		gzip_header_left_ -= ret;
		goto READ_AGAIN;
	}

	int  n;

	if (gzip_header_left_ > 0) {
		n = gzip_header_left_;
		gzip_header_left_ = 0;
	} else {
		n = 0;
	}

	if (!zstream_->unzip_update(buf + n, ret - n, &out)) {
		logger_error("unzip_update error");
		return -1;
	}

	n = (int) out.length() - saved_count;

	// 如果新解压数据为 0，则有可能是本次解压过程需要的压缩数据不完整，
	// 或有可能是此次读到了最后的 8 个字节的尾部字段所至，所以需要再
	// 尝试读一次，以期读到下一部分数据或读完所有的数据体
	if (n == 0) {
		goto READ_AGAIN;
	}

	return n;
}

int http_client::read_request_body(string& out, bool clean, int* real_size)
{
	if (real_size) {
		*real_size = 0;
	}
	if (body_finish_) {
		return last_ret_;
	}

	if (stream_ == NULL) {
		logger_error("client null");
		disconnected_ = true;
		return -1;
	}
	ACL_VSTREAM* vstream = stream_->get_vstream();
	if (vstream == NULL) {
		logger_error("client stream null");
		disconnected_ = true;
		return -1;
	}

	if (hdr_req_ == NULL) {
		logger_error("request header not get yet");
		disconnected_ = true;
		return -1;
	}
	if (req_ == NULL) {
		req_ = http_req_new(hdr_req_);
	}

	if (clean) {
		out.clear();
	}

	char  buf[8192];

	int ret = (int) http_req_body_get_sync(req_, vstream, buf, sizeof(buf));

	if (ret > 0) {
		out.append(buf, ret);
		if (real_size) {
			*real_size = ret;
		}
	} else {
		body_finish_ = true; // 表示数据已经读完
		if (ret < 0) {
			disconnected_ = true;
		}
		last_ret_ = ret;
	}
	return ret;
}

bool http_client::body_gets(string& out, bool nonl /* = true */,
	size_t* size /* = NULL */)
{
	if (buf_ == NULL) {
		buf_ = NEW string(4096);
	}

	size_t len, size_saved = out.length();

	// 首先判断是否已经读完 HTTP 数据体
	if (body_finish_) {
		if (buf_->empty()) {
			if (size) {
				*size = 0;
			}
			return false;
		}

		// 当读缓冲区数据非空时，先尝试从中获取一行数据
		if (buf_->scan_line(out, nonl, &len)) {
			if (size) {
				*size = out.length() - size_saved;
			}
			return true;
		}

		// 如果不能读到完整行数据且 HTTP 数据体读完的情况下则将读
		// 缓冲区内的数据都拷贝至目标缓冲区
		out.append(buf_);
		buf_->clear();

		if (size) {
			*size = out.length() - size_saved;
		}

		return true;
	}

	// 继续读 HTTP 数据体，并尝试从中读取一行数据

	len = 0;

	while (true) {
		if (!buf_->empty()) {
			if (buf_->scan_line(out, nonl, &len)) {
				if (size) {
					*size = out.length() - size_saved;
				}
				return true;
			}

			// 为了减少下次循环时调用 scan_line 的字符串查找次数，
			// 将读缓冲区中的数据先拷贝至目标缓冲区中

			len += buf_->length();
			out.append(buf_);
			buf_->clear();
		}

		if (body_finish_) {
			if (size) {
				*size = len;
			}
			return len > 0 ? true : false;
		}

		if (read_body(*buf_, false) <= 0) {
			body_finish_ = true;
		} else {
			body_finish_ = false;
		}
	}
}

HTTP_HDR_RES* http_client::get_respond_head(string* buf)
{
	if (hdr_res_ == NULL) {
		return (NULL);
	}
	if (buf) {
		ACL_VSTRING* vbf = buf->vstring();
		http_hdr_build(&hdr_res_->hdr, vbf);
	}
	return hdr_res_;
}

HTTP_HDR_REQ* http_client::get_request_head(string* buf)
{
	if (hdr_req_ == NULL) {
		return NULL;
	}
	if (buf) {
		ACL_VSTRING* vbf = buf->vstring();
		http_hdr_build(&hdr_req_->hdr, vbf);
	}
	return hdr_req_;
}

void http_client::print_header(const char* prompt /* = NULL */)
{
	if (hdr_res_) {
		http_hdr_print(&hdr_res_->hdr, prompt);
	} else if (hdr_req_) {
		http_hdr_print(&hdr_req_->hdr, prompt);
	}
}

void http_client::fprint_header(ostream& out, const char* prompt /* = NULL */)
{
	ACL_VSTREAM* fp = out.get_vstream();
	if (fp == NULL) {
		logger_error("fp stream null");
		return;
	}
	if (hdr_res_) {
		http_hdr_fprint(fp, &hdr_res_->hdr, prompt);
	} else if (hdr_req_) {
		http_hdr_fprint(fp, &hdr_req_->hdr, prompt);
	}
}

void http_client::sprint_header(string& out, const char* prompt /* = NULL */)
{
	ACL_VSTRING* bf = out.vstring();
	if (bf == NULL) {
		logger_error("vstring null");
		return;
	}
	if (hdr_res_) {
		http_hdr_sprint(bf, &hdr_res_->hdr, prompt);
	} else if (hdr_req_) {
		http_hdr_sprint(bf, &hdr_req_->hdr, prompt);
	}
}

}  // namespace acl
