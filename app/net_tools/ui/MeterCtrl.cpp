// MeterCtrl.cpp : 实现文件
//

#include "stdafx.h"
#pragma warning(disable:4312)
#include ".\Meterctrl.h"


// CMeterCtrl

IMPLEMENT_DYNAMIC(CMeterCtrl, CWnd)
CMeterCtrl::CMeterCtrl()
: m_nLower(0)
, m_nUpper(0)
, m_nPos(0)
{
}

CMeterCtrl::~CMeterCtrl()
{
}


BEGIN_MESSAGE_MAP(CMeterCtrl, CWnd)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()



// CMeterCtrl 消息处理程序


void CMeterCtrl::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	// TODO: 在此处添加消息处理程序代码
	// 不为绘图消息调用 CWnd::OnPaint()
	CRect rClient;
	GetClientRect(rClient);
	CRect rLeft = rClient;
	CRect rRight = rClient;
	UINT nRange = m_nUpper - m_nLower;

	rLeft.right = (LONG)(((m_nPos - m_nLower)  * rClient.Width())/ nRange);
	rRight.left = rLeft.right;

	CRgn rgnLeft, rgnRight;
	rgnLeft.CreateRectRgnIndirect(rLeft);
	rgnRight.CreateRectRgnIndirect(rRight);

	CBrush *pBrush = CBrush::FromHandle(GetSysColorBrush(COLOR_HIGHLIGHT));
	dc.FillRect(rLeft, pBrush);
	CString msg;

//	msg.Format(">>left=%d, right=%d, top=%d, bottom=%d, nrange=%d, width=%d, npos=%d, nlower=%d",
//		rLeft.left, rLeft.right, rLeft.top, rLeft.bottom,
//		nRange, rClient.Width(), m_nPos, m_nLower);
//	AfxMessageBox(msg);

	//CString strCaption;
	//GetWindowText(strCaption);

	//strCaption.Format("%d%% (%d)", ((m_nPos - m_nLower)  * 100)/ nRange, m_nPos);

	dc.SelectClipRgn(&rgnLeft);
	dc.SetBkMode(TRANSPARENT);
	dc.SetTextColor(GetSysColor(COLOR_HIGHLIGHTTEXT));
	dc.DrawText(m_sCaption, rClient, DT_BOTTOM | DT_CENTER);
	dc.SelectClipRgn(&rgnRight);
	dc.SetTextColor(GetSysColor(COLOR_HIGHLIGHT));
	dc.DrawText(m_sCaption, rClient, DT_BOTTOM | DT_CENTER);
}

BOOL CMeterCtrl::OnEraseBkgnd(CDC* pDC)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	CRect r;
	GetClientRect(r);
	CBrush *pBrush = CBrush::FromHandle(GetSysColorBrush(COLOR_BTNFACE));
	pDC->FillRect(r, pBrush);

	return TRUE;
//	return CWnd::OnEraseBkgnd(pDC);
}

BOOL CMeterCtrl::Create(LPCTSTR lpszWindowName, DWORD dwStyle,
		const RECT& rect, CWnd* pParentWnd, HMENU nID)
{
	// TODO: 在此添加专用代码和/或调用基类
	static CString className = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW);

	return CWnd::CreateEx(WS_EX_STATICEDGE, //WS_EX_CLIENTEDGE | WS_EX_STATICEDGE,
		className, lpszWindowName, dwStyle,
		rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
		pParentWnd->GetSafeHwnd(), (HMENU) nID);

//	return CWnd::Create(lpszClassName, lpszWindowName, dwStyle,
//			rect, pParentWnd, nID, pContext);
}

BOOL CMeterCtrl::CreateEx(DWORD dwExStyle, LPCTSTR lpszClassName,
		LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect,
		CWnd* pParentWnd, UINT nID, LPVOID lpParam)
{
	// TODO: 在此添加专用代码和/或调用基类

	static CString className = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW);

	return CWnd::CreateEx(dwExStyle, //WS_EX_CLIENTEDGE | WS_EX_STATICEDGE,
		lpszClassName, lpszWindowName, dwStyle,
		rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
		pParentWnd->GetSafeHwnd(), (HMENU) nID);

	//return CWnd::CreateEx(dwExStyle, lpszClassName, lpszWindowName,
	//			dwStyle, rect, pParentWnd, nID, lpParam);
}

UINT CMeterCtrl::SetPos(UINT nPos)
{
	ASSERT(nPos >= m_nLower && nPos <= m_nUpper);

	UINT nRange = m_nUpper - m_nLower;
	UINT nOld = m_nPos;
	m_nPos = nPos;

	m_sCaption.Format("%d%% (%d)", ((m_nPos - m_nLower)  * 100)/ nRange, m_nPos);
	InvalidateMeater();

	return nOld;
}

void CMeterCtrl::SetRange(UINT nLower, UINT nUpper)
{
	ASSERT(nLower >= 0 && nLower < 0xffff);
	ASSERT(nUpper > nLower && nUpper < 0xffff);

	m_nLower = nLower;
	m_nUpper = nUpper;

	InvalidateMeater();
}

void CMeterCtrl::StepIt(void)
{
	m_nPos++;
	if (m_nPos > m_nUpper)
		m_nPos = m_nLower;

	InvalidateMeater();
}

void CMeterCtrl::SetText(CString& msg)
{
	m_sCaption = msg;
	InvalidateMeater();
}

void CMeterCtrl::SetText(const char* pMsg)
{
	m_sCaption = pMsg;
	InvalidateMeater();
}

void CMeterCtrl::InvalidateMeater(void)
{
	CRect r;
	GetClientRect(r);
	InvalidateRect(r);
}
