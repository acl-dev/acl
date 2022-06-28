#include "StdAfx.h"
#include ".\regrun.h"

CRegRun::CRegRun(void)
: m_nErrno(ERROR_SUCCESS)
{

}

CRegRun::CRegRun(const char *pRegName)
: m_sRegName(pRegName)
, m_nErrno(ERROR_SUCCESS)
{
}

CRegRun::~CRegRun(void)
{
}

void CRegRun::Init(const char *procname)
{
	m_nErrno = ERROR_SUCCESS;
	m_sRegName.Format(procname);
}

static char sKey[] = "Software\\Microsoft\\Windows\\CurrentVersion\\Run";

BOOL CRegRun::AutoRun(BOOL bAutoRun, const char *procname)
{
	CRegKey regRun;
	int   ret;

	if (bAutoRun) {
		if (!IfAutoRun()) {
			ret = regRun.Create(HKEY_LOCAL_MACHINE, sKey);
			if (ret == ERROR_SUCCESS) {
				regRun.SetStringValue(m_sRegName.GetString(), procname, REG_SZ);
				regRun.Close();
			} else {
				m_sErrMsg.Format("CRegKey::Create sKey(%s) error(%d)", sKey, ret);
				m_nErrno = ret;
				return FALSE;
			}
		}
	} else {
		ret = regRun.Open(HKEY_LOCAL_MACHINE, sKey, KEY_ALL_ACCESS);
		if (ret == ERROR_SUCCESS) {
			regRun.DeleteValue(m_sRegName.GetString());
			regRun.Close();
		} else {
			m_sErrMsg.Format("CRegKey::Open sKey(%s) error(%d)", sKey, ret);
			m_nErrno = ret;
			return FALSE;
		}
	}

	return TRUE;
}

BOOL CRegRun::IfAutoRun()
{
	CRegKey regRun;
	int   ret;

	ret = regRun.Open(HKEY_LOCAL_MACHINE, sKey, KEY_READ);
	if (ret == ERROR_SUCCESS) {
		char  buf[256];
		DWORD dwCount, dwType;

		dwCount = sizeof(buf);
		ret = regRun.QueryValue(m_sRegName.GetString(), &dwType, buf, &dwCount);
		regRun.Close();
		if (ret == ERROR_SUCCESS) {
			return TRUE;
		} else {
			return FALSE;
		}
	} else {
		m_sErrMsg.Format("CRegKey::Open sKey(%s) error(%d)", sKey, ret);
		m_nErrno = ret;
		return FALSE;
	}
}