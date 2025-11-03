# ACL Stream 架构设计

## 总体架构

ACL Stream 模块采用面向对象的设计，通过类继承体系提供统一的流接口。整个架构分为以下几个层次：

### 1. 核心层 (Core Layer)

#### 1.1 stream 基类

`stream` 是所有流类型的抽象基类，定义了流的基本属性和操作：

**核心功能：**
- 流的打开、关闭状态管理
- 读写超时设置
- 上下文对象绑定
- Hook 机制支持
- 缓冲区管理

**主要接口：**
```cpp
class stream {
public:
    bool close();                           // 关闭流
    bool eof() const;                       // 判断是否到达流末尾
    bool opened() const;                    // 判断流是否打开
    ACL_VSTREAM* get_vstream() const;       // 获取底层流对象
    bool set_rw_timeout(int n);             // 设置读写超时
    stream_hook* setup_hook(stream_hook*);  // 设置 Hook
    string& get_buf();                      // 获取内部缓冲区
};
```

**设计要点：**
- 使用虚析构函数，支持多态
- 提供 Hook 机制，允许自定义 I/O 行为
- 内部封装 ACL_VSTREAM，提供跨平台支持
- 支持多个上下文对象绑定（通过 key-value 映射）

#### 1.2 istream 输入流类

继承自 `stream`，提供输入操作接口：

**核心功能：**
- 原始数据读取
- 按行读取
- 按类型读取（整数、字符串等）
- Peek 操作（预读不消费）
- 标签分隔读取

**主要接口：**
```cpp
class istream : virtual public stream {
public:
    int read(void* buf, size_t size, bool loop = true);
    bool gets(string& s, bool nonl = true);
    bool read(int& n, bool loop = true);
    bool readtags(string& s, const string& tag);
    int getch();                            // 读取单个字符
    bool read_peek(string& buf);            // 预读数据
};
```

**设计特点：**
- 使用虚继承避免菱形继承问题
- 提供多种读取方式满足不同需求
- Peek 功能支持数据预览

#### 1.3 ostream 输出流类

继承自 `stream`，提供输出操作接口：

**核心功能：**
- 原始数据写入
- 格式化输出
- 向量写入（writev）
- UDP 数据发送
- 缓冲区刷新

**主要接口：**
```cpp
class ostream : virtual public stream {
public:
    int write(const void* data, size_t size, bool loop = true);
    int format(const char* fmt, ...);       // 格式化输出
    int writev(const struct iovec* v, int count);
    bool fflush();                          // 刷新缓冲区
    int sendto(const void* data, size_t size, const char* addr);
};
```

**设计特点：**
- 虚继承支持多重继承
- 支持缓冲和非缓冲写入
- 提供类似 C 标准库的格式化输出

### 2. 应用层 (Application Layer)

#### 2.1 fstream 文件流类

同时继承 `istream` 和 `ostream`，提供文件 I/O：

**核心功能：**
- 文件打开/创建/删除
- 文件定位（seek）
- 文件锁
- 文件大小查询

**主要接口：**
```cpp
class fstream : public istream, public ostream {
public:
    bool open(const char* path, unsigned int oflags, int mode);
    bool create(const char* path);
    bool remove();
    long long int fseek(long long int offset, int whence);
    bool lock(bool exclude = true);
};
```

**设计要点：**
- 支持多种打开模式（只读、只写、读写、追加等）
- 提供文件锁机制保证并发安全
- 跨平台文件操作封装

#### 2.2 socket_stream 网络流类

提供 TCP/UDP 网络通信：

**核心功能：**
- TCP 连接
- UDP 通信
- 多播支持
- 套接字选项设置
- 本地/远程地址获取

**主要接口：**
```cpp
class socket_stream : public istream, public ostream {
public:
    bool open(const char* addr, int conn_timeout, int rw_timeout);
    bool bind_udp(const char* addr, int rw_timeout = -1);
    const char* get_peer(bool full = false) const;
    socket_stream& set_tcp_nodelay(bool on);
    bool alive() const;                     // 检测连接是否存活
};
```

**设计亮点：**
- 支持多种地址格式（IP、域名、Unix 套接字）
- 提供连接存活检测
- 丰富的 TCP 选项配置

