#pragma once

class CWinEchodDlg;

class CFiberConnect : public acl::fiber
{
public:
	CFiberConnect(CWinEchodDlg& hWin, const char* serverAddr, int count);

private:
	~CFiberConnect(void);

	// @override
	void run(void);

private:
	CWinEchodDlg& m_hWin;
	acl::string m_serverAddr;
	int         m_count;

	void doEcho(socket_t sock);
	void doEcho(acl::socket_stream& conn);
};

