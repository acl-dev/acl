#pragma once
class CFiberClient : public acl::fiber
{
public:
	CFiberClient(SOCKET sock);

private:
	~CFiberClient(void);

protected:
	void run(void);

private:
	SOCKET m_sock;
};

