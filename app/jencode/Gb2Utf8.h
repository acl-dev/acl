#pragma once

class CGb2Utf8 : public acl::thread
{
public:
	CGb2Utf8(HWND hWnd, CString &sPath, CString &dPath);
	CGb2Utf8(const char* fromCharset, const char* toCharset);
	~CGb2Utf8(void);

	void Init(HWND hWnd, CString &sPath, CString &dPath);

	bool TransformPath(const char *pFrom, const char *pTo);
	bool TransformFile(const char *pFrom, const char *pTo);

private:
	// @override
	void *run(void);

private:
	HWND m_hWnd;
	CString m_sPath;
	CString m_dPath;
	CString m_fromCharset;
	CString m_toCharset;
	int  m_nMsgTransing;
	int  m_nMsgTransEnd;

public:
	void OnTransing(int nMsg);
	void OnTransEnd(int nMsg);
};
