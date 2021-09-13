#pragma once

class CHttpGetDlg;

enum {
	HTTP_MSG_ERR,
	HTTP_MSG_REQ,
	HTTP_MSG_RES,
	HTTP_MSG_TOTAL_LENGTH,
	HTTP_MSG_LENGTH,
};

class CHttpMsg {
public:
	CHttpMsg(const char* s, int type) : m_buf(s), m_length(0), m_type(type) {}
	CHttpMsg(long long len, int type) : m_length(len), m_type(type) {}
	~CHttpMsg(void) {}

	CString m_buf;
	long long m_length;
	int m_type;
};

class CHttpClient
{
public:
	CHttpClient(CHttpGetDlg& hWin, const CString& url);
	CHttpClient(acl::fiber_tbox<CHttpMsg>& mBox, const CString& url);
	~CHttpClient();

	void run();

private:
	CHttpGetDlg* m_hWin;
	acl::fiber_tbox<CHttpMsg>* m_box;
	CString m_url;

private:
	void SetError(const char* fmt, ...);
	void SetRequestHead(const char* data);
	void SetResponseHead(const char* data);
	void SetBodyTotalLength(long long length);
	void SetBodyLength(long long length);
	void SetEnd();
};

