#pragma once

void dgate_service_start(void);

void dgate_push_request(acl::socket_stream* server, const char* peer_addr,
	const char* data, size_t dlen);
