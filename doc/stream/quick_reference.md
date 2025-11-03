# ACL Stream 快速参考卡

## 常用代码片段

### 文件操作

```cpp
// 读取文本文件
acl::fstream file;
if (file.open_read("data.txt")) {
    acl::string line;
    while (file.gets(line)) {
        // 处理每一行
    }
}

// 写入文件
acl::fstream file;
if (file.create("output.txt")) {
    file.format("Hello %s\n", "World");
    file.write(data, len);
}

// 追加到文件
acl::fstream file;
file.open("log.txt", O_WRONLY | O_APPEND | O_CREAT, 0644);
file.write(log_msg, strlen(log_msg));
```

### TCP 客户端

```cpp
// 连接服务器
acl::socket_stream conn;
if (conn.open("127.0.0.1:8080", 10, 30)) {
    // 发送数据
    conn.write(request, len);
    
    // 接收数据
    char buf[4096];
    int ret = conn.read(buf, sizeof(buf));
    
    // 按行接收
    acl::string line;
    while (conn.gets(line)) {
        // 处理行数据
    }
}
```

### TCP 服务器

```cpp
// 创建监听
acl::server_socket server(0, 128);
if (server.open("0.0.0.0:8080")) {
    // 接受连接
    while (true) {
        acl::socket_stream* client = server.accept();
        if (client) {
            // 处理客户端
            handle_client(client);
            delete client;
        }
    }
}
```

### UDP 通信

```cpp
// UDP 服务器
acl::socket_stream udp;
if (udp.bind_udp("0.0.0.0:9000", 30)) {
    char buf[1024];
    int ret = udp.read(buf, sizeof(buf));
    udp.write(response, len);  // 回复最后一个发送者
}

// UDP 客户端
acl::socket_stream udp;
if (udp.bind_udp("0.0.0.0:0", 30)) {
    udp.set_peer("127.0.0.1:9000");
    udp.write(data, len);
    udp.read(buf, sizeof(buf));
}
```

### HTTPS 客户端

```cpp
#include "acl_cpp/stream/openssl_conf.hpp"
#include "acl_cpp/stream/openssl_io.hpp"

acl::openssl_conf ssl_conf;
acl::socket_stream conn;

if (conn.open("www.example.com:443", 10, 30)) {
    acl::openssl_io* ssl = new acl::openssl_io(ssl_conf, false);
    ssl->set_sni_host("www.example.com");
    
    if (conn.setup_hook(ssl) == ssl && ssl->handshake()) {
        // 使用加密连接
        conn.write(request, len);
        conn.read(buf, sizeof(buf));
    }
}
```

### 异步服务器

```cpp
// 客户端回调
class my_callback : public acl::aio_callback {
    bool read_callback(char* data, int len) override {
        // 处理数据
        conn_->write(data, len);  // 回显
        conn_->read();  // 继续读取
        return true;
    }
    void close_callback() override {
        delete this;
    }
private:
    acl::aio_socket_stream* conn_;
};

// 监听回调
class accept_callback : public acl::aio_callback {
    bool accept_callback(acl::aio_socket_stream* client) override {
        my_callback* cb = new my_callback;
        client->add_read_callback(cb);
        client->read();
        return true;
    }
};

// 主函数
acl::aio_handle handle(acl::ENGINE_KERNEL);
acl::aio_listen_stream* listener = new acl::aio_listen_stream(&handle);
listener->open("0.0.0.0:8080");
listener->add_accept_callback(new accept_callback);

while (!handle.check()) {
    // 事件循环
}
```

## 常用 API 速查

### stream 基类

| 方法 | 功能 |
|------|------|
| `close()` | 关闭流 |
| `eof()` | 是否到达末尾 |
| `opened()` | 是否已打开 |
| `set_rw_timeout(n)` | 设置超时（秒） |
| `set_time_unit(unit)` | 设置时间单位 |
| `setup_hook(hook)` | 设置 Hook |
| `get_buf()` | 获取内部缓冲区 |

### istream 输入流

| 方法 | 功能 |
|------|------|
| `read(buf, size)` | 读取原始数据 |
| `gets(str)` | 读取一行 |
| `read(int&)` | 读取整数 |
| `getch()` | 读取单字节 |
| `readtags(str, tag)` | 读取到标签 |
| `read_peek(str)` | 预读数据 |
| `readable()` | 是否可读 |

