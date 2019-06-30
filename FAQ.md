## acl库使用FAQ

* [一、基础问题](#一基础问题)
    * [1、acl 库是啥、主要包含哪些功能？](#1acl-库是啥主要包含哪些功能)
    * [2、acl 库支持哪些平台？](#2acl-库支持哪些平台)
    * [3、acl 库主要包含几个库？](#3acl-库主要包含几个库)
    * [4、acl 库有哪些外部依赖库？](#4acl-库有哪些外部依赖库)
    * [5、没有这些第三方库 acl 能否使用？](#5没有这些第三方库-acl-能否使用)
    * [6、acl 库的源码下载位置在哪儿？](#6acl-库的源码下载位置在哪儿)
* [二、编译&使用问题](#二编译使用问题)
    * [1、acl 库的编译过程？](#1acl-库的编译过程)
    * [2、Linux 平台下库的编译顺序问题？](#2linux-平台下库的编译顺序问题)
    * [3、Unix/Linux 平台下编译 acl 库时的编译选项是什么](#3unixlinux-平台下编译-acl-库时的编译选项是什么)
    * [4、请给出 Linux 平台下最简单的一个 Makefile？](#4请给出-linux-平台下最简单的一个-makefile)
    * [5、Linux 平台下找不到 libz.a 库怎么办？](#5linux-平台下找不到-libza-库怎么办)
    * [6、Linux 平台下 acl 库能打包成一个库吗？](#6linux-平台下-acl-库能打包成一个库吗)
    * [7、Linux 平台下如何使用 ssl 功能？](#7linux-平台下如何使用-ssl-功能)
    * [8、Linux 下如何使用 mysql 功能？](#8linux-下如何使用-mysql-功能)
* [三、各个功能模块的使用问题](#三各个功能模块的使用问题)
    * [（一）、网络通信](#一网络通信)
    * [（二）、HTTP 模块](#二http-模块)
        * [1. acl HTTP 服务器是否支持断点下载功能？](#1-acl-http-服务器是否支持断点下载功能)
        * [2. acl HTTP 服务器是否支持文件上传功能？](#2-acl-http-服务器是否支持文件上传功能)
        * [3. acl HTTP 模块是否支持服务器/客户端两种模式？](#3-acl-http-模块是否支持服务器客户端两种模式)
        * [4. acl HTTP 模块是否支持 websocket 通信协议？](#4-acl-http-模块是否支持-websocket-通信协议)
        * [5. acl HTTP 模块是否支持 session？](#5-acl-http-模块是否支持-session)
        * [6. HttpServletRequest 为何读不到 json 或 xml 数据体？](#6-HttpServletRequest-为何读不到-json-或-xml-数据体)
        * [7. http_request 因未设 Host 字段而出错的问题](#7-http_request-因未设-Host-字段而出错的问题)
    * [（三）、Redis 模块](#三redis-模块)
        * [1. acl redis 库是否支持集群功能？](#1-acl-redis-库是否支持集群功能)
        * [2. acl redis 库是如何划分的？](#2-acl-redis-库是如何划分的)
        * [3. acl redis 库中的哪些类对象操作是线程安全的？](#3-acl-redis-库中的哪些类对象操作是线程安全的)
        * [4. acl redis 客户库当连接断开时是否会重连？](#4-acl-redis-客户库当连接断开时是否会重连)
    * [（四）、协程模块](#四协程模块)
        * [1. 协程里面的非阻塞是什么原理的？](#1-协程里面的非阻塞是什么原理的)
        * [2. acl 协程库是否支持多线程？](#2-acl-协程库是否支持多线程)
        * [3. acl 协程库如何支持多核 CPU？](#3-acl-协程库如何支持多核-cpu)
        * [4. acl 协程里针对 mysql 客户端库的协程化是否修改了 mysql 库的源码？](#4-acl-协程里针对-mysql-客户端库的协程化是否修改了-mysql-库的源码)
        * [5. acl 协程库支持域名解析功能吗？](#5-acl-协程库支持域名解析功能吗)
        * [6. acl 协程库的系统 errno 号如何处理？](#6-acl-协程库的系统-errno-号如何处理)
        * [7. 如果启动的协程过多，会不会对于后端例如 mysql 之类服务造成并发压力？如何避免？](#7-如果启动的协程过多会不会对于后端例如-mysql-之类服务造成并发压力如何避免)
    * [（五）、服务器模块](#五服务器模块)
        * [1. 有几种服务器编程模型？均有何特点？](#1-有几种服务器编程模型均有何特点)
        * [2. acl_master 的作用是什么？支持哪些平台？](#2-acl_master-的作用是什么支持哪些平台)
        * [3. 没有 acl_master 控制管理，服务子进程是否可以单独运行？](#3-没有-acl_master-控制管理服务子进程是否可以单独运行)
        * [4. 手工模式下运行时遇到“idle timeout -- exiting, idle”怎么办？](#4-手工模式下运行时遇到idle-timeout----exiting-idle怎么办)
        * [5. acl_master 控制模式下，服务子进程如何预启动多个进程？](#5-acl_master-控制模式下服务子进程如何预启动多个进程)
        * [6. acl_master 控制模式下，如何只监听内网地址？](#6-acl_master-控制模式下如何只监听内网地址)
    * [（六）、数据库模块](#六数据库模块)
        * [1. acl 数据库客户端支持哪些数据库？](#1-acl-数据库客户端支持哪些数据库)
        * [2. acl 数据库模块如何使用？](#2-acl-数据库模块如何使用)
    * [（七）、邮件&mime模块](#七邮件mime模块)

### 一、基础问题
### 1、acl 库是啥、主要包含哪些功能？
acl 工程是一个跨平台（支持LINUX，WIN32，Solaris，MacOS，FreeBSD）的网络通信库及服务器编程框架，同时提供更多的实用功能库。通过该库，用户可以非常容易地编写支持多种模式(多线程、多进程、非阻塞、触发器、UDP方式、协程方式)的服务器程序，WEB 应用程序，数据库应用程序。此外，该库还提供了常见应用的客户端通信库（如：HTTP、SMTP、ICMP、redis、disque、memcache、beanstalk、handler socket），常见流式编解码库：XML/JSON/MIME/BASE64/UUCODE/QPCODE/RFC2047 等。
### 2、acl 库支持哪些平台？
目前主要支持 Linux/Windows/Macos/Freebsd/Solaris(x86)。
### 3、acl 库主要包含几个库？
主要包括：lib_acl（用 C 语言写的基础库）、lib_protocol（用 C 语言写的一些网络应用协议库）、lib_acl_cpp（用 C++ 语言编写，封装了 lib_acl/lib_protocol 两个库，同时增加更多实用的功能库）、 lib_fiber（用 C 语言编写的支持高性能、高并发的网络协程库）、lib_rpc（用C++语言编写的封装了 google protobuf 网络库）。
### 4、acl 库有哪些外部依赖库？
lib_acl/lib_protocol/lib_fiber 仅依赖系统基础库；lib_acl_cpp 库的 db 模块依赖于 mysql 客户端库、sqlite 库，stream 流模块依赖于 polarssl 库（该库源码附在 acl/resource 目录下），另外，在 UNIX/LINUX 平台下还需要压缩库 --- libz 库（一般 LINUX 会自带该压缩库）；lib_rpc 依赖于 protobuf 库。
### 5、没有这些第三方库 acl 能否使用？
可以。默认情况下，没有这些第三方库编译和使用 acl 库是没有问题的，只是不能使用 mysql/sqlite/ssl/protobuf 功能。
### 6、acl 库的源码下载位置在哪儿？
- github：https://github.com/acl-dev/acl/
- oschina: https://git.oschina.net/acl-dev/acl/
- sourceforge：https://sourceforge.net/projects/acl/
 
### 二、编译&使用问题
### 1、acl 库的编译过程？
acl 库的编译过程请参考：[acl 的编译与使用](http://zsxxsz.iteye.com/blog/1506554) 博客。
### 2、Linux 平台下库的编译顺序问题？
lib_acl 库是 acl 库中的基础库，其它库均依赖于该库，库的依赖顺序为：lib_protocol 依赖于 lib_acl，lib_acl_cpp 依赖于 lib_acl 和 lib_protocol，lib_fiber 依赖于 lib_acl，lib_rpc 依赖于 lib_acl/lib_protocol/lib_acl_cpp。因此当应用在连接 acl 库时，需要注意连接的顺序为：-l_acl_cpp -l_protocol -l_acl。
### 3、Unix/Linux 平台下编译 acl 库时的编译选项是什么？
在 Unix/Linux 平台下编译 acl 库时需要指明 gcc 的编译选项，acl 库自带的 Makefile 会自动识别操作系统而选择不同的编译选项，下面列出不同 Unix 平台的不同编译选项（当前版本已自动识别系统类型，无需添加如下编译选项）：
- Linux 平台：-DLINUX2
- MacOS 平台：-DMACOSX
- FreeBSD 平台：-DFREEBSD
- Solaris(x86) 平台：-DSUNOS5  

### 4、请给出 Linux 平台下最简单的一个 Makefile？
下面是使用 acl 库的最简单的编译选项（因为排版问题，当拷贝下面内容至 Makefile 时，需要注意将每行前空格手工转成 TAB 键）：
~~~
fiber: main.o
	g++ -o fiber main.o \
		-L./lib_fiber/lib -lfiber_cpp \
		-L./lib_acl_cpp/lib -l_acl_cpp \
		-L./lib_protocol/lib -l_protocol \
		-L./lib_acl/lib -l_acl \
		-L./lib_fiber/lib -lfiber \
		-lz -lpthread -ldl
main.o: main.cpp
	g++ -O3 -Wall -c main.cpp -DLINUX2 \
		-I./lib_acl/include \
		-I./lib_acl_cpp/include \
		-I./lib_fiber/cpp/include \
		-I./lib_fiber/c/include
~~~
### 5、Linux 平台下找不到 libz.a 库怎么办？
一般 Unix/Linux 平台下系统会自带 libz.a 或 libz.so 压缩库，如果找不到该库，则可以在线安装或采用编译安装 zlib 库，针对 Centos 和 Ubuntu 可分别通过以下方式在线安装（均需切换至 root 身份）：
- Centos：yum install zlib-devel
- Ubuntu：apt-get install zlib1g.dev

### 6、Linux 平台下 acl 库能打包成一个库吗？
可以。在 acl 目录下运行：make build_one 则可以将 lib_acl/lib_protocol/lib_acl_cpp 打包成一个完整的库：lib_acl.a/lib_acl.so，则应用最终使用时可以仅连接这一个库即可。

### 7、Linux 平台下如何使用 ssl 功能？
目前 acl 中的 lib_acl_cpp C++ 库通过集成 polarssl 支持 ssl 功能，所支持的 polarssl 源码的下载位置：https://github.com/acl-dev/third_party, 老版本 acl 通过静态连接 libpolarssl.a 实现对 ssl 的支持，当前版本则是通过动态加载 libpolarssl.so 方式实现了对 ssl 的支持，此动态支持方式更加灵活方便，无须特殊编译条件，也更为通用。
#### 7.1、老版本 acl 对 ssl 的支持方式
如果使用上面统一的 acl 库，则可以在 acl 根目录下编译时运行：make build_one polarssl=on；如果使用三个库：lib_acl.a，lib_protocol.a，lib_acl_cpp.a，则在编译前需要先指定环境变量：export ENV_FLAGS=HAS_POLARSSL，然后分别编译这三个库；解压 polarssl-1.2.19-gpl.tgz，然后进入 polarssl-1.2.19 目录运行：make 编译后在 polarssl-1.2.19/library 目录得到 libpolarssl.a 库；最后在编译应用时将 libpolarssl.a 连接进你的工程中即可。
#### 7.2、当前版本 acl 对 ssl 的支持方式
- 首先下载解压 polarssl 库后进入polarssl-1.2.19 目录，运行 make lib SHARED=yes，在library 目录下会生成 libpolarssl.so 动态库;
- 在 acl 根目录下运行 make build_one，则会将 acl 的三个基础库：libacl.a, libprotocol.a, libacl_cpp.a 合成 libacl_all.a 一个静态库，将 libacl.so, libprotocol.so, lib_acl_cpp.so 合成 libacl_all.so 一个动态库;
- 当程序启动时添加代码：acl::polarssl_conf::set_libpath("libpolarssl.so"); 其中的路径根据实际位置而定，这样 acl 模块在需要 ssl 通信时会自动切换至 ssl 方式。

### 8、Linux 下如何使用 mysql 功能？
lib_acl_cpp 库是以动态加载方式加载 mysql 动态库的，所以在编译 lib_acl_cpp 时，mysql 功能就已经被编译进去 acl库中了。用户仅需要将 mysql 动态库通过函数 acl::db_handle::set_loadpath 注册进 acl 库中即可；至于 mysql 客户端库，用户可以去 mysql 官方下载或在 acl/resource 目录下编译 mysql-connector-c-6.1.6-src.tar.gz。
 
### 三、各个功能模块的使用问题
### （一）、网络通信
### （二）、HTTP 模块
#### 1. acl HTTP 服务器是否支持断点下载功能？
支持。acl HTTP 模块支持断点续传功能，一个支持断点下载的服务器示例参照：acl\app\wizard_demo\httpd_download。

#### 2. acl HTTP 服务器是否支持文件上传功能？
支持。参考示例：acl\app\wizard_demo\httpd_upload。

#### 3. acl HTTP 模块是否支持服务器/客户端两种模式？
支持。目前 acl 的 HTTP 协议模块同时支持客户端及服务端模式，即你既可以使用 acl HTTP 编写客户端程序，又可以编写服务器程序，其中 acl 中的 http_request/http_request_pool/http_request_manager 类用来编写客户端程序，http_response/HttpServlet/HttpServletRequest/HttpServeletResponse 用来编写服务器程序。

#### 4. acl HTTP 模块是否支持 websocket 通信协议？
支持。可以参考示例：lib_acl_cpp\samples\websocket。

#### 5. acl HTTP 模块是否支持 session？
支持。acl HTTP 模块当用在服务器编程时支持 session 存储，目前支持使用 memcached 或 redis 存储 session 数据。
 
#### 6. HttpServletRequest 为何读不到 json 或 xml 数据体
当 HTTP 客户端请求的数据体为 json 或 xml 时，默认情况下从 acl::HttpServletRequest 对象中是读不到 json/xml 数据的，主要原因在于 HttpServletRequest 内置了自动读取并解析 json/xml/x-www-form-urlencoded 类型数据的功能，使用者只需直接获取解析后的对象即可，如针对 json 类数据体：

```c++
void get_json(acl::HttpServletRequest& req)
{
	acl::json* json = req.getJson();
	...
}
```

如果应用想自己读取并解析 json 数据，则需要在调用 acl::HttpServlet::setParseBody(false)，禁止 acl::HttpServletRequest 类对象内部自动读取数据。

#### 7. http_request 因未设 Host 字段而出错的问题
在使用 acl::http_request 类对象访问标准 WEB 服务器（如：nginx）时，如果没有设置 HTTP 请求头中的 Host 字段，nginx 会返回 400 错误，主要是 HTTP/1.1 协议要求 HTTP 客户端必须设置 Host 字段，方法如下：

```c++
bool http_client(void)
{
	acl::http_request req("www.sina.com.cn:80");
	acl::http_header& hdr = req.request_header();
	hdr.set_url("/").set_host("www.sina.com.cn");
	if (!req.request(NULL, 0)) {
		return false;
	}
	acl::string body;
	if (req.get_body(body)) {
		printf("%s\r\n", body.c_str());
	}
	
	... 
}

```

### （三）、Redis 模块
#### 1. acl redis 库是否支持集群功能？
答案：是，acl redis 客户端库同时支持集群和单机方式的 redis-server。

#### 2. acl redis 库是如何划分的？
acl redis 客户端库主要分为两类：命令类和连接类：
- **命令类主要有**：redis_key, redis_string, redis_hash, redis_list, redis_set, redis_zset, redis_cluster, redis_geo, redis_hyperloglog, redis_pubsub, redis_transaction, redis_server, redis_script, 这些类都继承于基类 redis_command，同时子类 redis 又继承了所有这些命令类，以便于用户可以直接使用 acl::redis 操作所有的 redis 客户端命令；
- **连接类主要有**：redis_client, redis_client_pool, redis_client_cluster，命令类对象通过这些连接类对象与 redis-server 进行交互，redis_client 为单连接类，redis_client_pool 为连接池类，这两个类仅能在非集群模式的 redis-server 环境中使用，不支持 redis-server 的集群模式，必须使用 redis_client_cluster 连接集群模式的 redis-server，同时 redis_client_cluster 也兼容非集群模式的连接。

#### 3. acl redis 库中的哪些类对象操作是线程安全的？
acl redis 库中的所有命令类对象及 redis_client 单连接类对象不能同时被多个线程使用（就象 std::string 一样不能跨线程使用）；redis_client_pool，redis_client_cluster 两个连接类对象是线程操作安全的，同一个对象可以被多个线程同时使用。

#### 4. acl redis 客户库当连接断开时是否会重连？
acl redis 库中的连接类中：redis_client，redis_client_pool，redis_client_cluster 三个连接类对象当检测到网络连接异常断开时会尝试自动重连，上层使用者无需考虑连接断开重试的情况。
 
### （四）、协程模块
#### 1. 协程里面的非阻塞是什么原理的？
比如我现在需要访问数据库，这个动作肯定是个阻塞的操作，如果有10个协程进行数据库访问，这个非阻塞是怎么理解的？
协程方式在底层将系统的 IO API（read/write 等）都 hook 了，数据库操作也要调用这些API，所以表面上的阻塞式DB操作在协程底层的IO也会被转为非阻塞模式。

#### 2. acl 协程库是否支持多线程？
acl 协程库支持多线程方式，只是支持的方式与 go 语言有所不同。用户可以创建多个线程，每个线程一个协程调度器，线程之间的协程调度是相互隔离的，正如多进程与多线程之间的关系一样（每个进程内可以启动多个线程，但进程之间的线程的调度过程是隔离的），在每个线程内部可以创建大量 acl 协程，每个协程均由其所属的线程内的协程调度器调度运行。

#### 3. acl 协程库如何支持多核 CPU？
正如 2）所说，用户可以启动多个线程，每个线程一个 acl 协程调度器，每个协程调度器负责调度与其同属相同线程的协程运行状态。

#### 4. acl 协程里针对 mysql 客户端库的协程化是否修改了 mysql 库的源码？
没有。mysql 客户端库使用的系统 IO API 为 read/write/poll，而 acl 协程库 HOOK 了系统底层的 IO 过程，因此当将用户程序与 mysql 库及 acl 协程库一起编译后，mysql 库的 IO 过程直接被 acl 协程库 HOOK 的 API 接管，从而将 mysql 客户端库协程化而无须修改一行 mysql 库代码。

#### 5. acl 协程库支持域名解析功能吗？
支持。很多 C/C++ 实现的协程库并未实现 gethostbyname(_r) 函数，导致用户在使用协程编程遇到域名解析时还需要借助单独的线程来完成，acl 库本身从 DNS 协议层次实现了域名解析过程，acl 协程库基于此功能模块 HOOK 了系统的 gethostbyname(_r) API 而无须借助第三方函数库或起单独的线程完成域名解析。

#### 6. acl 协程库的系统 errno 号如何处理？
acl 协程库实现了协程安全的 errno 号，正如之前使用多线程编程时 errno 可以与每个线程绑定一样，在 acl 协程库里 errno 也是与每个 acl 协程进行绑定的。因此，当你调用 strerror(errno) 时也是协程安全的。

#### 7. 如果启动的协程过多，会不会对于后端例如 mysql 之类服务造成并发压力？如何避免？
当启动用协程较多且都需要 mysql 操作时的确会造成 mysql 服务器的并发压力。为避免此并发压力，acl 协程库提供了协程信号量，用来针对后端不支持高并发的服务提供连接保护。
 
### （五）、服务器模块
#### 1. 有几种服务器编程模型？均有何特点？
目前 acl 库中提供：进程池模型、线程池模型、非阻塞模型、协程模型、UDP 通信模型、触发器模型。其中各个编程模型的特点如下：  

__进程池模型：__ 每个进程处理一个客户端连接，当需要处理多个连接时则需要启动多个进程，此模型的最大缺点是并发度低，优点是编程简单；  
__线程池模型：__ 由多个线程组成线程池处理大量的客户端连接，只有当某个连接有数据可读时该连接才会与一个线程绑定，处理完毕则线程归还给线程池，此模型的优点是启动少数线程便处理较大并发，缺点是需要注意线程编程时的线程安全问题；  
__非阻塞模型：__ 一个进程内仅有一个工作线程，通过采用非阻塞通信方式可以支持非常大的客户端并发连接，优点是资源消耗小、支持大并发、性能高，缺点是编程复杂度高；  
__协程模型：__ 每个客户端连接与一个协程绑定，每个进程内一个工作线程，每个线程内可以创建大 量的协程，优点是支持大并发、性能高、编程简单、应用场景比较广，缺点是占用内存要比非阻塞模型高；  
__UDP通信模型：__ 支持简单的 UDP 通信方式；触发器模型：常用在定时任务的应用场景中。  
以上的服务模型均可启动多个进程，通过配置文件的配置项来决定启动进程的数量。

#### 2. acl_master 的作用是什么？支持哪些平台？
acl_master 为由以上各个服务器编写的服务进程的控制管理程序， acl_master
启动、停止各个服务子进程，控制子进程的启动数量及预启动策略，监控子进程的异常情况；acl_master 有点类似于 LINUX 下的 xinetd 服务进程，不同之处是 acl_master 功能更完善强大，支持服务子进程的常驻留、半驻留，而 xinetd 则只针对一个连接创建一个进程，不能用于高并发的服务应用场景。  
目前 acl_master 进程仅支持 LINUX/MACOS/FREEBSD/SOLARIS(X86）等 UNIX 平台，不支持 WINDOWS 平台。  
#### 3. 没有 acl_master 控制管理，服务子进程是否可以单独运行？

可以。在没有 acl_master 的情况下，由以上各个服务模型编写的服务程序可以通过手工方式启动。启动方式一般为：./xxxx alone xxxx.cf，这样服务程序便以 alone 模式启动运行，具体情况可以参考 main.cpp 里的启动方式；在 WINDOWS 平台下只能是以 alone 模式手工启动运行。

#### 4. 手工模式下运行时遇到"idle timeout -- exiting, idle"怎么办？
因为 acl 的服务器编程模型均支持半驻留方式（即运行空闲一段时间或处理连接次数达到设定值后会自动退出，这样的好处是：可以定期通过进程退出释放可能存在的资源泄露，另一方面便于用户在开发时通过 valgrind 进行内存检查），如果让进程不退出，可以在 alone 模式下给服务程序传递启动配置文件，如启动方式为：./xxxx alone xxxx.cf（传递方式可以看 main.cpp 和相关头文件），不同的服务器模型分别采取下面不同的配置项：  

__程池模型：__ 将配置项 single_use_limit 和 single_idle_limit 设为 0；  
__线程池模型：__ 将配置项 ioctl_use_limit 和 ioctl_idle_limit 设为 0；  
__非阻塞模型：__ 将配置项 aio_use_limit 和 aio_idle_limit 设为 0；  
__协程模型：__ 将配置项 fiber_use_limit 和 fiber_idle_limit 设为 0；  
__UDP通信模型：__ 将配置项 udp_use_limit 和 udp_idle_limit 设为 0；  
__触发器模型：__ 将配置项 trigger_use_limit 设为 0。

#### 5. acl_master 控制模式下，服务子进程如何预启动多个进程？
需要修改每个服务子进程的配置文件，将配置项：master_maxproc 及 master_prefork 设置成要启动的进程数（设置值需相同），同时需要将 xxx_use_limit 及 xxx_idle_limit 配置项设成 0 以防止子进程空闲退出，xxx_use_limit 及 xxx_idle_limit  的依每种服务器模型而不同，具体可参考上面（4）中的说明。

#### 6. acl_master 控制模式下，如何只监听内网地址？
在 acl_master 模式下，可以将 master_service 配置项支持模糊匹配方式，即可以将监听地址写成 `192.168.*.*:8192` 或 `10.0.*.*:8192` 方式，这样 acl_master 会自动扫描服务器所有的网卡地址，但只监听服务匹配条件的内网地址，这样为统一部署提供方便。
 
### （六）、数据库模块
#### 1. acl 数据库客户端支持哪些数据库？
当前 acl 数据库客户端库支持的数据库有：mysql，postgresql，sqlite。
#### 2. acl 数据库模块如何使用？
acl 数据库模块封装了官方数据库的驱动（包括 mysql，postgresql，sqlite），所以使用者应先下载所对应的官方数据库驱动，考虑到版本的一致性，建议从 https://github.com/acl-dev/third_party 处下载；
另外，acl 数据库模块是采用动态加载方式加载数据库驱动的，所以使用者应将编译好的数据库动态库放置在合适的位置，并调用 `acl::db_handle::set_loadpath()` 设置数据库驱动动态库的全路径，以便于 acl 数据库模块内部可以使用该路径进行动态加载。

### （七）、邮件&mime模块
。。。
 
- 微博：http://weibo.com/zsxxsz
- qq 群：242722074
