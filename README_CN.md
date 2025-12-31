
[English](README.md)

# Acl -- 强大的跨平台 C/C++ 网络通信库与服务框架



# 0. 关于 Acl 项目
Acl（Advanced C/C++ Library，先进的 C/C++ 库）是一个功能强大的跨平台网络通信库和服务器编程框架，支持 Linux、Windows、Solaris、FreeBSD、macOS、Android、iOS 及鸿蒙系统。众多基于 Acl 开发的应用程序运行在各类设备上，为上亿用户提供稳定可靠的服务。

Acl 项目包含丰富的功能模块：网络通信、服务器框架、应用协议、多种编解码器等。内置实现了常用网络协议，如 HTTP/SMTP/ICMP/MQTT/Redis/Memcached/Beanstalk/Handler Socket 等，以及完整的编解码库，如 XML/JSON/MIME/BASE64/UUCODE/QPCODE/RFC2047/RFC1035 等。此外，Acl 还为主流数据库（MySQL、PostgreSQL、SQLite）提供了统一的抽象接口，让开发者能够更加轻松、快速、安全地编写数据库应用程序。

## 软件架构图
 ![整体架构图](res/img/architecture_en.png)

<hr>

* [0. 关于 Acl 项目](#0-关于-acl-项目)
* [1. 六个核心模块](#1-六个核心模块)
    * [1.1. 基础网络模块](#11-基础网络模块)
    * [1.2. 协程](#12-协程)
    * [1.3. HTTP 模块](#13-http-模块)
    * [1.4. Redis 客户端](#14-redis-客户端)
    * [1.5. MQTT 模块](#15-mqtt-模块)
    * [1.6. 服务器框架](#16-服务器框架)

* [2. 其他重要模块](#2-其他重要模块)
    * [2.1. MIME 模块](#21-mime-模块)
    * [2.2. 编解码模块](#22-编解码模块)
    * [2.3. 数据库模块](#23-数据库模块)
    * [2.4. 连接池管理器](#24-连接池管理器)
    * [2.5. 其他客户端库](#25-其他客户端库)
    * [2.6. DNS 模块](#26-dns-模块)

* [3. 平台支持和编译](#3-平台支持和编译)
    * [3.1. 在不同平台上编译 Acl](#31-在不同平台上编译-acl)
    * [3.2. 在 Windows 上编译的注意事项](#32-在-windows-上编译的注意事项)

* [4. 快速开始](#4-快速开始)
    * [4.1. 第一个 Acl 示例](#41-第一个-acl-示例)
    * [4.2. 简单的 TCP 服务器](#42-简单的-tcp-服务器)
    * [4.3. 简单的 TCP 客户端](#43-简单的-tcp-客户端)
    * [4.4. 协程 TCP 服务器](#44-协程-tcp-服务器)
    * [4.5. HTTP 客户端示例](#45-http-客户端示例)
    * [4.6. 协程 HTTP 服务器](#46-协程-http-服务器)
    * [4.7. Redis 客户端示例](#47-redis-客户端示例)

* [5. 更多信息](#5-更多信息)
    * [5.1. 示例代码](#51-示例代码)
    * [5.2. 更多简单示例](#52-更多简单示例)
    * [5.3. 常见问题（FAQ）](#53-常见问题faq)
    * [5.4. 开源许可证](#54-开源许可证)
    * [5.5. 相关链接](#55-相关链接)
    * [5.6. 致谢](#56-致谢)

* [6. 附录](#6-附录)
    * [6.1. 软件分层架构](#61-软件分层架构)
    * [6.2. 项目目录结构](#62-项目目录结构)

<hr>

# 1. 六个核心模块
作为一个功能完备的 C/C++ 基础库，Acl 为应用程序开发提供了丰富而实用的功能。其中最核心的六大模块包括：网络通信、协程、HTTP、Redis 客户端、MQTT 和服务器框架。

## 1.1. 基础网络模块
**流处理模块**  
该模块是整个 Acl 网络通信的基石，提供统一的流式通信接口，同时支持网络流和文件流。主要功能包括：
  - 按行读取数据，自动兼容 Windows 下的 `\r\n` 和 UNIX 下的 `\n` 结尾符
  - 按行读取数据并自动删除尾部的换行符（`\n` 或 `\r\n`）
  - 以自定义字符串分隔符读取数据
  - 读取指定长度的数据
  - 尝试性读取一行数据或指定长度数据
  - 检测网络 IO 状态
  - 写入一行数据
  - 格式化写入数据（类似于 `fprintf`）
  - 文件流定位操作（类似于 `fseek`）
  - 批量写入数据（类似于 UNIX 的 `writev`）
  - 截断文件（类似于 `ftruncate`）
  - 获取文件大小和当前文件流指针位置（类似于 `ftell`）
  - 获取网络流的本地地址和远程地址

**网络操作模块**  
该模块提供完整的网络操作功能，包括：
  - 网络服务器监听（支持 TCP/UDP/UNIX 域套接字）
  - 网络客户端连接（支持 TCP/UNIX 域套接字）
  - DNS 域名查询和结果缓存（支持系统 API 调用和直接 DNS 协议两种方式）
  - 套接字（socket）操作及本地网卡信息获取

**非阻塞网络流**  
全面支持非阻塞模式的网络操作，包括非阻塞连接、读取（按行读取、指定长度读取）、写入（按行写入、指定长度写入、批量写入）等。

**常见网络应用协议库**  
内置实现了常用网络应用协议，如 HTTP、SMTP、ICMP 等，其中 HTTP 和 ICMP 模块同时支持阻塞和非阻塞通信方式。HTTP 协议在 C++ 版本的 lib_acl_cpp 中提供了服务器端和客户端两种模式：
  - **服务器端模式**：提供类似 Java HttpServlet 的接口，支持 Cookie、Session、HTTP MIME 文件上传等功能
  - **客户端模式**：支持连接池和集群管理、分块传输、自动字符集转换、自动解压缩、断点续传等丰富特性

**常见网络通信库**  
提供 Memcached、Beanstalk、Handler Socket 等客户端通信库，均支持连接池模式。

## 1.2. 协程
Acl 的协程模块是一个成熟稳定的跨平台协程库，已在众多重要项目中得到广泛应用和验证。

**平台支持**
- 支持 Linux、macOS、Windows、iOS 和 Android 等主流操作系统
- 支持 x86、ARM 等多种 CPU 架构
- 支持 select/poll/epoll/kqueue/iocp/win32 GUI 消息等多种事件引擎

**核心特性**
- **完整的 DNS 协议实现**：DNS 协议已在协程中原生实现，DNS API 可直接在协程中使用
- **系统 API Hook**：在 Unix 和 Windows 平台上自动 Hook 系统 IO API，实现协程化
  - **读取 API**：read/readv/recv/recvfrom/recvmsg
  - **写入 API**：write/writev/send/sendto/sendmsg/sendfile64
  - **套接字 API**：socket/listen/accept/connect/setsockopt
  - **事件 API**：select/poll/epoll_create/epoll_ctl/epoll_wait
  - **DNS API**：gethostbyname/gethostbyname_r/getaddrinfo/freeaddrinfo
- **共享栈模式**：支持共享栈模式，可显著降低内存占用

**同步原语**
- **协程互斥锁**和**信号量**：用于协程间同步
- **协程事件**：支持协程与线程间的同步通信

更多详细信息，请参见 **[使用 Acl 协程库](lib_fiber/README_en.md)**

## 1.3. HTTP 模块
完整实现 HTTP/1.1 协议，同时支持客户端和服务器端应用开发。

**主要特性**
- **类 Java HttpServlet 接口**（服务器端）：提供熟悉的编程接口，降低学习成本
- **连接池模式**（客户端）：高效管理连接资源，提升性能
- **分块传输**（Chunked Transfer）：支持流式数据传输
- **压缩传输**：内置 Gzip 压缩/解压缩支持
- **SSL/TLS 加密**：支持安全加密传输
- **断点续传**：支持大文件的断点续传功能
- **Cookie 管理**：完整的 Cookie 设置和获取功能
- **Session 管理**（服务器端）：内置会话管理机制
- **WebSocket 支持**：支持 WebSocket 协议
- **HTTP MIME 格式**：支持 MIME 多部分数据格式
- **同步/异步模式**：灵活选择通信模式

## 1.4. Redis 客户端
Acl 的 Redis 客户端模块功能强大、性能卓越且易于使用，是生产环境的理想选择。

**功能特性**
- **丰富的命令支持**：支持 Bitmap/String/Hash/List/Set/Sorted Set/PubSub/HyperLogLog/Geo/Script/Stream/Server/Cluster 等 Redis 数据类型和命令
- **STL 风格接口**：为每个 Redis 命令提供类似 STL 的 C++ 接口，符合 C++ 编程习惯
- **智能集群管理**：客户端自动缓存和适应 Redis 集群哈希槽的变化，无需手动干预
- **多种通信模式**：支持单机、集群和 Pipeline 模式，接口统一
- **连接池支持**：内置连接池，支持单机和集群模式，提升资源利用率
- **高性能**：在集群和 Pipeline 模式下表现出色
- **自动重试**：网络错误时自动重试，提高可靠性
- **协程友好**：可在共享栈协程模式下使用

更多详细信息，请参见 **[使用 Acl Redis 客户端](lib_acl_cpp/samples/redis/README.md)**

## 1.5. MQTT 模块
Acl 完整实现了 MQTT 3.1.1 协议，采用流式解析器设计，可灵活适配各种 IO 模式。

**核心特性**
- **完整的 MQTT 3.1.1 协议支持**：实现了所有标准命令
  - CONNECT/CONNACK/PUBLISH/PUBACK/PUBREC/PUBREL/PUBCOMP
  - SUBSCRIBE/SUBACK/UNSUBSCRIBE/UNSUBACK
  - PINGREQ/PINGRESP/DISCONNECT
- **面向对象设计**：每个 MQTT 命令对应一个独立的类，结构清晰
- **流式解析器**：独立于 IO 模式，可与任何网络通信方式结合
- **解析与通信分离**：数据解析与网络通信完全解耦，灵活性高
- **双端支持**：可同时用于客户端和服务器端开发

更多详细信息，请参见 **[使用 Acl MQTT](lib_acl_cpp/samples/mqtt/README.md)**

## 1.6. 服务器框架
服务器框架是 Acl 中最核心的模块，帮助开发者快速构建高性能的后端服务（如 Web 服务）。通过 `app/wizard` 中的代码生成工具，可在几秒钟内生成完整的服务代码框架。

**架构设计**  
Acl 服务器框架由两部分组成：
1. **服务管理器（acl_master）**：源自著名的 Postfix MTA 的 master 进程，经过大量扩展成为通用的服务管理器
2. **服务模板**：提供多种服务模板供开发者选择

**六种服务模板**

- **进程服务模型**  
  一个连接对应一个进程。
  - 优点：编程简单、安全稳定
  - 缺点：并发能力有限
  - 适用场景：对安全性要求高、并发量不大的场景

- **线程服务模型**  
  每个进程通过线程池处理所有客户端连接，采用 IO 事件触发机制。
  - 优点：用少量线程处理大量连接，编程相对简单
  - 特点：连接有数据时才绑定线程，处理完立即释放
  - 适用场景：高并发场景，比 AIO 模型更易开发

- **AIO 服务模型（非阻塞）**  
  类似 Nginx/Squid/IRCd，单线程以非阻塞 IO 方式处理大量连接。
  - 优点：处理效率高、资源消耗低
  - 缺点：编程复杂度较高
  - 适用场景：超高并发、对性能要求极高的场景

- **协程服务模型**  
  结合了非阻塞模型的高并发能力和同步编程的简单性。
  - 优点：高并发 + 低编程复杂度，顺序 IO 编程方式
  - 特点：自动将阻塞操作转换为非阻塞过程，提升并发能力
  - 适用场景：高并发场景的首选，兼顾性能和开发效率

- **UDP 服务模型**  
  专门用于 UDP 通信的服务模型。
  - 适用场景：需要 UDP 协议的应用

- **触发器服务模型**  
  用于处理定时任务的后台服务（类似系统的 crontab）。
  - 适用场景：定时任务、后台调度

# 2. 其他重要模块

## 2.1. MIME 模块
MIME（多用途互联网邮件扩展）是一种重要的数据格式标准，广泛应用于电子邮件和 Web 应用程序。

**功能特性**
- 完整实现 MIME 相关 RFC 标准：RFC2045/RFC2047/RFC822
- 提供独立于 IO 模型的流式 MIME 数据解析器
- 可灵活用于同步或异步 IO 程序
- 支持 MIME 数据的解析和构建

## 2.2. 编解码模块
Acl 提供了丰富的编解码器，所有编解码器均采用流式解析设计，独立于 IO 通信层。

**支持的编解码格式**
- **JSON**：流式 JSON 解析器和构建器，支持 JSON 与 C 结构体的序列化/反序列化，大幅提升开发效率
- **XML**：流式 XML 解析器和构建器
- **Base64**：Base64 编解码
- **URL**：URL 编解码
- 其他：UUCODE、QPCODE、RFC2047 等

## 2.3. 数据库模块
Acl 提供了统一的数据库抽象接口，简化数据库应用开发。

**核心特性**
- **统一接口**：为 MySQL、PostgreSQL、SQLite 提供统一的操作接口
- **SQL 安全**：内置 SQL 编解码器，自动转义特殊字符，有效防止 SQL 注入攻击
- **动态加载**：采用动态库加载方式，不使用数据库功能时无需关心依赖
- **连接池支持**：内置数据库连接池管理

## 2.4. 连接池管理器
Acl 提供了通用的连接池管理器，被广泛应用于Acl库各个客户端通信模块。

**应用场景**
- Redis 客户端连接池
- 数据库连接池
- HTTP 客户端连接池
- 其他网络客户端连接池

## 2.5. 其他客户端库
除 Redis 客户端外，Acl 还实现了多种常用的客户端通信库。

**支持的客户端**
- **Memcached**：支持连接池
- **Handler Socket**：MySQL 的 Handler Socket 插件客户端
- **Beanstalk**：消息队列客户端
- **Disque**：分布式消息队列客户端

## 2.6. DNS 模块
Acl 提供了完整的 DNS 解决方案，既可以使用系统 API，也可以直接实现 DNS 协议。

**功能特性**
- **系统 API 封装**：封装 `getaddrinfo` 和 `gethostbyname` 等系统 API
- **协议实现**：完整实现 RFC1035 规定的 DNS 协议
- **双端支持**：可用于实现 DNS 客户端或 DNS 服务器
- **结果缓存**：支持 DNS 查询结果缓存，提升性能

# 3. 平台支持和编译

## 3.1. 在不同平台上编译 Acl
Acl 是一个跨平台库，支持主流操作系统和多种编译工具链。

**支持的平台**
- Linux、Windows、macOS、FreeBSD、Solaris
- Android、iOS、鸿蒙（Harmony）

**编译方式**

**Linux/UNIX 平台**
- **编译器**：gcc/clang
- **Make 方式**：
  ```bash
  cd acl/
  make
  ```
  编译完成后会在 `acl/` 目录生成：
  - `libacl_all.a`（静态库，包含 lib_acl.a、lib_protocol.a 和 libacl_cpp.a）
  - `libacl_all.so`（动态库）

- **CMake 方式**：
  ```bash
  ./cmake-build.sh
  ```

- **XMake 方式**：
  ```bash
  xmake
  ```

**Windows 平台**
- 支持 Visual Studio 2003/2008/2010/2012/2013/2015/2019 等版本
- 如需使用 VS6/VS2005，可参考 VS2003 的编译配置

**macOS/iOS 平台**
- 使用 Xcode 进行编译

**Android 平台**
- 使用 **Android Studio** 打开 **`acl/android/acl_ndk20b/`** 项目

**鸿蒙（Harmony）平台**
- 使用 **DevEco Studio** 打开 **`acl/harmony/api13/`** 项目

**跨平台编译**
- 支持使用 **CMake** 进行跨平台编译

## 3.2. Windows 平台编译注意事项
在 Windows 环境下使用 Acl 动态库时，需要在项目中添加相应的预定义宏。

**预定义宏说明**

| 使用的库 | 需要预定义的宏 | 说明 |
|---------|--------------|------|
| lib_acl 动态库 | `ACL_DLL` | 基础库 |
| lib_protocol HTTP 库 | `HTTP_DLL` | HTTP 协议库 |
| lib_protocol ICMP 库 | `ICMP_DLL` | ICMP 协议库 |
| lib_acl_cpp 动态库 | `ACL_CPP_DLL` | C++ 库 |

**详细说明**
- 详细的编译过程和配置方法，请参见：[Acl 库的编译和使用](BUILD.md)

# 4. 快速开始

## 4.1. 第一个 Acl 示例
这是一个最简单的 Acl 程序，展示如何使用 Acl 的字符串类。
```c++
#include <iostream>
#include "acl_cpp/lib_acl.hpp"

int main() {
  acl::string buf = "hello world!\r\n";
  std::cout << buf.c_str() << std::endl;
  return 0;
}
```

## 4.2. 简单的 TCP 服务器
这个示例展示如何使用 Acl 创建一个简单的 TCP 回显服务器，使用多线程处理客户端连接。
```c++
#include <thread>
#include "acl_cpp/lib_acl.hpp"

void run() {
  const char* addr = "127.0.0.1:8088";
  acl::server_socket server;
  if (!server.open(addr)) {  // 绑定并监听本地地址。
    return;
  }

  while (true) {
    acl::socket_stream* conn = server.accept(); // 等待连接。
    if (conn == NULL) {
      break;
    }
    std::thread thread([=] {  // 启动一个线程来处理连接。
      char buf[256];
      int ret = conn->read(buf, sizeof(buf), false);  // 读取数据。
      if (ret > 0) {
        conn->write(buf, ret);  // 写入接收到的数据。
      }
      delete conn;
    });
    thread.detach();
  }
}
```

## 4.3. 简单的 TCP 客户端
这个示例展示如何使用 Acl 创建一个 TCP 客户端，连接服务器并进行数据收发。
```c++
#include "acl_cpp/lib_acl.hpp"

void run() {
  const char* addr = "127.0.0.1:8088";
  int conn_timeout = 5, rw_timeout = 10;
  acl::socket_stream conn;
  if (!conn.open(addr, conn_timeout, rw_timeout)) { // 连接服务器。
    return;
  }
  const char data[] = "Hello world!\r\n";
  if (conn.write(data, sizeof(data) - 1) == -1) {  // 向服务器发送数据。
    return;
  }
  char buf[256];
  int ret = conn.read(buf, sizeof(buf) - 1, false);
  if (ret > 0) {  // 从服务器读取。
    buf[ret] = 0;
    std::cout << buf << std::endl;
  }
}
```

## 4.4. 协程 TCP 服务器
这个示例展示如何使用 Acl 协程创建高并发的 TCP 服务器，用顺序编程方式实现异步处理。
```c++
#include "acl_cpp/lib_acl.hpp"
#include "fiber/go_fiber.hpp"

void run() {
  const char* addr = "127.0.0.1:8088";
  acl::server_socket server;
  if (!server.open(addr)) {
    return;
  }

  go[&] {  // 创建一个服务器协程来等待连接。
    while (true) {
      acl::shared_stream conn = server.shared_accept();
      if (conn == nullptr) {
        break;
      }

      go[conn] {  // 创建一个客户端协程来处理连接。
        while (true) {
          char buf[256];
          int ret = conn->read(buf, sizeof(buf), false);
          if (ret <= 0 || conn->write(buf, ret) != ret) {
            break;
          }
        }
      };
    }
  };

  acl::fiber::schedule();  // 启动协程调度过程。
}
```

## 4.5. HTTP 客户端示例
这个示例展示如何使用 Acl 创建 HTTP 客户端，发送 HTTP 请求并获取响应。
```c++
#include "acl_cpp/lib_acl.hpp"

bool run() {
  acl::http_request conn("www.baidu.com:80");
  acl::http_header& header = conn.request_header()
  header.set_url("/")
    .set_keep_alive(false);
    .set_content_type("text/html");

  if (!conn.request(NULL, 0)) {
    return false;
  }
  int status = conn.http_status();
  if (status != 200) {
    return false;
  }
  long long len = conn.body_length();
  if (len <= 0) {
    return true;
  }
  acl::string buf;
  if (!conn.get_body(body)) {
    return false;
  }
  return true;
}
```

## 4.6. 协程 HTTP 服务器
这个示例展示如何使用 Acl 协程创建一个功能完整的 HTTP 服务器，支持路由、配置管理等特性。
```c++
#include "acl_cpp/lib_acl.hpp"  // 必须在 http_server.hpp 之前
#include "fiber/http_server.hpp"

static char *var_cfg_debug_msg;
static acl::master_str_tbl var_conf_str_tab[] = {
  { "debug_msg", "test_msg", &var_cfg_debug_msg },
  { 0, 0, 0 }
};

static int  var_cfg_io_timeout;
static acl::master_int_tbl var_conf_int_tab[] = {
  { "io_timeout", 120, &var_cfg_io_timeout, 0, 0 },
  { 0, 0 , 0 , 0, 0 }
};

int main(int argc, char* argv[]) {
  const char* addr = "0.0.0.0|8194";
  const char* conf = argc >= 2 ? argv[1] : NULL;
  acl::http_server server;

  // 调用 acl::master_base 类中的方法。
  server.set_cfg_int(var_conf_int_tab).set_cfg_str(var_conf_str_tab);

  // 调用 acl::http_server 中的方法。
  server.on_proc_init([&addr] {
    printf("---> after process init: addr=%s, io_timeout=%d\r\n", addr, var_cfg_io_timeout);
  }).on_proc_exit([] {
    printf("---> before process exit\r\n");
  }).on_proc_sighup([] (acl::string& s) {
    s = "+ok";
    printf("---> process got sighup\r\n");
    return true;
  }).on_thread_accept([] (acl::socket_stream& conn) {
    printf("---> accept %d\r\n", conn.sock_handle());
    return true;
  }).Get("/", [](acl::HttpRequest&, acl::HttpResponse& res) {
    std::string buf("hello world1!\r\n");
    res.setContentLength(buf.size());
    return res.write(buf.c_str(), buf.size());
  }).Post("/json", [](acl::HttpRequest& req, acl::HttpResponse& res) {
    acl::json* json = req.getJson();
    if (json) {
      return res.write(*json);
    } else {
      std::string buf = "no json got\r\n";
      res.setContentLength(buf.size());
      return res.write(buf.c_str(), buf.size());
    }
  }).run_alone(addr, conf);

  return 0;
}
```

## 4.7. Redis 客户端示例
这个示例展示如何使用 Acl Redis 客户端进行多线程操作，包括连接池管理和基本的 Redis 命令。
```c++
#include <thread>
#include "acl_cpp/lib_acl.hpp"

static void thread_run(acl::redis_client_cluster& conns) {
  acl::redis cmd(&conns);
  std::map<acl::string, acl::string> attrs;
  attrs["name1"] = "value1";
  attrs["name2"] = "value2";
  attrs["name3"] = "value3";
  acl::string key = "hash-key-1";
  if (!cmd.hmset(key, attrs)) {
    printf("hmset error=%s\r\n", cmd.result_error());
    return;
  }

  attrs.clear();
  if (!cmd.hgetall(key, attrs)) {
    printf("hgetall error=%s\r\n", cmd.result_error());
  }
}

void run() {
  const char* addr = "126.0.0.1:6379";
  acl::redis_client_cluster conns;
  conns.set(addr, 0);

  const size_t nthreads = 10;
  std::thread threads[nthreads];
  // 创建一些线程来使用相同的 conns 测试 redis。
  for (size_t i = 0; i < nthreads; i++) {
    threads[i] = std::thread(thread_run, std::ref(conns));
  }
  // 等待所有线程退出
  for (size_t i = 0; i < nthreads; i++) {
    threads[i].join();
  }
}
```

# 5. 更多信息

## 5.1. 示例代码
Acl 库提供了大量的示例代码供学习参考，涵盖各个功能模块的使用方法。

**示例索引**：[SAMPLES.md](SAMPLES.md)

## 5.2. 更多简单示例
为了帮助开发者快速上手，我们提供了大量简单易懂的小示例。

**示例仓库**：[Acl Demos](https://github.com/acl-dev/demo/)

## 5.3. 常见问题（FAQ）
在使用 Acl 过程中遇到问题？请查看常见问题解答。

**FAQ 文档**：[FAQ.md](FAQ.md)

## 5.4. 开源许可证
Acl 采用 LGPL-v2.1 开源许可证，可自由用于商业和非商业项目。

**许可证详情**：[LICENSE.txt](LICENSE.txt)

## 5.5. 相关链接

**官方资源**
- 🌐 官方网站：https://acl-dev.cn/
- 💻 GitHub：https://github.com/acl-dev/acl
- 🇨🇳 Gitee：https://gitee.com/acl-dev/acl
- 📝 技术博客：https://blog.csdn.net/zsxxsz
- 🐦 微博：http://weibo.com/zsxxsz

**社区交流**
- 💬 QQ 群：705290654

## 5.6. 致谢

感谢 <a href=https://jb.gg/OpenSourceSupport target=_blank><img widith=100 height=50 src=res/logo/clion_icon.png /> </a> 为 Acl 项目提供的支持。

# 6. 附录

## 6.1. 软件分层架构

```
┌─────────────────────────────────────────────────────────┐
│                   应用层 (Application)                   │
│  业务代码、HTTP服务器、WebSocket服务、Redis客户端等          │
└─────────────────────────────────────────────────────────┘
                          ↓
┌────────────────────────────────────────────────────────┐
│             高级抽象层 (High-level API)                  │
│  ┌──────────┬──────────┬──────────┬──────────┐         │
│  │  HTTP    │  Redis   │   MQTT   │   DB     │         │
│  │ 模块      │  模块    │  模块     │  模块     │         │
│  └──────────┴──────────┴──────────┴──────────┘         │
│  ┌──────────┬──────────┬──────────┬──────────┐         │
│  │  Master  │  Fiber   │ ConnPool │  Stream  │         │
│  │  框架     │  协程库   │  连接池   │  流处理   │         │
│  └──────────┴──────────┴──────────┴──────────┘         │
└────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────┐
│             核心库层 (Core Library)                      │
│  - 网络 I/O (socket, stream, aio)                       │
│  - 事件驱动 (event, epoll, kqueue, iocp)                 │
│  - 内存管理 (memory pool, dbuf)                          │
│  - 数据结构 (string, array, hash, list)                  │
│  - 工具类 (log, config, thread, process)                 │
└─────────────────────────────────────────────────────────┘
                          ↓
┌───────────────────────────────────────────────────────---─---─┐
│             操作系统层 (OS Layer)                               │
│  Linux / FreeBSD / macOS / Windows / Android / iOS / Harmony  │
└────────────────────────────────────────────────────────------─┘
```

## 6.2. 项目目录结构

```
acl/
├── lib_acl/                    # 核心 C 库（基础库）
│   ├── include/                # 公共头文件
│   ├── src/                    # 源代码实现
│   │   ├── stdlib/             # 标准库（字符串、内存、文件等）
│   │   │   ├── common/         # 通用数据结构（哈希表、链表、队列、树等）
│   │   │   ├── memory/         # 内存管理（内存池、slab、dbuf）
│   │   │   ├── string/         # 字符串操作
│   │   │   ├── filedir/        # 文件和目录操作
│   │   │   ├── configure/      # 配置文件解析
│   │   │   ├── iostuff/        # IO 工具函数
│   │   │   ├── debug/          # 调试工具
│   │   │   └── sys/            # 系统相关封装
│   │   ├── net/                # 网络模块
│   │   │   ├── connect/        # 客户端连接
│   │   │   ├── listen/         # 服务端监听
│   │   │   └── dns/            # DNS 解析
│   │   ├── event/              # 事件引擎（epoll/kqueue/iocp/select/poll）
│   │   ├── aio/                # 异步 IO 模块
│   │   ├── thread/             # 线程和线程池
│   │   ├── master/             # 服务器框架（进程管理）
│   │   │   └── template/       # 服务模板（进程/线程/aio/协程/UDP/触发器）
│   │   ├── db/                 # 数据库模块
│   │   │   ├── mysql/          # MySQL 支持
│   │   │   ├── memdb/          # 内存数据库
│   │   │   └── zdb/            # ZDB 存储引擎
│   │   ├── json/               # JSON 解析器
│   │   ├── xml/                # XML 解析器
│   │   ├── code/               # 编解码（base64/url/html 等）
│   │   ├── msg/                # 消息队列
│   │   ├── init/               # 初始化模块
│   │   ├── ioctl/              # IO 控制
│   │   ├── proctl/             # 进程控制（Windows）
│   │   └── unit_test/          # 单元测试框架
│   └── samples/                # 大量示例代码
│
├── lib_protocol/               # 协议库（C 实现）
│   ├── include/                # 协议头文件
│   ├── src/                    # 协议实现
│   │   ├── http/               # HTTP 协议
│   │   ├── icmp/               # ICMP/Ping 协议
│   │   └── smtp/               # SMTP 邮件协议
│   └── samples/                # 协议示例
│
├── lib_acl_cpp/                # C++ 封装库（高级功能）
│   ├── include/                # C++ 头文件
│   │   ├── acl_cpp/            # 主要头文件目录
│   │   │   ├── stdlib/         # 标准库封装
│   │   │   ├── stream/         # 流处理
│   │   │   ├── http/           # HTTP 客户端和服务端
│   │   │   ├── redis/          # Redis 客户端
│   │   │   ├── mqtt/           # MQTT 协议
│   │   │   ├── db/             # 数据库封装
│   │   │   ├── mime/           # MIME 协议
│   │   │   ├── master/         # Master 框架封装
│   │   │   ├── connpool/       # 连接池
│   │   │   ├── session/        # Session 管理
│   │   │   ├── memcache/       # Memcached 客户端
│   │   │   ├── beanstalk/      # Beanstalk 客户端
│   │   │   ├── disque/         # Disque 客户端
│   │   │   ├── hsocket/        # Handler Socket 客户端
│   │   │   ├── ipc/            # 进程间通信
│   │   │   ├── queue/          # 文件队列
│   │   │   ├── serialize/      # 序列化（JSON/Gson）
│   │   │   ├── aliyun/         # 阿里云 SDK（OSS）
│   │   │   └── event/          # 事件封装
│   ├── src/                    # C++ 源码实现
│   │   ├── stdlib/             # 标准库实现（string/logger/charset/zlib 等）
│   │   ├── stream/             # 流实现（socket/ssl/aio）
│   │   ├── http/               # HTTP 实现
│   │   │   ├── h2/             # HTTP/2 支持（待实现）
│   │   │   └── h3/             # HTTP/3 支持（待实现）
│   │   ├── redis/              # Redis 客户端实现
│   │   ├── mqtt/               # MQTT 协议实现
│   │   ├── db/                 # 数据库实现
│   │   ├── mime/               # MIME 实现
│   │   ├── master/             # Master 框架实现
│   │   ├── connpool/           # 连接池实现
│   │   ├── session/            # Session 实现
│   │   ├── memcache/           # Memcached 实现
│   │   ├── beanstalk/          # Beanstalk 实现
│   │   ├── disque/             # Disque 实现
│   │   ├── hsocket/            # Handler Socket 实现
│   │   ├── ipc/                # IPC 实现
│   │   ├── queue/              # 队列实现
│   │   ├── serialize/          # 序列化实现
│   │   ├── smtp/               # SMTP 实现
│   │   ├── aliyun/             # 阿里云客户端（待实现）
│   │   └── net/                # 网络工具（DNS）
│   └── samples/                # 丰富的示例代码
│       ├── http/               # HTTP 示例
│       ├── redis/              # Redis 示例
│       ├── mqtt/               # MQTT 示例
│       ├── db/                 # 数据库示例
│       ├── master/             # Master 框架示例
│       ├── fiber/              # 协程示例
│       └── ...                 # 更多示例
│
├── lib_fiber/                  # 协程库（核心特性）
│   ├── c/                      # C 语言实现
│   │   ├── include/            # 协程头文件
│   │   └── src/                # 协程源码
│   │       ├── common/         # 通用模块
│   │       ├── fiber/          # 协程核心
│   │       ├── event/          # 事件引擎
│   │       ├── sync/           # 同步原语（mutex/sem/event）
│   │       ├── hook/           # 系统 API Hook
│   │       └── ...
│   ├── cpp/                    # C++ 封装
│   │   ├── include/            # C++ 头文件
│   │   └── src/                # C++ 实现
│   ├── samples-c/              # C 语言示例
│   ├── samples-c++/            # C++ 示例
│   ├── samples-c++1x/          # C++11/14/17 示例
│   ├── samples-gui/            # GUI 示例（Windows）
│   └── unit_test/              # 单元测试
│
├── lib_dict/                   # 字典库（可选）
│   ├── bdb/                    # Berkeley DB 支持
│   ├── cdb/                    # CDB 支持
│   └── tc/                     # Tokyo Cabinet 支持
│
├── lib_tls/                    # TLS/SSL 库（可选）
│   └── src/                    # TLS 实现
│
├── lib_rpc/                    # RPC 库（实验性）
│   └── src/                    # RPC 实现
│
├── app/                        # 应用程序和工具
│   ├── wizard/                 # 代码生成向导（重要工具）
│   │   └── tmpl/               # 服务模板（进程/线程/aio/协程/UDP/触发器）
│   ├── wizard_demo/            # 向导生成的示例项目
│   ├── master/                 # Master 服务管理器
│   │   ├── daemon/             # 守护进程
│   │   ├── tools/              # 管理工具
│   │   └── ...
│   ├── jaws/                   # Web 应用服务器
│   ├── redis_tools/            # Redis 工具集
│   ├── net_tools/              # 网络工具
│   ├── gson/                   # JSON 序列化工具
│   ├── jencode/                # 编码转换工具
│   ├── iconv/                  # 字符集转换
│   └── ...
│
├── doc/                        # 文档目录
│   ├── README.md               # 文档索引
│   ├── fiber/                  # 协程文档
│   ├── http/                   # HTTP 文档
│   ├── redis/                  # Redis 文档
│   ├── mqtt/                   # MQTT 文档
│   ├── db/                     # 数据库文档
│   ├── master/                 # Master 框架文档
│   ├── stream/                 # Stream 文档
│   ├── mime/                   # MIME 文档
│   ├── connpool/               # 连接池文档
│   ├── rfc/                    # RFC 文档
│   └── ...
│
├── include/                    # 第三方库头文件
│   ├── mysql/                  # MySQL 头文件
│   ├── pgsql/                  # PostgreSQL 头文件
│   ├── sqlite/                 # SQLite 头文件
│   ├── openssl-1.1.1q/         # OpenSSL 头文件
│   ├── mbedtls/                # MbedTLS 头文件
│   ├── polarssl/               # PolarSSL 头文件
│   ├── zlib-1.2.11/            # Zlib 头文件
│   └── ...
│
├── lib/                        # 预编译库文件（按平台分类）
│   ├── linux64/                # Linux 64 位
│   ├── linux32/                # Linux 32 位
│   ├── win32/                  # Windows 32 位
│   ├── win64/                  # Windows 64 位
│   ├── macos/                  # macOS
│   ├── freebsd/                # FreeBSD
│   └── solaris/                # Solaris
│
├── android/                    # Android 平台支持
│   ├── acl_ndk20b/             # NDK r20b 项目
│   ├── acl_ndk28c/             # NDK r28c 项目
│   └── samples/                # Android 示例
│
├── harmony/                    # 鸿蒙平台支持
│   ├── api9/                   # HarmonyOS API 9
│   ├── api12/                  # HarmonyOS API 12
│   ├── api13/                  # HarmonyOS API 13
│   └── api16/                  # HarmonyOS API 16
│
├── xcode/                      # Xcode 项目（macOS/iOS）
│   └── ...                     # Xcode 工程文件
│
├── dist/                       # 安装目录（make install 后）
│   ├── include/                # 安装的头文件
│   ├── lib/                    # 安装的库文件
│   └── master/                 # Master 配置和脚本
│
├── unit_test/                  # 单元测试
│   └── ...                     # 测试用例
│
├── packaging/                  # 打包配置
│   └── ...                     # RPM 打包脚本
│
├── CMakeLists.txt              # CMake 构建配置
├── Makefile                    # 主 Makefile
├── xmake.lua                   # XMake 构建配置
├── README.md                   # 英文说明文档
├── README_CN.md                # 中文说明文档
├── SAMPLES.md                  # 示例代码索引
├── FAQ.md                      # 常见问题
├── BUILD.md                    # 编译说明
├── LICENSE.txt                 # 开源许可证
└── changes.txt                 # 更新日志
```

