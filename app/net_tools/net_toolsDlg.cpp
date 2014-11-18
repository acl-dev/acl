// net_toolsDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "net_tools.h"
#include "ping/ping.h"
#include "dns/nslookup.h"
#include "upload/upload.h"
#include "rpc/rpc_manager.h"
#include "ui/TrayIcon.h"
#include "NetOption.h"
#include "net_store.h"
#include "global/util.h"
#include "OptionOnClose.h"
#include "net_toolsDlg.h"
#include ".\net_toolsdlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// Cnet_toolsDlg 对话框



Cnet_toolsDlg::Cnet_toolsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(Cnet_toolsDlg::IDD, pParent)
	, m_nPkt(100)
	, m_delay(1)
	, m_pingTimeout(5)
	, m_pingBusy(FALSE)
	, m_dosFp(NULL)
	, m_dnsIp("8.8.8.8")
	, m_dnsPort(53)
	, m_lookupTimeout(10)
	, m_pktSize(64)
	, m_dnsBusy(FALSE)
	, m_smtpAddr("smtpcom.263xmail.com")
	, m_smtpPort(25)
	, m_connecTimeout(60)
	, m_rwTimeout(60)
	, m_pop3Addr("popcom.263xmail.com")
	, m_pop3Port(110)
	, m_recvLimit(1)
	, m_recvAll(FALSE)
	, m_recvSave(FALSE)
	, m_smtpUser("")
	, m_smtpPass("")
	, m_recipients("wang.li@net263.com;shuxin.zheng@net263.com;jian.shao@net263.com")
	, m_trayIcon(IDR_MENU_ICON)
	, m_bShutdown(FALSE)
	, m_ipFilePath("263ip.txt")
	, m_domainFilePath("263domain.txt")
	, m_attachFilePath("ReadMe.txt")
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	acl_netdb_cache_init(0, 1);
}

Cnet_toolsDlg::~Cnet_toolsDlg()
{
	if (m_dosFp)
	{
		fclose(m_dosFp);
		FreeConsole();
	}
}

void Cnet_toolsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_NPKT, m_nPkt);
	DDX_Text(pDX, IDC_DELAY, m_delay);
	DDX_Text(pDX, IDC_TIMEOUT, m_pingTimeout);
	DDX_Text(pDX, IDC_DNS_IP, m_dnsIp);
	DDX_Text(pDX, IDC_DNS_PORT, m_dnsPort);
	DDX_Text(pDX, IDC_LOOKUP_TIMEOUT, m_lookupTimeout);
	DDX_Text(pDX, IDC_PKT_SIZE, m_pktSize);
	DDX_Text(pDX, IDC_RECV_LIMIT, m_recvLimit);
	DDX_Check(pDX, IDC_RECV_ALL, m_recvAll);
	DDX_Check(pDX, IDC_RECV_SAVE, m_recvSave);
	DDX_Text(pDX, IDC_IP_FILE_PATH, m_ipFilePath);
	DDX_Text(pDX, IDC_DOMAIN_FILE, m_domainFilePath);
	DDX_Text(pDX, IDC_FILE, m_attachFilePath);
}

