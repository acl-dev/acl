#pragma once
#include "lib_protocol.h"

class CHttpReport {
public:
	CHttpReport(int type);
	CHttpReport(int type, double nTime);
	CHttpReport(int type, const char *data, int len);
	CHttpReport(int type, int len, int dummy);
	CHttpReport(int type, int len, double nTime, int dummy);
	~CHttpReport();

	int  m_type;
#define TYPE_COMPLETE		0
#define TYPE_HDR_REQ		1
#define TYPE_HDR_RES		2
#define TYPE_BODY_RES		3
#define TYPE_TIME_RES		4
#define TYPE_DOWN_LEN		5
#define TYPE_TOTAL_LEN		6
#define TYPE_ERROR_DNS		7
#define TYPE_ERROR_CONNECT	8
#define TYPE_ERROR_WRITE	9
#define TYPE_ERROR_READ		10

	double m_timeRes;
	int m_nContentLength;
	char *m_pBuf;
	int m_nBuf;
	int m_nDownLen;

private:
	void Init(void);
};

class CHttpClient
{
public:
	CHttpClient(void);
	~CHttpClient(void);
	void OnDataCallback(HWND hWnd, int nMsg);
private:
	HWND m_hWnd;
	int  m_nMsg;
	CString m_sReqUrl;
	BOOL m_bLocalSave;
	CString m_sLocalFile;
public:
	int GetUrl(const CString& url);
	void SaveAs(const CString& fileName);
private:
	void ReportComplete(void);
	void ReportReqHdr(const char *data, int len);
	void ReportResHdr(const char *data, int len);
	void ReportResBody(const char *data, int len);
	void ReportResContentLength(int len);
	void ReportResDownLength(double nTime, int len);
	void ReportErrorDns(const char *msg);
	void ReportErrConnect(const char *msg);
	void ReportErrorWrite(const char *msg);
	void ReportErrorRead(const char *msg);
	void ReportTime(double nTime);
	void ReportMsg(CHttpReport *, int nMsg);
	static void *DoRequestThread(void* arg);
	BOOL m_bDisplayReqHdr;
	BOOL m_bDisplayResHdr;
	BOOL m_bDisplayResBody;
	BOOL m_bHttp11;
	BOOL m_bZip;
	BOOL m_bKeepAlive;
public:
	void DisplayReqHdr(BOOL enable);
	void DisplayResHdr(BOOL enable);
	void DisplayResBody(BOOL enable);
	void EnableHttp11(BOOL enable);
	void EnableZip(BOOL enable);
	void EnableKeepAlive(BOOL enable);
	CString m_sHttpHdrAppend;
	CString m_sHttpBody;
	CString m_sAccept;
	CString m_sCtype;
	BOOL m_bUseAddr;
	CString m_sServerAddr;
	BOOL m_bForwardAuto;
	UINT m_nMaxTry;
	BOOL m_bPostMethod;
};
