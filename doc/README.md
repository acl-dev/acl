# ACL 项目架构与特点总结

## 目录

- [项目概述](#项目概述)
  - [核心定位](#核心定位)
- [整体架构](#整体架构)
  - [分层架构](#分层架构)
  - [核心模块](#核心模块)
    - [1. Stream 流处理模块](#1-stream-流处理模块)
    - [2. Fiber 协程模块](#2-fiber-协程模块)
    - [3. Master 服务器框架](#3-master-服务器框架)
    - [4. HTTP 模块](#4-http-模块)
    - [5. Redis 模块](#5-redis-模块)
    - [6. 数据库模块](#6-数据库模块)
    - [7. 连接池模块 (ConnPool)](#7-连接池模块-connpool)
    - [8. MQTT 模块](#8-mqtt-模块)
    - [9. MIME 模块](#9-mime-模块)
- [设计原则与特点](#设计原则与特点)
  - [1. 分层设计](#1-分层设计)
  - [2. 面向对象设计](#2-面向对象设计)
  - [3. 高性能设计](#3-高性能设计)
  - [4. 跨平台支持](#4-跨平台支持)
  - [5. 易用性设计](#5-易用性设计)
  - [6. 安全性](#6-安全性)
  - [7. 可扩展性](#7-可扩展性)
- [技术亮点](#技术亮点)
  - [1. 协程实现](#1-协程实现)
  - [2. 连接池设计](#2-连接池设计)
  - [3. Redis 集群支持](#3-redis-集群支持)
  - [4. 数据库抽象](#4-数据库抽象)
  - [5. SSL/TLS 集成](#5-ssltls-集成)
- [平台支持](#平台支持)
  - [操作系统](#操作系统)
  - [编译器](#编译器)
  - [事件引擎支持](#事件引擎支持)
- [编译与安装](#编译与安装)
  - [Linux/Unix](#linuxunix)
  - [Windows](#windows)
  - [依赖库](#依赖库)
- [快速开始](#快速开始)
  - [1. 协程示例](#1-协程示例)
  - [2. HTTP 客户端示例](#2-http-客户端示例)
  - [3. Redis 客户端示例](#3-redis-客户端示例)
  - [4. HTTP 服务器示例](#4-http-服务器示例)
- [项目结构](#项目结构)
- [适用场景](#适用场景)
  - [1. 高并发网络服务](#1-高并发网络服务)
  - [2. 分布式系统组件](#2-分布式系统组件)
  - [3. 网络工具](#3-网络工具)
  - [4. 嵌入式系统](#4-嵌入式系统)
- [性能特点](#性能特点)
  - [1. 高并发能力](#1-高并发能力)
  - [2. 低延迟](#2-低延迟)
  - [3. 内存效率](#3-内存效率)
  - [4. CPU 效率](#4-cpu-效率)
- [文档资源](#文档资源)
  - [官方文档](#官方文档)
  - [模块文档](#模块文档)
  - [示例代码](#示例代码)
- [社区支持](#社区支持)
- [总结](#总结)

---

## 项目概述

ACL (Advanced C/C++ Library) 是一个高性能、跨平台的 C/C++ 网络通信库，提供了完整的网络编程框架和丰富的功能模块。项目采用分层架构设计，支持同步/异步 I/O、协程、多线程/多进程等多种编程模型，适用于构建高并发网络服务。

### 核心定位

- **高性能网络库**: 提供底层网络 I/O 抽象，支持多种事件引擎（epoll、kqueue、iocp 等）
- **服务器框架**: 提供多种服务器模型（线程池、进程池、异步 I/O、协程等）
- **协议支持**: 完整的 HTTP/HTTPS、WebSocket、MQTT、Redis、数据库等协议客户端/服务端实现
- **协程支持**: 完整的协程库，支持类似 Golang 的协程编程体验

## 整体架构

### 分层架构

```
┌─────────────────────────────────────────────────────────┐
│                   应用层 (Application)                   │
│  业务代码、HTTP服务器、WebSocket服务、Redis客户端等          │
└─────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────┐
│             高级抽象层 (High-level API)                  │
│  ┌──────────┬──────────┬──────────┬──────────┐         │
│  │  HTTP    │  Redis   │   MQTT   │   DB     │         │
│  │ 模块      │  模块    │  模块     │  模块     │         │
│  └──────────┴──────────┴──────────┴──────────┘         │
│  ┌──────────┬──────────┬──────────┬──────────┐         │
│  │  Master  │  Fiber   │ ConnPool │  Stream  │         │
│  │  框架     │  协程库   │  连接池   │  流处理   │         │
│  └──────────┴──────────┴──────────┴──────────┘         │
└─────────────────────────────────────────────────────────┘
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
┌─────────────────────────────────────────────────────────┐
│             操作系统层 (OS Layer)                         │
│  Linux / FreeBSD / macOS / Windows / Android / iOS      │
└─────────────────────────────────────────────────────────┘
```

### 核心模块

#### 1. Stream 流处理模块

**设计理念**: 统一的流接口，支持文件流、网络流、同步/异步流

**核心特性**:
- 面向对象的流接口设计（`istream`、`ostream`、`fstream`、`socket_stream`）
- 同步和异步 I/O 支持
- SSL/TLS 支持（OpenSSL、MbedTLS、PolarSSL）
- Hook 机制，允许自定义 I/O 行为
- 跨平台统一接口

**类层次结构**:
```
stream (基类)
├── istream (输入流)
│   ├── fstream (文件流)
│   ├── socket_stream (网络流)
│   └── stdin_stream (标准输入)
├── ostream (输出流)
│   ├── fstream (文件流)
│   ├── socket_stream (网络流)
│   └── stdout_stream (标准输出)
└── aio_stream (异步流基类)
    ├── aio_fstream (异步文件流)
    └── aio_socket_stream (异步网络流)
```

#### 2. Fiber 协程模块

**设计理念**: 类似 Golang 的协程编程体验，同步写法，异步执行

**核心特性**:
- 支持多种事件引擎（epoll、kqueue、iocp、io_uring、poll、select）
- Go 风格的语法（`go` 关键字启动协程）
- 完整的同步原语（互斥锁、条件变量、信号量、事件等）
- Channel 通信机制
- 协程池管理
- 跨线程同步支持（`fiber_mutex`、`fiber_cond`）

**适用场景**:
- 高并发网络服务（数万到数十万并发连接）
- I/O 密集型应用
- 需要同步编程风格但要求高并发的场景

#### 3. Master 服务器框架

**设计理念**: 提供多种服务器模型，适应不同应用场景

**支持的服务器模型**:

| 模型 | 特点 | 适用场景 |
|------|------|---------|
| `master_threads` | 线程池模型 | 中等并发，长连接，需要维护状态 |
| `master_proc` | 多进程模型 | 高稳定性要求，进程隔离 |
| `master_aio` | 异步 I/O 模型 | 高并发，事件驱动，熟悉异步编程 |
| `master_fiber` | 协程模型 | 高并发，同步写法，异步执行 |
| `master_udp` | UDP 模型 | UDP 协议服务器 |
| `master_trigger` | 定时触发模型 | 定时任务，周期性检查 |

**设计特点**:
- 统一的配置管理（`master_conf`）
- 完整的生命周期回调
- 支持 daemon 模式和独立运行模式
- 自动资源管理

#### 4. HTTP 模块

**核心功能**:
- **HTTP 客户端**: 同步/异步客户端，支持连接池、断点续传、GZIP 压缩
- **HTTP 服务端**: Servlet 风格框架，支持 Session、Cookie、文件上传
- **WebSocket**: 完整的 WebSocket 协议实现（RFC 6455）
- **SSL/TLS**: HTTPS 和 WSS 支持

**架构设计**:
```
HTTP客户端体系:
├── http_client         # 底层HTTP协议处理
├── http_request        # HTTP请求封装（同步）
├── http_aclient        # HTTP请求封装（异步）
├── http_request_pool   # 连接池管理
└── http_download       # 文件下载

HTTP服务端体系:
├── HttpServlet         # Servlet基类
├── HttpServletRequest   # 请求封装
├── HttpServletResponse  # 响应封装
├── HttpSession         # Session管理
└── WebSocketServlet    # WebSocket Servlet
```

#### 5. Redis 模块

**核心功能**:
- 完整的 Redis 命令支持（150+ 命令，12 个操作类）
- 支持单机模式、集群模式、Pipeline 模式
- 自动处理 MOVE/ASK 重定向
- 连接池管理
- SSL/TLS 支持

**架构设计**:
```
redis (统一接口类 - 多重继承所有命令类)
  ├── redis_command (基类)
  │     ├── 连接管理
  │     ├── 命令构建
  │     ├── 结果解析
  │     └── 内存管理
  ├── redis_string (字符串操作)
  ├── redis_hash (哈希表操作)
  ├── redis_list (列表操作)
  ├── redis_set (集合操作)
  ├── redis_zset (有序集合操作)
  ├── redis_key (键操作)
  ├── redis_pubsub (发布订阅)
  ├── redis_transaction (事务)
  ├── redis_script (Lua脚本)
  └── redis_geo/redis_stream (新特性)
```

#### 6. 数据库模块

**核心功能**:
- 统一接口访问 MySQL、PostgreSQL、SQLite
- 连接池管理
- 异步服务支持
- SQL 注入防护（参数化查询）
- 动态库加载

**架构分层**:
```
应用层
  ↓
异步服务层 (db_service)
  ↓
连接池管理层 (db_pool, db_manager)
  ↓
数据库抽象层 (db_handle, query)
  ↓
数据结构层 (db_rows, db_row)
  ↓
底层驱动层 (libmysqlclient, libpq, libsqlite3)
```

#### 7. 连接池模块 (ConnPool)

**设计理念**: 通用的、高性能的网络连接池管理框架

**核心特性**:
- 四层架构：应用层 → 管理层 → 连接池层 → 连接层
- 连接复用（LIFO + LRU 混合策略）
- 自动故障检测与恢复
- 健康检查机制
- 负载均衡（轮询、哈希）
- 线程绑定模式（适合协程环境）

**已实现的连接池**:
- TCP 连接池
- Redis 连接池
- MySQL 连接池
- Memcache 连接池

#### 8. MQTT 模块

**核心功能**:
- 完整的 MQTT 3.1.1 协议实现
- 同步/异步客户端
- 支持所有消息类型（CONNECT、PUBLISH、SUBSCRIBE 等）
- QoS 0/1/2 支持
- SSL/TLS 支持

#### 9. MIME 模块

**核心功能**:
- 邮件解析和构建
- Base64、Quoted-Printable 编码/解码
- RFC 822、RFC 2047 标准支持
- 附件和图片处理

## 设计原则与特点

### 1. 分层设计

- **清晰的层次结构**: 从底层操作系统抽象到高层业务 API
- **职责分离**: 每个模块专注于特定功能
- **依赖关系清晰**: 上层依赖下层，避免循环依赖

### 2. 面向对象设计

- **抽象基类**: 定义统一接口（如 `stream`、`db_handle`、`connect_client`）
- **多态支持**: 通过虚函数实现多态
- **模板方法模式**: 基类定义算法骨架，子类实现具体步骤
- **工厂模式**: 连接池通过工厂方法创建连接

### 3. 高性能设计

- **内存池管理**: 使用 `dbuf_pool` 减少内存分配开销
- **连接复用**: 连接池避免频繁创建/销毁连接
- **零拷贝**: 支持 `sendfile`、`TransmitFile` 等零拷贝技术
- **事件驱动**: 高效的事件引擎（epoll、kqueue、iocp）
- **批量操作**: Pipeline、批量查询等减少网络往返

### 4. 跨平台支持

- **统一接口**: 屏蔽平台差异，提供一致的 API
- **多平台支持**: Linux、FreeBSD、macOS、Windows、Android、iOS
- **多编译器支持**: GCC、Clang、MSVC、Android NDK
- **动态库加载**: 支持运行时动态加载第三方库

### 5. 易用性设计

- **RAII 原则**: 自动资源管理（如 `db_guard`、`connect_guard`）
- **链式调用**: 配置类支持链式调用风格
- **同步编程**: 协程提供同步写法，异步执行
- **丰富的示例**: 提供大量示例代码

### 6. 安全性

- **SQL 注入防护**: 参数化查询（`query` 类）
- **SSL/TLS 支持**: 加密通信
- **输入验证**: 完善的参数验证机制
- **错误处理**: 统一的错误处理接口

### 7. 可扩展性

- **Hook 机制**: Stream 模块支持自定义 I/O 行为
- **插件化设计**: 易于扩展新功能
- **开闭原则**: 对扩展开放，对修改封闭

## 技术亮点

### 1. 协程实现

- **用户态调度**: 无需内核参与，上下文切换开销小
- **多种事件引擎**: 自动选择最优事件引擎
- **Hook 系统 API**: 使阻塞式网络库协程化
- **跨线程同步**: `fiber_mutex`、`fiber_cond` 支持跨线程使用

### 2. 连接池设计

- **四层架构**: 清晰的层次划分
- **线程绑定**: 适合协程环境，减少锁竞争
- **自动故障恢复**: 连接失败自动重试
- **健康检查**: 后台监控线程维护连接池健康

### 3. Redis 集群支持

- **自动哈希槽路由**: 自动计算 key 的哈希槽
- **MOVE/ASK 重定向**: 自动处理集群重定向
- **自动发现节点**: 通过 CLUSTER SLOTS 自动发现集群拓扑
- **线程安全**: 集群模式下线程安全

### 4. 数据库抽象

- **统一接口**: 相同 API 访问不同数据库
- **动态加载**: 运行时加载数据库客户端库
- **异步服务**: 后台线程池执行，不阻塞主线程
- **事务支持**: 统一的事务接口

### 5. SSL/TLS 集成

- **多种 SSL 库支持**: OpenSSL、MbedTLS、PolarSSL
- **动态加载**: 按需加载 SSL 库，减少编译依赖
- **阻塞/非阻塞**: 支持两种模式
- **简化 API**: 封装复杂的 SSL 操作

## 平台支持

### 操作系统

- **Linux**: 主流发行版（CentOS、Ubuntu、Debian 等）
- **FreeBSD**: 完整支持
- **macOS**: 完整支持
- **Windows**: Windows XP+，支持 VC2003/2008/2010/2012/2015/2017/2019/2022
- **Android**: NDK 12b/16b/28c
- **iOS**: 完整支持
- **Solaris**: 支持

### 编译器

- **GCC**: 4.8+
- **Clang**: 3.5+
- **MSVC**: 2015+
- **Android NDK**: 支持多个版本

### 事件引擎支持

| 引擎 | 平台 | 性能 |
|------|------|------|
| epoll | Linux | 高 |
| kqueue | FreeBSD/macOS | 高 |
| IOCP | Windows | 高 |
| io_uring | Linux 5.1+ | 高 |
| poll | 通用 | 中 |
| select | 通用 | 低 |

## 编译与安装

### Linux/Unix

```bash
# Make 方式
make && make packinstall

# CMake 方式
mkdir build && cd build
cmake ..
make
```

### Windows

使用 Visual Studio 打开对应的工程文件（如 `acl_cpp_vc2019.sln`）

### 依赖库

- **必需**: 无（核心功能无需外部依赖）
- **可选**: 
  - zlib（压缩功能）
  - OpenSSL/MbedTLS/PolarSSL（SSL/TLS 功能）
  - MySQL/PostgreSQL/SQLite（数据库功能）

**注意**: ACL 支持动态加载第三方库，即使需要这些功能，也无需编译时依赖。

## 快速开始

### 1. 协程示例

```cpp
#include <acl_cpp/lib_acl.hpp>
#include <fiber/libfiber.hpp>

void fiber_func() {
    printf("Hello from fiber %u\n", acl::fiber::self());
}

int main() {
    go fiber_func;  // 启动协程
    acl::fiber::schedule();  // 启动调度器
    return 0;
}
```

### 2. HTTP 客户端示例

```cpp
#include "acl_cpp/lib_acl.hpp"

int main() {
    acl::http_request req("www.example.com");
    req.request_header().set_url("/api/test");
    
    if (req.request(NULL, 0)) {
        acl::string body;
        req.get_body(body);
        printf("响应: %s\n", body.c_str());
    }
    return 0;
}
```

### 3. Redis 客户端示例

```cpp
#include "acl_cpp/lib_acl.hpp"

int main() {
    acl::acl_cpp_init();
    
    acl::redis_client client("127.0.0.1:6379", 10, 10);
    acl::redis_string cmd(&client);
    
    cmd.set("name", "Alice");
    
    acl::string value;
    cmd.clear();
    cmd.get("name", value);
    printf("GET: %s\n", value.c_str());
    
    return 0;
}
```

### 4. HTTP 服务器示例

```cpp
#include "acl_cpp/lib_acl.hpp"

class MyServlet : public acl::HttpServlet {
protected:
    bool doGet(acl::HttpServletRequest& req, 
               acl::HttpServletResponse& res) override {
        acl::string body("Hello, world!");  
        res.setStatus(200)
           .setContentType("text/plain")
           .setContentLength(body.size());
        return res.write(body);
    }
};
```

## 项目结构

```
acl/
├── lib_acl/              # 核心 C 库
│   ├── include/          # 头文件
│   └── src/              # 源码
├── lib_acl_cpp/          # C++ 封装库
│   ├── include/          # 头文件
│   ├── src/              # 源码
│   └── samples/          # 示例代码
├── lib_fiber/            # 协程库
│   ├── c/                # C 实现
│   ├── cpp/              # C++ 封装
│   └── samples/          # 示例代码
├── lib_protocol/         # 协议库
├── lib_tls/              # SSL/TLS 库
├── doc/                  # 文档目录
│   ├── fiber/            # 协程文档
│   ├── http/             # HTTP 文档
│   ├── redis/            # Redis 文档
│   ├── db/               # 数据库文档
│   ├── master/            # Master 框架文档
│   ├── stream/            # Stream 文档
│   └── ...
└── app/                   # 应用示例
```

## 适用场景

### 1. 高并发网络服务

- **Web 服务器**: HTTP/HTTPS 服务器
- **API 网关**: 微服务网关
- **实时通信**: WebSocket 服务器、IM 服务
- **代理服务器**: 反向代理、负载均衡

### 2. 分布式系统组件

- **缓存客户端**: Redis、Memcache 客户端
- **消息队列**: MQTT 客户端/服务端
- **数据库访问**: MySQL、PostgreSQL、SQLite 客户端

### 3. 网络工具

- **HTTP 客户端**: 爬虫、API 调用
- **网络监控**: 健康检查、性能监控
- **文件传输**: 断点续传、批量下载

### 4. 嵌入式系统

- **IoT 设备**: MQTT 客户端
- **移动应用**: Android/iOS 网络库

## 性能特点

### 1. 高并发能力

- **协程模型**: 支持数万到数十万并发连接
- **异步 I/O**: 事件驱动，高吞吐量
- **连接池**: 连接复用，减少连接开销

### 2. 低延迟

- **用户态调度**: 协程切换开销小
- **零拷贝**: 支持 sendfile 等零拷贝技术
- **事件引擎**: 高效的事件多路复用

### 3. 内存效率

- **内存池**: 减少内存分配开销
- **共享栈**: 协程支持共享栈模式
- **对象复用**: 连接池、对象池复用对象

### 4. CPU 效率

- **多核支持**: 多线程/多进程充分利用多核
- **批量操作**: Pipeline、批量查询提高吞吐量
- **异步处理**: 不阻塞主线程

## 文档资源

### 官方文档

- **项目主页**: https://github.com/acl-dev/acl
- **Gitee 镜像**: https://gitee.com/acl-dev/acl
- **在线文档**: https://acl-dev.cn/

### 模块文档

- **Fiber 协程**: `doc/fiber/README.md`
- **HTTP 模块**: `doc/http/README.md`
- **Redis 模块**: `doc/redis/README.md`
- **数据库模块**: `doc/db/README.md`
- **Master 框架**: `doc/master/README.md`
- **Stream 模块**: `doc/stream/README.md`
- **连接池**: `doc/connpool/README.md`
- **MQTT 模块**: `doc/mqtt/README.md`
- **MIME 模块**: `doc/mime/README.md`

### 示例代码

- **C++ 示例**: `lib_acl_cpp/samples/`
- **协程示例**: `lib_fiber/samples-c++/`
- **应用示例**: `app/`

## 社区支持

- **QQ群**: 705290654
- **GitHub Issues**: https://github.com/acl-dev/acl/issues
- **许可证**: Apache License 2.0

## 总结

ACL 项目是一个**成熟、完整、高性能**的 C/C++ 网络编程库，具有以下核心特点：

1. **架构清晰**: 分层设计，职责明确，易于理解和扩展
2. **功能完整**: 涵盖网络编程的各个方面，从底层 I/O 到高层协议
3. **性能优异**: 协程、异步 I/O、连接池等多种优化手段
4. **易于使用**: 统一的 API、丰富的示例、完善的文档
5. **跨平台**: 支持主流操作系统和编译器
6. **生产级**: 经过大量实践验证，适用于生产环境

无论是构建高并发 Web 服务、开发分布式系统组件，还是编写网络工具，ACL 都能提供强大而灵活的支持。

---

