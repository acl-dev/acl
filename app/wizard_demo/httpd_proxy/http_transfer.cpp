#include "stdafx.h"
#include "http_transfer.h"

http_transfer::http_transfer(request_t& req, response_t& res)
: req_(req)
, res_(res)
, peer_(NULL)
{}

http_transfer::~http_transfer(void) {}

void http_transfer::set_peer(http_transfer& peer) {
	peer_ = &peer;
}

void http_transfer::wait(void) {
	(void) box_.pop();
}

void http_transfer::run(void) {

	if (peer_) {
		peer_->kill();
	}

	box_.push(NULL);
}
