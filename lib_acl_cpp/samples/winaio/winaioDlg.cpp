// winaioDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "lib_acl.h"
#include <iostream>
#include "acl_cpp/stream/aio_handle.hpp"
#include "acl_cpp/stream/aio_stream.hpp"
#include "AioServer.h"
#include "AioClient.h"
#include "AioTimer.hpp"
#include "winaio.h"
//#include "vld.h"
//#include "heap_hook.h"
#include "winaioDlg.h"
#include ".\winaiodlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#ifdef WIN32
# ifndef snprintf
#  define snprintf _snprintf
# endif
#endif

//CHeapHook2 vld2;

using namespace acl;

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


// CwinaioDlg 对话框



CwinaioDlg::CwinaioDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CwinaioDlg::IDD, pParent)
	, sstream_(NULL)
	, handle_(NULL)
	, keep_timer_(false)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CwinaioDlg::~CwinaioDlg()
{
	std::list<CAioTimer*>::iterator it = timers_.begin(), it_next;
	for (it_next = it; it != timers_.end(); it = it_next)
	{
		it_next++;
		handle_->del_timer(*it);
		timers_.erase(it);
	}
	if (handle_)
		delete handle_;
}

void CwinaioDlg::InitCtx()
{
	memset(&client_ctx_, 0, sizeof(client_ctx_));
	client_ctx_.connect_timeout = 5;
	client_ctx_.nopen_limit = 10;
	client_ctx_.id_begin = 1;
	client_ctx_.nwrite_limit = 10;
	client_ctx_.debug = false;
	snprintf(client_ctx_.addr, sizeof(client_ctx_.addr), "127.0.0.1:9001");
	client_ctx_.handle = handle_;
}

void CwinaioDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CwinaioDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_LISTEN, OnBnClickedListen)
	ON_BN_CLICKED(IDC_CONNECT, OnBnClickedConnect)
	ON_BN_CLICKED(IDC_SET_TIMER, OnBnClickedSetTimer)
	ON_BN_CLICKED(IDC_DEL_TIMER, OnBnClickedDelTimer)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_BN_CLICKED(IDC_BUTTON_KEEP_TIMER, OnBnClickedButtonKeepTimer)
	ON_BN_CLICKED(IDC_BUTTON_NO_KEEP_TIMER, OnBnClickedButtonNoKeepTimer)
	ON_BN_CLICKED(IDC_BUTTON_MEMTEST, OnBnClickedButtonMemtest)
END_MESSAGE_MAP()


// CwinaioDlg 消息处理程序

BOOL CwinaioDlg::OnInitDialog()
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
	GetDlgItem(IDC_SET_TIMER)->EnableWindow(TRUE);
	GetDlgItem(IDC_DEL_TIMER)->EnableWindow(FALSE);

	GetDlgItem(IDC_BUTTON_KEEP_TIMER)->EnableWindow(TRUE);
	GetDlgItem(IDC_BUTTON_NO_KEEP_TIMER)->EnableWindow(FALSE);

	FILE *fp;
	AllocConsole();
	fp = freopen("CONOUT$","w+t",stdout);

	handle_ = new acl::aio_handle(ENGINE_WINMSG);
	InitCtx();
	
	return TRUE;  // 除非设置了控件的焦点，否则返回 TRUE
}

void CwinaioDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CwinaioDlg::OnPaint() 
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
HCURSOR CwinaioDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CwinaioDlg::OnBnClickedListen()
{
	// TODO: 在此添加控件通知处理程序代码

	sstream_ = new aio_listen_stream(handle_);
	const char* addr = "127.0.0.1:9001";
	// 监听指定的地址
	if (sstream_->open(addr) == false)
	{
		std::cout << "open " << addr << " error!" << std::endl;
		sstream_->close(); // 释放监听流所占资源
		handle_->check(); // 清空所有异步流所占资源
		acl_pthread_end(); // 清除所有线程所占资源
		return;
	}
	GetDlgItem(IDC_LISTEN)->EnableWindow(FALSE);
	// 创建回调类对象，当有新连接到达时自动调用此类对象的回调过程
	sstream_->add_accept_callback(&callback_);
	std::cout << "Listen: " << addr << " ok!" << std::endl;
}

void CwinaioDlg::OnBnClickedConnect()
{
	// TODO: 在此添加控件通知处理程序代码
	if (CConnectClientCallback::connect_server(&client_ctx_,
			client_ctx_.id_begin) == false)
	{
		std::cout << "connect " << client_ctx_.addr << " error!" << std::endl;
	}
	else
		GetDlgItem(IDC_CONNECT)->EnableWindow(FALSE);
}

void CwinaioDlg::OnBnClickedSetTimer()
{
	// TODO: 在此添加控件通知处理程序代码

	GetDlgItem(IDC_SET_TIMER)->EnableWindow(FALSE);
	GetDlgItem(IDC_DEL_TIMER)->EnableWindow(TRUE);
	__int64 inter = 1000000;
	for (int i = 0; i < 10; i++)
	{
		CAioTimer* timer = new CAioTimer(handle_, i, keep_timer_);
		handle_->set_timer(timer, (i + 1) * inter, 1);
		if (timer->keep_timer())
			timers_.push_back(timer);
	}
}

void CwinaioDlg::OnBnClickedDelTimer()
{
	// TODO: 在此添加控件通知处理程序代码
	GetDlgItem(IDC_SET_TIMER)->EnableWindow(TRUE);
	GetDlgItem(IDC_DEL_TIMER)->EnableWindow(FALSE);

	std::list<CAioTimer*>::iterator it = timers_.begin(), it_next;
	for (it_next = it; it != timers_.end(); it = it_next)
	{
		it_next++;
		handle_->del_timer(*it);
		timers_.erase(it);
	}
}

void CwinaioDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	if (sstream_)
	{
		sstream_->close(); // 释放监听流资源
		handle_->check(); // 最后清空一下所有异步流所占的资源
	}
	acl_pthread_end(); // 清除所有线程所占资源

	OnOK();
}

void CwinaioDlg::on_increase()
{

}

void CwinaioDlg::on_decrease()
{
	/* 获得异步引擎中受监控的异步流个数 */
	int nleft = handle_->length();
	if (client_ctx_.nopen_total == client_ctx_.nopen_limit && nleft == 1)
	{
		std::cout << "All client streams over! just one listen stream: "
			<< std::endl;

		InitCtx();
		GetDlgItem(IDC_CONNECT)->EnableWindow(TRUE);
	}
}
void CwinaioDlg::OnBnClickedButtonKeepTimer()
{
	// TODO: 在此添加控件通知处理程序代码
	keep_timer_ = true;
	GetDlgItem(IDC_BUTTON_KEEP_TIMER)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_NO_KEEP_TIMER)->EnableWindow(TRUE);
}

void CwinaioDlg::OnBnClickedButtonNoKeepTimer()
{
	// TODO: 在此添加控件通知处理程序代码
	keep_timer_ = false;
	GetDlgItem(IDC_BUTTON_KEEP_TIMER)->EnableWindow(TRUE);
	GetDlgItem(IDC_BUTTON_NO_KEEP_TIMER)->EnableWindow(FALSE);
}

void CwinaioDlg::OnBnClickedButtonMemtest()
{
	// TODO: 在此添加控件通知处理程序代码
	CMemory* m = new CMemory;
	delete m;
}