### ostream 输出流

| 方法 | 功能 |
|------|------|
| `write(data, size)` | 写入原始数据 |
| `format(fmt, ...)` | 格式化输出 |
| `fflush()` | 刷新缓冲区 |
| `writev(iov, cnt)` | 向量写入 |
| `sendto(data, size, addr)` | UDP 发送 |

### fstream 文件流

| 方法 | 功能 |
|------|------|
| `open(path, flags, mode)` | 打开文件 |
| `create(path)` | 创建文件 |
| `open_trunc(path)` | 截断打开 |
| `fseek(offset, whence)` | 定位 |
| `ftell()` | 获取位置 |
| `fsize()` | 获取大小 |
| `lock(exclusive)` | 文件锁 |

### socket_stream 网络流

| 方法 | 功能 |
|------|------|
| `open(addr, conn_to, rw_to)` | 连接服务器 |
| `bind_udp(addr, rw_to)` | 绑定 UDP |
| `get_peer()` | 获取远程地址 |
| `get_local()` | 获取本地地址 |
| `set_tcp_nodelay(on)` | 禁用 Nagle |
| `set_tcp_sendbuf(size)` | 设置发送缓冲 |
| `alive()` | 检测连接 |

### server_socket 服务器

| 方法 | 功能 |
|------|------|
| `open(addr)` | 绑定监听 |
| `accept(timeout)` | 接受连接 |
| `close()` | 关闭监听 |
| `get_addr()` | 获取监听地址 |

### aio_handle 异步引擎

| 方法 | 功能 |
|------|------|
| `check()` | 事件循环 |
| `stop()` | 停止循环 |
| `set_timer(cb, delay, id)` | 设置定时器 |
| `del_timer(cb)` | 删除定时器 |
| `set_delay_sec(n)` | 设置等待时间 |

### aio_stream 异步流

| 方法 | 功能 |
|------|------|
| `close(flush)` | 关闭流 |
| `add_read_callback(cb)` | 添加读回调 |
| `add_write_callback(cb)` | 添加写回调 |
| `add_close_callback(cb)` | 添加关闭回调 |
| `get_peer()` | 获取远程地址 |

## 常用标志和常量

### 文件打开标志

```cpp
O_RDONLY    // 只读
O_WRONLY    // 只写
O_RDWR      // 读写
O_CREAT     // 创建
O_TRUNC     // 截断
O_APPEND    // 追加
O_EXCL      // 排他
```

### 时间单位

```cpp
time_unit_s     // 秒
time_unit_ms    // 毫秒
time_unit_us    // 微秒
time_unit_ns    // 纳秒
```

### 事件引擎类型

```cpp
ENGINE_SELECT   // select
ENGINE_POLL     // poll
ENGINE_KERNEL   // epoll/kqueue/iocp
ENGINE_WINMSG   // Windows 消息
```

### 监听标志

```cpp
OPEN_FLAG_NONE              // 无
OPEN_FLAG_NONBLOCK          // 非阻塞
OPEN_FLAG_REUSEPORT         // 端口复用
OPEN_FLAG_FASTOPEN          // TCP Fast Open
OPEN_FLAG_EXCLUSIVE         // 排他
OPEN_FLAG_MULTICAST_LOOP    // 多播回环
```

## 错误处理模式

```cpp
// 模式 1: 检查返回值
if (!stream.open(...)) {
    std::cerr << "错误: " << acl::last_serror() << std::endl;
    return;
}

// 模式 2: 检查 EOF
int ret = stream.read(buf, size);
if (ret == -1) {
    if (stream.eof()) {
        // 正常关闭
    } else {
        // 读取错误
    }
}

// 模式 3: 异步回调
bool read_callback(char* data, int len) override {
    if (len <= 0) {
        return false;  // 关闭连接
    }
    // 处理数据
    return true;  // 继续
}
```

## 性能优化技巧

