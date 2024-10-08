// SimpleWin.cpp : 定义应用程序的入口点。
//

#include "pch.h"
#include "framework.h"
#include "FiberServer.h"
#include "FiberConnect.h"
#include "SimpleWin.h"

#define MAX_LOADSTRING 100

// 全局变量:
HINSTANCE hInst;                                // 当前实例
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名
CFiberServer* fiber_server = NULL;              // 监听协程对象
FILE* dos_handle = NULL;                        // DOS 窗口句柄
const char* server_addr = "127.0.0.1";
int server_port = 8088;

// 此代码模块中包含的函数的前向声明:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: 在此处放置代码。}

    // 初始化全局字符串
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_SIMPLEWIN, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // 执行应用程序初始化:
    if (!InitInstance (hInstance, nCmdShow)) {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SIMPLEWIN));

    MSG msg;

    // 主消息循环:
    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}

//
//  函数: MyRegisterClass()
//
//  目标: 注册窗口类。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SIMPLEWIN));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_SIMPLEWIN);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

static bool InitSocket(void)
{
    WORD version = 0;
    WSADATA data;

    FillMemory(&data, sizeof(WSADATA), 0);

    version = MAKEWORD(2, 0);

    if (WSAStartup(version, &data) != 0) {
        return false;
    }
    if (LOBYTE(data.wVersion) != 2 || HIBYTE(data.wVersion) != 0) {
        WSACleanup();
        return false;
    }

    return true;
}

//
//   函数: InitInstance(HINSTANCE, int)
//
//   目标: 保存实例句柄并创建主窗口
//
//   注释:
//
//        在此函数中，我们在全局变量中保存实例句柄并
//        创建和显示主程序窗口。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // 将实例句柄存储在全局变量中

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd) {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   HMENU hMenu = GetMenu(hWnd);
   EnableMenuItem(hMenu, IDM_STOP_LISTENER, MF_DISABLED);
   EnableMenuItem(hMenu, IDM_OPEN_DOS, MF_ENABLED);
   EnableMenuItem(hMenu, IDM_CLOSE_DOS, MF_DISABLED);

   // 设置协程调度的事件引擎，同时将协程调度设为自动启动模式，不能在进程初始化时启动
   // 协程调试器，必须在界面消息引擎正常运行后才启动协程调度器！
   acl::fiber::init(acl::FIBER_EVENT_T_WMSG, true);
   acl::fiber::winapi_hook();

   InitSocket();

   return TRUE;
}

static bool GetHostByname(const char* name, std::vector<std::string>* addrs)
{
    printf("\r\n");
    printf(">>>call gethostbyname, name=%s, thread=%d\r\n", name, GetCurrentThreadId());
    struct hostent* ent = gethostbyname(name);
    if (ent == NULL) {
        printf("gethostbyname error, name=%s, thread=%d\r\n", name, GetCurrentThreadId());
        return false;
    }

    for (int i = 0; ent->h_addr_list[i]; i++) {
        char* addr = ent->h_addr_list[i];
        char  ip[64];
        const char* ptr = inet_ntop(ent->h_addrtype, addr, ip, sizeof(ip));
        if (ptr) {
            addrs->push_back(ptr);
        }
        else {
            printf(">>>inet_ntop error\r\n");
        }
    }

    return true;
}

static void GetAddrInfo(const std::string& name)
{
    struct addrinfo* res0, * res;
    int ret = getaddrinfo(name.c_str(), NULL, NULL, &res0);
    if (ret != 0) {
        printf("getaddrinfo error=%d, %s, domain=%s\r\n",
            ret, gai_strerrorA(ret), name.c_str());
        if (res0) {
            freeaddrinfo(res0);
        }
        return;
    }

    printf("\r\n");
    printf("getaddrinfo: domain=%s, thread=%d\r\n", name.c_str(), GetCurrentThreadId());
    for (res = res0; res; res = res->ai_next) {
        const void* addr;
        char ip[64];
        if (res->ai_family == AF_INET) {
            const struct sockaddr_in* in =
                (const struct sockaddr_in*)res->ai_addr;
            addr = (const void*)&in->sin_addr;
        } else if (res->ai_family == AF_INET6) {
            const struct sockaddr_in6* in =
                (const struct sockaddr_in6*)res->ai_addr;
            addr = (const void*)&in->sin6_addr;
        } else {
            printf("Unknown ai_family=%d\r\n", res->ai_family);
            continue;
        }

        if (inet_ntop(res->ai_family, addr, ip, sizeof(ip)) != NULL) {
            printf(">>ip=%s\r\n", ip);
        } else {
            printf(">>inet_ntop error\r\n");
        }
    }
}

