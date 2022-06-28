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
	m_hWnd  = hWnd;
	m_sPath = sPath;
}

void CDelBOM::OnDeleting(int nMsg)
{
	m_nMsgDeleting = nMsg;
}

void CDelBOM::OnDeleted(int nMsg)
{
	m_nMsgDeleted = nMsg;
}

void* CDelBOM::run(void)
{
	ScanDel();
	::PostMessage(m_hWnd, m_nMsgDeleted, 0, 0);
	return NULL;
}

void CDelBOM::ScanDel(void)
{
	if (m_sPath.GetLength() == 0) {
		MessageBox(NULL, "文件路径为空！", "Error", 0);
		return;
	}

	acl::scan_dir scan;
	if (!scan.open(m_sPath.GetString())) {
		MessageBox(NULL, "打开文件路径失败！", "Error", 0);
		return;
	}

	logger("open %s and start scanning for deleting BOM ...",
		m_sPath.GetString());

	const char* pFile;
	while ((pFile = scan.next_file(true)) != NULL) {
		acl::string path(pFile);
		if (path.end_with("resource.h", false)) {
			logger(">>skip file: %s", pFile);
		} else if (path.end_with(".c") || path.end_with(".h") ||
			path.end_with(".cpp") || path.end_with(".cxx") ||
			path.end_with(".hpp") || path.end_with(".hxx") ||
			path.end_with(".java") || path.end_with(".txt") ||
			path.end_with(".php") || path.end_with(".html") ||
			path.end_with(".js") || path.end_with(".css") ||
			path.end_with(".d") || path.end_with(".py") ||
			path.end_with(".perl") || path.end_with(".cs") ||
			path.end_with(".as") || path.end_with(".go") ||
			path.end_with(".rust") || path.end_with(".erl")) {

			DeleteBOM(path);
		} else {
			logger(">>skip file: %s", pFile);
		}
	}

	logger("%s: all text files have been deleted BOM!", m_sPath.GetString());
}

bool CDelBOM::DeleteBOM(const acl::string& filePath)
{
	acl::string buf;
	if (!acl::ifstream::load(filePath.c_str(), &buf)) {
		logger_error("load from %s error %s",
			filePath.c_str(), acl::last_serror());
		return false;
	}
	if (buf.size() < 3) {
		return false;
	}
	// 先判断文件内容前缀是否是BOM格式
	if (buf[0] != (char) 0xEF || buf[1] != (char) 0xBB
		|| buf[2] != (char) 0xBF) {

		return false;
	}

	// 将内容指针偏移 3 个字节，即去掉BOM格式
	char* ptr = buf.c_str() + 3;

	acl::ofstream fp;
	if (!fp.open_write(filePath.c_str(), true)) {
		logger_error("open %s error %s for write",
			filePath.c_str(), acl::last_serror());
		return false;
	}
	size_t len = buf.size() - 3;
	if (fp.write(ptr, len) != len) {
		logger_error("write to %s error %s",
			filePath.c_str(), acl::last_serror());
		return false;
	}
	return true;
}
