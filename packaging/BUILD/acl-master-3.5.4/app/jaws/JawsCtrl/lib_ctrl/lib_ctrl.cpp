// lib_ctrl.cpp : 定义 DLL 的初始化例程。
//

#include "stdafx.h"
#include <afxdllx.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

static AFX_EXTENSION_MODULE lib_ctrlDLL = { NULL, NULL };

extern "C" int APIENTRY
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	// 如果使用 lpReserved，请将此移除
	UNREFERENCED_PARAMETER(lpReserved);

	if (dwReason == DLL_PROCESS_ATTACH)
	{
		TRACE0("lib_ctrl.DLL 正在初始化！\n");
		
		// 扩展 DLL 一次性初始化
		if (!AfxInitExtensionModule(lib_ctrlDLL, hInstance))
			return 0;

		// 将此 DLL 插入到资源链中
		// 注意: 如果此扩展 DLL 由
		//  MFC 规则 DLL(如 ActiveX 控件)隐式链接到
		//  而不是由 MFC 应用程序链接到，则需要
		//  将此行从 DllMain 中移除并将其放置在一个
		//  从此扩展 DLL 导出的单独的函数中。使用此扩展 DLL 的
		//  规则 DLL 然后应显式
		//  调用该函数以初始化此扩展 DLL。否则，
		//  CDynLinkLibrary 对象不会附加到
		//  规则 DLL 的资源链，并将导致严重的
		//  问题。

		new CDynLinkLibrary(lib_ctrlDLL);

	}
	else if (dwReason == DLL_PROCESS_DETACH)
	{
		TRACE0("lib_ctrl.DLL 正在终止！\n");

		// 在调用析构函数之前终止该库
		AfxTermExtensionModule(lib_ctrlDLL);
	}
	return 1;   // 确定
}
