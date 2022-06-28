
// WinEchodDlg.h : 头文件
//

#pragma once

// CWinEchodDlg 对话框
class CWinEchodDlg : public CDialogEx
{
// 构造
public:
	CWinEchodDlg(CWnd* pParent = NULL);	// 标准构造函数
	~CWinEchodDlg();

// 对话框数据
	enum { IDD = IDD_WINECHOD_DIALOG };

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
public:
	afx_msg void OnBnClickedOpenDos();
private:
	FILE* m_dosFp;
	UINT m_listenPort;
	CString m_listenIP;
	acl::string m_listenAddr;
	acl::server_socket m_listen;
	acl::fiber* m_fiberListen;
public:
	afx_msg void OnBnClickedListen();
	afx_msg void OnBnClickedCreateTimer();
	afx_msg void OnBnClickedConnect();
private:
	UINT m_cocurrent;
	UINT m_count;
	void Uni2Str(const CString& in, acl::string& out);
	void InitFiber();

public:
	void OnFiberConnectExit(void);
	afx_msg void OnBnClickedOk();
};
