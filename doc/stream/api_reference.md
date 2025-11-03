# ACL Stream API 参考

本文档提供 ACL Stream 模块主要类的 API 详细说明。

## 目录

- [stream 基类](#stream-基类)
- [istream 输入流](#istream-输入流)
- [ostream 输出流](#ostream-输出流)
- [fstream 文件流](#fstream-文件流)
- [socket_stream 网络流](#socket_stream-网络流)
- [server_socket 服务器套接字](#server_socket-服务器套接字)
- [aio_handle 异步事件引擎](#aio_handle-异步事件引擎)
- [aio_stream 异步流](#aio_stream-异步流)
- [stream_hook Hook 机制](#stream_hook-hook-机制)

---

## stream 基类

所有流类型的抽象基类。

### 构造和析构

```cpp
stream();
virtual ~stream() = 0;
```

### 状态管理

#### close()
```cpp
bool close();
```
关闭流并释放资源。

**返回值：**
- `true`: 关闭成功
- `false`: 关闭失败

#### eof()
```cpp
bool eof() const;
```
判断流是否已到达末尾或已关闭。

**返回值：**
- `true`: 流已结束
- `false`: 流未结束

#### opened()
```cpp
bool opened() const;
```
判断流是否处于打开状态。

**返回值：**
- `true`: 流已打开
- `false`: 流未打开

#### clear_eof()
```cpp
void clear_eof();
```
清除流的 EOF 标志位。

### 超时设置

#### set_rw_timeout()
```cpp
bool set_rw_timeout(int n, bool use_sockopt = false);
```
设置读写超时时间。

**参数：**
- `n`: 超时时间，单位取决于时间单位设置
- `use_sockopt`: 是否使用 setsockopt 设置超时

**返回值：**
- `true`: 设置成功
- `false`: 设置失败

#### get_rw_timeout()
```cpp
int get_rw_timeout(bool use_sockopt = false) const;
```
获取读写超时时间（秒）。

#### set_time_unit()
```cpp
void set_time_unit(time_unit_t unit);
```
设置时间单位。

**参数：**
- `unit`: 时间单位
  - `time_unit_s`: 秒
  - `time_unit_ms`: 毫秒
  - `time_unit_us`: 微秒
  - `time_unit_ns`: 纳秒

### 底层访问

#### get_vstream()
```cpp
ACL_VSTREAM* get_vstream() const;
```
获取底层 ACL_VSTREAM 对象指针。

#### unbind()
```cpp
ACL_VSTREAM* unbind();
```
解除与底层 ACL_VSTREAM 的绑定，并返回该对象。调用后，流对象不再拥有底层资源的所有权。

### 上下文管理

#### set_ctx()
```cpp
bool set_ctx(void* ctx, const char* key = NULL, bool replace = true);
```
设置流的绑定对象。

**参数：**
- `ctx`: 上下文对象指针
- `key`: 标识 ctx 的键（NULL 表示默认上下文）
- `replace`: 当对应的 key 存在时是否覆盖

**返回值：**
- `true`: 设置成功
- `false`: 设置失败（replace=false 且 key 已存在）

#### get_ctx()
```cpp
void* get_ctx(const char* key = NULL) const;
```
获取绑定的对象。

**参数：**
- `key`: 键值（NULL 获取默认上下文）

**返回值：**
- 上下文对象指针，不存在时返回 NULL

#### del_ctx()
```cpp
void* del_ctx(const char* key = NULL);
```
删除并返回绑定的对象。

### Hook 机制

#### setup_hook()
```cpp
stream_hook* setup_hook(stream_hook* hook);
```
设置读写 Hook 对象。

**参数：**
- `hook`: Hook 对象指针

**返回值：**
- 成功时返回 hook 参数本身
- 失败时返回其他值

#### get_hook()
```cpp
stream_hook* get_hook() const;
```
获取当前的 Hook 对象。

#### remove_hook()
```cpp
stream_hook* remove_hook();
```
删除当前的 Hook 对象并返回，恢复默认读写行为。

### 缓冲区

#### get_buf()
```cpp
string& get_buf();
```
获取流内部的字符串缓冲区。

#### get_dbuf()
```cpp
dbuf_pool& get_dbuf();
```
获取流的动态缓冲池。

---

## istream 输入流

输入流类，提供各种读取操作。

### 原始数据读取

#### read()
```cpp
int read(void* buf, size_t size, bool loop = true);
```
从流中读取数据。

**参数：**
- `buf`: 用户缓冲区
- `size`: 缓冲区大小
- `loop`: 是否循环读取直到填满缓冲区

**返回值：**
- 读取的字节数
- `-1`: 读取失败或连接关闭

### 按行读取

#### gets()
```cpp
bool gets(void* buf, size_t* size_inout, bool nonl = true);
bool gets(string& s, bool nonl = true, size_t max = 0);
bool gets(string* s, bool nonl = true, size_t max = 0);
```
读取一行数据。

**参数：**
- `buf`: 缓冲区
- `size_inout`: 输入时表示缓冲区大小，输出时表示实际读取长度
- `s`: 字符串对象
- `nonl`: 是否去除行尾的 `\r\n` 或 `\n`
- `max`: 最大长度限制（0 表示无限制）

**返回值：**
- `true`: 成功读取一行
- `false`: 读取失败或到达末尾

### 类型化读取

#### read(int&)
```cpp
bool read(int& n, bool loop = true);
```
读取一个 32 位整数。

#### read(short&)
```cpp
bool read(short& n, bool loop = true);
```
读取一个 16 位整数。

#### read(long long&)
```cpp
bool read(long long int& n, bool loop = true);
```
读取一个 64 位整数。

#### read(char&)
```cpp
bool read(char& ch);
```
读取一个字节。

#### read(string&)
```cpp
bool read(string& s, bool loop = true);
bool read(string& s, size_t max, bool loop = true);
```
读取字符串数据。

**参数：**
- `s`: 字符串对象
- `max`: 读取的最大字节数
- `loop`: 是否循环读取

### 标签分隔读取

#### readtags()
```cpp
bool readtags(void* buf, size_t* inout, const char* tag, size_t len);
bool readtags(string& s, const string& tag);
```
读取数据直到遇到指定的标签字符串。

**参数：**
- `buf`: 缓冲区
- `inout`: 输入/输出长度
- `tag`: 标签字符串
- `len`: 标签长度
- `s`: 字符串对象

### 单字符操作

#### getch()
```cpp
int getch();
```
读取一个字节。

**返回值：**
- 字节的 ASCII 值（0-255）
- `-1`: 读取失败

#### ugetch()
```cpp
int ugetch(int ch);
```
将一个字节放回缓冲区。

**参数：**
- `ch`: 要放回的字节

### Peek 操作

#### read_peek()
```cpp
int read_peek(void* buf, size_t size);
bool read_peek(string& buf, bool clear = false);
```
预读数据，不从缓冲区移除。

**参数：**
- `buf`: 缓冲区
- `size`: 缓冲区大小
- `clear`: 是否清空字符串缓冲区

**返回值：**
- `int` 版本：读取的字节数，-1 表示失败
- `bool` 版本：是否成功

#### gets_peek()
```cpp
bool gets_peek(string& buf, bool nonl = true, bool clear = false, int max = 0);
```
预读一行数据。

#### readn_peek()
```cpp
bool readn_peek(string& buf, size_t cnt, bool clear = false);
```
预读指定长度的数据。

### 等待和检测

#### readable()
```cpp
bool readable() const;
```
检测流是否可读。

#### read_wait()
```cpp
bool read_wait(int timeo) const;
```
等待流变为可读状态。

**参数：**
- `timeo`: 超时时间（秒）

### 流操作符

```cpp
istream& operator>>(string& s);
istream& operator>>(int& n);
istream& operator>>(short& n);
istream& operator>>(long long int& n);
istream& operator>>(char& ch);
```

### 静态方法

#### set_rbuf_size()
```cpp
static void set_rbuf_size(size_t n);
```
设置全局读缓冲区大小（默认 4096 字节）。

---

## ostream 输出流

输出流类，提供各种写入操作。

### 原始数据写入

#### write()
```cpp
int write(const void* data, size_t size, bool loop = true, bool buffed = false);
```
写入数据到流。

**参数：**
- `data`: 数据指针
- `size`: 数据长度
- `loop`: 是否循环写入直到全部完成
- `buffed`: 是否先缓冲

**返回值：**
- 实际写入的字节数
- `-1`: 写入失败

### UDP 发送

#### sendto()
```cpp
int sendto(const void* data, size_t size, const char* dest_addr, int flags = 0);
int sendto(const void* data, size_t size, const struct sockaddr* dest_addr, 
           int addrlen, int flags = 0);
```
使用 UDP 方式发送数据。

**参数：**
- `data`: 数据指针
- `size`: 数据长度
- `dest_addr`: 目标地址
- `flags`: 发送标志

### 缓冲区刷新

#### fflush()
```cpp
bool fflush();
```
刷新写缓冲区。

**返回值：**
- `true`: 刷新成功
- `false`: 刷新失败

### 等待和检测

#### write_wait()
```cpp
bool write_wait(int timeo) const;
```
等待流变为可写状态。

### 向量写入

#### writev()
```cpp
int writev(const struct iovec* v, int count, bool loop = true);
```
使用 writev 方式写入数据。

**参数：**
- `v`: iovec 结构数组
- `count`: 数组元素个数
- `loop`: 是否循环写入

### 格式化输出

#### format()
```cpp
int format(const char* fmt, ...);
```
类似 fprintf 的格式化输出。

**参数：**
- `fmt`: 格式字符串
- `...`: 可变参数

**返回值：**
- 写入的字节数
- `-1`: 失败

#### vformat()
```cpp
int vformat(const char* fmt, va_list ap);
```
使用 va_list 的格式化输出。

### 类型化写入

#### write(int)
```cpp
int write(int n);
```
写入一个 32 位整数。

#### write(short)
```cpp
int write(short n);
```
写入一个 16 位整数。

#### write(long long)
```cpp
int write(long long int n);
```
写入一个 64 位整数。

#### write(char)
```cpp
int write(char ch);
```
写入一个字节。

#### write(string)
```cpp
int write(const string& s, bool loop = true);
```
写入字符串。

### 行输出

#### puts()
```cpp
int puts(const char* s);
```
写入字符串并添加 `\r\n`。

### 流操作符

```cpp
ostream& operator<<(const string& s);
ostream& operator<<(const char* s);
ostream& operator<<(int n);
ostream& operator<<(short n);
ostream& operator<<(long long int n);
ostream& operator<<(char ch);
```

### 静态方法

#### set_wbuf_size()
```cpp
static void set_wbuf_size(size_t n);
```
设置全局写缓冲区大小（默认 512 字节）。

---

## fstream 文件流

文件流类，同时继承 istream 和 ostream。

### 文件打开

#### open()
```cpp
bool open(const char* path, unsigned int oflags, int mode);
```
打开文件。

**参数：**
- `path`: 文件路径
- `oflags`: 打开标志位（O_RDONLY, O_WRONLY, O_RDWR, O_CREAT, O_TRUNC 等）
- `mode`: 文件权限（如 0644）

**返回值：**
- `true`: 打开成功
- `false`: 打开失败

#### open_trunc()
```cpp
bool open_trunc(const char* path);
```
以读写方式打开文件，如果文件存在则截断为 0。

#### create()
```cpp
bool create(const char* path);
```
创建新文件（文件权限 0700）。

### 文件操作

#### remove()
```cpp
bool remove();
```
删除文件（需要知道文件路径）。

#### rename()
```cpp
bool rename(const char* from_path, const char* to_path);
```
重命名文件。

### 文件定位

#### fseek()
```cpp
long long int fseek(long long int offset, int whence);
```
移动文件指针。

**参数：**
- `offset`: 偏移量
- `whence`: 起始位置
  - `SEEK_SET`: 文件开始
  - `SEEK_CUR`: 当前位置
  - `SEEK_END`: 文件末尾

**返回值：**
- 新位置（>= 0）
- `-1`: 失败

#### ftell()
```cpp
long long int ftell();
```
获取当前文件指针位置。

#### fsize()
```cpp
long long int fsize() const;
static long long int fsize(const char* path);
```
获取文件大小。

### 文件截断

#### ftruncate()
```cpp
bool ftruncate(long long int length);
```
截断文件到指定长度。

### 文件锁

#### lock()
```cpp
bool lock(bool exclude = true);
```
对文件加锁。

**参数：**
- `exclude`: true 为独占锁，false 为共享锁

**返回值：**
- `true`: 加锁成功
- `false`: 加锁失败

#### try_lock()
```cpp
bool try_lock(bool exclude = true);
```
尝试对文件加锁（非阻塞）。

#### unlock()
```cpp
bool unlock();
```
解锁文件。

### 其他

#### file_path()
```cpp
const char* file_path() const;
```
获取文件路径。

#### file_handle()
```cpp
int file_handle() const;  // Unix
void* file_handle() const;  // Windows
```
获取系统文件句柄。

---

## socket_stream 网络流

网络流类，支持 TCP/UDP 通信。

### TCP 连接

#### open()
```cpp
bool open(const char* addr, int conn_timeout, int rw_timeout, 
          time_unit_t unit = time_unit_s);
```
连接到远程服务器。

**参数：**
- `addr`: 服务器地址（格式：host:port 或 /path/to/unix.sock）
- `conn_timeout`: 连接超时
- `rw_timeout`: 读写超时
- `unit`: 时间单位

**返回值：**
- `true`: 连接成功
- `false`: 连接失败

#### open(fd)
```cpp
bool open(int fd, bool udp_mode = false);  // Unix
bool open(SOCKET fd, bool udp_mode = false);  // Windows
```
从已有套接字创建流。

### UDP 绑定

#### bind_udp()
```cpp
bool bind_udp(const char* addr, int rw_timeout = -1, unsigned flags = 0);
```
绑定 UDP 地址。

**参数：**
- `addr`: 本地地址（格式：ip:port）
- `rw_timeout`: 读写超时
- `flags`: 标志位

### 多播

#### bind_multicast()
```cpp
bool bind_multicast(const char* addr, const char* iface, int port,
                    int rw_timeout = -1, unsigned flags = 0);
```
绑定多播组。

**参数：**
- `addr`: 多播地址
- `iface`: 网络接口 IP
- `port`: 端口号
- `rw_timeout`: 读写超时

#### multicast_set_ttl()
```cpp
bool multicast_set_ttl(int ttl);
```
设置多播 TTL。

#### multicast_drop()
```cpp
bool multicast_drop(const char* addr, const char* iface);
```
退出多播组。

### 地址信息

#### get_peer()
```cpp
const char* get_peer(bool full = false) const;
```
获取远程地址。

**参数：**
- `full`: true 返回 IP:PORT，false 仅返回 IP

#### get_peer_ip()
```cpp
const char* get_peer_ip() const;
```
获取远程 IP 地址。

#### set_peer()
```cpp
bool set_peer(const char* addr);
```
设置远程地址（用于 UDP）。

#### get_local()
```cpp
const char* get_local(bool full = false) const;
```
获取本地地址。

#### get_local_ip()
```cpp
const char* get_local_ip() const;
```
获取本地 IP 地址。

### 套接字选项

#### set_tcp_nodelay()
```cpp
socket_stream& set_tcp_nodelay(bool on);
```
设置 TCP_NODELAY（禁用 Nagle 算法）。

#### set_tcp_solinger()
```cpp
socket_stream& set_tcp_solinger(bool on, int linger);
```
设置 SO_LINGER 选项。

#### set_tcp_sendbuf()
```cpp
socket_stream& set_tcp_sendbuf(int size);
```
设置发送缓冲区大小。

#### set_tcp_recvbuf()
```cpp
socket_stream& set_tcp_recvbuf(int size);
```
设置接收缓冲区大小。

#### set_tcp_non_blocking()
```cpp
socket_stream& set_tcp_non_blocking(bool on);
```
设置非阻塞模式。

### 状态检测

#### alive()
```cpp
bool alive(double* tc1 = NULL, double* tc2 = NULL) const;
```
检测连接是否存活。

**参数：**
- `tc1`: 记录第一阶段耗时（毫秒）
- `tc2`: 记录第二阶段耗时（毫秒）

**返回值：**
- `true`: 连接正常
- `false`: 连接已断开

### 关闭控制

#### shutdown_read()
```cpp
bool shutdown_read();
```
关闭读端。

#### shutdown_write()
```cpp
bool shutdown_write();
```
关闭写端。

### 其他

#### sock_handle()
```cpp
int sock_handle() const;  // Unix
SOCKET sock_handle() const;  // Windows
```
获取套接字句柄。

#### unbind_sock()
```cpp
int unbind_sock();  // Unix
SOCKET unbind_sock();  // Windows
```
解绑套接字并返回句柄。

---

## server_socket 服务器套接字

服务器监听套接字类。

### 构造函数

```cpp
server_socket(unsigned flag, int backlog);
server_socket(int fd);
server_socket(SOCKET fd);
```

**参数：**
- `flag`: 打开标志
  - `OPEN_FLAG_NONBLOCK`: 非阻塞
  - `OPEN_FLAG_REUSEPORT`: 端口复用
  - `OPEN_FLAG_FASTOPEN`: TCP Fast Open
- `backlog`: 监听队列长度

### 监听

#### open()
```cpp
bool open(const char* addr);
```
绑定地址并开始监听。

**参数：**
- `addr`: 监听地址
  - TCP: `ip:port`
  - Unix 域套接字: `/path/to/sock`
  - Linux Abstract Socket: `@name`

**返回值：**
- `true`: 成功
- `false`: 失败

### 接受连接

#### accept()
```cpp
socket_stream* accept(int timeout = -1, bool* etimed = NULL);
```
接受客户端连接。

**参数：**
- `timeout`: 超时时间（秒），-1 表示永久等待
- `etimed`: 输出是否超时

**返回值：**
- 客户端流对象（需要 delete 释放）
- `NULL`: 失败或超时

#### shared_accept() (C++11)
```cpp
shared_stream shared_accept(int timeout = -1, bool* etimed = NULL);
```
返回智能指针管理的客户端连接。

### 状态管理

#### opened()
```cpp
bool opened() const;
```
判断是否已打开。

#### close()
```cpp
bool close();
```
关闭监听套接字。

### 其他

#### get_addr()
```cpp
const char* get_addr() const;
```
获取监听地址。

#### sock_handle()
```cpp
int sock_handle() const;  // Unix
SOCKET sock_handle() const;  // Windows
```
获取套接字句柄。

#### unbind()
```cpp
int unbind();  // Unix
SOCKET unbind();  // Windows
```
解绑套接字。

#### set_tcp_defer_accept()
```cpp
void set_tcp_defer_accept(int timeout);
```
设置 TCP_DEFER_ACCEPT（仅 Linux）。

---

## aio_handle 异步事件引擎

异步 I/O 事件处理器。

### 构造函数

```cpp
aio_handle(aio_handle_type engine_type = ENGINE_SELECT, unsigned int nMsg = 0);
```

**参数：**
- `engine_type`: 引擎类型
  - `ENGINE_SELECT`: select 模式
  - `ENGINE_POLL`: poll 模式
  - `ENGINE_KERNEL`: 内核模式（epoll/kqueue/iocp）
  - `ENGINE_WINMSG`: Windows 消息循环
- `nMsg`: Windows 消息 ID（仅 ENGINE_WINMSG）

### 事件循环

#### check()
```cpp
bool check();
```
执行一次事件循环。

**返回值：**
- `true`: 应该停止循环
- `false`: 继续循环

#### stop()
```cpp
void stop();
```
停止事件循环。

#### reset()
```cpp
void reset();
```
重置内部状态。

### 定时器

#### set_timer()
```cpp
long long int set_timer(aio_timer_callback* callback, 
                        long long int delay, 
                        unsigned int id = 0);
```
设置定时器。

**参数：**
- `callback`: 定时器回调对象
- `delay`: 延迟时间（微秒）
- `id`: 定时器 ID

**返回值：**
- 定时器到期时间（微秒，从 1970-01-01 起算）

#### del_timer()
```cpp
long long int del_timer(aio_timer_callback* callback);
long long int del_timer(aio_timer_callback* callback, unsigned int id);
```
删除定时器。

### DNS 设置

#### set_dns() / dns_add()
```cpp
void set_dns(const char* addrs, int timeout);
void dns_add(const char* addrs, int timeout);
```
设置 DNS 服务器地址。

**参数：**
- `addrs`: DNS 服务器地址列表（格式：ip1:port1;ip2:port2）
- `timeout`: DNS 查询超时（秒）

#### dns_del()
```cpp
void dns_del(const char* addrs);
```
删除 DNS 服务器。

#### dns_clear()
```cpp
void dns_clear();
```
清空 DNS 服务器列表。

### 参数设置

#### set_delay_sec()
```cpp
void set_delay_sec(int n);
```
设置事件循环等待时间（秒）。

#### set_delay_usec()
```cpp
void set_delay_usec(int n);
```
设置事件循环等待时间（微秒）。

#### set_rbuf_size()
```cpp
void set_rbuf_size(int n);
```
设置读缓冲区大小。

### 其他

#### get_handle()
```cpp
ACL_AIO* get_handle() const;
```
获取底层 ACL_AIO 对象。

#### length()
```cpp
int length() const;
```
获取当前监控的异步流数量。

---

## aio_stream 异步流

异步流基类。

### 生命周期

#### close()
```cpp
void close(bool flush_out = false);
```
关闭异步流。

**参数：**
- `flush_out`: 是否先刷新写缓冲

### 回调管理

#### add_close_callback()
```cpp
void add_close_callback(aio_callback* callback);
```
添加关闭回调。

#### add_timeout_callback()
```cpp
void add_timeout_callback(aio_callback* callback);
```
添加超时回调。

#### del_close_callback()
```cpp
int del_close_callback(aio_callback* callback = NULL);
```
删除关闭回调。

#### del_timeout_callback()
```cpp
int del_timeout_callback(aio_callback* callback = NULL);
```
删除超时回调。

#### disable_close_callback()
```cpp
int disable_close_callback(aio_callback* callback = NULL);
```
禁用关闭回调（不删除）。

#### enable_close_callback()
```cpp
int enable_close_callback(aio_callback* callback = NULL);
```
启用关闭回调。

### 信息获取

#### get_astream()
```cpp
ACL_ASTREAM* get_astream() const;
```
获取底层 ACL_ASTREAM 对象。

#### get_vstream()
```cpp
ACL_VSTREAM* get_vstream() const;
```
获取底层 ACL_VSTREAM 对象。

#### get_socket()
```cpp
int get_socket() const;  // Unix
SOCKET get_socket() const;  // Windows
```
获取套接字句柄。

#### get_peer()
```cpp
const char* get_peer(bool full = false) const;
```
获取远程地址。

#### get_local()
```cpp
const char* get_local(bool full = false) const;
```
获取本地地址。

#### get_handle()
```cpp
aio_handle& get_handle() const;
```
获取异步事件引擎。

---

## stream_hook Hook 机制

自定义 I/O 行为的 Hook 接口。

### 虚函数

#### read()
```cpp
virtual int read(void* buf, size_t len) = 0;
```
自定义读取操作。

**参数：**
- `buf`: 缓冲区
- `len`: 缓冲区大小

**返回值：**
- 读取的字节数
- `< 0`: 失败

#### send()
```cpp
virtual int send(const void* buf, size_t len) = 0;
```
自定义发送操作。

**参数：**
- `buf`: 数据缓冲区
- `len`: 数据长度

**返回值：**
- 发送的字节数
- `< 0`: 失败

#### open()
```cpp
virtual bool open(ACL_VSTREAM* s) = 0;
```
Hook 初始化回调。

**参数：**
- `s`: 流对象

**返回值：**
- `true`: 成功
- `false`: 失败

#### on_close()
```cpp
virtual bool on_close(bool alive);
```
流关闭前回调。

**参数：**
- `alive`: 流是否仍然存活

**返回值：**
- `true`: 继续关闭
- `false`: 取消关闭

#### destroy()
```cpp
virtual void destroy();
```
Hook 销毁回调。

---

## aio_callback 异步回调

异步流的回调接口。

### 回调方法

#### open_callback()
```cpp
virtual bool open_callback();
```
连接建立成功回调。

**返回值：**
- `true`: 继续
- `false`: 关闭连接

#### read_callback()
```cpp
virtual bool read_callback(char* data, int len);
```
数据读取完成回调。

**参数：**
- `data`: 数据指针
- `len`: 数据长度

**返回值：**
- `true`: 继续
- `false`: 关闭连接

#### write_callback()
```cpp
virtual bool write_callback();
```
数据写入完成回调。

#### close_callback()
```cpp
virtual void close_callback();
```
连接关闭回调。

#### timeout_callback()
```cpp
virtual bool timeout_callback();
```
超时回调。

**返回值：**
- `true`: 继续
- `false`: 关闭连接

---

## 常量和枚举

### 时间单位
```cpp
enum time_unit_t {
    time_unit_s,   // 秒
    time_unit_ms,  // 毫秒
    time_unit_us,  // 微秒
    time_unit_ns   // 纳秒
};
```

### 事件引擎类型
```cpp
enum aio_handle_type {
    ENGINE_SELECT,  // select
    ENGINE_POLL,    // poll
    ENGINE_KERNEL,  // epoll/kqueue/iocp
    ENGINE_WINMSG   // Windows 消息
};
```

### 打开标志
```cpp
enum {
    OPEN_FLAG_NONE           = 0,
    OPEN_FLAG_NONBLOCK       = 1,        // 非阻塞
    OPEN_FLAG_REUSEPORT      = (1 << 1), // 端口复用
    OPEN_FLAG_FASTOPEN       = (1 << 2), // TCP Fast Open
    OPEN_FLAG_EXCLUSIVE      = (1 << 3), // 禁止地址复用
    OPEN_FLAG_MULTICAST_LOOP = (1 << 4)  // 多播回环
};
```

---

## 错误处理

### 获取错误信息

```cpp
// 在 acl 命名空间中
int last_error();           // 获取错误码
const char* last_serror();  // 获取错误字符串描述
```

### 示例

```cpp
acl::socket_stream conn;
if (!conn.open("127.0.0.1:8080", 10, 30)) {
    int err = acl::last_error();
    const char* msg = acl::last_serror();
    std::cerr << "连接失败: " << msg << " (" << err << ")" << std::endl;
}
```

---

## 使用建议

1. **检查返回值**: 每次 I/O 操作后检查返回值
2. **设置超时**: 避免无限期阻塞
3. **资源管理**: 使用 RAII 或及时 delete
4. **线程安全**: 每个流对象仅在一个线程中使用
5. **缓冲调优**: 根据场景调整缓冲区大小
6. **错误处理**: 完善的错误处理逻辑

---

## 相关资源

- [架构文档](./architecture.md)
- [使用指南](./usage_guide.md)
- [示例代码](./examples.md)
- [ACL 官方文档](https://github.com/acl-dev/acl)

