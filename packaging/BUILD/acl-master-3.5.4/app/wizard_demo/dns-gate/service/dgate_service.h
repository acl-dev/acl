#pragma once

class dgate_db;

void dgate_service_start(void);

void dgate_push_request(dgate_db* db, acl::socket_stream* server,
	const char* peer_addr, const char* data, size_t dlen);
