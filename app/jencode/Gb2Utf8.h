#pragma once

class CGb2Utf8
{
public:
	CGb2Utf8(HWND hWnd, CString &sPath, CString &dPath);
	CGb2Utf8(const char* fromCharset, const char* toCharset);
	void Init(HWND hWnd, CString &sPath, CString &dPath);
	~CGb2Utf8(void);

	void Run(void);
	int TransformPath(CString *pFrom, CString *pTo);
	int TransformFile(const char *pFrom, const char *pTo);
private:
	HWND m_hWnd;
	CString m_sPath;
	CString m_dPath;
	CString m_fromCharset;
	CString m_toCharset;
	int  m_nMsgTransing;
	int  m_nMsgTransEnd;

	static void *RunThread(void *arg);
public:
	void OnTransing(int nMsg);
	void OnTransEnd(int nMsg);
};
