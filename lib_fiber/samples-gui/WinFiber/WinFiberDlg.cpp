
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
, m_listenPort(9101)
, m_listenIP(_T("127.0.0.1"))
, m_listenAddr("127.0.0.1:9101")
, m_fiberListen(NULL)
, m_httpdAddr("127.0.0.1:8088")
, m_cocurrent(100)
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
	ON_BN_CLICKED(IDC_AWAIT_DNS, &CWinFiberDlg::OnBnClickedAwaitDns)
	ON_BN_CLICKED(IDC_RESOLVE, &CWinFiberDlg::OnBnClickedResolve)
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
	//acl::fiber::schedule_with(acl::FIBER_EVENT_T_WMSG);
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

	printf("Begin connect %s with cocurrent=%d, count=%d\r\n",
		m_listenAddr.c_str(), m_cocurrent, m_count);

	go[&] {
		for (UINT i = 0; i < m_cocurrent; i++) {
			acl::fiber* fb = new CFiberConnect(
				*this, m_listenAddr.c_str(), m_count);
			m_clientFibers.insert(fb);
			fb->start();
			acl::fiber::delay(10);
			printf("One client fiber started!\r\n");
		}
	};
}

void CWinFiberDlg::OnFiberConnectExit(acl::fiber* fb)
{
	std::set<acl::fiber*>::iterator it = m_clientFibers.find(fb);
	if (it != m_clientFibers.end()) {
		m_clientFibers.erase(it);
		delete fb;
	} else {
		printf("Not found fb=%p\r\n", fb);
	}

	if (m_clientFibers.empty()) {
		GetDlgItem(IDC_CONNECT)->EnableWindow(TRUE);
		printf("All connect fibers=%d count=%d finished now!\r\n",
			m_cocurrent, m_count);
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
	GetDlgItem(IDC_START_HTTPD)->EnableWindow(FALSE);
	acl::fiber* fb = new CFiberHttpd(m_httpdAddr.c_str());
	fb->start();
	//GetDlgItem(IDC_START_HTTPD)->EnableWindow(FALSE);
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

static bool ResolveDNS(const char* name, std::vector<std::string>* addrs)
{
	struct hostent *ent = gethostbyname(name);
	if (ent == NULL) {
		printf("gethostbyname error: %s, name=%s\r\n", acl::last_serror(), name);
		return false;
	}

	for (int i = 0; ent->h_addr_list[i]; i++) {
		char* addr = ent->h_addr_list[i];
		char  ip[64];
		const char* ptr = inet_ntop(ent->h_addrtype, addr, ip, sizeof(ip));
		if (ptr) {
			addrs->push_back(ptr);
		} else {
			printf(">>>inet_ntop error\r\n");
		}
	}

	return true;
}

static void fiber_resolve(void)
{
	std::string name = "www.google.com";
	std::vector<std::string> addrs;

	// 因为需要等待线程解析完成，所以此处可使得引用方式传参
	go_wait[&] {
		if (!ResolveDNS(name.c_str(), &addrs)) {
			printf(">>>resolve DNS error, name=%s\r\n", name.c_str());
		}
	};

	printf(">>>resolve done: name=%s, result count=%zd\r\n",
		name.c_str(), addrs.size());

	for (std::vector<std::string>::const_iterator cit = addrs.begin();
		cit != addrs.end(); ++cit) {
		printf(">>>ip=%s\r\n", (*cit).c_str());
	}
}

static void fiber_one(ACL_FIBER*, void* ctx)
{
	acl::fiber_tbox<int>* box = (acl::fiber_tbox<int>*) ctx;
	acl::fiber::delay(2000);
	printf(">>>call box push\r\n");
	box->push(NULL);
}

static void fiber_main(ACL_FIBER*, void*)
{
	acl::fiber_tbox<int> box;
	acl_fiber_create(fiber_one, &box, 256000);
	printf(">>>>call box pop\r\n");
	(void) box.pop();
	printf(">>fiber_main over!\r\n");
}

void CWinFiberDlg::OnBnClickedAwaitDns()
{
	// TODO: 在此添加控件通知处理程序代码
#if 1
	go[] { fiber_resolve(); };

	//fiber_resolve();
#else
	acl_fiber_create(fiber_main, NULL, 256000);
#endif
}

void CWinFiberDlg::OnBnClickedResolve()
{
	// TODO: 在此添加控件通知处理程序代码
	std::string name = "www.baidu.com";

	// 因为 go 调用方式启动协程是异步的，所以通过 lambda 表达式传递参数时
	// 需要用拷贝方式，而不能为引用方式

	go[=] {
		struct addrinfo *res0, *res;
		int ret = getaddrinfo(name.c_str(), NULL, NULL, &res0) ;
		if (ret != 0) {
			printf("getaddrinfo error=%d, %s, domain=%s\r\n",
				ret, gai_strerrorA(ret), name.c_str());
			if (res0) {
				freeaddrinfo(res0);
			}
			return;
		}

		printf("\r\n");
		printf("getaddrinfo: domain=%s\r\n", name.c_str());
		for (res = res0; res; res = res->ai_next) {
			const void *addr;
			char ip[64];
			if (res->ai_family == AF_INET) {
				const struct sockaddr_in *in =
					(const struct sockaddr_in*) res->ai_addr;
				addr = (const void*) &in->sin_addr;
			} else if (res->ai_family == AF_INET6) {
				const struct sockaddr_in6 *in =
					(const struct sockaddr_in6*) res->ai_addr;
				addr = (const void*) &in->sin6_addr;
			} else {
				printf("Unknown ai_family=%d\r\n", res->ai_family);
				continue;
			}

			if (inet_ntop(res->ai_family, addr, ip, sizeof(ip)) != NULL) {
				printf(">>ip=%s\r\n", ip);
			} else {
				printf(">>inet_ntop error=%s\r\n", acl::last_serror());
			}
		}
	};

	go[=] {
		struct hostent *ent = gethostbyname(name.c_str());
		if (ent == NULL) {
			printf("gethostbyname error, domain=%s\r\n", name.c_str());
			return;
		}

		printf("\r\n");
		printf("gethostbyname: domain=%s\r\n", name.c_str());
		printf(">>h_name=%s, h_length=%d, h_addrtype=%d\r\n",
			ent->h_name, ent->h_length, ent->h_addrtype);
		for (int i = 0; ent->h_addr_list[i]; i++) {
			char *addr = ent->h_addr_list[i];
			char ip[64];
			const char *ptr = inet_ntop(ent->h_addrtype, addr, ip, sizeof(ip));
			if (ptr) {
				printf(">>ip=%s\r\n", ip);
			} else {
				printf(">>inet_ntop error\r\n");
			}
		}
	};

	go[=] {
		std::vector<std::string> addrs;
		go_wait[&] {
			if (!ResolveDNS(name.c_str(), &addrs)) {
				printf("ResolveDNS error\r\n");
			}
		};

		printf("\r\n");
		printf(">>>resolve done: name=%s, result count=%zd\r\n",
			name.c_str(), addrs.size());

		for (std::vector<std::string>::const_iterator cit = addrs.begin();
			 cit != addrs.end(); ++cit) {
			printf(">>>ip=%s\r\n", (*cit).c_str());
		}
	};
}
