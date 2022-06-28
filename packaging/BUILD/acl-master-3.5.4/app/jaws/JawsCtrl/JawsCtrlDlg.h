// JawsCtrlDlg.h : 头文件
//

#pragma once
#include "Trayicon.h"
#include "ProcCtrl.h"
#include "HttpService.h"
#include "DialogExpand.h"
#include "RegRun.h"
#include "afxcmn.h"

// CJawsCtrlDlg 对话框
class CJawsCtrlDlg : public CDialog
{
// 构造
public:
	CJawsCtrlDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_JAWSCTRL_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;
	CTrayIcon m_trayIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg LRESULT OnTrayNotification(WPARAM uID, LPARAM lEvent);
	DECLARE_MESSAGE_MAP()
public:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnOpenMain();
	afx_msg void OnQuit();
	afx_msg void OnNcPaint();
	afx_msg void OnClose();
private:
	BOOL m_bShutdown;
public:
	afx_msg void OnDestroy();
private:
	CIPAddressCtrl m_listenIpCtrl;
	CString m_listenIp;
	long m_listenPort;

	CProcCtrl m_procCtrl;
public:
	afx_msg void OnBnClickedButtonStart();
	afx_msg void OnBnClickedButtonStop();
	afx_msg void OnBnClickedButtonQuit();
private:
	CString m_sJawsName;
	CString m_sJawsCtrlName;
	CString m_httpVhostPath;
	CString m_httpVhostDefault;
	CString m_httpTmplPath;
private:
	CString m_httpFilter;
	UINT m_nHttpFilter;

	CHttpService *m_pService;
public:
	afx_msg void OnBnClickedMore();
private:
	CDialogExpand m_dVerticalExpand;
	CDialogExpand m_dHorizontalExpand;
	void ExpandDialog(BOOL bExpand);

	CRegRun m_regRun;
public:
	afx_msg void OnBnClickedAutoRun();
};
