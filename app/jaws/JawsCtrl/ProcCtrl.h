#pragma once
#include "lib_acl.h"
#include "ProcService.h"

class CProcCtrl
{
public:
	CProcCtrl(void);
	~CProcCtrl(void);
	void RunThread(void);
private:
	char m_exePath[MAX_PATH];
	char m_logPath[MAX_PATH];
	char m_procName[MAX_PATH];
	CProcService m_service;
	static void *StartThread(void*);
public:
	void StartOne(CProcService&);
	void StopOne(CProcService&);
};
