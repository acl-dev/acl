#pragma once
#include "procservice.h"

class CHttpService : public CProcService
{
public:
	CHttpService(
		const char *procname,
		const char *addr,
		const char * vhostPath,
		const char * vhostDefault,
		const char * tmplPath,
		const char * filterInfo);
	~CHttpService(void);
private:
	CString m_addr;
	CString m_vhostPath;
	CString m_vhostDefault;
	CString m_tmplPath;
	CString m_filterInfo;
};
