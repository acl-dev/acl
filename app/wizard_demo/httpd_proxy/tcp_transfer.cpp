#include "stdafx.h"
#include "tcp_transfer.h"

tcp_transfer::tcp_transfer(ACL_FIBER* parent, acl::socket_stream& in,
	acl::socket_stream& out, bool running)
: acl::fiber(running)
, parent_(parent)
, me_(NULL)
, in_(in)
, out_(out)
, peer_(NULL)
, is_local_(false)
{
} 

tcp_transfer::~tcp_transfer(void)
{
}

void tcp_transfer::set_peer(tcp_transfer* peer)
{
	peer_ = peer;
}

void tcp_transfer::set_local(bool yes)
{
	is_local_ = yes;
}

void tcp_transfer::unset_peer(void)
{
	peer_ = NULL;
}

void tcp_transfer::close(void)
{
	printf(">>>close sockfd=%d, curr=%p, me=%p, parent=%p\r\n",
		in_.sock_handle(), acl_fiber_running(), me_, parent_);
	int fd = in_.sock_handle();
	//in_.close();
	in_.unbind_sock();
	acl_fiber_close(fd);
	//printf(">>>after close sockfd=%d\r\n", in_.sock_handle());
}

void tcp_transfer::wait(void)
{
	(void) box_.pop();
}

void tcp_transfer::run(void)
{
	me_ = acl_fiber_running();

	char buf[8192];

	while (true) {
		int fd = in_.sock_handle();
		int ret = in_.read(buf, sizeof(buf) - 1, false);
		if (ret == -1) {
			printf("%s: me=%p, parent=%p, peer=%p, read error %s,"
				" fd=%d, %d\r\n", __FUNCTION__, me_, parent_,
				peer_ ? peer_->peer_fiber() : NULL,
				acl::last_serror(), in_.sock_handle(), fd);
			break;
		}

		buf[ret] = 0;

#if 0
		printf("send from %s data, in=%d, out=%d\r\n",
			is_local_ ? "local" : "remote",
			in_->sock_handle(), out_->sock_handle());
#endif

		if (out_.write(buf, ret) == -1) {
			printf(">>>write error\n");
			break;
		}

#if 0
		printf("send to %s data, in=%d, out=%d ok\r\n",
			is_local_ ? "local" : "remote",
			in_->sock_handle(), out_->sock_handle());
#endif
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
