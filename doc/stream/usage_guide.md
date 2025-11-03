# ACL Stream 使用指南

## 目录

1. [基础流操作](#基础流操作)
2. [文件流](#文件流)
3. [网络流](#网络流)
4. [异步流](#异步流)
5. [SSL/TLS 流](#ssltls-流)
6. [Hook 机制](#hook-机制)
7. [高级特性](#高级特性)

## 基础流操作

### 流的生命周期

```cpp
// 1. 创建流对象
acl::socket_stream conn;

// 2. 打开流
if (!conn.open("www.example.com:80", 10, 10)) {
    std::cerr << "连接失败" << std::endl;
    return;
}

// 3. 使用流
conn.write("GET / HTTP/1.0\r\n\r\n", 18);

// 4. 关闭流（可选，析构时自动关闭）
conn.close();
```

### 超时设置

```cpp
acl::socket_stream conn;

// 连接超时 10 秒，读写超时 20 秒
conn.open("127.0.0.1:8080", 10, 20);

// 动态修改读写超时（秒）
conn.set_rw_timeout(30);

// 设置时间单位为毫秒
conn.set_time_unit(acl::time_unit_ms);
conn.set_rw_timeout(5000);  // 5000 毫秒
```

### 错误处理

```cpp
acl::socket_stream conn;

if (!conn.open("127.0.0.1:8080", 10, 10)) {
    // 获取错误信息
    std::cerr << "连接失败: " << acl::last_serror() << std::endl;
    return;
}

// 检查流状态
if (conn.eof()) {
    std::cout << "连接已关闭" << std::endl;
}

// 读取数据时检查返回值
char buf[1024];
int ret = conn.read(buf, sizeof(buf));
if (ret == -1) {
    std::cerr << "读取失败" << std::endl;
}
```

## 文件流

### 打开和创建文件

```cpp
#include "acl_cpp/lib_acl.hpp"

acl::fstream file;

// 1. 只读方式打开
if (file.open_read("test.txt")) {
    // 读取文件内容
}

// 2. 创建新文件或截断现有文件
if (file.open_trunc("output.txt")) {
    // 写入内容
}

// 3. 创建文件（如果存在则失败）
if (file.create("newfile.txt")) {
    // 写入内容
}

// 4. 使用自定义标志打开
unsigned int oflags = O_RDWR | O_CREAT | O_APPEND;
if (file.open("log.txt", oflags, 0644)) {
    // 追加写入
}
```

### 读取文件

```cpp
acl::fstream file;
file.open_read("data.txt");

// 1. 按行读取
acl::string line;
while (file.gets(line)) {
    std::cout << line.c_str() << std::endl;
}

// 2. 读取固定大小数据
char buf[4096];
int ret = file.read(buf, sizeof(buf));
if (ret > 0) {
    // 处理数据
}

// 3. 读取整数
int number;
if (file.read(number)) {
    std::cout << "Number: " << number << std::endl;
}

// 4. 读取字符串（指定长度）
acl::string str;
file.read(str, 100);  // 读取 100 字节
```

### 写入文件

```cpp
acl::fstream file;
file.create("output.txt");

// 1. 写入原始数据
const char* data = "Hello, World!\n";
file.write(data, strlen(data));

// 2. 格式化写入
file.format("Number: %d, String: %s\n", 123, "test");

// 3. 写入整数
int num = 42;
file.write(num);

// 4. 写入字符串
acl::string str = "ACL Stream";
file.write(str);

// 5. 刷新缓冲区
file.fflush();
```

### 文件定位

```cpp
acl::fstream file;
file.open("data.bin", O_RDWR, 0644);

// 1. 移动到文件开头
file.fseek(0, SEEK_SET);

// 2. 移动到文件末尾
file.fseek(0, SEEK_END);

// 3. 相对当前位置移动
file.fseek(100, SEEK_CUR);

// 4. 获取当前位置
long long pos = file.ftell();

// 5. 获取文件大小
long long size = file.fsize();
```

### 文件锁

```cpp
acl::fstream file;
file.open("shared.dat", O_RDWR | O_CREAT, 0644);

// 1. 独占锁（写锁）
if (file.lock(true)) {
    // 执行写操作
    file.unlock();
}

// 2. 共享锁（读锁）
if (file.lock(false)) {
    // 执行读操作
    file.unlock();
}

// 3. 非阻塞锁
if (file.try_lock(true)) {
    // 获得锁
    file.unlock();
} else {
    std::cout << "文件已被锁定" << std::endl;
}
```

## 网络流

### TCP 客户端

```cpp
#include "acl_cpp/lib_acl.hpp"

acl::socket_stream conn;

// 1. 连接到服务器
if (!conn.open("www.example.com:80", 10, 30)) {
    std::cerr << "连接失败" << std::endl;
    return;
}

// 2. 发送 HTTP 请求
const char* request = "GET / HTTP/1.1\r\n"
                      "Host: www.example.com\r\n"
                      "\r\n";
if (conn.write(request, strlen(request)) == -1) {
    std::cerr << "发送失败" << std::endl;
    return;
}

// 3. 接收响应
acl::string line;
while (conn.gets(line)) {
    std::cout << line.c_str();
    if (line.empty() || line == "\r\n") {
        break;  // HTTP 头结束
    }
}

// 4. 读取响应体
char buf[4096];
while (true) {
    int ret = conn.read(buf, sizeof(buf) - 1, false);
    if (ret == -1) {
        break;
    }
    buf[ret] = '\0';
    std::cout << buf;
}
```

### TCP 服务器

```cpp
#include "acl_cpp/lib_acl.hpp"

// 1. 创建监听套接字
acl::server_socket server(acl::OPEN_FLAG_REUSEPORT, 128);

// 2. 绑定地址
if (!server.open("0.0.0.0:8080")) {
    std::cerr << "绑定失败" << std::endl;
    return;
}

std::cout << "服务器监听在: " << server.get_addr() << std::endl;

// 3. 接受客户端连接
while (true) {
    acl::socket_stream* client = server.accept();
    if (client == NULL) {
        std::cerr << "接受连接失败" << std::endl;
        break;
    }

    // 4. 处理客户端请求
    handle_client(client);
    
    // 5. 关闭客户端连接
    delete client;
}

void handle_client(acl::socket_stream* client) {
    // 获取客户端信息
    std::cout << "客户端连接: " << client->get_peer() << std::endl;
    
    // 读取客户端数据
    acl::string request;
    if (client->gets(request)) {
        std::cout << "收到: " << request.c_str() << std::endl;
        
        // 发送响应
        const char* response = "Hello, Client!\n";
        client->write(response, strlen(response));
    }
}
```

### UDP 通信

```cpp
// UDP 服务器
acl::socket_stream udp_server;
if (udp_server.bind_udp("0.0.0.0:9000", 30)) {
    char buf[1024];
    while (true) {
        int ret = udp_server.read(buf, sizeof(buf) - 1);
        if (ret > 0) {
            buf[ret] = '\0';
            std::cout << "收到: " << buf << std::endl;
            
            // 回复（使用最后一次接收到的地址）
            udp_server.write(buf, ret);
        }
    }
}

// UDP 客户端
acl::socket_stream udp_client;
if (udp_client.bind_udp("0.0.0.0:0", 30)) {
    // 设置目标地址
    udp_client.set_peer("127.0.0.1:9000");
    
    // 发送数据
    const char* msg = "Hello, UDP Server!";
    udp_client.write(msg, strlen(msg));
    
    // 接收响应
    char buf[1024];
    int ret = udp_client.read(buf, sizeof(buf) - 1);
    if (ret > 0) {
        buf[ret] = '\0';
        std::cout << "响应: " << buf << std::endl;
    }
}
```

### Unix 域套接字

```cpp
// Unix 域套接字服务器
acl::server_socket unix_server(0, 128);
if (unix_server.open("/tmp/test.sock")) {
    while (true) {
        acl::socket_stream* client = unix_server.accept();
        if (client) {
            // 处理客户端
            delete client;
        }
    }
}

// Unix 域套接字客户端
acl::socket_stream unix_client;
if (unix_client.open("/tmp/test.sock", 10, 30)) {
    unix_client.write("Hello", 5);
}

// Linux Abstract Unix Socket
acl::server_socket abstract_server(0, 128);
abstract_server.open("@my_socket");  // @ 开头表示 abstract socket
```

### 套接字选项

```cpp
acl::socket_stream conn;
conn.open("127.0.0.1:8080", 10, 30);

// 1. 禁用 Nagle 算法
conn.set_tcp_nodelay(true);

// 2. 设置 SO_LINGER
conn.set_tcp_solinger(true, 10);  // 延迟 10 秒

// 3. 设置发送缓冲区
conn.set_tcp_sendbuf(65536);  // 64KB

// 4. 设置接收缓冲区
conn.set_tcp_recvbuf(65536);

// 5. 设置非阻塞模式
conn.set_tcp_non_blocking(true);

// 6. 检查连接是否存活
if (conn.alive()) {
    std::cout << "连接正常" << std::endl;
}
```

### 多播

```cpp
acl::socket_stream mcast;

// 加入多播组
if (mcast.bind_multicast("239.255.0.1", "192.168.1.100", 9000, 30)) {
    // 设置 TTL
    mcast.multicast_set_ttl(64);
    
    // 接收多播数据
    char buf[1024];
    while (true) {
        int ret = mcast.read(buf, sizeof(buf));
        if (ret > 0) {
            // 处理数据
        }
    }
    
    // 退出多播组
    mcast.multicast_drop("239.255.0.1", "192.168.1.100");
}
```

## 异步流

### 异步事件引擎

```cpp
#include "acl_cpp/lib_acl.hpp"

// 1. 创建事件引擎
acl::aio_handle handle(acl::ENGINE_KERNEL);

// 2. 设置事件循环参数
handle.set_delay_sec(1);    // 等待 1 秒
handle.set_delay_usec(0);   // 0 微秒

// 3. 运行事件循环
while (!handle.check()) {
    // 循环处理事件
}

// 4. 停止事件循环
handle.stop();
```

### 异步客户端

```cpp
#include "acl_cpp/lib_acl.hpp"

// 1. 定义回调类
class my_callback : public acl::aio_callback {
public:
    // 连接成功回调
    bool open_callback() override {
        std::cout << "连接成功" << std::endl;
        
        // 发送请求
        const char* request = "GET / HTTP/1.0\r\n\r\n";
        conn_->write(request, strlen(request));
        
        // 开始读取响应
        conn_->gets(10);  // 超时 10 秒
        return true;
    }
    
    // 读取数据回调
    bool read_callback(char* data, int len) override {
        std::cout.write(data, len);
        
        // 继续读取
        conn_->gets(10);
        return true;
    }
    
    // 关闭回调
    void close_callback() override {
        std::cout << "连接关闭" << std::endl;
        delete this;  // 释放回调对象
    }
    
    void set_conn(acl::aio_socket_stream* conn) {
        conn_ = conn;
    }
    
private:
    acl::aio_socket_stream* conn_;
};

// 2. 使用异步客户端
void async_http_client() {
    acl::aio_handle handle(acl::ENGINE_KERNEL);
    
    // 创建异步连接
    acl::aio_socket_stream* conn = acl::aio_socket_stream::open(
        &handle, "www.example.com:80", 10, 30);
    
    if (conn == NULL) {
        std::cerr << "连接失败" << std::endl;
        return;
    }
    
    // 设置回调
    my_callback* callback = new my_callback;
    callback->set_conn(conn);
    
    conn->add_open_callback(callback);
    conn->add_read_callback(callback);
    conn->add_close_callback(callback);
    
    // 运行事件循环
    while (!handle.check()) {
        // 处理事件
    }
}
```

### 异步服务器

```cpp
#include "acl_cpp/lib_acl.hpp"

// 1. 定义客户端处理回调
class client_callback : public acl::aio_callback {
public:
    client_callback(acl::aio_socket_stream* conn) : conn_(conn) {}
    
    // 读取数据回调
    bool read_callback(char* data, int len) override {
        std::cout << "收到数据: " << std::string(data, len) << std::endl;
        
        // 回显数据
        conn_->write(data, len);
        
        // 继续读取
        conn_->read();
        return true;
    }
    
    // 写完成回调
    bool write_callback() override {
        std::cout << "数据发送完成" << std::endl;
        return true;
    }
    
    // 关闭回调
    void close_callback() override {
        std::cout << "客户端断开: " << conn_->get_peer() << std::endl;
        delete this;
    }
    
private:
    acl::aio_socket_stream* conn_;
};

// 2. 定义监听回调
class accept_callback : public acl::aio_callback {
public:
    accept_callback(acl::aio_handle* handle) : handle_(handle) {}
    
    // 接受连接回调
    bool accept_callback(acl::aio_socket_stream* client) override {
        std::cout << "新客户端: " << client->get_peer() << std::endl;
        
        // 创建客户端回调
        client_callback* callback = new client_callback(client);
        client->add_read_callback(callback);
        client->add_write_callback(callback);
        client->add_close_callback(callback);
        
        // 开始读取
        client->read();
        
        return true;
    }
    
private:
    acl::aio_handle* handle_;
};

// 3. 启动异步服务器
void async_server() {
    acl::aio_handle handle(acl::ENGINE_KERNEL);
    
    // 创建监听流
    acl::aio_listen_stream* listener = new acl::aio_listen_stream(&handle);
    
    // 绑定地址
    if (!listener->open("0.0.0.0:8080")) {
        std::cerr << "监听失败" << std::endl;
        return;
    }
    
    // 设置接受回调
    accept_callback* callback = new accept_callback(&handle);
    listener->add_accept_callback(callback);
    
    std::cout << "服务器启动，监听 8080 端口" << std::endl;
    
    // 运行事件循环
    while (!handle.check()) {
        // 处理事件
    }
}
```

### 异步定时器

```cpp
#include "acl_cpp/lib_acl.hpp"

class my_timer : public acl::aio_timer_callback {
public:
    my_timer(int max_count) : count_(0), max_count_(max_count) {}
    
    // 定时器回调
    void timer_callback(unsigned int id) override {
        std::cout << "定时器触发，ID: " << id 
                  << ", 计数: " << ++count_ << std::endl;
        
        if (count_ >= max_count_) {
            std::cout << "达到最大计数，停止定时器" << std::endl;
            // 不需要手动删除，返回后自动删除
        }
    }
    
    // 销毁回调
    void destroy() override {
        std::cout << "定时器销毁" << std::endl;
        delete this;
    }
    
private:
    int count_;
    int max_count_;
};

// 使用定时器
void use_timer() {
    acl::aio_handle handle(acl::ENGINE_KERNEL);
    
    // 设置定时器，每 1000 毫秒（1秒）触发一次
    my_timer* timer = new my_timer(10);  // 触发 10 次
    handle.set_timer(timer, 1000000, 1);  // 1000000 微秒 = 1 秒，ID=1
    
    // 运行事件循环
    while (!handle.check()) {
        // 处理事件
    }
}
```

## SSL/TLS 流

### OpenSSL 客户端

```cpp
#include "acl_cpp/lib_acl.hpp"
#include "acl_cpp/stream/openssl_conf.hpp"
#include "acl_cpp/stream/openssl_io.hpp"

// 1. 初始化 OpenSSL 配置
acl::openssl_conf ssl_conf;
ssl_conf.enable_cache(true);  // 启用会话缓存

// 2. 创建套接字连接
acl::socket_stream conn;
if (!conn.open("www.example.com:443", 10, 30)) {
    std::cerr << "连接失败" << std::endl;
    return;
}

// 3. 创建 SSL IO
acl::openssl_io* ssl = new acl::openssl_io(ssl_conf, false);  // false 表示客户端模式

// 4. 设置 SNI
ssl->set_sni_host("www.example.com");

// 5. 绑定到流
if (conn.setup_hook(ssl) != ssl) {
    std::cerr << "SSL 绑定失败" << std::endl;
    delete ssl;
    return;
}

// 6. 执行 SSL 握手
if (!ssl->handshake()) {
    std::cerr << "SSL 握手失败" << std::endl;
    return;
}

// 7. 使用加密连接
const char* request = "GET / HTTP/1.1\r\n"
                      "Host: www.example.com\r\n"
                      "\r\n";
conn.write(request, strlen(request));

acl::string response;
while (conn.gets(response)) {
    std::cout << response.c_str();
}
```

### OpenSSL 服务器

```cpp
#include "acl_cpp/lib_acl.hpp"
#include "acl_cpp/stream/openssl_conf.hpp"
#include "acl_cpp/stream/openssl_io.hpp"

// 1. 初始化服务器 SSL 配置
acl::openssl_conf ssl_conf;

// 加载证书和私钥
if (!ssl_conf.add_cert("server.crt", "server.key")) {
    std::cerr << "加载证书失败" << std::endl;
    return;
}

// 2. 创建服务器
acl::server_socket server(0, 128);
if (!server.open("0.0.0.0:8443")) {
    std::cerr << "监听失败" << std::endl;
    return;
}

std::cout << "SSL 服务器启动在 8443 端口" << std::endl;

// 3. 接受客户端连接
while (true) {
    acl::socket_stream* client = server.accept();
    if (client == NULL) {
        continue;
    }
    
    std::cout << "新连接: " << client->get_peer() << std::endl;
    
    // 4. 为客户端连接创建 SSL IO
    acl::openssl_io* ssl = new acl::openssl_io(ssl_conf, true);  // true 表示服务器模式
    
    if (client->setup_hook(ssl) != ssl) {
        std::cerr << "SSL 绑定失败" << std::endl;
        delete ssl;
        delete client;
        continue;
    }
    
    // 5. 执行 SSL 握手
    if (!ssl->handshake()) {
        std::cerr << "SSL 握手失败" << std::endl;
        delete client;
        continue;
    }
    
    // 6. 处理客户端请求
    acl::string request;
    if (client->gets(request)) {
        std::cout << "收到: " << request.c_str() << std::endl;
        
        const char* response = "HTTP/1.1 200 OK\r\n"
                               "Content-Type: text/plain\r\n"
                               "\r\n"
                               "Hello, SSL Client!\r\n";
        client->write(response, strlen(response));
    }
    
    delete client;
}
```

### mbed TLS

```cpp
#include "acl_cpp/lib_acl.hpp"
#include "acl_cpp/stream/mbedtls_conf.hpp"
#include "acl_cpp/stream/mbedtls_io.hpp"

// 客户端配置
acl::mbedtls_conf ssl_conf;

acl::socket_stream conn;
conn.open("www.example.com:443", 10, 30);

acl::mbedtls_io* ssl = new acl::mbedtls_io(ssl_conf, false);
conn.setup_hook(ssl);
ssl->handshake();

// 使用加密连接...
```

## Hook 机制

### 自定义 Hook

```cpp
#include "acl_cpp/lib_acl.hpp"

// 1. 实现自定义 Hook（例如：数据压缩）
class compress_hook : public acl::stream_hook {
public:
    bool open(ACL_VSTREAM* s) override {
        // 初始化压缩上下文
        stream_ = s;
        return true;
    }
    
    int read(void* buf, size_t len) override {
        // 读取压缩数据
        char compressed[4096];
        int ret = acl_vstream_read(stream_, compressed, sizeof(compressed));
        if (ret <= 0) {
            return ret;
        }
        
        // 解压缩数据
        return decompress(compressed, ret, buf, len);
    }
    
    int send(const void* buf, size_t len) override {
        // 压缩数据
        char compressed[4096];
        int compressed_len = compress(buf, len, compressed, sizeof(compressed));
        if (compressed_len <= 0) {
            return -1;
        }
        
        // 发送压缩数据
        return acl_vstream_writen(stream_, compressed, compressed_len);
    }
    
    bool on_close(bool alive) override {
        // 清理压缩上下文
        return true;
    }
    
private:
    ACL_VSTREAM* stream_;
    
    int compress(const void* in, size_t in_len, void* out, size_t out_len) {
        // 实现压缩逻辑（如 zlib）
        return in_len;  // 简化示例
    }
    
    int decompress(const void* in, size_t in_len, void* out, size_t out_len) {
        // 实现解压逻辑
        return in_len;  // 简化示例
    }
};

// 2. 使用自定义 Hook
acl::socket_stream conn;
conn.open("127.0.0.1:8080", 10, 30);

compress_hook* hook = new compress_hook;
if (conn.setup_hook(hook) == hook) {
    // Hook 设置成功，现在所有读写都会经过压缩/解压
    conn.write("Hello", 5);  // 自动压缩
    
    char buf[1024];
    int ret = conn.read(buf, sizeof(buf));  // 自动解压
}
```

### Hook 链

```cpp
// 可以叠加多个 Hook（需要自定义实现）
class hook_chain : public acl::stream_hook {
public:
    void add_hook(acl::stream_hook* hook) {
        hooks_.push_back(hook);
    }
    
    int read(void* buf, size_t len) override {
        // 依次调用每个 Hook
        for (auto* hook : hooks_) {
            // 实现 Hook 链逻辑
        }
        return 0;
    }
    
private:
    std::vector<acl::stream_hook*> hooks_;
};
```

## 高级特性

### 上下文绑定

```cpp
acl::socket_stream conn;
conn.open("127.0.0.1:8080", 10, 30);

// 绑定用户数据
struct UserData {
    int user_id;
    std::string session_id;
};

UserData* data = new UserData{123, "abc123"};
conn.set_ctx(data, "user_data");

// 获取用户数据
UserData* retrieved = (UserData*)conn.get_ctx("user_data");

// 删除上下文
UserData* removed = (UserData*)conn.del_ctx("user_data");
delete removed;
```

### 缓冲区管理

```cpp
// 1. 获取流内部缓冲区
acl::socket_stream conn;
acl::string& buf = conn.get_buf();

// 2. 获取 dbuf 动态缓冲池
acl::dbuf_pool& pool = conn.get_dbuf();

// 3. 设置全局缓冲区大小
acl::istream::set_rbuf_size(8192);  // 读缓冲 8KB
acl::ostream::set_wbuf_size(1024);  // 写缓冲 1KB
```

### Peek 操作

```cpp
acl::socket_stream conn;
conn.open("127.0.0.1:8080", 10, 30);

// 预读数据（不从缓冲区移除）
acl::string buf;
if (conn.read_peek(buf)) {
    std::cout << "预读: " << buf.c_str() << std::endl;
    
    // 数据仍在缓冲区，可以再次读取
    acl::string actual;
    conn.gets(actual);  // 读取实际数据
}

// 预读指定长度
if (conn.readn_peek(buf, 100)) {
    // 预读 100 字节
}

// 预读一行
if (conn.gets_peek(buf, true, false, 1024)) {
    // 预读一行，最大 1024 字节
}
```

### 零拷贝

```cpp
acl::socket_stream conn;
conn.open("127.0.0.1:8080", 10, 30);

// 启用零拷贝（需要内核支持）
conn.set_zerocopy(true);

// 发送文件（零拷贝）
acl::fstream file;
if (file.open_read("large_file.dat")) {
    // 使用 sendfile 或 TransmitFile
    // 具体实现依赖于平台
}
```

### 向量写入

```cpp
acl::socket_stream conn;
conn.open("127.0.0.1:8080", 10, 30);

// 使用 writev 减少系统调用
struct iovec iov[3];

const char* header = "HTTP/1.1 200 OK\r\n";
const char* content_type = "Content-Type: text/plain\r\n\r\n";
const char* body = "Hello, World!";

iov[0].iov_base = (void*)header;
iov[0].iov_len = strlen(header);

iov[1].iov_base = (void*)content_type;
iov[1].iov_len = strlen(content_type);

iov[2].iov_base = (void*)body;
iov[2].iov_len = strlen(body);

conn.writev(iov, 3);
```

### 流操作符

```cpp
acl::socket_stream conn;
conn.open("127.0.0.1:8080", 10, 30);

// 使用 << 操作符写入
conn << "Hello, " << "World!" << 123 << '\n';

// 使用 >> 操作符读取
acl::string str;
int num;
conn >> str >> num;
```

### C++11 特性

```cpp
#if __cplusplus >= 201103L

// 使用 shared_ptr 管理连接
acl::server_socket server(0, 128);
server.open("0.0.0.0:8080");

acl::shared_stream client = server.shared_accept();
if (client) {
    // 使用智能指针，自动释放
    client->write("Hello", 5);
}
// client 超出作用域时自动删除

#endif
```

## 最佳实践

### 1. 错误处理

```cpp
acl::socket_stream conn;

if (!conn.open("127.0.0.1:8080", 10, 30)) {
    int err = acl::last_error();
    std::cerr << "连接失败: " << acl::last_serror() 
              << " (errno=" << err << ")" << std::endl;
    return;
}

char buf[1024];
int ret = conn.read(buf, sizeof(buf));
if (ret == -1) {
    if (conn.eof()) {
        std::cout << "连接正常关闭" << std::endl;
    } else {
        std::cerr << "读取错误: " << acl::last_serror() << std::endl;
    }
}
```

### 2. 资源管理

```cpp
// 使用 RAII
void handle_request() {
    acl::socket_stream conn;  // 栈上分配
    
    if (!conn.open("127.0.0.1:8080", 10, 30)) {
        return;  // 自动调用析构函数关闭连接
    }
    
    // 使用连接...
    
}  // 离开作用域自动关闭
```

### 3. 性能优化

```cpp
// 1. 调整缓冲区大小
acl::istream::set_rbuf_size(65536);  // 64KB 读缓冲
acl::ostream::set_wbuf_size(65536);  // 64KB 写缓冲

// 2. 禁用 Nagle 算法（低延迟场景）
conn.set_tcp_nodelay(true);

// 3. 使用向量写入
conn.writev(iov, count);

// 4. 批量操作
for (int i = 0; i < 100; i++) {
    conn.write(data, len, true, true);  // 先缓冲
}
conn.fflush();  // 一次性发送
```

### 4. 线程安全

```cpp
// 每个线程使用独立的流对象
void thread_func(int thread_id) {
    acl::socket_stream conn;  // 线程本地
    
    if (conn.open("127.0.0.1:8080", 10, 30)) {
        // 安全使用
    }
}

// 多线程服务器
std::vector<std::thread> threads;
for (int i = 0; i < 4; i++) {
    threads.emplace_back(thread_func, i);
}
```

### 5. 超时处理

```cpp
acl::socket_stream conn;

// 分别设置连接超时和读写超时
conn.open("slow.server.com:80", 5, 30);  // 5秒连接超时，30秒读写超时

// 动态调整超时
conn.set_rw_timeout(60);  // 延长到 60 秒

// 检查操作结果
char buf[1024];
int ret = conn.read(buf, sizeof(buf));
if (ret == -1 && !conn.eof()) {
    // 可能是超时
    std::cerr << "读取超时或错误" << std::endl;
}
```

## 常见问题

### Q1: 如何判断连接是否关闭？

```cpp
if (conn.eof()) {
    // 连接已关闭或到达文件末尾
}
```

### Q2: 如何实现连接池？

```cpp
class ConnectionPool {
public:
    acl::socket_stream* get_connection() {
        if (!pool_.empty()) {
            auto* conn = pool_.back();
            pool_.pop_back();
            return conn;
        }
        
        auto* conn = new acl::socket_stream;
        if (!conn->open("127.0.0.1:8080", 10, 30)) {
            delete conn;
            return nullptr;
        }
        return conn;
    }
    
    void release_connection(acl::socket_stream* conn) {
        if (conn->alive()) {
            pool_.push_back(conn);
        } else {
            delete conn;
        }
    }
    
private:
    std::vector<acl::socket_stream*> pool_;
};
```

### Q3: 如何处理大文件？

```cpp
// 使用缓冲读取
acl::fstream file;
file.open_read("large_file.dat");

char buf[65536];  // 64KB 缓冲
while (true) {
    int ret = file.read(buf, sizeof(buf), false);
    if (ret <= 0) {
        break;
    }
    // 处理数据
    process_data(buf, ret);
}
```

### Q4: 如何实现超时重连？

```cpp
bool connect_with_retry(acl::socket_stream& conn, 
                        const char* addr, 
                        int max_retries = 3) {
    for (int i = 0; i < max_retries; i++) {
        if (conn.open(addr, 5, 30)) {
            return true;
        }
        std::cerr << "连接失败，重试 " << (i + 1) << "/" << max_retries << std::endl;
        sleep(1);
    }
    return false;
}
```

### Q5: 如何处理部分读取？

```cpp
// loop=false 表示可能返回部分数据
int total = 0;
int need = 1024;
char buf[1024];

while (total < need) {
    int ret = conn.read(buf + total, need - total, false);
    if (ret <= 0) {
        break;  // 错误或关闭
    }
    total += ret;
}
```