BEGIN_MESSAGE_MAP(Cnet_toolsDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_LOAD_IP, OnBnClickedLoadIp)
	ON_BN_CLICKED(IDC_PING, OnBnClickedPing)
	ON_BN_CLICKED(IDC_LOAD_DOMAIN, OnBnClickedLoadDomain)
	ON_BN_CLICKED(IDC_NSLOOKUP, OnBnClickedNslookup)
	ON_BN_CLICKED(IDC_OPEN_DOS, OnBnClickedOpenDos)
	ON_BN_CLICKED(IDC_OPTION, OnBnClickedOption)
	ON_BN_CLICKED(IDC_TESTALL, OnBnClickedTestall)
	ON_COMMAND(ID_OPEN_MAIN, OnOpenMain)
	ON_COMMAND(ID_QUIT, OnQuit)
	ON_WM_CLOSE()
	ON_WM_NCPAINT()
	ON_MESSAGE(WM_MY_TRAY_NOTIFICATION, OnTrayNotification)
	ON_WM_CREATE()
	ON_BN_CLICKED(IDC_LOAD_FILE, OnBnClickedLoadFile)
	ON_BN_CLICKED(IDC_SEND_MAIL, OnBnClickedSendMail)
	ON_BN_CLICKED(IDC_RECV_MAIL, OnBnClickedRecvMail)
	ON_EN_SETFOCUS(IDC_IP_FILE_PATH, OnEnSetfocusIpFilePath)
	ON_EN_SETFOCUS(IDC_DOMAIN_FILE, OnEnSetfocusDomainFile)
	ON_EN_SETFOCUS(IDC_FILE, OnEnSetfocusFile)
	ON_BN_CLICKED(IDC_RECV_ALL, OnBnClickedRecvAll)
	ON_WM_DESTROY()
	ON_EN_KILLFOCUS(IDC_IP_FILE_PATH, OnEnKillfocusIpFilePath)
	ON_EN_KILLFOCUS(IDC_NPKT, OnEnKillfocusNpkt)
	ON_EN_KILLFOCUS(IDC_DELAY, OnEnKillfocusDelay)
	ON_EN_KILLFOCUS(IDC_TIMEOUT, OnEnKillfocusTimeout)
	ON_EN_KILLFOCUS(IDC_PKT_SIZE, OnEnKillfocusPktSize)
	ON_EN_KILLFOCUS(IDC_DOMAIN_FILE, OnEnKillfocusDomainFile)
	ON_EN_KILLFOCUS(IDC_DNS_PORT, OnEnKillfocusDnsPort)
	ON_EN_KILLFOCUS(IDC_LOOKUP_TIMEOUT, OnEnKillfocusLookupTimeout)
	ON_EN_KILLFOCUS(IDC_FILE, OnEnKillfocusFile)
	ON_EN_KILLFOCUS(IDC_RECV_LIMIT, OnEnKillfocusRecvLimit)
	ON_BN_KILLFOCUS(IDC_RECV_ALL, OnBnKillfocusRecvAll)
	ON_BN_CLICKED(IDC_RECV_SAVE, OnBnClickedRecvSave)
END_MESSAGE_MAP()


// Cnet_toolsDlg 消息处理程序

BOOL Cnet_toolsDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 将\“关于...\”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	//ShowWindow(SW_MAXIMIZE);

	// TODO: 在此添加额外的初始化代码

	theApp.m_singleCtrl.Register();

	// 添加状态栏
	int aWidths[3] = {50, 400, -1};
	m_wndMeterBar.SetParts(3, aWidths);

	m_wndMeterBar.Create(WS_CHILD | WS_VISIBLE | WS_BORDER
		| CCS_BOTTOM | SBARS_SIZEGRIP,
		CRect(0,0,0,0), this, 0); 
	m_wndMeterBar.SetText("就绪", 0, 0);
	m_wndMeterBar.SetText("", 1, 0);
	m_wndMeterBar.SetText("", 2, 0);

	// 取得本机的DNS服务器
	std::vector<acl::string> dns_list;
	if (util::get_dns(dns_list) > 0)
	{
		m_dnsIp = dns_list[0];
		UpdateData(FALSE);
	}

	const char* path = acl_getcwd();
	acl::string logpath;
	logpath.format("%s/net_tools.txt", path);
	logger_open(logpath.c_str(), "net_tools");

	// 从数据库中读取配置项
	net_store* ns = new net_store(m_smtpAddr, m_smtpPort, m_pop3Addr, m_pop3Port,
		m_smtpUser, m_smtpPass, m_recipients, this);
	rpc_manager::get_instance().fork(ns);

	//DisableAll();
	if (m_ipFilePath.IsEmpty())
		GetDlgItem(IDC_PING)->EnableWindow(FALSE);
	if (m_domainFilePath.IsEmpty())
		GetDlgItem(IDC_NSLOOKUP)->EnableWindow(FALSE);

	return TRUE;  // 除非设置了控件的焦点，否则返回 TRUE
}

void Cnet_toolsDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void Cnet_toolsDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标显示。
HCURSOR Cnet_toolsDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void Cnet_toolsDlg::OnBnClickedOpenDos()
{
	// TODO: 在此添加控件通知处理程序代码
	if (m_dosFp == NULL)
	{
		//GetDlgItem(IDC_OPEN_DOS)->EnableWindow(FALSE);
		AllocConsole();
		m_dosFp = freopen("CONOUT$","w+t",stdout);
		printf("DOS opened now!\r\n");
		const char* path = acl_getcwd();
		printf("current path: %s\r\n", path);
		GetDlgItem(IDC_OPEN_DOS)->SetWindowText("关闭 DOS 窗口");
		acl::log::stdout_open(true);
		logger_close();
	}
	else
	{
		fclose(m_dosFp);
		m_dosFp = NULL;
		FreeConsole();
		GetDlgItem(IDC_OPEN_DOS)->SetWindowText("打开 DOS 窗口");
		acl::log::stdout_open(false);
		const char* path = acl_getcwd();
		acl::string logpath;
		logpath.format("%s/net_tools.txt", path);
		printf("current path: %s\r\n", path);
		logger_open(logpath.c_str(), "net_tools");
	}
}

