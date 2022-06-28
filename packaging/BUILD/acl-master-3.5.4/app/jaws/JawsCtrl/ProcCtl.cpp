// ProcCtl.cpp : 实现文件
//

#include "stdafx.h"
#include "JawsCtrl.h"
#include "ProcCtl.h"
#include ".\procctl.h"


// CProcCtl

IMPLEMENT_DYNCREATE(CProcCtl, CWinThread)

CProcCtl::CProcCtl()
{
}

CProcCtl::~CProcCtl()
{
}

BOOL CProcCtl::InitInstance()
{
	// TODO: 在此执行任意逐线程初始化
	return TRUE;
}

int CProcCtl::ExitInstance()
{
	// TODO: 在此执行任意逐线程清理
	return CWinThread::ExitInstance();
}

BEGIN_MESSAGE_MAP(CProcCtl, CWinThread)
END_MESSAGE_MAP()


// CProcCtl 消息处理程序

int CProcCtl::Run()
{
	// TODO: 在此添加专用代码和/或调用基类

	return CWinThread::Run();
}
