#include "StdAfx.h"
#include "AclTrans.h"

CAclTrans::CAclTrans(void)
: m_hWnd(0)
, m_sPath(_T(""))
, m_nMsgTransing(0)
, m_nMsgTransEnd(0)
{
}

CAclTrans::~CAclTrans(void)
{
}

CAclTrans::CAclTrans(HWND hWnd, CString &sPath)
: m_nMsgTransing(0)
, m_nMsgTransEnd(0)
{
	Init(hWnd, sPath);
}

void CAclTrans::Init(HWND hWnd, CString &sPath)
{
	m_hWnd =hWnd;
	m_sPath = sPath;
}

void CAclTrans::OnTransing(int nMsg)
{
	m_nMsgTransing = nMsg;
}

void CAclTrans::OnTransEnd(int nMsg)
{
	m_nMsgTransEnd = nMsg;
}

void *CAclTrans::RunThread(void *arg)
{
	CAclTrans *pAclTrans = (CAclTrans*) arg;

	pAclTrans->ScanPath(&pAclTrans->m_sPath);
	::PostMessage(pAclTrans->m_hWnd, pAclTrans->m_nMsgTransEnd, 0, 0);
	return NULL;
}

void CAclTrans::Run(BOOL bTrans)
{
	acl_pthread_t tid;

	m_bTrans = bTrans;
	if (m_sPath.GetLength() == 0)
		return;
	acl_pthread_create(&tid, NULL, RunThread, this);
}

void CAclTrans::Restore(char *psBuf)
{
	char *pTagBegin, *ptr, *pBegin;
	int  len = (int) strlen("_acl");

	while (1)
	{
		pTagBegin = strcasestr(psBuf, "_acl");
		if (pTagBegin == NULL)
			break;
		if (strncmp(pTagBegin, "_acl", len) == 0)
		{
			if (*(pTagBegin + 1) == '.' && *(pTagBegin + 2) == 'h')
			{
				// skip "lib_acl.h"
				psBuf = pTagBegin + len;
				continue;
			}
			ptr = pTagBegin;
			pBegin = NULL;
			while (ptr >= psBuf)
			{
				if (*ptr == '>') {
					if (pBegin == NULL)
						pBegin = ptr + 1;
					break;
				}
				if (*ptr == '<') {
					pBegin = NULL;
					break;
				}
				if (*ptr == ' ' || *ptr == '\t'
					|| *ptr == '\r' || *ptr == '\n'
					|| *ptr == '*' || *ptr == '('
					|| *ptr == '{')
				{
					if (pBegin == NULL)
						pBegin = ptr + 1;
				}
				ptr--;
			}
			if (pBegin == NULL) {
				psBuf = pTagBegin + len;
				continue;
			}

			memcpy(pBegin + len, pBegin, pTagBegin - pBegin);
			memcpy(pBegin, "acl_", len);
			psBuf = pTagBegin + len;
		}
		else if (strncmp(pTagBegin, "_ACL", len) == 0)
		{
			ptr = pTagBegin;
			pBegin = NULL;
			while (ptr >= psBuf)
			{
				if (*ptr == '>') {
					if (pBegin == NULL)
						pBegin = ptr + 1;
					break;
				}
				if (*ptr == '<') {
					pBegin = NULL;
					break;
				}
				if (*ptr == ' ' || *ptr == '\t'
					|| *ptr == '\r' || *ptr == '\n'
					|| *ptr == '*' || *ptr == '('
					|| *ptr == '{')
				{
					if (pBegin == NULL)
						pBegin = ptr + 1;
				}
				ptr--;
			}
			if (pBegin == NULL) {
				psBuf = pTagBegin + len;
				continue;
			}
			memcpy(pBegin + len, pBegin, pTagBegin - pBegin);
			memcpy(pBegin, "ACL_", len);
			psBuf = pTagBegin + len;
		}
		else
			psBuf = pTagBegin + len;
	}
}

void CAclTrans::Trans(char *psBuf)
{
	char *pTagBegin, *pTagEnd, *pNextChar;
	int  len = (int) strlen("acl_");
	long int n;

	while (1)
	{
AGAIN_TAG:
		pTagBegin = strcasestr(psBuf, "acl_");
		if (pTagBegin == NULL)
			break;
		if (strncmp(pTagBegin, "acl_", len) == 0)
		{
			pNextChar = pTagBegin + len;
			pTagEnd = pNextChar;
			while (*pTagEnd)
			{
				if (*pTagEnd == '(')
					break;
				if (*pTagEnd == ' ' || *pTagEnd == '\t'
					|| *pTagEnd == '\r' || *pTagEnd == '\n'
					|| *pTagEnd == '.' || *pTagEnd == '\''
					|| *pTagEnd == '\"')
				{
					psBuf = pTagEnd;
					goto AGAIN_TAG;
				}
				pTagEnd++;
			}
			if (*pTagEnd == 0) {
				psBuf = pTagEnd;
				continue;
			}
			n = pTagEnd - pNextChar;
			memcpy(pTagBegin, pNextChar, n);
			memcpy(pTagBegin + n, "_acl", len);
			psBuf = pTagEnd;
		}
		else if (strncmp(pTagBegin, "ACL_", len) == 0)
		{
			pNextChar = pTagBegin + len;
			pTagEnd = pNextChar;
			while (*pTagEnd)
			{
				if (*pTagEnd == ' ' || *pTagEnd == '\t'
					|| *pTagEnd == ';' || *pTagEnd == ')'
					|| *pTagEnd == '}' || *pTagEnd == '*')
					break;
				if (*pTagEnd == '\r' || *pTagEnd == '\n'
					|| *pTagEnd == '.' || *pTagEnd == '\''
					|| *pTagEnd == '\"' || *pTagEnd == '(')
				{
					psBuf = pTagEnd;
					goto AGAIN_TAG;
				}
				pTagEnd++;
			}
			if (*pTagEnd == 0)
			{
				psBuf = pTagEnd;
				continue;
			}
			n = pTagEnd - pNextChar;
			memcpy(pTagBegin, pNextChar, n);
			memcpy(pTagBegin + n, "_ACL", len);
			psBuf = pTagEnd;
		}
		else
			psBuf += len;
	}
}

int CAclTrans::TransFile(const char *psPath)
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

	sBuf = acl_vstream_loadfile(psPath);
	if (sBuf == NULL || *sBuf == 0)
		RETURN (-1);

	iLen = strlen(sBuf);
	if (m_bTrans)
		Trans(sBuf);  // ¿ªÊ¼×ª»»
	else if (strstr(psPath, ".html") != NULL)
		Restore(sBuf);
	else
		RETURN (0);

	ACL_VSTREAM *fp;
	int   ret;

	fp = acl_vstream_fopen(psPath, O_RDWR | O_TRUNC | O_BINARY | O_APPEND, 0600, 1024);
	if (fp == NULL)
		RETURN (-1);
	ret = acl_vstream_writen(fp, sBuf, iLen);
	acl_vstream_close(fp);
	RETURN (ret == ACL_VSTREAM_EOF ? -1 : 0);
}

int CAclTrans::ScanPath(CString *psPath)
{
	ACL_SCAN_DIR *scan_src;

	scan_src = acl_scan_dir_open(psPath->GetString(), 1);
	if (scan_src == NULL)
	{
		CString msg;

		msg.Format("Open src path %s error", psPath->GetString());
		MessageBox(NULL, msg, "Open path", 0);
		return (-1);
	}

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
		TransFile(fPath.GetString());
	}
	acl_scan_dir_close(scan_src);
	return (0);
}