void Cnet_toolsDlg::upload_report(const char* msg, size_t total,
	size_t curr, const UPLOAD_METER& meter)
{
	if (total > 0)
	{
		int  nStept;

		nStept = (int) ((curr * 100) / total);
		m_wndMeterBar.GetProgressCtrl().SetPos(nStept);
	}

	m_wndMeterBar.SetText(msg, 1, 0);
}

void Cnet_toolsDlg::load_db_callback(const char* smtp_addr, int smtp_port,
	const char* pop3_addr, int pop3_port, const char* user,
	const char* pass, const char* recipients, bool store)
{
	if (smtp_addr && *smtp_addr)
		m_smtpAddr = smtp_addr;
	m_smtpPort = smtp_port;
	if (pop3_addr && *pop3_addr)
		m_pop3Addr = pop3_addr;
	m_pop3Port = pop3_port;
	if (user && *user)
		m_smtpUser = user;
	if (pass && *pass)
		m_smtpPass = pass;
	if (recipients && *recipients)
		m_recipients = recipients;

	check();

	// 如果有一个必填的配置项非空，则强制用户填写
	if (m_smtpAddr.IsEmpty() || m_pop3Addr.IsEmpty()
		|| m_smtpUser.IsEmpty() || m_smtpPass.IsEmpty()
		|| m_recipients.IsEmpty())
	{
		OnBnClickedOption();
		return;
	}

	if (store == false)
		UpdateData(FALSE);
}

void Cnet_toolsDlg::OnBnClickedOption()
{
	// TODO: 在此添加控件通知处理程序代码
	//CNetOption option(m_smtpAddr, m_pop3Addr, m_smtpUser, m_smtpPass,
	//	m_recipients);
	CNetOption option;
	option.SetSmtpAddr(m_smtpAddr, m_smtpPort)
		.SetPop3Addr(m_pop3Addr, m_pop3Port)
		.SetUserAccount(m_smtpUser)
		.SetUserPasswd(m_smtpPass)
		.SetRecipients(m_recipients);
	if (option.DoModal() == IDOK)
	{
		m_smtpAddr = option.GetSmtpAddr();
		m_pop3Addr = option.GetPop3Addr();
		m_smtpPort = option.getSmtpPort();
		m_pop3Port = option.getPop3Port();
		m_smtpUser = option.GetUserAccount();
		m_smtpPass = option.GetUserPasswd();
		CString tmp = option.GetRecipients();
		ACL_ARGV* tokens = acl_argv_split(tmp.GetString(), " \t,;\r\n");
		ACL_ITER iter;
		acl::string buf;
		acl_foreach(iter, tokens)
		{
			if (iter.i > 0)
				buf << ",";
			buf << (char*) iter.data;
		}
		acl_argv_free(tokens);
		m_recipients = buf.c_str();

		net_store* ns = new net_store(m_smtpAddr.GetString(), m_smtpPort,
			m_pop3Addr.GetString(), m_pop3Port,
			m_smtpUser.GetString(), m_smtpPass.GetString(),
			m_recipients.GetString(), this, true);
		rpc_manager::get_instance().fork(ns);
		check();
	}
	else
	{
		check();
	}
}

void Cnet_toolsDlg::OnOpenMain()
{
	// TODO: 在此添加命令处理程序代码
	ShowWindow(SW_NORMAL);
}

void Cnet_toolsDlg::OnQuit()
{
	// TODO: 在此添加命令处理程序代码
	m_bShutdown = TRUE;
	SendMessage(WM_CLOSE);
}

void Cnet_toolsDlg::OnClose()
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	//__super::OnClose();
	acl::string buf;

	net_store::get_key("QuitOnClose", buf);
	if (m_bShutdown || buf.compare("yes", false) == 0)
	{
		CDialog::OnClose();
		return;
	}
	else if (buf.compare("no", false) == 0)
	{
		ShowWindow(SW_HIDE);
		return;
	}

	COptionOnClose dlg;
	dlg.init(FALSE);
	if (dlg.DoModal() == IDOK)
	{
		BOOL quit = dlg.m_QuitClose;
		BOOL save = dlg.m_SaveOption;
		if (save)
			net_store::set_key("QuitOnClose", quit ? "yes" : "no");
		if (quit)
			CDialog::OnClose();
		else
			ShowWindow(SW_HIDE);
	}
}

