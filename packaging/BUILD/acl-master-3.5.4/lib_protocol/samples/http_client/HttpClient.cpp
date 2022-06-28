#include "StdAfx.h"
#include "lib_acl.h"
#include "lib_protocol.h"
#include ".\httpclient.h"

void CHttpReport::Init(void)
{
	m_nBuf = 0;
	m_pBuf = NULL;
}

CHttpReport::CHttpReport(int type)
{
	Init();
	m_type = type;
}

CHttpReport::CHttpReport(int type, double nTime)
{
	Init();
	m_type = type;
	m_timeRes = nTime;
}

CHttpReport::CHttpReport(int type, const char *data, int len)
{
	Init();
	m_pBuf = (char*) acl_mymalloc((size_t) len + 1);
	memcpy(m_pBuf, data, (size_t) len);
	m_pBuf[len] = 0;
	m_nBuf = len;
	m_type = type;
}

CHttpReport::CHttpReport(int type, int len, int dummy)
{
	Init();
	m_type = type;
	m_nContentLength = len;
}

CHttpReport::CHttpReport(int type, int len, double nTime, int dummy)
{
	Init();
	m_type = type;
	m_nDownLen = len;
	m_timeRes = nTime;
}

CHttpReport::~CHttpReport()
{
	if (m_pBuf)
		acl_myfree(m_pBuf);
}

CHttpClient::CHttpClient(void)
: m_sReqUrl(_T(""))
, m_bLocalSave(FALSE)
, m_sLocalFile(_T(""))
, m_nMsg(0)
, m_bDisplayReqHdr(TRUE)
, m_bDisplayResHdr(TRUE)
, m_bDisplayResBody(FALSE)
, m_bHttp11(TRUE)
, m_bZip(FALSE)
, m_bKeepAlive(FALSE)
, m_sHttpHdrAppend(_T(""))
, m_bUseAddr(FALSE)
, m_sServerAddr(_T(""))
, m_bForwardAuto(FALSE)
, m_nMaxTry(10)
{
}

CHttpClient::~CHttpClient(void)
{
}

void CHttpClient::OnDataCallback(HWND hWnd, int nMsg)
{
	m_hWnd = hWnd;
	m_nMsg = nMsg;
}

void CHttpClient::ReportMsg(CHttpReport *pReport, int nMsg)
{
	::PostMessage(m_hWnd, WM_USER_DISPLAY, (WPARAM) pReport, 0);
}

void CHttpClient::ReportComplete(void)
{
	if (m_nMsg <= 0)
		return;

	CHttpReport *pReport = new CHttpReport(TYPE_COMPLETE);
	ReportMsg(pReport, m_nMsg);
}

void CHttpClient::ReportReqHdr(const char *data, int len)
{
	if (m_nMsg <= 0)
		return;
	if (!m_bDisplayReqHdr)
		return;
	CHttpReport *pReport = new CHttpReport(TYPE_HDR_REQ, data, len);
	ReportMsg(pReport, m_nMsg);
}

void CHttpClient::ReportResHdr(const char *data, int len)
{
	if (m_nMsg <= 0)
		return;
	if (!m_bDisplayResHdr)
		return;
	CHttpReport *pReport = new CHttpReport(TYPE_HDR_RES, data, len);
	ReportMsg(pReport, m_nMsg);
}


void CHttpClient::ReportResBody(const char *data, int len)
{
	if (m_nMsg <= 0)
		return;
	if (!m_bDisplayResBody)
		return;
	CHttpReport *pReport = new CHttpReport(TYPE_BODY_RES, data, len);
	ReportMsg(pReport, m_nMsg);
}

void CHttpClient::ReportResContentLength(int len)
{
	if (m_nMsg <= 0)
		return;
	CHttpReport *pReport = new CHttpReport(TYPE_TOTAL_LEN, len, 0);
	ReportMsg(pReport, m_nMsg);
}

void CHttpClient::ReportResDownLength(double nTime, int len)
{
	if (m_nMsg <= 0)
		return;
	CHttpReport *pReport = new CHttpReport(TYPE_DOWN_LEN, len, nTime, 0);
	ReportMsg(pReport, m_nMsg);
}

void CHttpClient::ReportErrorDns(const char *msg)
{
	if (m_nMsg <= 0)
		return;
	CHttpReport *pReport = new CHttpReport(TYPE_ERROR_DNS, msg, (int) strlen(msg));
	ReportMsg(pReport, m_nMsg);
}

void CHttpClient::ReportErrConnect(const char *msg)
{
	if (m_nMsg <= 0)
		return;
	CHttpReport *pReport = new CHttpReport(TYPE_ERROR_CONNECT, msg, (int) strlen(msg));
	ReportMsg(pReport, m_nMsg);
}

void CHttpClient::ReportErrorWrite(const char *msg)
{
	if (m_nMsg <= 0)
		return;
	CHttpReport *pReport = new CHttpReport(TYPE_ERROR_WRITE, msg, (int) strlen(msg));
	ReportMsg(pReport, m_nMsg);
}

