#pragma once
#include "IConnection.h"

class ClientConnection : public IConnection
{
public:
	ClientConnection(acl::aio_socket_stream* conn, int ttl);
	~ClientConnection();

	bool expired() const;

protected:
	// »ùÀà´¿Ðéº¯Êý
	void run();

private:
	long long int expire_;
};
