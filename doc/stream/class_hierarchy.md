# ACL Stream 类层次结构

## 完整类图

```
acl::noncopyable
├── acl::stream (抽象基类)
│   ├── acl::istream (虚继承)
│   │   ├── acl::fstream (同时继承 ostream)
│   │   ├── acl::socket_stream (同时继承 ostream)
│   │   ├── acl::stdin_stream
│   │   └── acl::ifstream
│   │
│   └── acl::ostream (虚继承)
│       ├── acl::fstream (同时继承 istream)
│       ├── acl::socket_stream (同时继承 istream)
│       ├── acl::stdout_stream
│       └── acl::ofstream
│
├── acl::aio_stream (异步流抽象基类)
│   ├── acl::aio_istream
│   ├── acl::aio_ostream
│   ├── acl::aio_fstream
│   ├── acl::aio_socket_stream
│   └── acl::aio_listen_stream
│
├── acl::aio_handle (异步事件引擎)
│
├── acl::server_socket (服务器监听)
│
├── acl::stream_hook (Hook 接口)
│   └── acl::sslbase_io
│       ├── acl::openssl_io
│       ├── acl::mbedtls_io
│       └── acl::polarssl_io
│
├── acl::sslbase_conf (SSL 配置基类)
│   ├── acl::openssl_conf
│   ├── acl::mbedtls_conf
│   └── acl::polarssl_conf
│
├── acl::aio_callback (异步回调接口)
│   └── (用户自定义回调类)
│
└── acl::aio_timer_callback (定时器回调接口)
    └── (用户自定义定时器类)
```

## 同步流体系

### 基础流类

#### stream (基类)
- **文件**: `stream.hpp`
- **功能**: 所有流的抽象基类
- **特性**: 
  - 流状态管理（打开/关闭/EOF）
  - 超时设置
  - 上下文绑定
  - Hook 机制
  - 缓冲区管理

#### istream (输入流)
- **文件**: `istream.hpp`
- **继承**: virtual public stream
- **功能**: 输入操作接口
- **特性**:
  - 原始数据读取
  - 按行读取
  - 类型化读取（整数、字符串等）
  - Peek 操作
  - 标签分隔读取

#### ostream (输出流)
- **文件**: `ostream.hpp`
- **继承**: virtual public stream
- **功能**: 输出操作接口
- **特性**:
  - 原始数据写入
  - 格式化输出
  - 向量写入（writev）
  - UDP 发送
  - 缓冲区刷新

### 文件流类

#### fstream (文件流)
- **文件**: `fstream.hpp`
- **继承**: public istream, public ostream
- **功能**: 文件 I/O
- **特性**:
  - 文件打开/创建/删除
  - 文件定位（seek）
  - 文件锁
  - 文件大小查询
  - 支持多种打开模式

#### ifstream (只读文件流)
- **文件**: `ifstream.hpp`
- **继承**: public istream
- **功能**: 只读文件访问

#### ofstream (只写文件流)
- **文件**: `ofstream.hpp`
- **继承**: public ostream
- **功能**: 只写文件访问

### 网络流类

#### socket_stream (网络流)
- **文件**: `socket_stream.hpp`
- **继承**: public istream, public ostream
- **功能**: TCP/UDP 网络通信
- **特性**:
  - TCP 连接
  - UDP 通信
  - 多播支持
  - 套接字选项设置
  - 地址查询
  - 连接存活检测

#### server_socket (服务器套接字)
- **文件**: `server_socket.hpp`
- **继承**: public noncopyable
- **功能**: 服务端监听和接受连接
- **特性**:
  - 地址绑定和监听
  - 接受客户端连接
  - 支持 TCP Fast Open
  - 端口复用
  - Unix 域套接字支持

### 标准流类

#### stdin_stream (标准输入流)
- **文件**: `stdin_stream.hpp`
- **继承**: public istream
- **功能**: 标准输入封装

#### stdout_stream (标准输出流)
- **文件**: `stdout_stream.hpp`
- **继承**: public ostream
- **功能**: 标准输出封装

## 异步流体系

### 异步基础类

