# acl -- one advanced C/C++ lib for UNIX and WINDOWS

## 描述
acl 工程是一个跨平台（支持LINUX，WIN32，Solaris，MacOS，FreeBSD）的网络通信库及服务器编程框架，同时提供更多的实用功能库。通过该库，用户可以非常容易地编写支持多种模式(多线程、多进程、非阻塞、触发器、UDP方式)的服务器程序，WEB 应用程序，数据库应用程序。此外，该库还提供了常见应用的客户端通信库（如：HTTP、SMTP、ICMP、memcache、beanstalk），常见流式编解码库：XML/JSON/MIME/BASE64/UUCODE/QPCODE/RFC2047 etc。

本工程主要包含 5 个库及大量示例。5 个库的说明如下：
-    1) lib_acl: 该库是最基础的库，其它 4 个库均依赖于该库; 该库以 C 语言实现。
-    2) lib_protocol: 该库主要实现了 http 协议及 icmp/ping 协议; 该库以 C 语言实现。
-    3) lib_acl_cpp: 该库用 C++ 语言封装了 lib_acl/lib_protocol 两个库，同时增加了一些其它有价值的功能应用。
-    4) lib_dict: 该库主要实现了 KEY-VALUE 的字典式存储库，该库另外还依赖于 BDB, CDB 以及 tokyocabinet 库。
-    5) lib_tls: 该库封装了 openssl 库，使 lib_acl 的通信模式可以支持 ssl。

## 平台支持及编译
整个工程目前支持 Linux(AS4,5,6, CS4,5,6), Windows, MacOS, FreeBSD, Solaris。
* Linux/UNIX: 编译器为 gcc，直接在终端命令行方式下分别进入 lib_acl/lib_protocol/lib_acl_cpp/lib_dict/lib_tls 目录下，运行 make 命令即可。
* Windows: 可以用 VC2003/VC2008/VC2010/VC2012 进行编译。(如果需要用 VC6/VC2005 编译，可以参考 VC2003 的编译条件)。

当在 WIN32 环境下使用动态库时有几点需要注意：
* 使用 lib_acl 的动态库时，需要在用户的工程预定义: ACL_DLL;
* 使用 lib_protocol 动态库中的 HTTP 库或 ICMP 库时，需要在工程中预定义 HTTP_DLL 或 ICMP_DLL;
* 使用 lib_acl_cpp 的动态库时，需要在工程中预定义 ACL_CPP_DLL，如果您使用用 VC2003 编译环境则还需要预定义 VC2003;
* 使用 lib_dict 的动态库时，需要在工程中预定义 DICT_DLL;
* 使用 lib_tls 的动态库时，需要在工程中预定义 TLS_DLL。

## 目录结构说明

### lib_acl
-    1 init : 主要用于初始化 acl 基础库
-    2 stdlib : 是一些比较基础的功能函数库，在 stdlib/ 根目录下主要包括一些有关日志记录、网络/文件流处理、VSTRING缓冲操作等功能函数；在 stdlib/ 下还有二级目录，如下：
-    2.1 common : 该目录主要为一些常用的数据结构及算法的功能函数库，象哈希表、链表、队列、动态数组、堆栈、缓存、平衡二叉树、模式匹配树等；
-    2.2 memory : 该目录主要包含与内存操作相关的函数库，象内存基础分配与校验、内存池管理、内存切片管理等；
-    2.3 filedir : 该目录主要包含与目录遍历、目录创建等相关的库；
-    2.4 configure : 该目录主要包含配置文件的分析库；
-    2.5 iostuff : 该目录主要包含一些常用的IO操作的函数库，象读/写超时、设置IO句柄的阻塞模式等；
-    2.6 string : 该目录主要包含一些常用的字符串操作的库，提供了比标准C更灵活高效的字符串操作功能；
-    2.7 debug : 主要用于协助调试内存的泄露等功能；
-    2.8 sys : 主要是与不同操作系统平台相关的API的封装函数库；
-    **3 net: 是与网络操作相关的函数库，包含网络监听、网络连接、DNS查询、套接口参数设置等功能；**
-    3.1 connect : 主要是与网络连接相关的函数库，包含网络连接、域套接口连接等；
-    3.2 listen : 主要是与网络监听相关的函数库，包含网络监听、域套接口监听等；
-    3.3 dns : 主要是与DNS域名查询相关的函数库，包含对 gethostbyname 等接口的封装、按RFC1035标准直接发送UDP包方式进行查询等功能；
-    **4 event : 主要封装了 select/poll/epoll/iocp/win message/kqueue/devpoll 等系统API接口，使处理网络事件更加灵活、高效、简单，另外还包含定时器接口，acl 中的很多网络应用都会用到这些接口，象 aio、master 等模块；**
-    **5 aio : 主要包含网络异步操作的功能函数，该套函数库在处理高并发时有非常高的效率，而且提供了比基础API更为高级的调用方式，比使用象 libevent 之类的函数库更为简单，而且是线程安全的；**
-    6 msg : 主要包含了基于线程的消息事件及基于网络的消息事件功能；
-    7 thread : 主要是封装了各个OS平台下的基础线程API，使对外接口保持一致性，消除了平台的差异性，同时还提供了半驻留线程池的函数库，以及对于线程局部变量的扩展；
-    8 db : 主要是一些与数据库有关的功能库，定义了一个通用的数据库连接池的框架（并且实现了mysql的连接池实例）；一个简单的内存数据库（由哈希表、链表、平衡二叉树组合而成）；ZDB数据存储引擎，这是一个高效的基于数字键的存储引擎；
-    9 proctl : win32 平台下父子进程控制功能库；
-    10 code : 常见编码函数库，包括 base64编解码、URL编解码以及一些汉字字符集编码等；
-    11 unit_test : 包含有关进行 C 语言单元测试的功能库；
-    12 xml: 是一个流式的 xml 解析器及构造器，可以支持阻塞及阻塞式网络通信；
-    13 json: 是一个流式的 json 解析器及构造器，可以支持阻塞及阻塞式网络通信；
-    **14 master: 是在 UNIX 环境下支持多种服务器模式的服务器框架，目前主要支持多进程模式、多进程多线程模式、多进程非阻塞模式、多进程触发器模式及 UDP 通信模式；**

