// MeterBar.cpp : 实现文件
//

#include "stdafx.h"
#include "http_client.h"
#include "MeterBar.h"
#include ".\meterbar.h"


// CMeterBar

IMPLEMENT_DYNAMIC(CMeterBar, CStatusBarCtrl)
CMeterBar::CMeterBar()
: m_pWidths(NULL)
{
}

CMeterBar::~CMeterBar()
{
	delete m_pWidths;
}


BEGIN_MESSAGE_MAP(CMeterBar, CStatusBarCtrl)
	ON_WM_CREATE()
	ON_WM_SIZE()
END_MESSAGE_MAP()

BOOL CMeterBar::SetParts(int nParts, int* pWidths)
{
	m_nParts = nParts;
	m_pWidths = new int[nParts];
	int  i;
	for (i = 0; i < nParts; i++)
	{
		m_pWidths[i] = pWidths[i];
	}
	return (TRUE);
}

// CMeterBar 消息处理程序


int CMeterBar::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CStatusBarCtrl::OnCreate(lpCreateStruct) == -1)
		return -1;

	ASSERT(m_pWidths);
	CStatusBarCtrl::SetParts(m_nParts, m_pWidths);
	//int tmp;
	//int n = GetParts(0, &tmp);

	// TODO:  在此添加您专用的创建代码
	//m_meter.Create(WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), this, 101);
	//m_meter.Create("Press start timer to see me go",
	//	WS_VISIBLE | WS_CHILD | WS_EX_CLIENTEDGE, CRect(0, 0, 0, 0), this, (HMENU) 101);
	static CString className = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW);
	DWORD dwExStyle = WS_EX_STATICEDGE; //WS_EX_CLIENTEDGE | WS_EX_STATICEDGE;
	m_meter.CreateEx(dwExStyle, className,
		"Press start timer to see me go", WS_VISIBLE | WS_CHILD,
		CRect(0, 0, 0, 0), this, 101);

	m_meter.SetRange(0, 100);
	m_meter.SetPos(0);
	//m_meter.SetStep(1);

	return 0;
}

void CMeterBar::OnSize(UINT nType, int cx, int cy)
{
	CStatusBarCtrl::OnSize(nType, cx, cy);

	// TODO: 在此处添加消息处理程序代码
	int nTemp;
	int nCount = GetParts(0, &nTemp);
	if (nCount > 0) {
		CRect r;
		GetRect(nCount - 1, r);
		m_meter.MoveWindow(r);
	}
}
