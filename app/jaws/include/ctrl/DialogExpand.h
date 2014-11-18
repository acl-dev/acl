#pragma once

class AFX_EXT_CLASS CDialogExpand
{
public:
	CDialogExpand(void);
	CDialogExpand(CDialog *pDialog, int nResourceID, BOOL bVertical);
	~CDialogExpand(void);

	void Init(CDialog *pDialog, int nResourceID, BOOL bVertical);
	void Expand(BOOL bExpand);
private:
	BOOL m_bVertical;
	int m_nResourceID;
	CRect m_rcHorizontalLarge;
	CRect m_rcHorizontalSmall;
	CRect m_rcVerticalLarge;
	CRect m_rcVerticalSmall;
	CDialog *m_pDialog;

	void VerticalExpand(BOOL bExpand);
	void HorizontalExpand(BOOL bExpand);
	void EnableVisibleChildren(BOOL bVertical, int height);
};
