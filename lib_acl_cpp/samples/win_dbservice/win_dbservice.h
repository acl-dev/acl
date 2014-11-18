// win_dbservice.h : PROJECT_NAME 应用程序的主头文件
//

#pragma once

#ifndef __AFXWIN_H__
	#error 在包含用于 PCH 的此文件之前包含“stdafx.h”
#endif

#include "resource.h"		// 主符号


// Cwin_dbserviceApp:
// 有关此类的实现，请参阅 win_dbservice.cpp
//

class Cwin_dbserviceApp : public CWinApp
{
public:
	Cwin_dbserviceApp();

// 重写
	public:
	virtual BOOL InitInstance();

// 实现

	DECLARE_MESSAGE_MAP()
};

extern Cwin_dbserviceApp theApp;
