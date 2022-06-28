
// EchoServerDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "EchoServer.h"
#include "EchoServerDlg.h"
#include "afxdialogex.h"

#include "FiberServer.h"

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

// CEchoServerDlg 对话框

CEchoServerDlg::CEchoServerDlg(CWnd* pParent /*=nullptr*/)
: CDialogEx(IDD_ECHOSERVER_DIALOG, pParent)
, m_port(8088)
, m_listenFiber(NULL)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CEchoServerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_PORT, m_port);
	DDX_Control(pDX, IDC_IPADDR, m_listenAddr);
}

BEGIN_MESSAGE_MAP(CEchoServerDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_LISTEN, &CEchoServerDlg::OnBnClickedListen)
	ON_BN_CLICKED(IDC_STOP_LISTEN, &CEchoServerDlg::OnBnClickedStopListen)
	ON_BN_CLICKED(IDOK, &CEchoServerDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CEchoServerDlg::OnBnClickedCancel)
END_MESSAGE_MAP()

// CEchoServerDlg 消息处理程序

BOOL CEchoServerDlg::OnInitDialog()
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

	BYTE ip[4];
	ip[0] = 0;
	ip[1] = 0;
	ip[2] = 0;
	ip[3] = 0;
	m_listenAddr.SetAddress(ip[0], ip[1], ip[2], ip[3]);

	GetDlgItem(IDC_STOP_LISTEN)->EnableWindow(FALSE);
	InitFiber();

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CEchoServerDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CEchoServerDlg::OnPaint()
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
HCURSOR CEchoServerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CEchoServerDlg::InitFiber()
{
	// 设置协程调度的事件引擎，同时将协程调度设为自动启动模式，不能在进程初始化时启动
	// 协程调试器，必须在界面消息引擎正常运行后才启动协程调度器！
	acl::fiber::init(acl::FIBER_EVENT_T_WMSG, true);
	acl::fiber::winapi_hook();
}

void CEchoServerDlg::StopFiber()
{
	// 停止协程调度过程
	acl::fiber::schedule_stop();
}

void CEchoServerDlg::OnBnClickedListen()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);

	BYTE ip[4];
	m_listenAddr.GetAddress(ip[0], ip[1], ip[2], ip[3]);
	CString ipAddr;
	ipAddr.Format(_T("%d.%d.%d.%d"), ip[0], ip[1], ip[2], ip[3]);

	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) {
		MessageBox("can't create socket!");
		return;
	}

	struct sockaddr_in in;
	in.sin_family = AF_INET;
	in.sin_port   = htons(m_port);
	if (inet_pton(AF_INET, ipAddr.GetString(), &in.sin_addr) == -1) {
		CString buf;
		buf.Format("inet_pton error, addr=%s", ipAddr.GetString());
		MessageBox(buf);
		closesocket(sock);
		return;
	}

	if (bind(sock, (const sockaddr*)&in, sizeof(in)) == -1) {
		CString buf;
		buf.Format("bind %s:%d error", ipAddr.GetString(), m_port);
		MessageBox(buf);
		closesocket(sock);
		return;
	}

	if (listen(sock, 128) == -1) {
		CString buf;
		buf.Format("listen %s:%d error", ipAddr.GetString(), m_port);
		MessageBox(buf);
		closesocket(sock);
		return;
	}

	GetDlgItem(IDC_LISTEN)->EnableWindow(FALSE);
	GetDlgItem(IDC_STOP_LISTEN)->EnableWindow(TRUE);

	m_listenFiber = new CFiberServer(sock);
	m_listenFiber->start();
}

void CEchoServerDlg::OnBnClickedStopListen()
{
	if (m_listenFiber) {
		acl::fiber* fb = m_listenFiber;
		m_listenFiber = NULL;
		fb->kill();
		GetDlgItem(IDC_LISTEN)->EnableWindow(TRUE);
		GetDlgItem(IDC_STOP_LISTEN)->EnableWindow(FALSE);
	}
}

void CEchoServerDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnOK();
	StopFiber();
}

void CEchoServerDlg::OnBnClickedCancel()
{
	// TODO: 在此添加控件通知处理程序代码
	CDialogEx::OnCancel();
	StopFiber();
}
