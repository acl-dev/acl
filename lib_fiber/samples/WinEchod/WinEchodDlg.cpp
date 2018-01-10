
// WinEchodDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "Listener.h"
#include "WinEchod.h"
#include "WinEchodDlg.h"
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
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CWinEchodDlg 对话框


CWinEchodDlg::CWinEchodDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CWinEchodDlg::IDD, pParent)
	, m_dosFp(NULL)
	, m_listenPort(9001)
	, m_listenIP(_T("127.0.0.1"))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CWinEchodDlg::~CWinEchodDlg()
{
	if (m_dosFp)
	{
		fclose(m_dosFp);
		FreeConsole();
		m_dosFp = NULL;
	}
}

void CWinEchodDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_LISTEN_PORT, m_listenPort);
	DDV_MinMaxUInt(pDX, m_listenPort, 0, 65535);
	DDX_Text(pDX, IDC_LISTEN_IP, m_listenIP);
	DDV_MaxChars(pDX, m_listenIP, 64);
}

BEGIN_MESSAGE_MAP(CWinEchodDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_OPEN_DOS, &CWinEchodDlg::OnBnClickedOpenDos)
	ON_BN_CLICKED(IDC_LISTEN, &CWinEchodDlg::OnBnClickedListen)
END_MESSAGE_MAP()


// CWinEchodDlg 消息处理程序

BOOL CWinEchodDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
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

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CWinEchodDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CWinEchodDlg::OnPaint()
{
	if (IsIconic())
	{
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
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CWinEchodDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CWinEchodDlg::OnBnClickedOpenDos()
{
	// TODO: 在此添加控件通知处理程序代码
	if (m_dosFp == NULL)
	{
		//GetDlgItem(IDC_OPEN_DOS)->EnableWindow(FALSE);
		UpdateData();
		AllocConsole();
		m_dosFp = freopen("CONOUT$","w+t",stdout);
		printf("DOS opened now, listen=%s:%d\r\n",
			m_listenIP.GetString(), m_listenPort);
		GetDlgItem(IDC_OPEN_DOS)->SetWindowText("关闭 DOS 窗口");
	}
	else
	{
		fclose(m_dosFp);
		m_dosFp = NULL;
		FreeConsole();
		GetDlgItem(IDC_OPEN_DOS)->SetWindowText("打开 DOS 窗口");
	}
}


void CWinEchodDlg::OnBnClickedListen()
{
	// TODO: 在此添加控件通知处理程序代码
	CString addr;
	addr.Format("%s:%d", m_listenIP.GetString(), m_listenPort);
	if (m_listen.open(addr) == false)
	{
		printf("listen %s error %s\r\n", addr.GetString());
		return;
	}
	printf("listen %s ok\r\n", addr.GetString());
	acl::fiber* fb = new CListener(m_listen);
	fb->start();
	acl::fiber::schedule(acl::FIBER_EVENT_T_WMSG);
}
