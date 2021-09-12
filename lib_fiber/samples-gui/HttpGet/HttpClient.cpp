#include "pch.h"
#include "HttpGetDlg.h"
#include "HttpClient.h"

CHttpClient::CHttpClient(CHttpGetDlg& hWin, const CString& url)
: m_hWin(hWin)
, m_url(url)
{
}

CHttpClient::~CHttpClient(void) {}

void CHttpClient::run()
{
	acl::http_url hu;
	if (!hu.parse(m_url.GetString())) {
		m_hWin.SetError("Invalid url=%s", m_url.GetString());
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
	m_hWin.SetRequestHead(head.c_str());

	if (!request.request(NULL, 0)) {
		m_hWin.SetError("Send request to %s error: %s",
			addr.GetString(), acl::last_serror());
		return;
	}

	acl::http_client* client = request.get_client();
	head.clear();
	client->sprint_header(head);
	m_hWin.SetResponseHead(head);

	long long length = request.body_length();
	m_hWin.SetBodyTotalLength(length);

	length = 0;
	char buf[8192];
	while (true) {
		int ret = request.read_body(buf, sizeof(buf));
		if (ret <= 0) {
			break;
		}

		length += ret;
		m_hWin.SetBodyLength(length);
	}

	m_hWin.SetBodyLength(length);
}
