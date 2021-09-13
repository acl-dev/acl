#include "pch.h"
#include "HttpGetDlg.h"
#include "HttpClient.h"

CHttpClient::CHttpClient(CHttpGetDlg& hWin, const CString& url)
: m_hWin(&hWin)
, m_box(NULL)
, m_url(url)
{
}

CHttpClient::CHttpClient(acl::fiber_tbox<CHttpMsg>& box, const CString& url)
: m_hWin(NULL)
, m_box(&box)
, m_url(url)
{
}

CHttpClient::~CHttpClient() {}

void CHttpClient::run()
{
	acl::http_url hu;
	if (!hu.parse(m_url.GetString())) {
		SetError("Invalid url=%s", m_url.GetString());
		SetEnd();
		return;
	}

	const char* domain  = hu.get_domain();
	unsigned short port = hu.get_port();
	CString addr;
	addr.Format("%s|%d", domain, port);
	acl::http_request request(addr.GetString());

	CString url;
	url.Format("%s", hu.get_url_path());
	const char* params = hu.get_url_params();
	if (*params != 0) {
		url.AppendFormat("?%s", params);
	}

	acl::http_header& header = request.request_header();
	header.set_url(url.GetString()).accept_gzip(true).set_host(domain);

	acl::string head;
	header.build_request(head);
	SetRequestHead(head.c_str());

	if (!request.request(NULL, 0)) {
		SetError("Send request to %s error: %s",
			addr.GetString(), acl::last_serror());
		SetEnd();
		return;
	}

	acl::http_client* client = request.get_client();
	head.clear();
	client->sprint_header(head);
	SetResponseHead(head);

	long long length = request.body_length();
	SetBodyTotalLength(length);

	length = 0;
	char buf[8192];
	while (true) {
		int ret = request.read_body(buf, sizeof(buf));
		if (ret <= 0) {
			break;
		}

		length += ret;
		SetBodyLength(length);
	}

	SetBodyLength(length);
	SetEnd();
}

void CHttpClient::SetError(const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	CString buf;
	buf.FormatV(fmt, ap);
	va_end(ap);

	if (m_box) {
		CHttpMsg* msg = new CHttpMsg(buf.GetString(), HTTP_MSG_ERR);
		m_box->push(msg);
	} else if (m_hWin) {
		m_hWin->SetError("%s", buf.GetString());
	}
}

void CHttpClient::SetRequestHead(const char* data)
{
	if (m_box) {
		CHttpMsg* msg = new CHttpMsg(data, HTTP_MSG_REQ);
		m_box->push(msg);
	} else if (m_hWin) {
		m_hWin->SetRequestHead(data);
	}
}

void CHttpClient::SetResponseHead(const char* data)
{
	if (m_box) {
		CHttpMsg* msg = new CHttpMsg(data, HTTP_MSG_RES);
		m_box->push(msg);
	} else if (m_hWin) {
		m_hWin->SetResponseHead(data);
	}
}

void CHttpClient::SetBodyTotalLength(long long length)
{
	if (m_box) {
		CHttpMsg* msg = new CHttpMsg(length, HTTP_MSG_TOTAL_LENGTH);
		m_box->push(msg);
	} else if (m_hWin) {
		m_hWin->SetBodyTotalLength(length);
	}
}

void CHttpClient::SetBodyLength(long long length)
{
	if (m_box) {
		CHttpMsg* msg = new CHttpMsg(length, HTTP_MSG_LENGTH);
		m_box->push(msg);
	} else if (m_hWin) {
		m_hWin->SetBodyLength(length);
	}
}

void CHttpClient::SetEnd() {
	if (m_box) {
		m_box->push(NULL);
	}
}