#### 2.3 server_socket 服务端套接字

用于服务端监听和接受连接：

**核心功能：**
- 地址绑定和监听
- 接受客户端连接
- 支持 TCP Fast Open
- 端口复用

**主要接口：**
```cpp
class server_socket {
public:
    bool open(const char* addr);
    socket_stream* accept(int timeout = -1);
    void set_tcp_defer_accept(int timeout);
};
```

#### 2.4 标准流类

- `stdin_stream`: 标准输入流封装
- `stdout_stream`: 标准输出流封装

**设计目的：**
提供统一的接口访问标准 I/O

### 3. 异步层 (Asynchronous Layer)

#### 3.1 aio_handle 异步事件引擎

**核心功能：**
- 事件循环管理
- 定时器支持
- 多种事件引擎（select、poll、epoll、kqueue、iocp）
- DNS 异步解析

**主要接口：**
```cpp
class aio_handle {
public:
    aio_handle(aio_handle_type engine_type = ENGINE_SELECT);
    bool check();                           // 事件循环
    void stop();                            // 停止事件循环
    long long int set_timer(aio_timer_callback*, long long int delay);
};
```

**支持的事件引擎：**
- `ENGINE_SELECT`: select 模式（跨平台）
- `ENGINE_POLL`: poll 模式（Unix）
- `ENGINE_KERNEL`: 高效内核模式
  - Linux: epoll
  - FreeBSD: kqueue
  - Solaris: devpoll
  - Windows: iocp
- `ENGINE_WINMSG`: Windows 消息循环模式

#### 3.2 aio_stream 异步流基类

**核心功能：**
- 异步回调管理
- 关闭回调
- 超时回调
- Hook 支持

**主要接口：**
```cpp
class aio_stream {
public:
    void close(bool flush_out = false);
    void add_close_callback(aio_callback* callback);
    void add_timeout_callback(aio_callback* callback);
    ACL_ASTREAM* get_astream() const;
};
```

**回调接口：**
```cpp
class aio_callback {
public:
    virtual bool read_callback(char* data, int len);
    virtual bool write_callback();
    virtual void close_callback();
    virtual bool timeout_callback();
};
```

#### 3.3 异步流派生类

- `aio_istream`: 异步输入流
- `aio_ostream`: 异步输出流
- `aio_fstream`: 异步文件流
- `aio_socket_stream`: 异步网络流
- `aio_listen_stream`: 异步监听流

### 4. 扩展层 (Extension Layer)

#### 4.1 SSL/TLS 支持

**类层次：**
```
sslbase_io (SSL 基类)
├── openssl_io (OpenSSL 实现)
├── mbedtls_io (mbed TLS 实现)
└── polarssl_io (PolarSSL 实现)
```

**核心功能：**
- SSL 握手
- SNI 支持
- 证书验证
- 加密通信

**配置类：**
```
sslbase_conf (配置基类)
├── openssl_conf
├── mbedtls_conf
└── polarssl_conf
```

#### 4.2 Hook 机制

`stream_hook` 允许用户自定义 I/O 行为：

```cpp
class stream_hook {
public:
    virtual int read(void* buf, size_t len) = 0;
    virtual int send(const void* buf, size_t len) = 0;
    virtual bool open(ACL_VSTREAM* s) = 0;
    virtual bool on_close(bool alive);
};
```

**应用场景：**
- SSL/TLS 加密
- 数据压缩
- 协议转换
- 流量统计
- 自定义加密

## 设计模式

### 1. 模板方法模式

基类定义算法骨架，子类实现具体步骤：
- `stream` 定义流操作流程
- 子类实现特定类型的 I/O

### 2. 策略模式

通过 Hook 机制实现不同的 I/O 策略：
- 默认策略：直接系统调用
- SSL 策略：加密传输
- 自定义策略：用户实现

### 3. 观察者模式

异步流的回调机制：
- `aio_stream` 是主题
- `aio_callback` 是观察者
- 支持多个观察者

### 4. 适配器模式

封装底层 C 接口为 C++ 接口：
- `ACL_VSTREAM` -> `stream`
- `ACL_ASTREAM` -> `aio_stream`

