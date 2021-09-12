#pragma once

class CHttpGetDlg;

class CHttpClient
{
public:
	CHttpClient(CHttpGetDlg& hWin, const CString& url);
	~CHttpClient();

	void run();

private:
	CHttpGetDlg& m_hWin;
	CString m_url;
};

