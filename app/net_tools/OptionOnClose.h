#pragma once
#include "afxwin.h"


// COptionOnClose 对话框

class COptionOnClose : public CDialog
{
	DECLARE_DYNCREATE(COptionOnClose)

public:
	COptionOnClose(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~COptionOnClose();

	void init(BOOL QuitOnClose);

// 对话框数据
	enum { IDD = IDD_DIALOG_QUIT};

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

public:
	BOOL m_QuitClose;
	BOOL m_MinOnClose;
	BOOL m_SaveOption;
};