## 内存管理

### 1. 缓冲区管理

- 每个流内置 `string` 缓冲区
- 支持 `dbuf_pool` 动态缓冲池
- 自动扩展，避免频繁分配

### 2. 对象生命周期

**同步流：**
- 用户手动管理（栈或堆分配）
- RAII 原则，析构自动清理

**异步流：**
- 只能在堆上分配
- 通过 `destroy()` 自动释放
- 延迟释放机制（`aio_delay_free`）

### 3. 资源释放

- `close()` 关闭流
- `unbind()` 解绑底层资源
- 析构函数确保资源释放

## 线程安全

### 同步流

- **非线程安全**：同一个流对象不应被多线程同时访问
- **建议**：每个线程使用独立的流对象

### 异步流

- **单线程模型**：异步流绑定到特定的 `aio_handle`
- **回调执行**：所有回调在事件循环线程执行
- **注意**：避免在回调中执行阻塞操作

## 性能优化

### 1. 缓冲策略

- 读缓冲：默认 4KB，可通过 `set_rbuf_size()` 调整
- 写缓冲：默认 512B，可通过 `set_wbuf_size()` 调整
- 批量写入：使用 `writev()` 减少系统调用

### 2. 零拷贝

- 支持 `sendfile` (Linux)
- 支持 `TransmitFile` (Windows)
- Peek 操作避免数据复制

### 3. 事件引擎选择

- Linux: 优先 epoll
- FreeBSD: 优先 kqueue
- Windows: 优先 iocp
- 通用: select/poll 作为后备

### 4. TCP 优化

- `set_tcp_nodelay()`: 禁用 Nagle 算法
- `set_tcp_sendbuf/recvbuf()`: 调整缓冲区
- `set_tcp_defer_accept()`: 延迟接受连接

## 错误处理

### 1. 错误检测

- 返回值检查：`false` 或 `-1` 表示错误
- `eof()` 检查流状态
- `acl::last_error()` 获取系统错误码

### 2. 超时处理

- 读写超时自动返回
- 异步流通过 `timeout_callback()` 处理
- 可设置不同的连接超时和读写超时

### 3. 异常安全

- 不抛出异常，使用返回值表示错误
- RAII 确保资源正确释放
- 析构函数不会失败

## 平台兼容性

### 支持的平台

- Linux (kernel 2.6+)
- FreeBSD
- macOS
- Solaris
- Windows (XP+)

### 平台差异处理

- 套接字类型：Unix 使用 `int`，Windows 使用 `SOCKET`
- 文件句柄：Unix 使用 `int`，Windows 使用 `HANDLE`
- 路径分隔符：自动处理
- Unix 域套接字：Windows 10+ 支持

## 扩展性

### 1. 自定义流类型

继承 `istream` 和/或 `ostream`：

```cpp
class my_stream : public istream, public ostream {
    // 实现自定义流
};
```

### 2. 自定义 Hook

实现 `stream_hook` 接口：

```cpp
class my_hook : public stream_hook {
    int read(void* buf, size_t len) override;
    int send(const void* buf, size_t len) override;
    bool open(ACL_VSTREAM* s) override;
};
```

### 3. 自定义事件引擎

虽然不常见，但可以扩展 `aio_handle` 支持新的事件机制。

## 最佳实践

1. **使用 RAII**：让对象生命周期管理资源
2. **检查返回值**：每次 I/O 操作后检查
3. **设置超时**：避免无限期阻塞
4. **合理缓冲**：根据应用场景调整缓冲区大小
5. **异步优先**：高并发场景使用异步 I/O
6. **复用连接**：减少连接建立开销
7. **错误处理**：完善的错误处理逻辑

## 总结

ACL Stream 模块提供了一套完整、高效、易用的流 I/O 框架：

- **统一接口**：同步/异步、文件/网络使用相似 API
- **高性能**：优化的缓冲管理和事件引擎
- **可扩展**：Hook 机制支持功能扩展
- **跨平台**：封装平台差异，统一接口
- **SSL 支持**：内置多种 SSL 实现
- **易用性**：符合 C++ 习惯的 API 设计

