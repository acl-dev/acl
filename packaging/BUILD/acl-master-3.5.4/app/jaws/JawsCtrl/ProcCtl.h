#pragma once

#include "lib_acl.h"

// CProcCtl

class CProcCtl : public CWinThread
{
	DECLARE_DYNCREATE(CProcCtl)

protected:
	CProcCtl();           // 动态创建所使用的受保护的构造函数
	virtual ~CProcCtl();

public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

protected:
	DECLARE_MESSAGE_MAP()
public:
	virtual int Run();
};


