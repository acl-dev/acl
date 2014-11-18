#pragma once
#include <vector>

class ServerConnection;

class ServerManager : public acl::singleton <ServerManager>
{
public:
	ServerManager() {}
	~ServerManager() {}

	void set(ServerConnection* conn);
	void del(ServerConnection* conn);
	ServerConnection* min();

	size_t length() const
	{
		return conns_.size();
	}

private:
	std::vector<ServerConnection*> conns_;
};
