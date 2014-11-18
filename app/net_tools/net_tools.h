// net_tools.h : PROJECT_NAME 应用程序的主头文件
//

#pragma once

#ifndef __AFXWIN_H__
	#error 在包含用于 PCH 的此文件之前包含“stdafx.h”
#endif

#include "SingleCtrl.h"
#include "resource.h"		// 主符号


// Cnet_toolsApp:
// 有关此类的实现，请参阅 net_tools.cpp
//

class Cnet_toolsApp : public CWinApp
{
public:
	Cnet_toolsApp();

// 重写
public:
	virtual BOOL InitInstance();

// 实现

	DECLARE_MESSAGE_MAP()

public:
	CSingleCtrl m_singleCtrl;
};

extern Cnet_toolsApp theApp;