```cpp
// 1. 调整缓冲区大小
acl::istream::set_rbuf_size(65536);  // 64KB
acl::ostream::set_wbuf_size(65536);

// 2. 禁用 Nagle（低延迟）
conn.set_tcp_nodelay(true);

// 3. 使用向量写入
struct iovec iov[3];
conn.writev(iov, 3);

// 4. 批量写入
for (int i = 0; i < 100; i++) {
    conn.write(data, len, true, true);  // 缓冲
}
conn.fflush();  // 一次性发送

// 5. 设置 TCP 缓冲区
conn.set_tcp_sendbuf(256 * 1024);  // 256KB
conn.set_tcp_recvbuf(256 * 1024);

// 6. 使用异步 I/O（高并发）
acl::aio_handle handle(acl::ENGINE_KERNEL);
```

## 常见问题解决

### Q: 如何判断连接关闭？
```cpp
if (conn.eof()) {
    // 连接已关闭
}
```

### Q: 如何设置毫秒超时？
```cpp
conn.set_time_unit(acl::time_unit_ms);
conn.set_rw_timeout(5000);  // 5000 毫秒
```

### Q: 如何读取完整数据？
```cpp
// 方法 1: loop=true（默认）
int ret = conn.read(buf, size, true);

// 方法 2: 循环读取
int total = 0;
while (total < need) {
    int ret = conn.read(buf + total, need - total, false);
    if (ret <= 0) break;
    total += ret;
}
```

### Q: 如何处理部分写入？
```cpp
// loop=true 保证全部写入
int ret = conn.write(data, len, true);

// 或手动循环
int total = 0;
while (total < len) {
    int ret = conn.write(data + total, len - total, false);
    if (ret <= 0) break;
    total += ret;
}
```

### Q: 如何实现超时重连？
```cpp
bool connect_with_retry(const char* addr, int max_retries = 3) {
    acl::socket_stream conn;
    for (int i = 0; i < max_retries; i++) {
        if (conn.open(addr, 5, 30)) {
            return true;
        }
        sleep(1);
    }
    return false;
}
```

### Q: 如何在异步流中保存上下文？
```cpp
class my_callback : public acl::aio_callback {
public:
    my_callback(void* ctx) : ctx_(ctx) {}
private:
    void* ctx_;  // 保存在回调对象中
};
```

## 编译链接

### 编译选项
```bash
g++ -o app main.cpp -I/path/to/acl/include -L/path/to/acl/lib \
    -lacl_cpp -lacl -lpthread -ldl -lz
```

### 链接 SSL
```bash
g++ -o app main.cpp -lacl_cpp -lacl -lssl -lcrypto -lpthread -ldl -lz
```

### CMake 配置
```cmake
find_library(ACL_LIB acl)
find_library(ACL_CPP_LIB acl_cpp)

target_link_libraries(myapp ${ACL_CPP_LIB} ${ACL_LIB} pthread dl z)
```

## 最佳实践清单

- [ ] 每次 I/O 操作后检查返回值
- [ ] 设置合理的超时时间
- [ ] 使用 RAII 管理资源
- [ ] 每个流对象仅在一个线程中使用
- [ ] 异步流只在堆上分配
- [ ] 不要在异步回调中执行阻塞操作
- [ ] 根据场景调整缓冲区大小
- [ ] 高并发场景使用异步 I/O
- [ ] SSL 应用加载证书和私钥
- [ ] 及时关闭不需要的连接

## 调试技巧

```cpp
// 1. 启用 ACL 日志
acl::log::open("app.log", "myapp");

// 2. 检查错误码
int err = acl::last_error();
const char* msg = acl::last_serror();
std::cerr << "Error: " << msg << " (" << err << ")" << std::endl;

// 3. 打印地址信息
std::cout << "Local: " << conn.get_local(true) << std::endl;
std::cout << "Peer: " << conn.get_peer(true) << std::endl;

// 4. 检查流状态
std::cout << "Opened: " << conn.opened() << std::endl;
std::cout << "EOF: " << conn.eof() << std::endl;

// 5. 监控异步流数量
std::cout << "Streams: " << handle.length() << std::endl;
```

## 资源链接

- [完整文档](./README.md)
- [架构设计](./architecture.md)
- [使用指南](./usage_guide.md)
- [示例代码](./examples.md)
- [API 参考](./api_reference.md)
- [类层次结构](./class_hierarchy.md)
- [ACL 官方](https://github.com/acl-dev/acl)

