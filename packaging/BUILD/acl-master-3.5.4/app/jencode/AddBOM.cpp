#include "stdafx.h"
#include "AddBOM.h"

CAddBOM::CAddBOM(void)
: m_nMsgAdding(0)
, m_nMsgAdded(0)
, m_hWnd(0)
, m_sPath(_T(""))
{
}

CAddBOM::~CAddBOM(void)
{
}

void CAddBOM::Init(HWND hWnd, CString &sPath)
{
	m_hWnd  = hWnd;
	m_sPath = sPath;
}
void CAddBOM::OnAdding(int nMsg)
{
	m_nMsgAdding = nMsg;
}

void CAddBOM::OnAdded(int nMsg)
{
	m_nMsgAdded = nMsg;
}

void* CAddBOM::run(void)
{
	ScanAdd();
	::PostMessage(m_hWnd, m_nMsgAdded, 0, 0);
	return NULL;
}

void CAddBOM::ScanAdd(void)
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

	logger("open %s and start scanning for adding BOM ...",
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

			AddBOM(path);
		} else {
			logger(">>skip file: %s", pFile);
		}
	}

	logger("%s: all text files have been added BOM!", m_sPath.GetString());
}

bool CAddBOM::AddBOM(const acl::string& filePath)
{
	acl::string buf;
	if (!acl::ifstream::load(filePath, &buf)) {
		logger_error("load %s error %s",
			filePath.c_str(), acl::last_serror());
		return false;
	}
	if (buf.size() >= 3) {
		if (buf[0] == (char) 0xEF && buf[1] == (char) 0xBB
			&& buf[2] == (char) 0xBF) {

			logger("%s: has BOM header!", filePath.c_str());
			return true;
		}
	}

	acl::string buf2(buf.size() + 4);
	buf2 << (char) 0xEF << (char) 0xBB << (char) 0xBF;
	buf2 += buf;

	acl::ofstream fp;
	if (!fp.open_write(filePath, true)) {
		logger_error("open_write %s error %s",
			filePath.c_str(), acl::last_serror());
		return false;
	}

	if (fp.write(buf2) != (int) buf2.size()) {
		logger_error("write to %s error %s",
			filePath.c_str(), acl::last_serror());
		return false;
	}

	logger("%s: add BOM ok!", filePath.c_str());
	return true;
}
