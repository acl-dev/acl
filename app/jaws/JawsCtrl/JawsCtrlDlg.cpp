// JawsCtrlDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "lib_acl.h"
#include "JawsCtrl.h"
#include "JawsCtrlDlg.h"
#include ".\jawsctrldlg.h"

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


// CJawsCtrlDlg 对话框

CJawsCtrlDlg::CJawsCtrlDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CJawsCtrlDlg::IDD, pParent)
	, m_trayIcon(IDR_MENU_ICON)
	, m_bShutdown(FALSE)
	, m_sJawsName(_T(""))
	, m_sJawsCtrlName(_T(""))
	, m_listenPort(8080)
	, m_listenIp(_T("127.0.0.1"))
	, m_httpVhostPath(_T(""))
	, m_httpVhostDefault(_T(""))
	, m_httpTmplPath(_T(""))
	, m_httpFilter(_T("HTTP_FILTER_HTTPD"))
	, m_nHttpFilter(IDC_RADIO_HTTPD)
	, m_pService(NULL)
	, m_regRun(_T("JawsCtrl"))
{
	char  buf[MAX_PATH];

	acl_proctl_daemon_path(buf, sizeof(buf));
	m_sJawsName.Format("%s\\Jaws.exe", buf);
	m_sJawsCtrlName.Format("%s\\JawsCtrl.exe", buf);

	//m_sJawsName.Format("C:\\\"Program Files\"\\\"Acl Project\"\\Jaws\\Jaws.exe");
	m_httpVhostPath.Format("%s\\conf\\www\\vhost", buf);
	m_httpVhostDefault.Format("%s\\conf\\www\\defaults.cf", buf);
	m_httpTmplPath.Format("%s\\conf\\www\\tmpl", buf);

	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_procCtrl.RunThread();
}

void CJawsCtrlDlg::DoDataExchange(CDataExchange* pDX)
{
	BYTE field1, field2, field3, field4;

	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_IPADDRESS_LISTEN, m_listenIpCtrl);
	if (pDX->m_bSaveAndValidate) {
		m_listenIpCtrl.GetAddress(field1, field2, field3, field4);
		m_listenIp.Format("%d.%d.%d.%d", field1, field2, field3, field4);
	}
	DDX_Text(pDX, IDC_EDIT_PORT, m_listenPort);
	DDV_MinMaxLong(pDX, m_listenPort, 0, 65535);
}

BEGIN_MESSAGE_MAP(CJawsCtrlDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_WM_CREATE()
	ON_COMMAND(ID_OPEN_MAIN, OnOpenMain)
	ON_COMMAND(ID_QUIT, OnQuit)
	ON_MESSAGE(WM_MY_TRAY_NOTIFICATION, OnTrayNotification)
	ON_WM_NCPAINT()
	ON_WM_CLOSE()
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON_START, OnBnClickedButtonStart)
	ON_BN_CLICKED(IDC_BUTTON_STOP, OnBnClickedButtonStop)
	ON_BN_CLICKED(IDC_BUTTON_QUIT, OnBnClickedButtonQuit)
	ON_BN_CLICKED(IDC_MORE, OnBnClickedMore)
	ON_BN_CLICKED(IDC_AUTO_RUN, OnBnClickedAutoRun)
END_MESSAGE_MAP()


// CJawsCtrlDlg 消息处理程序

BOOL CJawsCtrlDlg::OnInitDialog()
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

	// TODO: 在此添加额外的初始化代码
	m_dVerticalExpand.Init(this, IDC_DIVIDER, TRUE);
	ExpandDialog(FALSE);

	theApp.m_singleCtrl.Register();
	m_listenIpCtrl.SetAddress(127, 0, 0, 1);
	CheckRadioButton(IDC_RADIO_HTTPD, IDC_RADIO_HTTP_PROXY, IDC_RADIO_HTTP_PROXY);
	GetDlgItem(IDC_BUTTON_STOP)->EnableWindow(FALSE);

	// 启动 Jaws.exe
	if (m_regRun.IfAutoRun()) {
		OnBnClickedButtonStart();
		CheckDlgButton(IDC_AUTO_RUN, 1);
	} else
		CheckDlgButton(IDC_AUTO_RUN, 0);

	return TRUE;  // 除非设置了控件的焦点，否则返回 TRUE
}

void CJawsCtrlDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CJawsCtrlDlg::OnPaint() 
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
HCURSOR CJawsCtrlDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

int CJawsCtrlDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDialog::OnCreate(lpCreateStruct) == -1)
		return -1;

	// TODO:  在此添加您专用的创建代码
	m_trayIcon.SetNotificationWnd(this, WM_MY_TRAY_NOTIFICATION);
	m_trayIcon.SetIcon(IDI_MIN_ICON);
	return 0;
}