void Cnet_toolsDlg::OnNcPaint()
{
	// TODO: 在此处添加消息处理程序代码
	// 不为绘图消息调用 __super::OnNcPaint()
	//static int i = 2;
	//if(i > 0)
	//{
	//	i --;
	//	ShowWindow(SW_HIDE);
	//} else
	//{
		CDialog::OnNcPaint();
	//}
}

int Cnet_toolsDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (__super::OnCreate(lpCreateStruct) == -1)
		return -1;

	// TODO:  在此添加您专用的创建代码

	m_trayIcon.SetNotificationWnd(this, WM_MY_TRAY_NOTIFICATION);
	m_trayIcon.SetIcon(IDI_ICON_MIN);

	return 0;
}

afx_msg LRESULT Cnet_toolsDlg::OnTrayNotification(WPARAM uID, LPARAM lEvent)
{
	// let tray icon do default stuff
	return m_trayIcon.OnTrayNotification(uID, lEvent);
}

//////////////////////////////////////////////////////////////////////////

void Cnet_toolsDlg::OnEnSetfocusDomainFile()
{
	// TODO: 在此添加控件通知处理程序代码
	CString pathname;

	GetDlgItem(IDC_DOMAIN_FILE)->GetWindowText(pathname);
	if (pathname.IsEmpty())
	{
		GetDlgItem(IDC_LOAD_DOMAIN)->SetFocus();
		OnBnClickedLoadDomain();
	}
	check();
}

void Cnet_toolsDlg::OnBnClickedLoadDomain()
{
	// TODO: 在此添加控件通知处理程序代码
	CFileDialog file(TRUE,"文件","",OFN_HIDEREADONLY,"FILE(*.*)|*.*||",NULL);
	if(file.DoModal()==IDOK)
	{
		CString pathname;

		pathname=file.GetPathName();
		GetDlgItem(IDC_DOMAIN_FILE)->SetWindowText(pathname);
		GetDlgItem(IDC_NSLOOKUP)->EnableWindow(TRUE);
		check();
	}
}

void Cnet_toolsDlg::OnBnClickedNslookup()
{
	// TODO: 在此添加控件通知处理程序代码

	if (m_dnsBusy)
		return;

	UpdateData();

	GetDlgItem(IDC_NSLOOKUP)->EnableWindow(FALSE);

	CString filePath;
	GetDlgItem(IDC_DOMAIN_FILE)->GetWindowText(filePath);
	if (filePath.IsEmpty())
	{
		MessageBox("请先选择域名列表配置文件！");
		return;
	}

	m_dnsBusy = TRUE;

	GetDlgItem(IDC_LOAD_DOMAIN)->EnableWindow(FALSE);

	logger("dns_ip: %s, dns_port: %d, dns_timeout: %d",
		m_dnsIp.GetString(), m_dnsPort, m_lookupTimeout);

	nslookup* dns = new nslookup(filePath.GetString(), this,
		m_dnsIp.GetString(), m_dnsPort, m_lookupTimeout);
	rpc_manager::get_instance().fork(dns);
}

void Cnet_toolsDlg::nslookup_report(size_t total, size_t curr)
{
	if (total > 0)
	{
		int  nStept;

		nStept = (int) ((curr * 100) / total);
		m_wndMeterBar.GetProgressCtrl().SetPos(nStept);
	}

	CString msg;
	msg.Format("共 %d 个域名, 完成 %d 个域名", total, curr);
	m_wndMeterBar.SetText(msg, 1, 0);
}

void Cnet_toolsDlg::nslookup_finish(const char* dbpath)
{
	m_dnsBusy = FALSE;

	GetDlgItem(IDC_LOAD_DOMAIN)->EnableWindow(TRUE);
	CString filePath;
	GetDlgItem(IDC_DOMAIN_FILE)->GetWindowText(filePath);
	if (filePath.IsEmpty())
		GetDlgItem(IDC_NSLOOKUP)->EnableWindow(FALSE);
	else
		GetDlgItem(IDC_NSLOOKUP)->EnableWindow(TRUE);

	if (dbpath && *dbpath)
	{
		// 将数据库文件发邮件至服务器
		upload* up = new upload();
		(*up).set_callback(this)
			.add_file(dbpath)
			.set_server(m_smtpAddr.GetString(), m_smtpPort)
			.set_conn_timeout(m_connecTimeout)
			.set_rw_timeout(m_rwTimeout)
			.set_account(m_smtpUser.GetString())
			.set_passwd(m_smtpPass.GetString())
			.set_from(m_smtpUser.GetString())
			.set_subject("DNS 查询结果数据")
			.add_to(m_recipients.GetString());
		rpc_manager::get_instance().fork(up);
	}
}