#### aio_handle (异步事件引擎)
- **文件**: `aio_handle.hpp`
- **继承**: private noncopyable
- **功能**: 异步 I/O 事件循环
- **特性**:
  - 多种事件引擎（select/poll/epoll/kqueue/iocp）
  - 定时器支持
  - DNS 异步解析
  - 事件循环控制

#### aio_stream (异步流基类)
- **文件**: `aio_stream.hpp`
- **继承**: public noncopyable
- **功能**: 异步流抽象基类
- **特性**:
  - 异步回调管理
  - 关闭/超时回调
  - Hook 支持
  - 与 aio_handle 绑定

### 异步流派生类

#### aio_istream (异步输入流)
- **文件**: `aio_istream.hpp`
- **继承**: public aio_stream
- **功能**: 异步读取操作

#### aio_ostream (异步输出流)
- **文件**: `aio_ostream.hpp`
- **继承**: public aio_stream
- **功能**: 异步写入操作

#### aio_fstream (异步文件流)
- **文件**: `aio_fstream.hpp`
- **继承**: public aio_stream
- **功能**: 异步文件 I/O

#### aio_socket_stream (异步网络流)
- **文件**: `aio_socket_stream.hpp`
- **继承**: public aio_stream
- **功能**: 异步网络通信

#### aio_listen_stream (异步监听流)
- **文件**: `aio_listen_stream.hpp`
- **继承**: public aio_stream
- **功能**: 异步接受连接

### 异步回调接口

#### aio_callback (异步回调)
- **文件**: `aio_stream.hpp`
- **继承**: public noncopyable
- **功能**: 异步操作回调接口
- **方法**:
  - `open_callback()`: 连接建立
  - `read_callback()`: 数据读取
  - `write_callback()`: 数据写入
  - `close_callback()`: 连接关闭
  - `timeout_callback()`: 超时处理

#### aio_timer_callback (定时器回调)
- **文件**: `aio_timer_callback.hpp`
- **继承**: public noncopyable
- **功能**: 定时器回调接口
- **方法**:
  - `timer_callback()`: 定时器触发
  - `destroy()`: 销毁回调

## SSL/TLS 体系

### SSL 配置类

#### sslbase_conf (SSL 配置基类)
- **文件**: `sslbase_conf.hpp`
- **继承**: public noncopyable
- **功能**: SSL/TLS 配置抽象基类

#### openssl_conf
- **文件**: `openssl_conf.hpp`
- **继承**: public sslbase_conf
- **功能**: OpenSSL 配置

#### mbedtls_conf
- **文件**: `mbedtls_conf.hpp`
- **继承**: public sslbase_conf
- **功能**: mbed TLS 配置

#### polarssl_conf
- **文件**: `polarssl_conf.hpp`
- **继承**: public sslbase_conf
- **功能**: PolarSSL 配置

### SSL I/O 类

#### sslbase_io (SSL IO 基类)
- **文件**: `sslbase_io.hpp`
- **继承**: public stream_hook
- **功能**: SSL/TLS I/O 抽象基类
- **特性**:
  - SSL 握手
  - SNI 支持
  - 客户端/服务器模式

#### openssl_io
- **文件**: `openssl_io.hpp`
- **继承**: public sslbase_io
- **功能**: OpenSSL 实现

#### mbedtls_io
- **文件**: `mbedtls_io.hpp`
- **继承**: public sslbase_io
- **功能**: mbed TLS 实现

#### polarssl_io
- **文件**: `polarssl_io.hpp`
- **继承**: public sslbase_io
- **功能**: PolarSSL 实现

## Hook 机制

#### stream_hook (Hook 接口)
- **文件**: `stream_hook.hpp`
- **继承**: public noncopyable
- **功能**: 自定义 I/O 行为
- **方法**:
  - `read()`: 自定义读取
  - `send()`: 自定义发送
  - `open()`: 初始化
  - `on_close()`: 关闭前处理
  - `destroy()`: 销毁

## 辅助类

#### aio_delay_free
- **文件**: `aio_delay_free.hpp`
- **功能**: 延迟释放对象

## 类关系说明

### 继承关系

