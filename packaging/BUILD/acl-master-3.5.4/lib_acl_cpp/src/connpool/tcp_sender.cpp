#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stream/socket_stream.hpp"
#include "acl_cpp/connpool/tcp_sender.hpp"
#endif

namespace acl
{

tcp_sender::tcp_sender(socket_stream& conn)
: conn_(&conn)
{
	v2_ = (struct iovec*) acl_mymalloc(sizeof(struct iovec) * 2);
}

tcp_sender::~tcp_sender(void)
{
	acl_myfree(v2_);
}

bool tcp_sender::send(const void* data, unsigned int len)
{
	unsigned int n = htonl(len);

	v2_[0].iov_base = &n;
	v2_[0].iov_len  = sizeof(n);
	v2_[1].iov_base = (void*) data;
	v2_[1].iov_len  = len;

	return conn_->writev(v2_, 2) > 0;
}

} // namespace acl