//////////////////////////////////////////////////////////////////////////

void Cnet_toolsDlg::OnEnSetfocusIpFilePath()
{
	// TODO: 在此添加控件通知处理程序代码
	CString pathname;

	GetDlgItem(IDC_IP_FILE_PATH)->GetWindowText(pathname);
	if (pathname.IsEmpty())
	{
		GetDlgItem(IDC_LOAD_IP)->SetFocus();
		OnBnClickedLoadIp();
	}
	check();
}

void Cnet_toolsDlg::OnBnClickedLoadIp()
{
	// TODO: 在此添加控件通知处理程序代码
	CFileDialog file(TRUE,"文件","",OFN_HIDEREADONLY,"FILE(*.*)|*.*||",NULL);
	if(file.DoModal()==IDOK)
	{
		CString pathname;

		pathname=file.GetPathName();
		GetDlgItem(IDC_IP_FILE_PATH)->SetWindowText(pathname);
		GetDlgItem(IDC_PING)->EnableWindow(TRUE);
		check();
	}
}

void Cnet_toolsDlg::OnBnClickedPing()
{
	// TODO: 在此添加控件通知处理程序代码
	if (m_pingBusy)
		return;

	UpdateData();

	GetDlgItem(IDC_PING)->EnableWindow(FALSE);

	CString filePath;
	GetDlgItem(IDC_IP_FILE_PATH)->GetWindowText(filePath);
	if (filePath.IsEmpty())
	{
		MessageBox("请先选择 ip 列表配置文件！");

		return;
	}

	m_pingBusy = TRUE;

	GetDlgItem(IDC_LOAD_IP)->EnableWindow(FALSE);
	logger("npkt: %d, delay: %d, timeout: %d",
		m_nPkt, m_delay, m_pingTimeout);

	ping* p = new ping(filePath.GetString(), this,
		m_nPkt, m_delay, m_pingTimeout, m_pktSize);
	rpc_manager::get_instance().fork(p);
}

void Cnet_toolsDlg::ping_report(size_t total, size_t curr, size_t nerror)
{
	if (total > 0)
	{
		int  nStept;

		nStept = (int) ((curr * 100) / total);
		m_wndMeterBar.GetProgressCtrl().SetPos(nStept);
	}

	CString msg;
	msg.Format("%d/%d; failed: %d", curr, total, nerror);
	m_wndMeterBar.SetText(msg, 1, 0);
}

void Cnet_toolsDlg::ping_finish(const char* dbpath)
{
	m_pingBusy = FALSE;

	GetDlgItem(IDC_LOAD_IP)->EnableWindow(TRUE);
	CString filePath;
	GetDlgItem(IDC_IP_FILE_PATH)->GetWindowText(filePath);
	if (filePath.IsEmpty())
		GetDlgItem(IDC_PING)->EnableWindow(FALSE);
	else
		GetDlgItem(IDC_PING)->EnableWindow(TRUE);

	if (dbpath && *dbpath)
	{
		// 将数据库文件发邮件至服务器
		upload* up = new upload();
		(*up).set_callback(this)
			.add_file(dbpath)
			.set_server(m_smtpAddr.GetString(), m_smtpPort)
			.set_conn_timeout(m_connecTimeout)
			.set_rw_timeout(m_rwTimeout)
			.set_account(m_smtpUser.GetString())
			.set_passwd(m_smtpPass.GetString())
			.set_from(m_smtpUser.GetString())
			.set_subject("PING 结果数据")
			.add_to(m_recipients.GetString());
		rpc_manager::get_instance().fork(up);
	}
}

//////////////////////////////////////////////////////////////////////////

void Cnet_toolsDlg::OnEnSetfocusFile()
{
	// TODO: 在此添加控件通知处理程序代码
	CString pathname;

	GetDlgItem(IDC_FILE)->GetWindowText(pathname);
	if (pathname.IsEmpty())
	{
		GetDlgItem(IDC_LOAD_FILE)->SetFocus();
		OnBnClickedLoadFile();
	}
	check();
}

