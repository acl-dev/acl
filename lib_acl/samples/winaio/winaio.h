// winaio.h : PROJECT_NAME 应用程序的主头文件
//

#pragma once

#ifndef __AFXWIN_H__
	#error 在包含用于 PCH 的此文件之前包含“stdafx.h”
#endif

#include "resource.h"		// 主符号


// CwinaioApp:
// 有关此类的实现，请参阅 winaio.cpp
//

class CwinaioApp : public CWinApp
{
public:
	CwinaioApp();

// 重写
	public:
	virtual BOOL InitInstance();

// 实现

	DECLARE_MESSAGE_MAP()
};

extern CwinaioApp theApp;
