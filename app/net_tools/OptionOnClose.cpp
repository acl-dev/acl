// OptionOnClose.cpp : 实现文件
//

#include "stdafx.h"
#include "net_tools.h"
#include "OptionOnClose.h"


// COptionOnClose 对话框

IMPLEMENT_DYNCREATE(COptionOnClose, CDialog)

COptionOnClose::COptionOnClose(CWnd* pParent /*=NULL*/)
:  CDialog(COptionOnClose::IDD, pParent)
, m_MinOnClose(TRUE)
, m_QuitClose(!m_MinOnClose)
, m_SaveOption(FALSE)
{
}

COptionOnClose::~COptionOnClose()
{
}

void COptionOnClose::init(BOOL QuitOnClose)
{
	m_QuitClose = QuitOnClose;
	m_MinOnClose = !QuitOnClose;
	//UpdateData(FALSE);
}

void COptionOnClose::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_QUIT_NO, m_MinOnClose);
	DDX_Check(pDX, IDC_QUIT_YES, m_QuitClose);
	DDX_Check(pDX, IDC_CHECK_SAVE, m_SaveOption);
}

BOOL COptionOnClose::OnInitDialog()
{
	CDialog::OnInitDialog();
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

BEGIN_MESSAGE_MAP(COptionOnClose, CDialog)
END_MESSAGE_MAP()