void Cnet_toolsDlg::OnBnClickedLoadFile()
{
	// TODO: 在此添加控件通知处理程序代码
	CFileDialog file(TRUE,"文件","",OFN_HIDEREADONLY,"FILE(*.*)|*.*||",NULL);
	if(file.DoModal()==IDOK)
	{
		CString pathname;

		pathname=file.GetPathName();
		GetDlgItem(IDC_FILE)->SetWindowText(pathname);
		GetDlgItem(IDC_SEND_MAIL)->EnableWindow(TRUE);
		check();
	}
}

void Cnet_toolsDlg::OnBnClickedSendMail()
{
	// TODO: 在此添加控件通知处理程序代码
	// 发邮件至服务器

	UpdateData();

	GetDlgItem(IDC_SEND_MAIL)->EnableWindow(FALSE);

	CString filePath;
	GetDlgItem(IDC_FILE)->GetWindowText(filePath);
	if (filePath.IsEmpty())
	{
		MessageBox("请先选择附件！");
		return;
	}

	smtp_client* smtp = new smtp_client();
	(*smtp).set_callback(this)
		.add_file(filePath.GetString())
		.set_smtp(m_smtpAddr, m_smtpPort)
		.set_conn_timeout(m_connecTimeout)
		.set_rw_timeout(m_rwTimeout)
		.set_account(m_smtpUser.GetString())
		.set_passwd(m_smtpPass.GetString())
		.set_from(m_smtpUser.GetString())
		.set_subject("测试邮件发送过程!")
		.add_to(m_recipients.GetString());
	rpc_manager::get_instance().fork(smtp);
}

void Cnet_toolsDlg::smtp_report(const char* msg, size_t total,
	size_t curr, const SMTP_METER&)
{
	if (total > 0)
	{
		int  nStept;

		nStept = (int) ((curr * 100) / total);
		m_wndMeterBar.GetProgressCtrl().SetPos(nStept);
	}

	m_wndMeterBar.SetText(msg, 1, 0);
}

void Cnet_toolsDlg::smtp_finish(const char* dbpath)
{
	GetDlgItem(IDC_LOAD_FILE)->EnableWindow(TRUE);
	CString filePath;
	GetDlgItem(IDC_FILE)->GetWindowText(filePath);
	if (filePath.IsEmpty())
		GetDlgItem(IDC_SEND_MAIL)->EnableWindow(FALSE);
	else
		GetDlgItem(IDC_SEND_MAIL)->EnableWindow(TRUE);

	if (dbpath && *dbpath)
	{
		// 将数据库文件发邮件至服务器
		upload* up = new upload();
		(*up).set_callback(this)
			.add_file(dbpath)
			.set_server(m_smtpAddr.GetString(), m_smtpPort)
			.set_conn_timeout(m_connecTimeout)
			.set_rw_timeout(m_rwTimeout)
			.set_account(m_smtpUser.GetString())
			.set_passwd(m_smtpPass.GetString())
			.set_from(m_smtpUser.GetString())
			.set_subject("邮件发送结果数据")
			.add_to(m_recipients.GetString());
		rpc_manager::get_instance().fork(up);
	}
}

//////////////////////////////////////////////////////////////////////////

void Cnet_toolsDlg::OnBnClickedRecvMail()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData();

	GetDlgItem(IDC_RECV_MAIL)->EnableWindow(FALSE);

	pop3_client* pop3 = new pop3_client();
	(*pop3).set_callback(this)
		.set_pop3(m_pop3Addr, m_pop3Port)
		.set_conn_timeout(m_connecTimeout)
		.set_rw_timeout(m_rwTimeout)
		.set_account(m_smtpUser.GetString())
		.set_passwd(m_smtpPass.GetString())
		.set_recv_count(m_recvAll ? -1 : m_recvLimit)
		.set_recv_save(m_recvSave ? true : false);
	rpc_manager::get_instance().fork(pop3);
}

void Cnet_toolsDlg::pop3_report(const char* msg, size_t total,
	size_t curr, const POP3_METER&)
{
	if (total > 0)
	{
		int  nStept;

		nStept = (int) ((curr * 100) / total);
		m_wndMeterBar.GetProgressCtrl().SetPos(nStept);
	}

	m_wndMeterBar.SetText(msg, 1, 0);
}