void CHttpClient::ReportErrorRead(const char *msg)
{
	if (m_nMsg <= 0)
		return;
	CHttpReport *pReport = new CHttpReport(TYPE_ERROR_READ, msg, (int) strlen(msg));
	ReportMsg(pReport, m_nMsg);
}

void CHttpClient::ReportTime(double nTime)
{
	if (m_nMsg <= 0)
		return;
	CHttpReport *pReport = new CHttpReport(TYPE_TIME_RES, nTime);
	ReportMsg(pReport, m_nMsg);
}

int CHttpClient::GetUrl(const CString& url)
{
	m_sReqUrl = url;
	acl_pthread_t tid;

	acl_pthread_create(&tid, NULL, DoRequestThread, this);
	return 0;
}

void CHttpClient::SaveAs(const CString& fileName)
{
	m_sLocalFile = fileName;
	m_bLocalSave = TRUE;
}

static double stamp_sub(const struct timeval *from, const struct timeval *sub_by)
{
	struct timeval res;

	memcpy(&res, from, sizeof(struct timeval));

	res.tv_usec -= sub_by->tv_usec;
	if (res.tv_usec < 0) {
		--res.tv_sec;
		res.tv_usec += 1000000;
	}
	res.tv_sec -= sub_by->tv_sec;

	return (res.tv_sec * 1000.0 + res.tv_usec/1000.0);
}

