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
	m_hWnd  = hWnd;
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

bool CGb2Utf8::TransformPath(const char *path_from, const char *path_to)
{
	acl::scan_dir scan;
	if (!scan.open(path_from)) {
		CString msg;
		msg.Format("Open src path %s error", path_from);
		MessageBox(NULL, msg, "Open path", 0);
		return false;
	}

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

			TransformFile(pFile, pFile);
		} else {
			logger(">>skip file: %s", pFile);
		}
	}
	return true;
}

bool CGb2Utf8::TransformFile(const char *pFrom, const char *pTo)
{
	acl::string sBuf;
	if (!acl::ifstream::load(pFrom, sBuf)) {
		logger_error("load from %s error %s", pFrom, acl::last_serror());
		return false;
	}

	acl::charset_conv conv;
	acl::string tBuf;
	if (!conv.convert(m_fromCharset.GetString(), m_toCharset.GetString(),
		sBuf.c_str(), sBuf.size(), &tBuf)) {

		logger_error("conver from %s to %s error: %s, file: %s",
			m_fromCharset.GetString(), m_toCharset.GetString(),
			conv.serror(), pFrom);
		return false;
	}

	acl::ofstream fp;
	if (!fp.open_write(pTo, true)) {
		logger_error("open %s error %s", pTo, acl::last_serror());
		return false;
	}
	if (fp.write(tBuf) != (int) tBuf.size()) {
		logger_error("write to %s error %s", pTo, acl::last_serror());
	} else {
		logger("transer from %s to %s ok, file: %s",
			m_fromCharset.GetString(), m_toCharset.GetString(), pTo);
	}
	return true;
}

void *CGb2Utf8::run(void)
{
	if (m_dPath.IsEmpty() || m_sPath.IsEmpty()) {
		return NULL;
	}
	TransformPath(m_sPath, m_dPath);
	::PostMessage(m_hWnd, m_nMsgTransEnd, 0, 0);
	return NULL;
}