1. **虚继承**: `istream` 和 `ostream` 虚继承 `stream`，避免菱形继承问题
2. **多重继承**: `fstream` 和 `socket_stream` 同时继承输入输出流
3. **抽象基类**: `stream` 和 `aio_stream` 是抽象基类，不能直接实例化

### 组合关系

1. **aio_stream** 持有 **aio_handle** 引用
2. **stream** 可以持有 **stream_hook** 指针
3. **sslbase_io** 关联 **sslbase_conf**

### 依赖关系

1. SSL I/O 类依赖于相应的 SSL 库（OpenSSL/mbedTLS/PolarSSL）
2. 异步流依赖于异步事件引擎
3. 所有流类依赖于底层 ACL C 库

## 设计模式应用

### 模板方法模式
- **stream** 定义流操作骨架
- 子类实现具体 I/O 操作

### 策略模式
- **stream_hook** 允许替换 I/O 策略
- SSL 实现是 Hook 的应用

### 观察者模式
- **aio_callback** 作为观察者
- **aio_stream** 作为主题

### 适配器模式
- C++ 类封装底层 C 接口
- 提供面向对象的接口

### 外观模式
- **stream** 为复杂的底层 I/O 提供简单接口

## 使用场景映射

| 场景 | 推荐类 |
|------|--------|
| 读取文本文件 | `fstream` / `ifstream` |
| 写入日志文件 | `fstream` / `ofstream` |
| TCP 客户端 | `socket_stream` |
| TCP 服务器 | `server_socket` + `socket_stream` |
| UDP 通信 | `socket_stream` |
| HTTPS 客户端 | `socket_stream` + `openssl_io` |
| HTTPS 服务器 | `server_socket` + `socket_stream` + `openssl_io` |
| 高并发服务器 | `aio_handle` + `aio_socket_stream` |
| 异步 HTTP 客户端 | `aio_handle` + `aio_socket_stream` |
| 定时任务 | `aio_handle` + `aio_timer_callback` |
| 标准输入输出 | `stdin_stream` / `stdout_stream` |
| 自定义协议 | `stream` + 自定义 `stream_hook` |
| 数据压缩传输 | `socket_stream` + 压缩 `stream_hook` |

## 线程模型

### 同步流
- 每个流对象仅在一个线程中使用
- 不同流对象可以在不同线程中使用
- 需要用户自行管理线程安全

### 异步流
- 单线程模型，所有回调在事件循环线程执行
- 一个 `aio_handle` 通常对应一个线程
- 避免在回调中执行阻塞操作

## 内存管理

### 同步流
- 支持栈分配或堆分配
- 用户负责生命周期管理
- 使用 RAII 自动清理

### 异步流
- 只能在堆上分配
- 通过 `destroy()` 或 `close()` 自动释放
- 不要手动 delete

### Hook 对象
- 由 stream 管理生命周期
- stream 销毁时自动调用 `destroy()`

## 性能特性

| 类 | 适用数据量 | 并发能力 | 延迟 | CPU 开销 |
|----|-----------|---------|------|---------|
| fstream | 任意 | 单连接 | 低 | 低 |
| socket_stream | 中小 | 多线程 | 中 | 中 |
| aio_socket_stream | 任意 | 高并发 | 低 | 低 |

## 平台支持

| 类/功能 | Linux | Windows | macOS | FreeBSD |
|---------|-------|---------|-------|---------|
| stream | ✓ | ✓ | ✓ | ✓ |
| fstream | ✓ | ✓ | ✓ | ✓ |
| socket_stream | ✓ | ✓ | ✓ | ✓ |
| Unix 域套接字 | ✓ | ✓ (Win10+) | ✓ | ✓ |
| aio_handle (epoll) | ✓ | - | - | - |
| aio_handle (kqueue) | - | - | ✓ | ✓ |
| aio_handle (iocp) | - | ✓ | - | - |
| TCP Fast Open | ✓ | - | ✓ | ✓ |
| SO_REUSEPORT | ✓ | - | ✓ | ✓ |

## 版本演进

### 向后兼容
- 保持 API 稳定
- 新功能通过新方法添加
- 废弃的方法保留但标记

### 未来扩展
- HTTP/2 支持
- QUIC 协议支持
- io_uring 支持（Linux 5.1+）
- 协程支持（C++20）

