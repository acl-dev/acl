#pragma once

class CFiberServer : public acl::fiber
{
public:
	CFiberServer(SOCKET sock);

private:
	~CFiberServer(void);

protected:
	// @override
	void run(void);

private:
	SOCKET m_sock;
};

