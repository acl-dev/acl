#pragma once
#include <atlbase.h>

class AFX_EXT_CLASS CRegRun
{
public:
	CRegRun(void);
	CRegRun(const char *procname);
	~CRegRun(void);
	void Init(const char *procname);

	BOOL AutoRun(BOOL bAutoRun, const char *procname);
	BOOL IfAutoRun(void);

public:
	CString m_sErrMsg;
	int m_nErrno;
private:
	CString m_sRegName;
};