void *CHttpClient::DoRequestThread(void* arg)
{
	CHttpClient *phClient = (CHttpClient*) arg;
	HTTP_HDR_REQ *hdr_req = NULL;
	HTTP_HDR_RES *hdr_res = NULL;
	HTTP_RES *http_res = NULL;
	ACL_VSTRING *buf = acl_vstring_alloc(256);
	ACL_VSTREAM *server = NULL;
	const char *pHost = NULL;
	ACL_VSTREAM *fp = NULL;
	int   ret;
	UINT  nForward = 0;
	time_t begin = 0;
	struct timeval begin_tv, end_tv;
	double time_cost = 0;

#undef RETURN
#define RETURN(_x_) do  \
{  \
	if (hdr_req)  \
		http_hdr_req_free(hdr_req);  \
	if (buf)  \
		acl_vstring_free(buf);  \
	if (hdr_res)  \
		http_hdr_res_free(hdr_res);  \
	if (http_res) {  \
		http_res->hdr_res = NULL;  \
		http_res_free(http_res);  \
	}  \
	if (server)  \
		acl_vstream_close(server);  \
	if (fp)  \
		acl_vstream_close(fp);  \
	gettimeofday(&end_tv, NULL); \
	time_cost = stamp_sub(&end_tv, &begin_tv); \
	if (time_cost >= 0) \
		phClient->ReportTime(time_cost); \
	phClient->ReportComplete(); \
	return (_x_);  \
} while(0)

	const char *pUrl = phClient->m_sReqUrl.GetString();
	hdr_req = http_hdr_req_create(pUrl, phClient->m_bPostMethod ? "POST" : "GET",
		phClient->m_bHttp11 ? "HTTP/1.1" : "HTTP/1.0");
	ASSERT(hdr_req);

	if (phClient->m_bZip)
		http_hdr_put_str(&hdr_req->hdr, "Accept-Encoding", "gzip, deflate");
	if (phClient->m_bKeepAlive)
		http_hdr_entry_replace(&hdr_req->hdr, "Connection", "Keep-Alive", 1);
	if (phClient->m_bPostMethod)
		http_hdr_put_int(&hdr_req->hdr, "Content-Length",
			(int) phClient->m_sHttpBody.GetLength());
	if (!phClient->m_sAccept.IsEmpty())
		http_hdr_put_str(&hdr_req->hdr, "Accept", phClient->m_sAccept.GetString());
	if (!phClient->m_sCtype.IsEmpty())
		http_hdr_put_str(&hdr_req->hdr, "Content-Type", phClient->m_sCtype.GetString());

	if (phClient->m_sHttpHdrAppend.GetLength() > 0) {
		ACL_ARGV *argv;
		HTTP_HDR_ENTRY *entry;
		int   i;
		
		argv = acl_argv_split(phClient->m_sHttpHdrAppend.GetString(), "\r\n");
		for (i = 0; i < argv->argc; i++) {
			entry = http_hdr_entry_new(argv->argv[i]);
			if (entry == NULL)
				continue;
			http_hdr_append_entry(&hdr_req->hdr, entry);
		}
		acl_argv_free(argv);
	}

FORWARD:

	http_hdr_build_request(hdr_req, buf);
	pHost = http_hdr_req_host(hdr_req);
	ASSERT(pHost);

	phClient->ReportReqHdr(acl_vstring_str(buf), (int) ACL_VSTRING_LEN(buf));

	CString serverAddr;

	if (phClient->m_bUseAddr)
		serverAddr.Format("%s", phClient->m_sServerAddr);
	if (serverAddr.GetLength() == 0)
		serverAddr.Format("%s", pHost);
	if (strchr(serverAddr.GetString(), ':') == NULL)
		serverAddr.AppendFormat(":80");

	time(&begin);
	gettimeofday(&begin_tv, NULL);
	server = acl_vstream_connect(serverAddr.GetString(),
				ACL_BLOCKING, 10, 10, 4096);
	if (server == NULL) {
		CString msg;

		msg.Format("Connect server(%s) error", serverAddr.GetString());
		phClient->ReportErrConnect(msg.GetString());
		RETURN (NULL);
	}

	if (phClient->m_bLocalSave && fp == NULL) {
		fp = acl_vstream_fopen(phClient->m_sLocalFile.GetString(),
			O_WRONLY | O_CREAT | O_TRUNC, 0600, 4096);
		if (fp == NULL) {
			acl_msg_error("%s(%d): can't create file(%s)",
				__FILE__, __LINE__, phClient->m_sLocalFile.GetString());
			RETURN (NULL);
		}
	}

	ret = acl_vstream_writen(server, acl_vstring_str(buf), ACL_VSTRING_LEN(buf));
	if (ret == ACL_VSTREAM_EOF) {
		acl_msg_error("%s(%d): write error", __FILE__, __LINE__);
		RETURN (NULL);
	}

	if (phClient->m_bPostMethod && !phClient->m_sHttpBody.IsEmpty())
	{
		if (acl_vstream_writen(server, phClient->m_sHttpBody.GetString(),
			phClient->m_sHttpBody.GetLength()) == ACL_VSTREAM_EOF)
		{
			acl_msg_error("%s(%d): write body error", __FILE__, __LINE__);
			RETURN (NULL);
		}
	}

	hdr_res = http_hdr_res_new();
	if (http_hdr_res_get_sync(hdr_res, server, 10) < 0) {
		acl_msg_error("%s(%d): get res hdr error", __FILE__, __FILE__, __LINE__);
		RETURN (NULL);
	}

	if (http_hdr_res_parse(hdr_res) < 0) {
		acl_msg_error("%s(%d): parse hdr_res error", __FILE__, __LINE__);
		RETURN (NULL);
	}

	http_hdr_build(&hdr_res->hdr, buf);
	phClient->ReportResHdr(acl_vstring_str(buf), (int) ACL_VSTRING_LEN(buf));
	phClient->ReportResContentLength((int) hdr_res->hdr.content_length);

	if (hdr_res->reply_status > 300	&& hdr_res->reply_status < 400) {
		const char* pLocation;
		HTTP_HDR_REQ *hdr_req_tmp;

		if (!phClient->m_bForwardAuto)
			RETURN (NULL);

		if (nForward++ >= phClient->m_nMaxTry) {
			acl_msg_error("%s(%d): too many redirect, nForward(%d)",
				__FILE__, __LINE__, nForward);
			RETURN (NULL);
		}

		pLocation = http_hdr_entry_value(&hdr_res->hdr, "Location");
		if (pLocation == NULL || *pLocation == 0) {
			acl_msg_error("%s(%d): 302 reply with no Location", __FILE__, __LINE__);
			RETURN (NULL);
		}

		hdr_req_tmp = http_hdr_req_rewrite(hdr_req, pLocation);
		if (hdr_req_tmp == NULL)
			RETURN (NULL);
		http_hdr_req_free(hdr_req);
		http_hdr_res_free(hdr_res);
		hdr_req = hdr_req_tmp;
		goto FORWARD;
	}

	http_res = http_res_new(hdr_res);
	while (1) {
		char  tmp_buf[4096];

		ret = (int) http_res_body_get_sync2(http_res, server,
				tmp_buf, sizeof(tmp_buf) - 1);
		if (ret <= 0)
			break;

		phClient->ReportResBody(tmp_buf, ret);
		phClient->ReportResDownLength((int)(time(NULL) - begin), ret);

		if (fp != NULL && acl_vstream_writen(fp, tmp_buf, ret) == ACL_VSTREAM_EOF) {
			acl_msg_error("%s(%d): write to file error", __FILE__, __LINE__);
			break;
		}
	}
	RETURN (NULL);
}

void CHttpClient::DisplayReqHdr(BOOL enable)
{
	m_bDisplayReqHdr = enable;
}

void CHttpClient::DisplayResHdr(BOOL enable)
{
	m_bDisplayResHdr = enable;
}

void CHttpClient::DisplayResBody(BOOL enable)
{
	m_bDisplayResBody = enable;
}

void CHttpClient::EnableHttp11(BOOL enable)
{
	m_bHttp11 = enable;
}

void CHttpClient::EnableZip(BOOL enable)
{
	m_bZip = enable;
}

void CHttpClient::EnableKeepAlive(BOOL enable)
{
	m_bKeepAlive = enable;
}

