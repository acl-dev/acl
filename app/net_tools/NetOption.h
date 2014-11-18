#pragma once


// CNetOption 对话框

class CNetOption : public CDialog
{
	DECLARE_DYNAMIC(CNetOption)

public:
	CNetOption(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CNetOption();

// 对话框数据
	enum { IDD = IDD_OPTION };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

public:
	CNetOption& SetUserPasswd(const char* addr);
	CNetOption& SetSmtpAddr(const char* addr, int port);
	CNetOption& SetPop3Addr(const char* addr, int port);
	CNetOption& SetUserAccount(const char* s);
	CNetOption& SetRecipients(const char* s);

	const CString& GetSmtpAddr() const
	{
		return m_smtpAddr;
	}
	int getSmtpPort() const
	{
		return m_smtpPort;
	}
	const CString& GetPop3Addr() const
	{
		return m_pop3Addr;
	}
	int getPop3Port() const
	{
		return m_pop3Port;
	}
	const CString& GetUserAccount() const
	{
		return m_userAccount;
	}
	const CString& GetUserPasswd() const
	{
		return m_userPasswd;
	}
	const CString& GetRecipients() const
	{
		return m_recipients;
	}
//private:
	CString m_smtpAddr;
	int   m_smtpPort;
	CString m_pop3Addr;
	int   m_pop3Port;
	CString m_userAccount;
	CString m_userPasswd;
	CString m_recipients;
public:
	afx_msg void OnPaint();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnEnKillfocusUserAccount();
	afx_msg void OnEnKillfocusSmtpAddr();
	afx_msg void OnEnKillfocusPop3Addr();
	afx_msg void OnEnKillfocusUserPasswd();
	afx_msg void OnEnKillfocusRecipients();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedOk();
};
