#include "stdafx.h"
#include "tcp_transfer.h"

tcp_transfer::tcp_transfer(acl::socket_stream& in, acl::socket_stream& out)
: in_(in), out_(out), peer_(NULL)
{
} 

tcp_transfer::~tcp_transfer(void)
{
}

void tcp_transfer::set_peer(tcp_transfer& peer)
{
	peer_ = &peer;
}

void tcp_transfer::wait(void)
{
	(void) box_.pop();
}

void tcp_transfer::run(void)
{
	char buf[8192];
	while (true) {
		int ret = in_.read(buf, sizeof(buf) - 1, false);
		if (ret == -1) {
			break;
		}

		if (out_.write(buf, ret) == -1) {
			break;
		}
	}

	if (peer_) {
		peer_->kill();
	}

	box_.push(NULL);
}
