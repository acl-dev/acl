锘�// WinEchodDlg.cpp : 瀹炵幇鏂囦欢
//

#include "stdafx.h"
#include "FiberListener.h"
#include "FiberSleep.h"
#include "FiberConnect.h"
#include "WinEchod.h"
#include "WinEchodDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 鐢ㄤ簬搴旂敤绋嬪簭鈥滃叧浜庘€濊彍鍗曢」鐨� CAboutDlg 瀵硅瘽妗�

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 瀵硅瘽妗嗘暟鎹�
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 鏀寔

// 瀹炵幇
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


// CWinEchodDlg 瀵硅瘽妗�

CWinEchodDlg::CWinEchodDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CWinEchodDlg::IDD, pParent)
	, m_dosFp(NULL)
	, m_listenPort(9001)
	, m_listenIP(_T("127.0.0.1"))
	, m_listenAddr("127.0.0.1:9001")
	, m_fiberListen(NULL)
	, m_cocurrent(1)
	, m_count(100)
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
	DDX_Text(pDX, IDC_COCURRENT, m_cocurrent);
	DDV_MinMaxUInt(pDX, m_cocurrent, 1, 1000);
	DDX_Text(pDX, IDC_COUNT, m_count);
	DDV_MinMaxUInt(pDX, m_count, 1, 1000);
}

