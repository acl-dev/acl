
// WinFiberDlg.h: 头文件
//

#pragma once


// CWinFiberDlg 对话框
class CWinFiberDlg : public CDialogEx
{
// 构造
public:
	CWinFiberDlg(CWnd* pParent = nullptr);	// 标准构造函数
	~CWinFiberDlg(void);

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_WINFIBER_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持

// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

private:
	FILE* m_dosFp;
	UINT  m_listenPort;
	CString m_listenIP;
	acl::string m_listenAddr;
	acl::server_socket m_listen;
	acl::fiber* m_fiberListen;
	acl::string m_httpdAddr;

	std::set<acl::fiber*> m_clientFibers;
	UINT m_cocurrent;
	UINT m_count;
	void Uni2Str(const CString& in, acl::string& out);
	void InitFiber();
	void StopFiber();

public:
	void OnFiberConnectExit(acl::fiber* fb);

public:
	afx_msg void OnBnClickedOpenDos();
	afx_msg void OnBnClickedListen();
	afx_msg void OnBnClickedConnect();
	afx_msg void OnBnClickedCreateTimer();
	afx_msg void OnBnClickedStartHttpd();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedAwaitDns();
	afx_msg void OnBnClickedResolve();
};