### lib_protocol
-    1 http: HTTP 协议相关的库，支持 HTTP/1.1，通讯方式支持同步/异步方式
-    2 icmp: icmp/ping 协议库，支持同步/异步通信方式
-    3 smtp: 邮件客户端发信协议库

### lib_acl_cpp
-    1 stdlib: 主要包含字符串处理类(string)，xml/json 解析库，zlib 压缩库(依赖于 zlib 库), 日志记录类, 字符集转码(在UNIX环境下需要 iconv 库), memcached 客户库, 互斥类(支持线程锁、文件锁);
-    **2 stream: 支持网络流/文件流，支持阻塞/非阻塞两种通信方式，在非阻塞模式下支持 select/poll/epoll/iocp/win32 message/kqueue/devpoll；支持 ssl 加密传输(阻塞及非阻塞方式，需要 polarssl库);**
-    3 ipc: 在非阻塞通信方式，提供了阻塞模块与非阻塞模块整合的方式;
-    **4 http: 比较完整的 HTTP 通信库及协议解析库，支持客户端及服务端模式，支持 ssl/gzip 传输方式; 支持类似于 Java HttpServlet 方式的大部分接口，方便编写 CGI 及 WEB 服务器程序；**
-    5 db: 封装了 MYSQL/SQLITE 库，支持数据库连接池；
-    6 hsocket: 实现了完整的 handler-socket 客户端通信库；
-    **7 mime: 支持完整的与邮件编码相关的库(邮件的 rfc2045-rfc2047/rfc822/base64/uucode 编码及解码库).**
-    **8 master: 封装了 C 库的服务器框架库**
-    9 beanstalk: 消息队列应用 beanstalkd 的客户端通信库
-    10 connpool: 通用的连接池库
-    11 hscoket: mysql 插件 handle-socket 的客户端通信库
-    12 memcache: memcached 应用的客户端库
-    13 queue: 磁盘文件队列操作库
-    14 ipc: 阻塞/非阻塞通信整合库
-    15 session: HTTP 会话库

