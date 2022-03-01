#include "stdafx.h"
#include "tcp_transfer.h"

tcp_transfer::tcp_transfer(acl::socket_stream& in,
	acl::socket_stream& out, bool running)
: acl::fiber(running)
, in_(in), out_(out), peer_(NULL)
{
} 

tcp_transfer::~tcp_transfer(void)
{
}

void tcp_transfer::set_peer(tcp_transfer& peer)
{
	peer_ = &peer;
}

void tcp_transfer::unset_peer(void)
{
	peer_ = NULL;
}

void tcp_transfer::close(void)
{
	//printf(">>>close sockfd=%d\r\n", in_.sock_handle());
	int fd = in_.sock_handle();
	//in_.close();
	::close(fd);
	//printf(">>>after close sockfd=%d\r\n", in_.sock_handle());
}

void tcp_transfer::wait(void)
{
	(void) box_.pop();
}

void tcp_transfer::run(void)
{
	char buf[8192];
	while (true) {
		int fd = in_.sock_handle();
		int ret = in_.read(buf, sizeof(buf) - 1, false);
		if (ret == -1) {
			printf("%s: read error %s, fd=%d, %d\r\n", __FUNCTION__,
				acl::last_serror(), in_.sock_handle(), fd);
			break;
		}

		if (out_.write(buf, ret) == -1) {
			break;
		}
	}

	if (peer_) {
		peer_->unset_peer();
		//peer_->kill();
		peer_->close();
	}

	//printf("fd=%d, push\n", in_.sock_handle());
	box_.push(NULL);
	//printf("fd=%d, push done\n", in_.sock_handle());
}
