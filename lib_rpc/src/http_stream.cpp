#include "stdafx.h"
#include "google/protobuf/message_lite.h"
#include "google/protobuf/io/http_stream.h"
#include "acl_cpp/stream/socket_stream.hpp"
#include "acl_cpp/http/http_client.hpp"
#include "acl_cpp/http/http_header.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include "lib_acl.h"

namespace google {
namespace protobuf {
namespace io {

#ifdef	DEBUG
static double stamp_sub(const struct timeval *from, const struct timeval *sub_by)
{
	struct timeval res;

	memcpy(&res, from, sizeof(struct timeval));

	res.tv_usec -= sub_by->tv_usec;
	if (res.tv_usec < 0) {
		--res.tv_sec;
		res.tv_usec += 1000000;
	}
	res.tv_sec -= sub_by->tv_sec;

	return (res.tv_sec * 1000.0 + res.tv_usec/1000.0);
}
#endif

http_request::http_request(acl::http_request* request)
: request_(request)
, request_inner_(NULL)
, addr_(NULL)
, conn_timeout_(0)
, rw_timeout_(0)
{
	assert(request);
}

http_request::http_request(const char* addr, int conn_timeout /* = 60 */,
	int rw_timeout /* = 60 */)
: request_(NULL)
, request_inner_(NULL)
, conn_timeout_(conn_timeout)
, rw_timeout_(rw_timeout)
, request_spent_(0)
, response_spent_(0)
, build_spent_(0)
, parse_spent_(0)
{
	addr_ = acl_mystrdup(addr);
}

http_request::~http_request()
{
	if (request_inner_)
		delete request_inner_;
	if (addr_)
		acl_myfree(addr_);
}

bool http_request::rpc_request(const MessageLite& in, MessageLite* out)
{
	if (request_ == NULL)
	{
		if (addr_ == NULL)
			return false;
		request_inner_ = new acl::http_request(addr_,
			conn_timeout_, rw_timeout_);
		request_ = request_inner_;
	}

	std::string buf;

#ifdef	DEBUG
	struct timeval begin, end;
	gettimeofday(&begin, NULL);
#endif

	in.SerializeToString(&buf);

#ifdef	DEBUG
	gettimeofday(&end, NULL);
	build_spent_ = stamp_sub(&end, &begin);

	gettimeofday(&begin, NULL);
#endif
	acl::http_header& header = request_->request_header();
	header.set_content_length(buf.length());
	header.set_keep_alive(true);

	if (request_->request(buf.c_str(), buf.length()) == false)
		return false;

#ifdef	DEBUG
	gettimeofday(&end, NULL);
	request_spent_ = stamp_sub(&end, &begin);
#endif

	if (out == NULL)
		return true;

	buf.clear();
	acl::string tmp;
	int   ret;

#ifdef	DEBUG
	gettimeofday(&begin, NULL);
#endif
	while (true)
	{
		ret = request_->read_body(tmp, true);
		if (ret < 0)
			return false;
		else if (ret == 0)
			break;
		buf.append(tmp.c_str(), (size_t) ret);
	}

#ifdef	DEBUG
	gettimeofday(&end, NULL);
	response_spent_ = stamp_sub(&end, &begin);

	gettimeofday(&begin, NULL);
#endif

	if (out->ParseFromString(buf) == false)
		return false;

#ifdef	DEBUG
	gettimeofday(&end, NULL);
	parse_spent_ = stamp_sub(&end, &begin);
#endif

	return true;
}

//////////////////////////////////////////////////////////////////////////

http_response::http_response(acl::http_response* response)
: response_(response)
, header_spent_(0)
, body_spent_(0)
, parse_spent_(0)
, build_spent_(0)
, response_spent_(0)
{
	assert(response);
}

http_response::~http_response()
{
}

bool http_response::read_request(MessageLite* out)
{
	if (out == NULL)
		return false;

#ifdef	DEBUG
	struct timeval begin, end;
	gettimeofday(&begin, NULL);
#endif

	if (response_->read_header() == false)
		return false;

#ifdef	DEBUG
	gettimeofday(&end, NULL);
	header_spent_ = stamp_sub(&end, &begin);
#endif

	acl::string buf;

#ifdef	DEBUG
	gettimeofday(&begin, NULL);
#endif

	if (response_->get_body(buf) == false)
		return false;

#ifdef	DEBUG
	gettimeofday(&end, NULL);
	body_spent_ = stamp_sub(&end, &begin);
#endif

	std::string data(buf.c_str(), (int) buf.length());

#ifdef	DEBUG
	gettimeofday(&begin, NULL);
#endif

	bool ret = out->ParseFromString(data);

#ifdef	DEBUG
	gettimeofday(&end, NULL);
	parse_spent_ = stamp_sub(&end, &begin);
#endif

	return ret;
}

bool http_response::send_response(const MessageLite& in)
{
	std::string buf;

#ifdef	DEBUG
	struct timeval begin, end;
	gettimeofday(&begin, NULL);
#endif

	in.SerializeToString(&buf);

#ifdef	DEBUG
	gettimeofday(&end, NULL);
	build_spent_ = stamp_sub(&end, &begin);

	gettimeofday(&begin, NULL);
#endif

	acl::http_header& header = response_->response_header();
	header.set_status(200);
	header.set_content_length(buf.length());
	header.set_keep_alive(true);

	bool ret = response_->response(buf.c_str(), buf.length());

#ifdef	DEBUG
	gettimeofday(&end, NULL);
	response_spent_ = stamp_sub(&end, &begin);
#endif

	return ret;
}

}  // namespace io
}  // namespace protobuf
}  // namespace google
