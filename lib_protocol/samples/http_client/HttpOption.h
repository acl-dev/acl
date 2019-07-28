#pragma once
#include "afxwin.h"


// CHttpOption 对话框

class CHttpOption : public CDialog
{
	DECLARE_DYNAMIC(CHttpOption)

public:
	CHttpOption(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CHttpOption();

// 对话框数据
	enum { IDD = IDD_DIALOG_OPTION };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedCheckProto();
	afx_msg void OnBnClickedCheckZip();
	afx_msg void OnBnClickedCheckKeepAlive();
	BOOL m_bHttp11;
	BOOL m_bZip;
	BOOL m_bKeepAlive;
	BOOL m_bPostMethod;
	virtual BOOL OnInitDialog();
	CString m_sHttpHdrAppend;
	CString m_sHttpBody;
	CString m_sServerAddr;
	CString m_sCType;
	CString m_sAccept;
	BOOL m_bUseAddr;
	BOOL m_bForwardAuto;
	UINT m_nMaxTry;
	afx_msg void OnBnClickedPostMethod();
	afx_msg void OnBnClickedOk();
};