### 示例
- acl 项目有大量的测试及应用示例，主要有三个示例集合如下：
#### acl/samples：该目录下的例子主要是基于 lib_acl 及 lib_protocol 两个库的例子
-    1.1 acl: 打印当前 acl 库版本号程序
-    1.2 aio/client: 非阻塞 io 客户端
-    1.3 aio/server: 非阻塞 io 服务器
-    1.4 base64: base64 编/解码程序
-    1.5 btree: 二叉树程序
-    1.6 cache: 对象缓存程序
-    1.7 cache2: 对象缓存程序
-    1.8 cgi_env: CGI 程序用来获得 CGI 环境变量
-    1.9 chunk_chain: 二分块数据程序
-    1.10 configure: 配置文件处理程序
-    1.11 connect: 网络客户端连接程序
-    1.12 dbpool: 数据库连接池程序
-    1.13 dlink: 二分块查找算法程序
-    1.14 dns: 域名查询程序
-    1.15 dns_req: 域名查询程序
-    1.16 event: 事件引擎程序
-    1.17 fifo: 先进先出算法程序
-    1.18 file: 文件流处理程序
-    1.19 file_fmt: 将 UNIX 下的 \n 转为 WIN32 下的 \r\n 或者反向转换程序
-    1.20 FileDir: win32 下目录操作程序
-    1.21 flock: 文件锁处理程序
-    1.22 gc: 内存自动回收程序
-    1.23 htable: 哈希表处理程序
-    1.24 http/header: http 客户端程序
-    1.25 http/url_get1: 网页下载客户端程序
-    1.26 http/url_get2: 网页下载客户端程序
-    1.27 http/url_get3: 网页下载客户端程序
-    1.28 http_aio: 简单的 HTTP 异步下载程序
-    1.29 http_client: WIN32 下 HTTP 客户端程序
-    1.30 http_probe: HTTP 客户端程序
-    1.31 ifconf: 获取本机网卡的程序
-    1.32 iplink: IP 地址段管理程序
-    1.33 iterator: C 方式进行遍历的程序
-    1.34 json: json 对象处理程序
-    1.35 json2: json 对象处理程序
-    1.36 json3: json 对象处理程序
-    1.37 jt2ft: 简体转繁体程序
-    1.38 log: 日志处理程序
-    1.39 master/aio_echo: 非阻塞回显服务器程序
-    1.40 master/aio_proxy: 非阻塞 TCP 代理程序
-    1.41 master/ioctl_echo2: 多线程回显示服务器程序
-    1.42 master/ioctl_echo3: 多线程回显示服务器程序
-    1.43 master/master_notify: 多线程服务器程序
-    1.44 master/master_threads: 多线程服务器程序
-    1.45 master/single_echo: 多进程回显示服务器程序
-    1.46 master/trigger: 触发器服务器程序
-    1.47 master/udp_echo: UDP 回显服务器程序
-    1.48 memdb: 简单的内存数据库程序
-    1.49 mempool: 内存池程序
-    1.50 mkdir: 创建多级目录程序
-    1.51 net: 简单网络程序
-    1.52 ping: 阻塞/非阻塞 PING 程序
-    1.53 pipe: 管道处理程序
-    1.54 proctl: WIN32 下父子进程程序
-    1.55 resolve: 域名解析程序
-    1.56 server: 简单的服务器程序
-    1.57 server1: 简单的服务器程序
-    1.58 slice: 内存池切片程序
-    1.59 slice_mem: 内存池切片程序
-    1.60 smtp_client: smtp 客户端发信程序
-    1.61 string: acl 字符串处理程序
-    1.62 thread: 线程程序
-    1.63 token_tree: 256 叉树程序
-    1.64 udp_clinet: UDP 客户端程序
-    1.65 udp_server: UDP 服务器程序
-    1.66 urlcode: URL 编码处理程序
-    1.67 vstream: IO 网络流处理程序
-    1.68 vstream_client: 网络客户端流程序
-    1.69 vstream_fseek: 文件流处理程序
-    1.70 vstream_fseek2: 文件流处理程序
-    1.71 vstream_popen: 管道流处理程序
-    1.71 vstream_popen2: 管道流处理程序
-    1.71 vstream_popen3: 管道流处理程序
-    1.72 vstream_server: 网络服务端程序
-    1.73 xml: XML 解析程序
-    1.74 xml2: XML 解析程序
-    1.75 zdb: 数字KEY/VALUE 文件存储引擎程序

