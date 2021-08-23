#pragma once

class CWinFiberDlg;

class CFiberConnect : public acl::fiber
{
public:
	CFiberConnect(CWinFiberDlg& hWin, const char* serverAddr, int count);
	~CFiberConnect(void);

	// @override
	void run(void);

private:
	CWinFiberDlg& m_hWin;
	acl::string m_serverAddr;
	int         m_count;

	void doEcho(socket_t sock);
	void doEcho(acl::socket_stream& conn);
};

