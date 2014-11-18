#include "StdAfx.h"
#include "IdxTrans.h"

CIdxTrans::CIdxTrans(void)
: m_nMsgTransing(0)
, m_nMsgTransEnd(0)
, m_hWnd(0)
, m_sPath(_T(""))
{
}

CIdxTrans::~CIdxTrans(void)
{
}

void CIdxTrans::Init(HWND hWnd, CString &sPath)
{
	m_hWnd = hWnd;
	m_sPath = sPath;
}

void CIdxTrans::Run(void)
{
	acl_pthread_t tid;

	if (m_sPath.GetLength() == 0)
		return;
	acl_pthread_create(&tid, NULL, RunThread, this);
}

void CIdxTrans::OnTransing(int nMsg)
{
	m_nMsgTransing = nMsg;
}

void CIdxTrans::OnTransEnd(int nMsg)
{
	m_nMsgTransEnd = nMsg;
}

void CIdxTrans::Trans(void)
{
	char *sBuf;
	size_t size, size_saved;
	struct acl_stat stat_buf;

	if (m_sPath.GetLength() == 0)
	{
		MessageBox(NULL, "文件路径为空！", "Error", 0);
		return;
	}
	if (acl_stat(m_sPath, &stat_buf) < 0)
	{
		MessageBox(NULL, "无法获得文件长度！", "Error", 0);
		return;
	}
	size = (size_t) stat_buf.st_size;
	size_saved = size;

	sBuf = acl_vstream_loadfile(m_sPath);
	if (sBuf == NULL)
		return;

	char *ptr, *pBuf = sBuf;

	ptr = pBuf;
	while (1)
	{
		if (size <= 4)
			break;
		if (*ptr == '_' && *(ptr + 1) == 'a' && *(ptr + 2) == 'c' && *(ptr + 3) == 'l')
		{
			char *ptr1 = ptr, *pBegin = NULL;

			while (ptr1 > pBuf)
			{
				char ch = *(ptr1 - 1);

				if (ch == 'p')
				{
					if (ptr1 - 2 <= pBuf)
						break;
					ch = *(ptr1 - 2);
					if ((ch >= 'a' && ch <= 'z') || ch == '_' || ch == ' ')
					{
						ptr1--;
					}
					else
					{
						pBegin = ptr1;
						break;
					}
				}
				else if (ch == 'L' || ch == 'X')
				{
					pBegin = ptr1;
					break;
				}
				else if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')
					|| ch == '1' || ch == '2' || ch == '_')
				{
					ptr1--;
				}
				else
				{
					pBegin = ptr1;
					break;
				}
			}
			if (pBegin != NULL)
			{
				memcpy(pBegin + 4, pBegin, ptr - pBegin);
				memcpy(pBegin, "acl_", 4);
				pBuf = ptr + 4;
			}
			ptr += 4;
			size -= 4;
		}
		else if (*ptr == '_' && *(ptr + 1) == 'A' && *(ptr + 2) == 'C' && *(ptr + 3) == 'L')
		{
			char *ptr1  = ptr, *pBegin = NULL;

			while (ptr1 > pBuf)
			{
				char ch = *(ptr1 - 1);
				if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')
					|| ch == '1' || ch == '2' || ch == '_')
				{
					ptr1--;
				}
				else
				{
					pBegin = ptr1;
					break;
				}
			}
			if (pBegin != NULL)
			{
				memcpy(pBegin + 4, pBegin, ptr - pBegin);
				memcpy(pBegin, "ACL_", 4);
				pBuf = ptr + 4;
			}
			ptr += 4;
			size -= 4;
		}
		else
		{
			ptr++;
			size--;
		}
	}

	ACL_VSTREAM *fp;

	fp = acl_vstream_fopen("result.idx", O_CREAT | O_TRUNC | O_RDWR | O_BINARY, 0600, 1024);
	if (fp == NULL)
	{
		MessageBox(NULL, "创建文件失败！", "Error", 0);
	}
	else
	{
		if (acl_vstream_writen(fp, sBuf, size_saved) == ACL_VSTREAM_EOF) {
			MessageBox(NULL, "写文件失败！", "Error", 0);
		}
		acl_vstream_close(fp);
	}
	acl_myfree(sBuf);
}

void *CIdxTrans::RunThread(void *arg)
{
	CIdxTrans *pIdxTrans = (CIdxTrans*) arg;

	pIdxTrans->Trans();
	::PostMessage(pIdxTrans->m_hWnd, pIdxTrans->m_nMsgTransEnd, 0, 0);
	return (NULL);
}