BEGIN_MESSAGE_MAP(CWinEchodDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_OPEN_DOS, &CWinEchodDlg::OnBnClickedOpenDos)
	ON_BN_CLICKED(IDC_LISTEN, &CWinEchodDlg::OnBnClickedListen)
	ON_BN_CLICKED(IDC_CREATE_TIMER, &CWinEchodDlg::OnBnClickedCreateTimer)
	ON_BN_CLICKED(IDC_CONNECT, &CWinEchodDlg::OnBnClickedConnect)
	ON_BN_CLICKED(IDOK, &CWinEchodDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// CWinEchodDlg 娑堟伅澶勭悊绋嬪簭

BOOL CWinEchodDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 灏嗏€滃叧浜�...鈥濊彍鍗曢」娣诲姞鍒扮郴缁熻彍鍗曚腑銆�

	// IDM_ABOUTBOX 蹇呴』鍦ㄧ郴缁熷懡浠よ寖鍥村唴銆�
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

	// 璁剧疆姝ゅ璇濇鐨勫浘鏍囥€傚綋搴旂敤绋嬪簭涓荤獥鍙ｄ笉鏄璇濇鏃讹紝妗嗘灦灏嗚嚜鍔�
	//  鎵ц姝ゆ搷浣�
	SetIcon(m_hIcon, TRUE);			// 璁剧疆澶у浘鏍�
	SetIcon(m_hIcon, FALSE);		// 璁剧疆灏忓浘鏍�

	//ShowWindow(SW_MAXIMIZE);

	// TODO: 鍦ㄦ娣诲姞棰濆鐨勫垵濮嬪寲浠ｇ爜
	InitFiber();  // 鍒濆鍖栧崗绋嬬幆澧�
	return TRUE;  // 闄ら潪灏嗙劍鐐硅缃埌鎺т欢锛屽惁鍒欒繑鍥� TRUE
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

// 濡傛灉鍚戝璇濇娣诲姞鏈€灏忓寲鎸夐挳锛屽垯闇€瑕佷笅闈㈢殑浠ｇ爜
//  鏉ョ粯鍒惰鍥炬爣銆傚浜庝娇鐢ㄦ枃妗�/瑙嗗浘妯″瀷鐨� MFC 搴旂敤绋嬪簭锛�
//  杩欏皢鐢辨鏋惰嚜鍔ㄥ畬鎴愩€�

void CWinEchodDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 鐢ㄤ簬缁樺埗鐨勮澶囦笂涓嬫枃

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 浣垮浘鏍囧湪宸ヤ綔鍖虹煩褰腑灞呬腑
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 缁樺埗鍥炬爣
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//褰撶敤鎴锋嫋鍔ㄦ渶灏忓寲绐楀彛鏃剁郴缁熻皟鐢ㄦ鍑芥暟鍙栧緱鍏夋爣
//鏄剧ず銆�
HCURSOR CWinEchodDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CWinEchodDlg::OnBnClickedOpenDos()
{
	// TODO: 鍦ㄦ娣诲姞鎺т欢閫氱煡澶勭悊绋嬪簭浠ｇ爜
	if (m_dosFp == NULL)
	{
		//GetDlgItem(IDC_OPEN_DOS)->EnableWindow(FALSE);
		UpdateData();
		AllocConsole();
		m_dosFp = freopen("CONOUT$","w+t",stdout);
		printf("DOS opened now, listen=%s:%d\r\n",
			m_listenIP.GetString(), m_listenPort);
		CString info(_T("鍏抽棴 DOS 绐楀彛 "));
		GetDlgItem(IDC_OPEN_DOS)->SetWindowText(info);
	}
	else
	{
		fclose(m_dosFp);
		m_dosFp = NULL;
		FreeConsole();
		CString info(_T("鎵撳紑 DOS 绐楀彛"));
		GetDlgItem(IDC_OPEN_DOS)->SetWindowText(info);
	}
}

// UNICODE 杞瀛楃
void CWinEchodDlg::Uni2Str(const CString& in, acl::string& out)
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

void CWinEchodDlg::InitFiber(void)
{
	// 璁剧疆鍗忕▼璋冨害鐨勪簨浠跺紩鎿庯紝鍚屾椂灏嗗崗绋嬭皟搴﹁涓鸿嚜鍔ㄥ惎鍔ㄦā寮�
	acl::fiber::init(acl::FIBER_EVENT_T_WMSG, true);
	// HOOK ACL 搴撲腑鐨勭綉缁� IO 杩囩▼
	acl::fiber::acl_io_hook();
}

void CWinEchodDlg::OnBnClickedListen()
{
	// TODO: 鍦ㄦ娣诲姞鎺т欢閫氱煡澶勭悊绋嬪簭浠ｇ爜
	if (m_fiberListen == NULL)
	{
		UpdateData();
		Uni2Str(m_listenIP, m_listenAddr);
		m_listenAddr.format_append(":%d", m_listenPort);
		if (m_listen.open(m_listenAddr) == false)
		{
			printf("listen %s error %s\r\n", m_listenAddr.c_str());
			return;
		}
		CString info(_T("鍋滄鐩戝惉"));
		GetDlgItem(IDC_LISTEN)->SetWindowText(info);

		printf("listen %s ok\r\n", m_listenAddr.c_str());
		m_fiberListen = new CFiberListener(m_listen);
		m_fiberListen->start();
	}
	else if (acl::fiber::scheduled())
	{
		GetDlgItem(IDC_LISTEN)->EnableWindow(FALSE);
		m_fiberListen->kill();
		printf("listening fiber was killed\r\n");
		m_listen.close();
		printf("listening socket was closed\r\n");
		m_fiberListen = NULL;
		printf("fiber schedule stopped!\r\n");
		CString info(_T("寮€濮嬬洃鍚�"));
		GetDlgItem(IDC_LISTEN)->SetWindowText(info);
		GetDlgItem(IDC_LISTEN)->EnableWindow(TRUE);
	}
	else
	{
		GetDlgItem(IDC_LISTEN)->EnableWindow(FALSE);
		m_listen.close();
		printf("listening socket was closed\r\n");
		m_fiberListen = NULL;
		CString info(_T("寮€濮嬬洃鍚�"));
		GetDlgItem(IDC_LISTEN)->SetWindowText(info);
		GetDlgItem(IDC_LISTEN)->EnableWindow(TRUE);
	}
}

void CWinEchodDlg::OnBnClickedCreateTimer()
{
	// TODO: 鍦ㄦ娣诲姞鎺т欢閫氱煡澶勭悊绋嬪簭浠ｇ爜
	acl::fiber* fb = new CFiberSleep;
	fb->start();
}

void CWinEchodDlg::OnBnClickedConnect()
{
	// TODO: 鍦ㄦ娣诲姞鎺т欢閫氱煡澶勭悊绋嬪簭浠ｇ爜
	UpdateData();
	GetDlgItem(IDC_CONNECT)->EnableWindow(FALSE);

	UINT n = m_cocurrent;
	for (UINT i = 0; i < n; i++)
	{
		acl::fiber* fb = new CFiberConnect(
			*this, m_listenAddr.c_str(), m_count);
		fb->start();
	}
}

void CWinEchodDlg::OnFiberConnectExit(void)
{
	if (--m_cocurrent == 0)
	{
		GetDlgItem(IDC_CONNECT)->EnableWindow(TRUE);
		printf("All connect fibers finished now!\r\n");
	}
}


void CWinEchodDlg::OnBnClickedOk()
{
	// TODO: 鍦ㄦ娣诲姞鎺т欢閫氱煡澶勭悊绋嬪簭浠ｇ爜
	if (acl::fiber::scheduled())
	{
		acl::fiber::schedule_stop();
	}
	if (m_cocurrent > 0)
	{
		printf("there %d fibers connected\r\n", m_cocurrent);
	}

	CDialogEx::OnOK();
}