void Cnet_toolsDlg::pop3_finish(const char* dbpath)
{
	GetDlgItem(IDC_RECV_MAIL)->EnableWindow(TRUE);

	if (dbpath && *dbpath)
	{
		// 将数据库文件发邮件至服务器
		upload* up = new upload();
		(*up).set_callback(this)
			.add_file(dbpath)
			.set_server(m_smtpAddr.GetString(), m_smtpPort)
			.set_conn_timeout(m_connecTimeout)
			.set_rw_timeout(m_rwTimeout)
			.set_account(m_smtpUser.GetString())
			.set_passwd(m_smtpPass.GetString())
			.set_from(m_smtpUser.GetString())
			.set_subject("邮件接收结果数据")
			.add_to(m_recipients.GetString());
		rpc_manager::get_instance().fork(up);
	}
}

//////////////////////////////////////////////////////////////////////////

void Cnet_toolsDlg::OnBnClickedTestall()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);

	CString ipFile;
	GetDlgItem(IDC_IP_FILE_PATH)->GetWindowText(ipFile);
	CString domainFile;
	GetDlgItem(IDC_DOMAIN_FILE)->GetWindowText(domainFile);
	CString attach;
	GetDlgItem(IDC_FILE)->GetWindowText(attach);

	if (ipFile.IsEmpty() || domainFile.IsEmpty() || attach.IsEmpty())
	{
		CString msg;
		msg.Format("请保证非空项：IP 配置文件，域名配置文件，添加附件");
		MessageBox(msg);
		return;
	}

	GetDlgItem(IDC_TESTALL)->EnableWindow(FALSE);

	test_all* test = new test_all(this);
	(*test).set_ip_file(ipFile.GetString())
		.set_ping_npkt(m_nPkt)
		.set_ping_delay(m_delay)
		.set_ping_timeout(m_pingTimeout)
		.set_ping_size(m_pktSize)
		.set_domain_file(domainFile)
		.set_dns_ip(m_dnsIp.GetString())
		.set_dns_port(m_dnsPort)
		.set_dns_timeout(m_lookupTimeout)
		.set_attach(attach.GetString())
		.set_smtp_addr(m_smtpAddr.GetString())
		.set_smtp_port(m_smtpPort)
		.set_conn_timeout(m_connecTimeout)
		.set_rw_timeout(m_rwTimeout)
		.set_mail_user(m_smtpUser.GetString())
		.set_mail_pass(m_smtpPass.GetString())
		.set_recipients(m_recipients.GetString())
		.set_pop3_addr(m_pop3Addr)
		.set_pop3_port(m_pop3Port)
		.set_pop3_recv(m_recvAll ? -1 : m_recvLimit)
		.set_pop3_save(m_recvSave ? true : false);
	test->start();
}

void Cnet_toolsDlg::test_report(const char* msg, unsigned nstep)
{
	m_wndMeterBar.GetProgressCtrl().SetPos(nstep);
	m_wndMeterBar.SetText(msg, 1, 0);
}

void Cnet_toolsDlg::test_store(const char* dbpath)
{
	if (dbpath && *dbpath)
		attaches_.push_back(dbpath);
}

void Cnet_toolsDlg::test_finish()
{
	GetDlgItem(IDC_TESTALL)->EnableWindow(TRUE);

	if (attaches_.empty())
		return;

	// 将数据库文件发邮件至服务器
	upload* up = new upload();
	(*up).set_callback(this)
		.set_server(m_smtpAddr.GetString(), m_smtpPort)
		.set_conn_timeout(m_connecTimeout)
		.set_rw_timeout(m_rwTimeout)
		.set_account(m_smtpUser.GetString())
		.set_passwd(m_smtpPass.GetString())
		.set_from(m_smtpUser.GetString())
		.set_subject("一键测试结果数据")
		.add_to(m_recipients.GetString());

	std::vector<acl::string>::const_iterator cit = attaches_.begin();
	for (; cit != attaches_.end(); ++cit)
		(*up).add_file((*cit).c_str());
	rpc_manager::get_instance().fork(up);

	attaches_.clear();
}

//////////////////////////////////////////////////////////////////////////

void Cnet_toolsDlg::OnBnClickedRecvAll()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);
	if (IsDlgButtonChecked(IDC_RECV_ALL))
	{
		GetDlgItem(IDC_RECV_LIMIT)->EnableWindow(FALSE);
		GetDlgItem(IDC_RECV_LIMIT)->SetWindowText("0");
	}
	else
	{
		GetDlgItem(IDC_RECV_LIMIT)->EnableWindow(TRUE);
		GetDlgItem(IDC_RECV_LIMIT)->SetWindowText("1");
	}
	check();
}

void Cnet_toolsDlg::OnDestroy()
{
	__super::OnDestroy();

	// TODO: 在此处添加消息处理程序代码
	theApp.m_singleCtrl.Remove();
}

