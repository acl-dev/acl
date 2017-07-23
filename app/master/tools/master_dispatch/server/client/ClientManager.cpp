#include "stdafx.h"
#include "client/ClientConnection.h"
#include "client/ClientManager.h"

void ClientManager::set(ClientConnection* conn)
{
	std::vector<ClientConnection*>::iterator it = conns_.begin();
	for (; it != conns_.end(); ++it)
	{
		if ((*it) == conn)
		{
			logger_warn("duplicate ClientConnection!");
			return;
		}
	}

	conns_.push_back(conn);
}

void ClientManager::del(ClientConnection* conn)
{
	std::vector<ClientConnection*>::iterator it = conns_.begin();
	for (; it != conns_.end(); ++it)
	{
		if ((*it) == conn)
		{
			conns_.erase(it);
			break;
		}
	}
}

ClientConnection* ClientManager::pop()
{
	std::vector<ClientConnection*>::iterator it = conns_.begin();
	if (it == conns_.end())
		return NULL;

	ClientConnection* conn = *it;
	conns_.erase(it);
	return conn;
}
