
// WinFiberDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "WinFiber.h"
#include "WinFiberDlg.h"
#include "FiberListener.h"
#include "FiberConnect.h"
#include "FiberSleep.h"
#include "FiberHttpd.h"

#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

// CWinFiberDlg 对话框

CWinFiberDlg::CWinFiberDlg(CWnd* pParent /*=nullptr*/)
: CDialogEx(IDD_WINFIBER_DIALOG, pParent)
, m_dosFp(NULL)
, m_listenPort(9001)
, m_listenIP(_T("127.0.0.1"))
, m_listenAddr("127.0.0.1:9001")
, m_fiberListen(NULL)
, m_httpdAddr("127.0.0.1:8088")
, m_cocurrent(1)
, m_count(100)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CWinFiberDlg::~CWinFiberDlg(void)
{
	if (m_dosFp) {
		fclose(m_dosFp);
		m_dosFp = NULL;
		FreeConsole();
	}
}

void CWinFiberDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CWinFiberDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_OPEN_DOS, &CWinFiberDlg::OnBnClickedOpenDos)
	ON_BN_CLICKED(IDC_LISTEN, &CWinFiberDlg::OnBnClickedListen)
	ON_BN_CLICKED(IDC_CONNECT, &CWinFiberDlg::OnBnClickedConnect)
	ON_BN_CLICKED(IDC_CREATE_TIMER, &CWinFiberDlg::OnBnClickedCreateTimer)
	ON_BN_CLICKED(IDC_START_HTTPD, &CWinFiberDlg::OnBnClickedStartHttpd)
	ON_BN_CLICKED(IDOK, &CWinFiberDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CWinFiberDlg::OnBnClickedCancel)
END_MESSAGE_MAP()

// CWinFiberDlg 消息处理程序

BOOL CWinFiberDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr) {
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty()) {
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	InitFiber();  // 初始化协程环境
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CWinFiberDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX) {
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	} else {
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CWinFiberDlg::OnPaint()
{
	if (IsIconic()) {
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	} else {
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CWinFiberDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

// UNICODE 转宽字符
void CWinFiberDlg::Uni2Str(const CString& in, acl::string& out)
{
	int len = WideCharToMultiByte(CP_ACP, 0, in.GetString(), in.GetLength(),
		NULL, 0, NULL, NULL);
	char *buf = new char[len + 1];
	WideCharToMultiByte(CP_ACP, 0, in.GetString(), in.GetLength(),
		buf, len, NULL, NULL);
	buf[len] = 0;
	out = buf;
	delete buf;
}

void CWinFiberDlg::InitFiber(void)
{
	acl::acl_cpp_init();
	// 设置协程调度的事件引擎，同时将协程调度设为自动启动模式，不能在进程初始化时启动
	// 协程调试器，必须在界面消息引擎正常运行后才启动协程调度器！
	acl::fiber::init(acl::FIBER_EVENT_T_WMSG, true);
	acl::fiber::winapi_hook();
	// HOOK ACL 库中的网络 IO 过程
	//acl::fiber::acl_io_hook();
}

void CWinFiberDlg::StopFiber(void)
{
	// 停止协程调度过程
	acl::fiber::schedule_stop();
}

void CWinFiberDlg::OnBnClickedOpenDos()
{
	// TODO: 在此添加控件通知处理程序代码
	if (m_dosFp == NULL) {
		//GetDlgItem(IDC_OPEN_DOS)->EnableWindow(FALSE);
		UpdateData();
		AllocConsole();
		m_dosFp = freopen("CONOUT$","w+t",stdout);
		printf("DOS opened, listen=%s\r\n", m_listenAddr.c_str());
		CString info(_T("Close DOS"));
		GetDlgItem(IDC_OPEN_DOS)->SetWindowText(info);
	} else {
		fclose(m_dosFp);
		m_dosFp = NULL;
		FreeConsole();
		CString info(_T("Open DOS"));
		GetDlgItem(IDC_OPEN_DOS)->SetWindowText(info);
	}
}

void CWinFiberDlg::OnBnClickedListen()
{
	// TODO: 在此添加控件通知处理程序代码
		if (m_fiberListen == NULL) {
		UpdateData();
		Uni2Str(m_listenIP, m_listenAddr);
		m_listenAddr.format_append(":%d", m_listenPort);
		if (m_listen.open(m_listenAddr) == false) {
			printf("listen %s error %s\r\n", m_listenAddr.c_str(), acl::last_serror());
			return;
		}
		CString info(_T("Stop Listen"));
		GetDlgItem(IDC_LISTEN)->SetWindowText(info);

		printf("listen %s ok\r\n", m_listenAddr.c_str());
		m_fiberListen = new CFiberListener(m_listen);
		m_fiberListen->start();
	} else if (acl::fiber::scheduled()) {
		GetDlgItem(IDC_LISTEN)->EnableWindow(FALSE);
		m_fiberListen->kill();
		printf("listening fiber was killed\r\n");
		m_listen.close();
		printf("listening socket was closed\r\n");
		m_fiberListen = NULL;
		printf("fiber schedule stopped!\r\n");
		CString info(_T("Begin Listen"));
		GetDlgItem(IDC_LISTEN)->SetWindowText(info);
		GetDlgItem(IDC_LISTEN)->EnableWindow(TRUE);
	} else {
		GetDlgItem(IDC_LISTEN)->EnableWindow(FALSE);
		m_listen.close();
		printf("listening socket was closed\r\n");
		m_fiberListen = NULL;
		CString info(_T("Begin Listen"));
		GetDlgItem(IDC_LISTEN)->SetWindowText(info);
		GetDlgItem(IDC_LISTEN)->EnableWindow(TRUE);
	}
}

void CWinFiberDlg::OnBnClickedConnect()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData();
	GetDlgItem(IDC_CONNECT)->EnableWindow(FALSE);

	UINT n = m_cocurrent;
	for (UINT i = 0; i < n; i++) {
		acl::fiber* fb = new CFiberConnect(
			*this, m_listenAddr.c_str(), m_count);
		fb->start();
	}
}

void CWinFiberDlg::OnFiberConnectExit(void)
{
	if (--m_cocurrent == 0) {
		GetDlgItem(IDC_CONNECT)->EnableWindow(TRUE);
		printf("All connect fibers finished now!\r\n");
	}
}

void CWinFiberDlg::OnBnClickedCreateTimer()
{
	// TODO: 在此添加控件通知处理程序代码
	acl::fiber* fb = new CFiberSleep;
	fb->start();
}

void CWinFiberDlg::OnBnClickedStartHttpd()
{
	// TODO: 在此添加控件通知处理程序代码
	acl::fiber* fb = new CFiberHttpd(m_httpdAddr.c_str());
	fb->start();
}


void CWinFiberDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnOK();
	StopFiber();  // 停止协程调度过程
}


void CWinFiberDlg::OnBnClickedCancel()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnCancel();
	StopFiber();  // 停止协程调度过程
}
