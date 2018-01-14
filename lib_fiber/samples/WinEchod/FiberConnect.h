#pragma once
class CFiberConnect : public acl::fiber
{
public:
	CFiberConnect(const char* serverAddr, int count);

private:
	~CFiberConnect(void);

	// @override
	void run(void);

private:
	acl::string m_serverAddr;
	int         m_count;

	void doEcho(socket_t sock);
	void doEcho(acl::socket_stream& conn);
};