void CJawsCtrlDlg::OnOpenMain()
{
	// TODO: 在此添加命令处理程序代码
	ShowWindow(SW_NORMAL);
}

void CJawsCtrlDlg::OnQuit()
{
	// TODO: 在此添加命令处理程序代码
	m_bShutdown = TRUE;		// really exit
	SendMessage(WM_CLOSE);
}

afx_msg LRESULT CJawsCtrlDlg::OnTrayNotification(WPARAM uID, LPARAM lEvent)
{
	// let tray icon do default stuff
	return m_trayIcon.OnTrayNotification(uID, lEvent);
}
void CJawsCtrlDlg::OnNcPaint()
{
	// TODO: 在此处添加消息处理程序代码
	// 不为绘图消息调用 CDialog::OnNcPaint()
	static int i = 2;
	if(i > 0)
	{
		i --;
		ShowWindow(SW_HIDE);
	} else
	{
		CDialog::OnNcPaint();
	}
}

void CJawsCtrlDlg::OnClose()
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	if (m_bShutdown) {
		if (m_pService) {
			m_procCtrl.StopOne(*m_pService);

			delete m_pService;
			m_pService = NULL;
		}
		CDialog::OnClose();
	} else {
		ShowWindow(SW_HIDE);
	}
}

void CJawsCtrlDlg::OnDestroy()
{
	CDialog::OnDestroy();

	// TODO: 在此处添加消息处理程序代码
	theApp.m_singleCtrl.Remove();
}

void CJawsCtrlDlg::OnBnClickedButtonStart()
{
	// TODO: 在此添加控件通知处理程序代码
	CString addr;

	addr.Format("%s:%d", m_listenIp, m_listenPort);
	m_nHttpFilter = GetCheckedRadioButton(IDC_RADIO_HTTPD, IDC_RADIO_HTTP_PROXY);
	switch (m_nHttpFilter) {
	case IDC_RADIO_HTTPD:
		m_httpFilter.Format("HTTP_FILTER_HTTPD");
		break;
	case IDC_RADIO_HTTP_PROXY:
		m_httpFilter.Format("HTTP_FILTER_PROXY");
		break;
	default:
		m_httpFilter.Format("HTTP_FILTER_HTTPD");
		break;
	}

	ASSERT(m_pService == NULL);
	m_pService = new CHttpService(
		m_sJawsName.GetString(),
		addr.GetString(),
		m_httpVhostPath.GetString(),
		m_httpVhostDefault.GetString(),
		m_httpTmplPath.GetString(),
		m_httpFilter.GetString());

	m_pService->DebugArgv();
	m_procCtrl.StartOne(*m_pService);
	GetDlgItem(IDC_BUTTON_START)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_STOP)->EnableWindow(TRUE);
	GetDlgItem(IDC_RADIO_HTTPD)->EnableWindow(FALSE);
	GetDlgItem(IDC_RADIO_HTTP_PROXY)->EnableWindow(FALSE);
}

void CJawsCtrlDlg::OnBnClickedButtonStop()
{
	// TODO: 在此添加控件通知处理程序代码
	ASSERT(m_pService != NULL);
	GetDlgItem(IDC_BUTTON_START)->EnableWindow(TRUE);
	GetDlgItem(IDC_BUTTON_STOP)->EnableWindow(FALSE);
	m_procCtrl.StopOne(*m_pService);
	GetDlgItem(IDC_RADIO_HTTPD)->EnableWindow(TRUE);
	GetDlgItem(IDC_RADIO_HTTP_PROXY)->EnableWindow(TRUE);

	delete m_pService;
	m_pService = NULL;
}

void CJawsCtrlDlg::OnBnClickedButtonQuit()
{
	// TODO: 在此添加控件通知处理程序代码
	m_bShutdown = TRUE;		// really exit
	SendMessage(WM_CLOSE);
}

void CJawsCtrlDlg::OnBnClickedMore()
{
	// TODO: 在此添加控件通知处理程序代码
	static BOOL bExpand = TRUE;

	ExpandDialog(bExpand);
	bExpand = !bExpand;
}

void CJawsCtrlDlg::ExpandDialog(BOOL bExpand)
{
	CString sExpand;

	m_dVerticalExpand.Expand(bExpand);
	if (bExpand) {
		sExpand = "<<(&L)更少";
	} else {
		sExpand = "(&M)更多>>";
	}

	SetDlgItemText(IDC_MORE, sExpand);
}

void CJawsCtrlDlg::OnBnClickedAutoRun()
{
	// TODO: 在此添加控件通知处理程序代码
	if (IsDlgButtonChecked(IDC_AUTO_RUN))
		m_regRun.AutoRun(TRUE, m_sJawsCtrlName.GetString());
	else
		m_regRun.AutoRun(FALSE, m_sJawsCtrlName.GetString());
}