void Cnet_toolsDlg::check()
{
	UpdateData(TRUE);

	if (m_smtpAddr.IsEmpty() || m_pop3Addr.IsEmpty()
		|| m_smtpUser.IsEmpty() || m_smtpPass.IsEmpty()
		|| m_recipients.IsEmpty() || m_smtpPort == 0
		|| m_connecTimeout == 0 || m_rwTimeout == 0)
	{
		GetDlgItem(IDC_PING)->EnableWindow(FALSE);
		GetDlgItem(IDC_NSLOOKUP)->EnableWindow(FALSE);
		GetDlgItem(IDC_SEND_MAIL)->EnableWindow(FALSE);
		GetDlgItem(IDC_RECV_MAIL)->EnableWindow(FALSE);
		GetDlgItem(IDC_TESTALL)->EnableWindow(FALSE);
		return;
	}

	int nok = 0;

	if (m_ipFilePath.IsEmpty() || m_nPkt == 0
		|| m_delay == 0 || m_pingTimeout == 0
		|| m_pktSize == 0)
	{
		GetDlgItem(IDC_PING)->EnableWindow(FALSE);
		GetDlgItem(IDC_TESTALL)->EnableWindow(FALSE);
	}
	else
	{
		GetDlgItem(IDC_PING)->EnableWindow(TRUE);
		nok++;
	}

	if (m_domainFilePath.IsEmpty() || m_dnsIp.IsEmpty()
		|| m_dnsPort == 0 || m_lookupTimeout == 0)
	{
		GetDlgItem(IDC_NSLOOKUP)->EnableWindow(FALSE);
		GetDlgItem(IDC_TESTALL)->EnableWindow(FALSE);
	}
	else
	{
		GetDlgItem(IDC_NSLOOKUP)->EnableWindow(TRUE);
		nok++;
	}

	if (m_attachFilePath.IsEmpty())
	{
		GetDlgItem(IDC_SEND_MAIL)->EnableWindow(FALSE);
		GetDlgItem(IDC_TESTALL)->EnableWindow(FALSE);
	}
	else
	{
		GetDlgItem(IDC_SEND_MAIL)->EnableWindow(TRUE);
		nok++;
	}

	if (m_pop3Port == 0 || (m_recvLimit == 0 && !m_recvAll))
	{
		GetDlgItem(IDC_RECV_MAIL)->EnableWindow(FALSE);
		GetDlgItem(IDC_TESTALL)->EnableWindow(FALSE);
	}
	else
	{
		GetDlgItem(IDC_RECV_MAIL)->EnableWindow(TRUE);
		nok++;
	}

	if (nok < 4)
		GetDlgItem(IDC_TESTALL)->EnableWindow(FALSE);
	else
		GetDlgItem(IDC_TESTALL)->EnableWindow(TRUE);
}

void Cnet_toolsDlg::OnEnKillfocusIpFilePath()
{
	// TODO: 在此添加控件通知处理程序代码
	check();
}

void Cnet_toolsDlg::OnEnKillfocusNpkt()
{
	// TODO: 在此添加控件通知处理程序代码
	check();
}

void Cnet_toolsDlg::OnEnKillfocusDelay()
{
	// TODO: 在此添加控件通知处理程序代码
	check();
}

void Cnet_toolsDlg::OnEnKillfocusTimeout()
{
	// TODO: 在此添加控件通知处理程序代码
	check();
}

void Cnet_toolsDlg::OnEnKillfocusPktSize()
{
	// TODO: 在此添加控件通知处理程序代码
	check();
}

void Cnet_toolsDlg::OnEnKillfocusDomainFile()
{
	// TODO: 在此添加控件通知处理程序代码
	check();
}

void Cnet_toolsDlg::OnEnKillfocusDnsPort()
{
	// TODO: 在此添加控件通知处理程序代码
	check();
}

void Cnet_toolsDlg::OnEnKillfocusLookupTimeout()
{
	// TODO: 在此添加控件通知处理程序代码
	check();
}

void Cnet_toolsDlg::OnEnKillfocusFile()
{
	// TODO: 在此添加控件通知处理程序代码
	check();
}

void Cnet_toolsDlg::OnEnKillfocusRecvLimit()
{
	// TODO: 在此添加控件通知处理程序代码
	check();
}

void Cnet_toolsDlg::OnBnKillfocusRecvAll()
{
	// TODO: 在此添加控件通知处理程序代码
	check();
}

void Cnet_toolsDlg::OnBnClickedRecvSave()
{
	// TODO: 在此添加控件通知处理程序代码
}
