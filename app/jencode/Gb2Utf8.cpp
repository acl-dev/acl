#include "StdAfx.h"
#include "Gb2Utf8.h"

CGb2Utf8::CGb2Utf8(const char* fromCharset, const char* toCharset)
: m_hWnd(0)
, m_sPath(_T(""))
, m_dPath(_T(""))
, m_fromCharset(fromCharset)
, m_toCharset(toCharset)
{
}

CGb2Utf8::~CGb2Utf8(void)
{
}

CGb2Utf8::CGb2Utf8(HWND hWnd, CString &sPath, CString &dPath)
{
	Init(hWnd, sPath, dPath);
}

void CGb2Utf8::Init(HWND hWnd, CString &sPath, CString &dPath)
{
	m_hWnd =hWnd;
	m_sPath = sPath;
	m_dPath = dPath;
}

void CGb2Utf8::OnTransing(int nMsg)
{
	m_nMsgTransing = nMsg;
}

void CGb2Utf8::OnTransEnd(int nMsg)
{
	m_nMsgTransEnd = nMsg;
}

int CGb2Utf8::TransformPath(CString *path_from, CString *path_to)
{
	ACL_SCAN_DIR *scan_src; //, *scan_dst;

	scan_src = acl_scan_dir_open(path_from->GetString(), 1);
	if (scan_src == NULL) {
		CString msg;

		msg.Format("Open src path %s error", path_from->GetString());
		MessageBox(NULL, msg, "Open path", 0);
		return (-1);
	}
	//scan_dst = acl_scan_dir_open(to.GetString(), 1);
	//if (scan_dst == NULL) {
	//	MessageBox(NULL, "Open path", "Open dst path error", 0);
	//	return (-1);
	//}

	while (1)
	{
		const char *fName;
		CString fPath;

		fName = acl_scan_dir_next_file(scan_src);
		if (fName == NULL)
			break;
		fPath = acl_scan_dir_path(scan_src);
		fPath += "\\";
		fPath += fName;
		TransformFile(fPath.GetString(), NULL);
	}
	acl_scan_dir_close(scan_src);
	return (0);
}

int CGb2Utf8::TransformFile(const char *pFrom, const char *pTo)
{
	char *sBuf = NULL;
	size_t iLen;

#undef RETURN
#define RETURN(_x_) do \
{ \
	if (sBuf) \
		acl_myfree(sBuf); \
	return(_x_); \
} while(0);

	sBuf = acl_vstream_loadfile(pFrom);
	if (sBuf == NULL)
		RETURN (-1);
	if (*sBuf == 0)
		RETURN (-1);

	iLen = strlen(sBuf);

	acl::charset_conv conv;
	acl::string buf;
	if (conv.convert(m_fromCharset.GetString(), m_toCharset.GetString(),
		sBuf, iLen, &buf) == false)
	{
		logger_error("conver from %s to %s error: %s",
			m_fromCharset.GetString(), m_toCharset.GetString(),
			conv.serror());
		RETURN (-1);
	}

	ACL_VSTREAM *fp;
	fp = acl_vstream_fopen(pFrom, O_RDWR | O_TRUNC | O_BINARY | O_APPEND, 0600, 1024);
	if (fp == NULL)
		RETURN (-1);
 	int ret = acl_vstream_writen(fp, buf.c_str(), buf.length());
	acl_vstream_close(fp);
	RETURN (ret == ACL_VSTREAM_EOF ? -1 : 0);
}

void *CGb2Utf8::RunThread(void *arg)
{
	CGb2Utf8 *pGb2Utf8 = (CGb2Utf8*) arg;

	pGb2Utf8->TransformPath(&pGb2Utf8->m_sPath, &pGb2Utf8->m_dPath);
	::PostMessage(pGb2Utf8->m_hWnd, pGb2Utf8->m_nMsgTransEnd, 0, 0);
	return NULL;
}

void CGb2Utf8::Run(void)
{
	acl_pthread_t tid;

	if (m_dPath.GetLength() == 0 || m_sPath.GetLength() == 0)
		return;
	acl_pthread_create(&tid, NULL, RunThread, this);
}
