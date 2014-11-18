// net_toolsDlg.h : 头文件
//

#pragma once
#include "ui/MeterBar.h"
#include "ui/TrayIcon.h"
#include "ping/ping.h"
#include "upload/upload.h"
#include "dns/nslookup.h"
#include "mail/smtp_client.h"
#include "mail/pop3_client.h"
#include "test_all.h"
#include "net_store.h"

// Cnet_toolsDlg 对话框
class Cnet_toolsDlg : public CDialog
	, public ping_callback
	, public nslookup_callback
	, public upload_callback
	, public net_store_callback
	, public smtp_callback
	, public pop3_callback
	, public test_callback
{
// 构造
public:
	Cnet_toolsDlg(CWnd* pParent = NULL);	// 标准构造函数
	~Cnet_toolsDlg();
// 对话框数据
	enum { IDD = IDD_NET_TOOLS_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;
	CMeterBar m_wndMeterBar;
	CTrayIcon m_trayIcon;
	BOOL m_bShutdown;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedLoadIp();
	afx_msg void OnBnClickedPing();
	afx_msg void OnBnClickedLoadDomain();
	afx_msg void OnBnClickedNslookup();

private:
	FILE* m_dosFp;

	// ping 相关参数
	UINT m_nPkt;
	UINT m_delay;
	UINT m_pingTimeout;
	UINT m_pktSize;
	BOOL m_pingBusy;
	CString m_ipFilePath;
	CString m_pingDbPath;

	// dns 相关参数
	CString m_dnsIp;
	UINT m_dnsPort;
	UINT m_lookupTimeout;
	BOOL m_dnsBusy;
	CString m_domainFilePath;
	CString m_dnsDbPath;

	// 上传日志相关参数
	CString m_smtpAddr;
	int m_smtpPort;
	int m_connecTimeout;
	int m_rwTimeout;
	CString m_smtpUser;
	CString m_smtpPass;
	CString m_recipients;
	CString m_attachFilePath;

	CString m_pop3Addr;
	int m_pop3Port;
	UINT m_recvLimit;
	BOOL m_recvAll;
	BOOL m_recvSave;

protected:

	virtual void ping_report(size_t total, size_t curr, size_t nerror);
	virtual void ping_finish(const char* dbpath);

	virtual void nslookup_report(size_t total, size_t curr);
	virtual void nslookup_finish(const char* dbpath);

	virtual void smtp_report(const char* msg, size_t total,
		size_t curr, const SMTP_METER& meter);
	virtual void smtp_finish(const char* dbpath);

	virtual void pop3_report(const char* msg, size_t total,
		size_t curr, const POP3_METER& meter);
	virtual void pop3_finish(const char* dbpath);

	virtual void test_report(const char* msg, unsigned nstep);
	virtual void test_store(const char* dbpath);
	virtual void test_finish();

	virtual void upload_report(const char* msg, size_t total,
		size_t curr, const UPLOAD_METER& meter);

	virtual void load_db_callback(const char* smtp_addr, int smtp_port,
		const char* pop3_addr, int pop3_port,
		const char* user, const char* pass,
		const char* recipients, bool store);
private:
	std::vector<acl::string> attaches_;
public:
	afx_msg void OnBnClickedOpenDos();
	afx_msg void OnBnClickedOption();
	afx_msg void OnBnClickedTestall();
	afx_msg void OnOpenMain();
	afx_msg void OnQuit();
	afx_msg void OnClose();
	afx_msg void OnNcPaint();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg LRESULT OnTrayNotification(WPARAM uID, LPARAM lEvent);
	afx_msg void OnBnClickedLoadFile();
	afx_msg void OnBnClickedSendMail();
	afx_msg void OnBnClickedRecvMail();
	afx_msg void OnEnSetfocusIpFilePath();
	afx_msg void OnEnSetfocusDomainFile();
	afx_msg void OnEnSetfocusFile();
	afx_msg void OnBnClickedRecvAll();
private:
	void check();
public:
	afx_msg void OnDestroy();
	afx_msg void OnEnKillfocusIpFilePath();
	afx_msg void OnEnKillfocusNpkt();
	afx_msg void OnEnKillfocusDelay();
	afx_msg void OnEnKillfocusTimeout();
	afx_msg void OnEnKillfocusPktSize();
	afx_msg void OnEnKillfocusDomainFile();
	afx_msg void OnEnKillfocusDnsPort();
	afx_msg void OnEnKillfocusLookupTimeout();
	afx_msg void OnEnKillfocusFile();
	afx_msg void OnEnKillfocusRecvLimit();
	afx_msg void OnBnKillfocusRecvAll();
	afx_msg void OnBnClickedRecvSave();
};
