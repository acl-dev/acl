#include "StdAfx.h"
#include ".\dialogexpand.h"

CDialogExpand::CDialogExpand(void)
: m_pDialog(NULL)
, m_nResourceID(0)
, m_bVertical(FALSE)
{
	m_rcVerticalLarge.SetRectEmpty();
	m_rcHorizontalLarge.SetRectEmpty();
}

CDialogExpand::CDialogExpand(CDialog *pDialog, int nResourceID, BOOL bVertical)
: m_pDialog(pDialog)
, m_bVertical(FALSE)
{
	Init(pDialog, nResourceID, bVertical);
}

CDialogExpand::~CDialogExpand(void)
{
}

void CDialogExpand::Init(CDialog *pDialog, int nResourceID, BOOL bVertical)
{
	ASSERT(pDialog);
	ASSERT(nResourceID);

	m_pDialog = pDialog;
	m_nResourceID = nResourceID;
	m_bVertical = bVertical;
	if (bVertical) {
		CRect rcLandmark;
		CWnd *pWndLandmark = m_pDialog->GetDlgItem(nResourceID);
		ASSERT(pWndLandmark);

		m_pDialog->GetClientRect(m_rcVerticalLarge);
		pWndLandmark->GetWindowRect(rcLandmark);

		m_rcVerticalSmall = m_rcVerticalLarge;
		m_rcVerticalLarge.bottom += 32;
		m_rcVerticalSmall.bottom = rcLandmark.top;
	} else {
		CRect rcLandmark;
		CWnd *pWndLandmark = m_pDialog->GetDlgItem(nResourceID);
		ASSERT(pWndLandmark);

		m_pDialog->GetClientRect(m_rcHorizontalLarge);
		pWndLandmark->GetWindowRect(rcLandmark);

		m_rcHorizontalLarge.bottom += 32;
		m_rcHorizontalSmall = m_rcHorizontalLarge;
		//m_rcHorizontalLarge.right += 32;
		m_rcHorizontalSmall.right = rcLandmark.left;
	}
}

void CDialogExpand::Expand(BOOL bExpand)
{
	if (m_bVertical)
		VerticalExpand(bExpand);
	else
		HorizontalExpand(bExpand);
}

void CDialogExpand::VerticalExpand(BOOL bExpand)
{
	if (bExpand) {
		m_pDialog->SetWindowPos(NULL, 0, 0, m_rcVerticalLarge.Width(),
			m_rcVerticalLarge.Height(), SWP_NOMOVE | SWP_NOZORDER);
		EnableVisibleChildren(TRUE, m_rcVerticalSmall.Height());
	} else {
		m_pDialog->SetWindowPos(NULL, 0, 0, m_rcVerticalSmall.Width(),
			m_rcVerticalSmall.Height(), SWP_NOMOVE | SWP_NOZORDER);
		EnableVisibleChildren(TRUE, m_rcVerticalSmall.Height());
	}
}

void CDialogExpand::HorizontalExpand(BOOL bExpand)
{
	if (bExpand) {
		m_pDialog->SetWindowPos(NULL, 0, 0, m_rcHorizontalLarge.Width(),
			m_rcHorizontalLarge.Height(), SWP_NOMOVE | SWP_NOZORDER);
		EnableVisibleChildren(FALSE, m_rcHorizontalSmall.Width());
	} else {
		m_pDialog->SetWindowPos(NULL, 0, 0, m_rcHorizontalSmall.Width(),
			m_rcHorizontalSmall.Height(), SWP_NOMOVE | SWP_NOZORDER);
		EnableVisibleChildren(FALSE, m_rcHorizontalSmall.Width());
	}
}

void CDialogExpand::EnableVisibleChildren(BOOL bVertical, int len)
{
	CWnd *pWndCtl = m_pDialog->GetWindow(GW_CHILD);
	CRect rcTest;
	CRect rcControl;
	CRect rcShow;
	CRect rcSmall;

	m_pDialog->GetWindowRect(rcShow);
	rcSmall = rcShow;

	if (bVertical) {
		if (rcSmall.Height() > len)
			rcSmall.bottom -= rcSmall.Height() - len;
	} else {
		if (rcSmall.Width() > len)
			rcSmall.right -= rcSmall.Width() - len;
	}

	while (pWndCtl != NULL) {
		pWndCtl->GetWindowRect(rcControl);

		if (!rcTest.IntersectRect(rcSmall, rcControl)) {
			if (rcTest.IntersectRect(rcShow, rcControl))
				pWndCtl->EnableWindow(TRUE);
			else
				pWndCtl->EnableWindow(FALSE);
		}

		pWndCtl = pWndCtl->GetWindow(GW_HWNDNEXT);
	}
}