#### acl/lib_acl_cpp/samples：该目录下的例子基本是基于 lib_acl_cpp 库写的 C++ 例子
-    2.1 aio/aio_client: 非阻塞网络客户端程序
-    2.2 aio/aio_dns: 非阻塞域名解析客户端程序
-    2.3 aio/aio_echo: 非阻塞回显服务器程序
-    2.4 aio/aio_ipc: 阻塞/非阻塞整合的网络程序
-    2.5 aio/aio_server: 非阻塞服务器程序
-    2.6 beanstalk: 队列应用 beanstalkd 的客户端程序
-    2.7 benchmark: 与性能测试相关的程序集
-    2.8 cgi: 简单的 WEB CGI 程序
-    2.9 cgi_upload: 接收上传文件的 CGI 程序
-    2.10 charset: 字符集转换程序
-    2.11 check_trigger: 检测远程 HTTP 服务器状态的触发器程序
-    2.12 connect_manager: 客户端连接池集群管理程序
-    2.13 db_service: 与数据库相关的程序
-    2.14 dbpool: 使用数据库客户端连接池的程序
-    2.15 dircopy: 目录文件拷贝程序
-    2.16 final_class: 禁止继承类程序
-    2.17 flock: 文件锁处理程序
-    2.18 fs_benchmark: 文件系统压力测试程序
-    2.19 fstream: 文件流程序
-    2.20 gui_rpc: WIN32 下阻塞过程与 WIN32 界面的消息整合的例子
-    2.21 hsclient: handle-socket 客户端程序
-    2.22 http_client: HTTP 客户端程序
-    2.23 http_client2: HTTP 客户端程序
-    2.24 http_mime: HTTP 协议的 MIME 格式处理程序
-    2.25 http_request: 使用 http_request 类的 HTTP 客户端程序
-    2.26 http_request_manager: HTTP 客户端连接池集群程序
-    2.27 http_request_pool: HTTP 客户端连接池程序
-    2.28 http_request2: 使用 http_request 类的 HTTP 客户端程序
-    2.29 http_response: 使用 http_reponse 类响应 HTTP 客户端请求的程序
-    2.30 http_server: 简单的 HTTP 服务器程序
-    2.31 http_servlet: 类似于 JAVA HttpServlet 的程序
-    2.32 http_servlet2: 类似于 JAVA HttpServlet 的程序
-    2.33 HttpClient: 简单的 HTTP 客户端程序
-    2.34 json: json 字符串对象解析程序
-    2.35 logger: 日志程序
-    2.36 master_aio: 非阻塞服务器程序
-    2.37 master_aio_proxy: 非阻塞 TCP 代理服务器程序
-    2.38 master_http_aio: 简单的非阻塞 HTTP 服务器程序
-    2.39 master_http_rpc: 阻塞/非阻塞整合的 HTTP 服务器程序
-    2.40 master_http_threads: 多线程 HTTP 服务器程序
-    2.40 master_http_threads2: 多线程 HTTP 服务器程序
-    2.41 maser_proc: 进程池服务器程序
-    2.42 master_threads: 多线程服务器程序
-    2.43 master_trigger: 触发器服务器程序
-    2.44 master_udp: UDP 通信服务器程序
-    2.45 master_udp_threads: 多线程 UDP 通信服务器程序
-    2.46 md5: md5 处理程序
-    2.47 mem_cache: memcached 客户端程序
-    2.47 memcache_pool: memcached 支持连接池的客户端程序
-    2.48 mime: 邮件 MIME 解析处理程序
-    2.49 mime_base64: MIME BASE64 格式处理程序
-    2.50 mime_qp: MIME QP 格式处理程序
-    2.51 mime_xxcode: MIME XXCODE 格式处理程序
-    2.52 mysql: mysql 客户端程序
-    2.53 mysql2: mysql 客户端程序
-    2.54 rfc822: 邮件的 RFC822 协议处理程序
-    2.55 rfc2047: 邮件的 RFC2047 协议处理程序
-    2.56 rpc_download: 采用阻塞/非阻塞整合方式进行 HTTP 下载的程序
-    2.57 scan_dir: 目录递归扫描程序
-    2.58 singleton: 单例程序
-    2.59 session: 会话程序
-    2.60 socket_client: 网络客户端程序
-    2.61 socket_stream: 网络流处理程序
-    2.62 sqlite: sqlite 数据库程序
-    2.63 ssl_aio_client: SSL 非阻塞网络客户端程序
-    2.64 ssl_clinet: SSL 阻塞网络客户端程序
-    2.65 string: 动态缓冲区处理程序
-    2.66 string2: 动态缓冲区处理程序
-    2.67 thread: 多线程程序
-    2.68 thread_client: 多线程客户端程序
-    2.69 thread_pool: 线程池程序
-    2.70 udp_client: UDP 通信客户端程序
-    2.71 url_coder: URL 编、解码程序
-    2.72 win_dbservice: 基于 WIN32 图形界面的数据库处理程序
-    2.73 winaio: 基于 WIN32 图形界面的非阻塞客户端程序
-    2.74 xml: XML 对象解析处理程序
-    2.75 zlib: 压缩格式处理程序

#### acl/app：该目录下的例子主要是一些比较实用的例子
-    **3.1 wizard: 用来生成基于 acl 服务器框架的程序模板的程序**
-    3.2 gid: 用来产生全局唯一 ID 号的服务程序（含客户端库）
-    3.3 net_tools: 用来测试网络状态的程序
-    3.4 master_dispatch: 对后端服务器分配 TCP 连接的连接均衡程序
-    3.5 jaws（目前不可用）：基于 acl 的非阻塞通信模块和 HTTP 模块写的一个简易的 HTTP 高并发服务器程序

## 其它
- WEB 站点: http://www.iteye.com
- Download: https://sourceforge.net/projects/acl
- QQ 群: 242722074
