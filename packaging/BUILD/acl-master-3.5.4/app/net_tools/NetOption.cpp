// NetOption.cpp : 实现文件
//

#include "stdafx.h"
#include "net_tools.h"
#include "NetOption.h"
#include ".\netoption.h"


// CNetOption 对话框

IMPLEMENT_DYNAMIC(CNetOption, CDialog)

CNetOption::CNetOption(CWnd* pParent /*=NULL*/)
	: CDialog(CNetOption::IDD, pParent)
	, m_smtpPort(25)
	, m_pop3Port(110)
{

}

CNetOption::~CNetOption()
{
}

void CNetOption::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_SMTP_ADDR, m_smtpAddr);
	DDX_Text(pDX, IDC_SMTP_PORT, m_smtpPort);
	DDX_Text(pDX, IDC_POP3_ADDR, m_pop3Addr);
	DDX_Text(pDX, IDC_POP3_PORT, m_pop3Port);
	DDX_Text(pDX, IDC_USER_ACCOUNT, m_userAccount);
	DDX_Text(pDX, IDC_USER_PASSWD, m_userPasswd);
	DDX_Text(pDX, IDC_RECIPIENTS, m_recipients);
}

BOOL CNetOption::OnInitDialog()
{
	if (m_smtpAddr.IsEmpty())
		GetDlgItem(IDC_STATIC_SMTP_ADDR)->SetWindowText("X");
	else
		GetDlgItem(IDC_STATIC_SMTP_ADDR)->SetWindowText("√");
	if (m_pop3Addr.IsEmpty())
		GetDlgItem(IDC_STATIC_POP3_ADDR)->SetWindowText("X");
	else
		GetDlgItem(IDC_STATIC_POP3_ADDR)->SetWindowText("√");
	if (m_userAccount.IsEmpty())
		GetDlgItem(IDC_STATIC_USER)->SetWindowText("X");
	else
		GetDlgItem(IDC_STATIC_USER)->SetWindowText("√");
	if (m_userPasswd.IsEmpty())
		GetDlgItem(IDC_STATIC_PASS)->SetWindowText("X");
	else
		GetDlgItem(IDC_STATIC_PASS)->SetWindowText("√");
	if (m_recipients.IsEmpty())
		GetDlgItem(IDC_STATIC_RECIPIENTS)->SetWindowText("X");
	else
		GetDlgItem(IDC_STATIC_RECIPIENTS)->SetWindowText("√");

	if (m_smtpAddr.IsEmpty())
		GetDlgItem(IDC_SMTP_ADDR)->SetFocus();
	else if (m_pop3Addr.IsEmpty())
		GetDlgItem(IDC_POP3_ADDR)->SetFocus();
	else if (m_userAccount.IsEmpty())
		GetDlgItem(IDC_USER_ACCOUNT)->SetFocus();
	else if (m_userPasswd.IsEmpty())
		GetDlgItem(IDC_USER_PASSWD)->SetFocus();
	else if (m_recipients.IsEmpty())
		GetDlgItem(IDC_RECIPIENTS)->SetFocus();

	return FALSE;
}

BEGIN_MESSAGE_MAP(CNetOption, CDialog)
	ON_WM_PAINT()
	ON_WM_CREATE()
	ON_EN_KILLFOCUS(IDC_USER_ACCOUNT, OnEnKillfocusUserAccount)
	ON_EN_KILLFOCUS(IDC_SMTP_ADDR, OnEnKillfocusSmtpAddr)
	ON_EN_KILLFOCUS(IDC_POP3_ADDR, OnEnKillfocusPop3Addr)
	ON_EN_KILLFOCUS(IDC_USER_PASSWD, OnEnKillfocusUserPasswd)
	ON_EN_KILLFOCUS(IDC_RECIPIENTS, OnEnKillfocusRecipients)
	ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
END_MESSAGE_MAP()

CNetOption& CNetOption::SetSmtpAddr(const char* addr, int port)
{
	m_smtpAddr = addr;
	m_smtpPort = port;
	return *this;
}

CNetOption& CNetOption::SetPop3Addr(const char* addr, int port)
{
	m_pop3Addr = addr;
	m_pop3Port = port;
	return *this;
}

CNetOption& CNetOption::SetUserAccount(const char* s)
{
	m_userAccount = s;
	return *this;
}

CNetOption& CNetOption::SetUserPasswd(const char* s)
{
	m_userPasswd = s;
	return *this;
}

CNetOption& CNetOption::SetRecipients(const char* s)
{
	if (s == NULL || *s == 0)
		return *this;

	ACL_ARGV* tokens = acl_argv_split(s, ",;，； \t\r\n");
	ACL_ITER iter;
	acl::string buf;
	acl_foreach(iter, tokens)
	{
		if (iter.i > 0)
			buf << ";\r\n";
		buf << (char*) iter.data;
	}
	acl_argv_free(tokens);
	m_recipients = buf.c_str();
	return *this;
}

void CNetOption::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	// TODO: 在此处添加消息处理程序代码
	// 不为绘图消息调用 CDialog::OnPaint()
	UpdateData(FALSE);
}

int CNetOption::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDialog::OnCreate(lpCreateStruct) == -1)
		return -1;

	// TODO:  在此添加您专用的创建代码

	return 0;
}

void CNetOption::OnEnKillfocusSmtpAddr()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);
	if (m_smtpAddr.IsEmpty())
	{
		GetDlgItem(IDC_STATIC_SMTP_ADDR)->SetWindowText("X");
		GetDlgItem(IDC_SMTP_ADDR)->SetFocus();
	}
	else
		GetDlgItem(IDC_STATIC_SMTP_ADDR)->SetWindowText("√");
}

void CNetOption::OnEnKillfocusPop3Addr()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);
	if (m_pop3Addr.IsEmpty())
	{
		GetDlgItem(IDC_STATIC_POP3_ADDR)->SetWindowText("X");
		GetDlgItem(IDC_POP3_ADDR)->SetFocus();
	}
	else
		GetDlgItem(IDC_STATIC_POP3_ADDR)->SetWindowText("√");
}

void CNetOption::OnEnKillfocusUserAccount()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);
	if (m_userAccount.IsEmpty())
	{
		GetDlgItem(IDC_STATIC_USER)->SetWindowText("X");
		GetDlgItem(IDC_USER_ACCOUNT)->SetFocus();
	}
	else
		GetDlgItem(IDC_STATIC_USER)->SetWindowText("√");
}

void CNetOption::OnEnKillfocusUserPasswd()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);
	if (m_userPasswd.IsEmpty())
	{
		GetDlgItem(IDC_STATIC_PASS)->SetWindowText("X");
		GetDlgItem(IDC_USER_PASSWD)->SetFocus();
	}
	else
		GetDlgItem(IDC_STATIC_PASS)->SetWindowText("√");
}

void CNetOption::OnEnKillfocusRecipients()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);
	if (m_userAccount.IsEmpty())
	{
		GetDlgItem(IDC_STATIC_USER)->SetWindowText("X");
		GetDlgItem(IDC_RECIPIENTS)->SetFocus();
	}
	else
		GetDlgItem(IDC_STATIC_USER)->SetWindowText("√");
}

void CNetOption::OnBnClickedCancel()
{
	// TODO: 在此添加控件通知处理程序代码
	OnCancel();
}

void CNetOption::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	OnOK();
}
