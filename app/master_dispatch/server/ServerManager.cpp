#include "stdafx.h"
#include "ServerConnection.h"
#include "ServerManager.h"

ServerConnection* ServerManager::min()
{
	if (conns_.empty())
		return NULL;

	ServerConnection* conn = NULL;

	std::vector<ServerConnection*>::iterator it = conns_.begin();
	for (; it != conns_.end(); ++it)
	{
		if (conn == NULL || (*it)->get_nconns() < conn->get_nconns())
			conn = *it;
	}

	return conn;
}

void ServerManager::set(ServerConnection* conn)
{
	std::vector<ServerConnection*>::iterator it = conns_.begin();
	for (; it != conns_.end(); ++it)
	{
		if ((*it) == conn)
			return;
	}
	conns_.push_back(conn);
}

void ServerManager::del(ServerConnection* conn)
{
	std::vector<ServerConnection*>::iterator it = conns_.begin();
	for (; it != conns_.end(); ++it)
	{
		if ((*it) == conn)
		{
			conns_.erase(it);
			break;
		}
	}
}
