#pragma once


// CMeterCtrl

class CMeterCtrl : public CWnd
{
	DECLARE_DYNAMIC(CMeterCtrl)

public:
	CMeterCtrl();
	virtual ~CMeterCtrl();

protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	//virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL);
	virtual BOOL Create(LPCTSTR lpszWindowName, DWORD dwStyle,
		const RECT& rect, CWnd* pParentWnd, HMENU nID);
	virtual BOOL CreateEx(DWORD dwExStyle, LPCTSTR lpszClassName,
		LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect,
		CWnd* pParentWnd, UINT nID, LPVOID lpParam = NULL);
	UINT SetPos(UINT nPos);
	void SetRange(UINT nLower, UINT nUpper);
	void StepIt(void);
	void SetText(CString& msg);
	void SetText(const char* pMsg);
protected:
	void InvalidateMeater(void);
	UINT m_nLower;
	UINT m_nUpper;
	UINT m_nPos;
	CString m_sCaption;
};


