#pragma once
class CFiberConnect : public acl::fiber
{
public:
	CFiberConnect(const char* serverIP, int serverPort, UINT count);

private:
	~CFiberConnect(void);

	// @override
	void run(void);

private:
	CString  m_serverIP;
	int      m_serverPort;
	UINT     m_count;
	socket_t m_sock;

	void doEchoe(void);
};

