#pragma once

class CClientEcho : public acl::fiber
{
public:
	CClientEcho(acl::socket_stream* conn);

private:
	~CClientEcho(void);

	// @override
	void run(void);

	acl::socket_stream* m_conn;
};

