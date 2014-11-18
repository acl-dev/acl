#include "StdAfx.h"
#include "DelBom.h"

CDelBOM::CDelBOM(void)
: m_nMsgDeleting(0)
, m_nMsgDeleted(0)
, m_hWnd(0)
, m_sPath(_T(""))
{
}

CDelBOM::~CDelBOM(void)
{
}

void CDelBOM::Init(HWND hWnd, CString &sPath)
{
	m_hWnd = hWnd;
	m_sPath = sPath;
}

void CDelBOM::Run(void)
{
	acl_pthread_t tid;

	if (m_sPath.GetLength() == 0)
	{
		MessageBox(NULL, "文件路径为空！", "Error", 0);
		::PostMessage(m_hWnd, m_nMsgDeleted, 0, 0);
		return;
	}

	acl_pthread_create(&tid, NULL, RunThread, this);
}

void CDelBOM::OnDeleting(int nMsg)
{
	m_nMsgDeleting = nMsg;
}

void CDelBOM::OnDeleted(int nMsg)
{
	m_nMsgDeleted = nMsg;
}

bool CDelBOM::DeleteBOM(CString& filePath)
{
	char* sBuf;

	sBuf = acl_vstream_loadfile(filePath);
	if (sBuf == NULL)
		return false;
	size_t len = strlen(sBuf);
	if (len < 3)
	{
		acl_myfree(sBuf);
		return false;
	}

	// 先判断文件内容前缀是否是BOM格式
	if (sBuf[0] != (char) 0xEF || sBuf[1] != (char) 0xBB || sBuf[2] != (char) 0xBF)
	{
		acl_myfree(sBuf);
		return false;
	}

	// 将内容指针偏移 3 个字节，即去掉BOM格式
	len -= 3;
	char* ptr = sBuf + 3;

	ACL_VSTREAM* fp = acl_vstream_fopen(filePath,
		O_WRONLY | O_APPEND | O_TRUNC, 0600, 4096);
	if (fp == NULL)
	{
		acl_myfree(sBuf);
		return false;
	}
	else if (len == 0)
	{
		acl_vstream_fclose(fp);
		acl_myfree(sBuf);
		return true;
	}
	else if (acl_vstream_writen(fp, ptr, len) == ACL_VSTREAM_EOF)
	{
		acl_msg_error("write to file: %s error: %s",
			filePath.GetString(), acl_last_serror());
	}

	acl_vstream_fclose(fp);
	acl_myfree(sBuf);
	return true;
}

void* CDelBOM::RunThread(void *arg)
{
	CDelBOM* pDel = (CDelBOM*) arg;

	ACL_SCAN_DIR *scan = acl_scan_dir_open(pDel->m_sPath.GetString(), 1);
	if (scan == NULL)
	{
		CString msg;
		msg.Format("Open path %s error", pDel->m_sPath.GetString());
		MessageBox(NULL, msg, "Open path", 0);
		::PostMessage(pDel->m_hWnd, pDel->m_nMsgDeleted, 0, 0);
		return NULL;
	}

	while (true)
	{
		const char* pFile = acl_scan_dir_next_file(scan);
		if (pFile == NULL)
			break;

		// 过滤掉非纯文本的文件
		if (acl_strrncasecmp(pFile, ".c", 2) &&
			acl_strrncasecmp(pFile, ".cpp", 4) &&
			acl_strrncasecmp(pFile, ".cxx", 4) &&
			acl_strrncasecmp(pFile, ".h", 2) &&
			acl_strrncasecmp(pFile, ".hpp", 4) &&
			acl_strrncasecmp(pFile, ".hxx", 4) &&
			acl_strrncasecmp(pFile, ".java", 5) &&
			acl_strrncasecmp(pFile, ".txt", 4) &&
			acl_strrncasecmp(pFile, ".php", 4) &&
			acl_strrncasecmp(pFile, ".html", 5) &&
			acl_strrncasecmp(pFile, ".js", 3) &&
			acl_strrncasecmp(pFile, ".css", 4) &&
			acl_strrncasecmp(pFile, ".d", 2) &&
			acl_strrncasecmp(pFile, ".py", 3) &&
			acl_strrncasecmp(pFile, ".perl", 5) &&
			acl_strrncasecmp(pFile, ".cs", 3) &&
			acl_strrncasecmp(pFile, ".as", 3))
		{
			acl_msg_info(">>skip file: %s", pFile);
			continue;
		}
		CString filePath = acl_scan_dir_path(scan);
		filePath += "\\";
		filePath += pFile;
		if (pDel->DeleteBOM(filePath) == true)
			acl_msg_info(">>modify file %s", filePath.GetString());
		else
			acl_msg_info(">>skip file %s", filePath.GetString());
	}

	acl_scan_dir_close(scan);
	::PostMessage(pDel->m_hWnd, pDel->m_nMsgDeleted, 0, 0);
	acl_msg_info(">>scan over, msg: %d", pDel->m_nMsgDeleted);
	return NULL;
}
