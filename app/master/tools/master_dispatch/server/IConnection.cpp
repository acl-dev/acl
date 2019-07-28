#include "stdafx.h"
#include "IConnection.h"

int IConnection::sock_handle() const
{
	return conn_->sock_handle();
}

const char* IConnection::get_peer(bool full /* = true */) const
{
	return conn_->get_peer(full);
}
