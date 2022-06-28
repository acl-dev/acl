#pragma once

class CFiberClient : public acl::fiber
{
public:
	CFiberClient(acl::socket_stream* conn);

private:
	~CFiberClient(void);

	// @override
	void run(void);

	acl::socket_stream* m_conn;
};