//
//  函数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目标: 处理主窗口的消息。
//
//  WM_COMMAND  - 处理应用程序菜单
//  WM_PAINT    - 绘制主窗口
//  WM_DESTROY  - 发送退出消息并返回
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // 分析菜单选择:
            switch (wmId) {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            case IDM_START_LISTENER:
                {
                    fiber_server = new CFiberServer();
                    if (fiber_server->BindAndListen(server_port, server_addr)) {
                        HMENU hMenu = GetMenu(hWnd);
                        EnableMenuItem(hMenu, IDM_START_LISTENER, MF_DISABLED);
                        EnableMenuItem(hMenu, IDM_STOP_LISTENER, MF_ENABLED);
                        printf(">>>Start fiber to listen on %s:%d, thread=%d\r\n",
                            server_addr, server_port, GetCurrentThreadId());
                        fiber_server->start();
                    } else {
                        delete fiber_server;
                        fiber_server = NULL;
                    }
                }
                break;
            case IDM_STOP_LISTENER:
                {
                    if (fiber_server) {
                        fiber_server->kill();
                        fiber_server = NULL;
                        printf(">>>Listener fiber stopped!\r\n");
                    }
                    HMENU hMenu = GetMenu(hWnd);
                    EnableMenuItem(hMenu, IDM_START_LISTENER, MF_ENABLED);
                    EnableMenuItem(hMenu, IDM_STOP_LISTENER, MF_DISABLED);
                }
                break;
            case IDM_START_CONNECT:
                for (int i = 0; i < 10; i++) {
                    go[=] {
                        CFiberConnect client(server_addr, server_port, 10000);
                        client.Start();
                    };
                }
                break;
            case IDM_RESOLVE_THREAD:
                go[=]{
                    const std::string name = "www.google.com";
                    std::vector<std::string> addrs;
                    go_wait[&] {
                        if (!GetHostByname(name.c_str(), &addrs)) {
                            printf(">>>resolve DNS error, name=%s\r\n", name.c_str());
                        }
                    };

                    printf(">>>resolve done: name=%s, result count=%zd, thread=%d\r\n",
                        name.c_str(), addrs.size(), GetCurrentThreadId());

                    for (std::vector<std::string>::const_iterator cit = addrs.begin();
                         cit != addrs.end(); ++cit) {
                        printf(">>>ip=%s\r\n", (*cit).c_str());
                    }
                };
                break;
            case IDM_RESOLVE_FIBER:
                {
                    std::string name = "www.google.com";
                    go[=]{
                        GetAddrInfo(name);
                    };
                }
                break;
            case IDM_OPEN_DOS:
                if (dos_handle == NULL) {
                    AllocConsole();
                    dos_handle = _wfreopen(_T("CONOUT$"), _T("w+t"), stdout);
                    if (dos_handle != NULL) {
                        HMENU hMenu = GetMenu(hWnd);
                        EnableMenuItem(hMenu, IDM_OPEN_DOS, MF_DISABLED);
                        EnableMenuItem(hMenu, IDM_CLOSE_DOS, MF_ENABLED);
                        printf("DOS opened, current thread=%d!\r\n", GetCurrentThreadId());
                    }
                }
                break;
            case IDM_CLOSE_DOS:
                if (dos_handle) {
                    fclose(dos_handle);
                    dos_handle = NULL;
                    FreeConsole();
                    HMENU hMenu = GetMenu(hWnd);
                    EnableMenuItem(hMenu, IDM_OPEN_DOS, MF_ENABLED);
                    EnableMenuItem(hMenu, IDM_CLOSE_DOS, MF_DISABLED);
                }
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: 在此处添加使用 hdc 的任何绘图代码...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        acl::fiber::schedule_stop();
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// “关于”框的消息处理程序。
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message) {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
