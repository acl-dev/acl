#include "stdafx.h"
#include "fiber_transfer.h"

fiber_transfer::fiber_transfer(acl::socket_stream& in, acl::socket_stream& out)
: in_(in), out_(out), peer_(NULL)
{
} 

void fiber_transfer::set_peer(fiber_transfer& peer)
{
	peer_ = &peer;
}

fiber_transfer::~fiber_transfer(void)
{
}

void fiber_transfer::wait(void)
{
	(void) box_.pop();
}

void fiber_transfer::run(void)
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
