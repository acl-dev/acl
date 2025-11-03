# ACL Stream 示例代码

本文档提供了 ACL Stream 库的完整示例代码，涵盖各种常见应用场景。

## 目录

1. [文件操作示例](#文件操作示例)
2. [网络通信示例](#网络通信示例)
3. [异步 I/O 示例](#异步-io-示例)
4. [SSL/TLS 示例](#ssltls-示例)
5. [综合应用示例](#综合应用示例)

## 文件操作示例

### 示例 1: 文本文件读写

```cpp
#include "acl_cpp/lib_acl.hpp"
#include <iostream>

void text_file_example() {
    // 写入文本文件
    acl::fstream writer;
    if (writer.create("test.txt")) {
        writer.format("姓名: %s\n", "张三");
        writer.format("年龄: %d\n", 25);
        writer.format("城市: %s\n", "北京");
        writer.close();
        std::cout << "文件写入成功" << std::endl;
    }
    
    // 读取文本文件
    acl::fstream reader;
    if (reader.open_read("test.txt")) {
        acl::string line;
        std::cout << "文件内容:" << std::endl;
        while (reader.gets(line)) {
            std::cout << line.c_str() << std::endl;
        }
        reader.close();
    }
}
```

### 示例 2: 二进制文件操作

```cpp
#include "acl_cpp/lib_acl.hpp"

struct Record {
    int id;
    char name[32];
    double score;
};

void binary_file_example() {
    // 写入二进制数据
    acl::fstream file;
    if (file.create("records.dat")) {
        Record records[] = {
            {1, "Alice", 95.5},
            {2, "Bob", 87.3},
            {3, "Charlie", 92.1}
        };
        
        for (const auto& rec : records) {
            file.write(&rec, sizeof(rec));
        }
        file.close();
        std::cout << "写入 " << sizeof(records)/sizeof(Record) << " 条记录" << std::endl;
    }
    
    // 读取二进制数据
    if (file.open_read("records.dat")) {
        Record rec;
        int count = 0;
        
        while (file.read(&rec, sizeof(rec)) == sizeof(rec)) {
            count++;
            std::cout << "ID: " << rec.id 
                      << ", 姓名: " << rec.name 
                      << ", 分数: " << rec.score << std::endl;
        }
        
        file.close();
        std::cout << "读取 " << count << " 条记录" << std::endl;
    }
}
```

### 示例 3: 大文件复制

```cpp
#include "acl_cpp/lib_acl.hpp"

bool copy_large_file(const char* src, const char* dst) {
    acl::fstream in, out;
    
    if (!in.open_read(src)) {
        std::cerr << "无法打开源文件: " << src << std::endl;
        return false;
    }
    
    if (!out.create(dst)) {
        std::cerr << "无法创建目标文件: " << dst << std::endl;
        return false;
    }
    
    char buf[65536];  // 64KB 缓冲区
    long long total = 0;
    
    while (true) {
        int ret = in.read(buf, sizeof(buf), false);
        if (ret <= 0) {
            break;
        }
        
        if (out.write(buf, ret) != ret) {
            std::cerr << "写入失败" << std::endl;
            return false;
        }
        
        total += ret;
        
        // 显示进度
        if (total % (1024 * 1024) == 0) {
            std::cout << "已复制: " << (total / 1024 / 1024) << " MB\r" << std::flush;
        }
    }
    
    std::cout << "\n复制完成，共 " << total << " 字节" << std::endl;
    return true;
}
```

### 示例 4: 日志文件追加

```cpp
#include "acl_cpp/lib_acl.hpp"
#include <ctime>

class Logger {
public:
    Logger(const char* filename) {
        file_.open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644);
    }
    
    void log(const char* level, const char* message) {
        if (!file_.opened()) {
            return;
        }
        
        // 获取当前时间
        time_t now = time(nullptr);
        char timebuf[64];
        strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", localtime(&now));
        
        // 写入日志
        file_.format("[%s] [%s] %s\n", timebuf, level, message);
        file_.fflush();
    }
    
private:
    acl::fstream file_;
};

void logger_example() {
    Logger logger("app.log");
    
    logger.log("INFO", "应用程序启动");
    logger.log("DEBUG", "初始化配置");
    logger.log("WARN", "内存使用率较高");
    logger.log("ERROR", "连接数据库失败");
}
```

## 网络通信示例

### 示例 5: HTTP 客户端

```cpp
#include "acl_cpp/lib_acl.hpp"

bool http_get(const char* url, acl::string& response) {
    // 解析 URL
    acl::string host = "www.example.com";
    int port = 80;
    acl::string path = "/";
    
    // 连接服务器
    acl::socket_stream conn;
    acl::string addr;
    addr.format("%s:%d", host.c_str(), port);
    
    if (!conn.open(addr.c_str(), 10, 30)) {
        std::cerr << "连接失败: " << addr.c_str() << std::endl;
        return false;
    }
    
    // 发送 HTTP 请求
    acl::string request;
    request.format("GET %s HTTP/1.1\r\n", path.c_str());
    request.format("Host: %s\r\n", host.c_str());
    request.append("Connection: close\r\n");
    request.append("\r\n");
    
    if (conn.write(request.c_str(), request.size()) == -1) {
        std::cerr << "发送请求失败" << std::endl;
        return false;
    }
    
    // 接收响应
    acl::string line;
    bool header_end = false;
    
    // 读取响应头
    while (conn.gets(line)) {
        response.append(line);
        if (line.empty() || line == "\r\n") {
            header_end = true;
            break;
        }
    }
    
    // 读取响应体
    if (header_end) {
        char buf[4096];
        while (true) {
            int ret = conn.read(buf, sizeof(buf) - 1, false);
            if (ret <= 0) {
                break;
            }
            response.append(buf, ret);
        }
    }
    
    return true;
}

void http_client_example() {
    acl::string response;
    if (http_get("http://www.example.com/", response)) {
        std::cout << "响应长度: " << response.size() << " 字节" << std::endl;
        std::cout << response.c_str() << std::endl;
    }
}
```

### 示例 6: Echo 服务器（多线程）

```cpp
#include "acl_cpp/lib_acl.hpp"
#include <thread>
#include <vector>

void handle_client(acl::socket_stream* client) {
    std::cout << "客户端连接: " << client->get_peer() << std::endl;
    
    acl::string buf;
    while (client->gets(buf)) {
        if (buf.empty()) {
            break;
        }
        
        std::cout << "收到: " << buf.c_str();
        
        // 回显
        client->write(buf.c_str(), buf.size());
    }
    
    std::cout << "客户端断开: " << client->get_peer() << std::endl;
    delete client;
}

void echo_server() {
    acl::server_socket server(acl::OPEN_FLAG_REUSEPORT, 128);
    
    if (!server.open("0.0.0.0:8080")) {
        std::cerr << "监听失败" << std::endl;
        return;
    }
    
    std::cout << "Echo 服务器启动在端口 8080" << std::endl;
    
    std::vector<std::thread> threads;
    
    while (true) {
        acl::socket_stream* client = server.accept();
        if (client == nullptr) {
            std::cerr << "接受连接失败" << std::endl;
            continue;
        }
        
        // 为每个客户端创建线程
        threads.emplace_back(handle_client, client);
        
        // 分离线程
        threads.back().detach();
    }
}
```

### 示例 7: 简单 HTTP 服务器

```cpp
#include "acl_cpp/lib_acl.hpp"

void send_http_response(acl::socket_stream* client, 
                        int status_code, 
                        const char* content) {
    acl::string response;
    
    // 状态行
    response.format("HTTP/1.1 %d OK\r\n", status_code);
    
    // 响应头
    response.append("Content-Type: text/html; charset=utf-8\r\n");
    response.format("Content-Length: %zu\r\n", strlen(content));
    response.append("Connection: close\r\n");
    response.append("\r\n");
    
    // 响应体
    response.append(content);
    
    client->write(response.c_str(), response.size());
}

void http_server() {
    acl::server_socket server(0, 128);
    
    if (!server.open("0.0.0.0:8080")) {
        std::cerr << "监听失败" << std::endl;
        return;
    }
    
    std::cout << "HTTP 服务器启动在端口 8080" << std::endl;
    
    while (true) {
        acl::socket_stream* client = server.accept();
        if (client == nullptr) {
            continue;
        }
        
        // 读取请求
        acl::string request_line;
        if (client->gets(request_line)) {
            std::cout << "请求: " << request_line.c_str();
            
            // 读取并忽略其余头部
            acl::string line;
            while (client->gets(line)) {
                if (line.empty() || line == "\r\n") {
                    break;
                }
            }
            
            // 发送响应
            const char* html = "<!DOCTYPE html>\n"
                               "<html>\n"
                               "<head><title>ACL HTTP Server</title></head>\n"
                               "<body>\n"
                               "<h1>Hello from ACL!</h1>\n"
                               "<p>这是一个简单的 HTTP 服务器示例</p>\n"
                               "</body>\n"
                               "</html>\n";
            
            send_http_response(client, 200, html);
        }
        
        delete client;
    }
}
```

### 示例 8: UDP 聊天程序

```cpp
#include "acl_cpp/lib_acl.hpp"
#include <thread>

// UDP 接收线程
void udp_receiver(acl::socket_stream* udp) {
    char buf[1024];
    
    while (true) {
        int ret = udp->read(buf, sizeof(buf) - 1);
        if (ret > 0) {
            buf[ret] = '\0';
            std::cout << "\n收到消息: " << buf << std::endl;
            std::cout << "> " << std::flush;
        }
    }
}

void udp_chat_client() {
    acl::socket_stream udp;
    
    // 绑定本地端口
    if (!udp.bind_udp("0.0.0.0:0", 30)) {
        std::cerr << "绑定失败" << std::endl;
        return;
    }
    
    std::cout << "本地地址: " << udp.get_local() << std::endl;
    std::cout << "请输入对端地址 (例如: 127.0.0.1:9000): ";
    
    char peer[64];
    std::cin.getline(peer, sizeof(peer));
    udp.set_peer(peer);
    
    // 启动接收线程
    std::thread receiver(udp_receiver, &udp);
    receiver.detach();
    
    // 发送消息
    std::cout << "开始聊天，输入消息并回车发送:\n";
    
    while (true) {
        std::cout << "> " << std::flush;
        
        char msg[1024];
        if (!std::cin.getline(msg, sizeof(msg))) {
            break;
        }
        
        if (strlen(msg) == 0) {
            continue;
        }
        
        if (udp.write(msg, strlen(msg)) == -1) {
            std::cerr << "发送失败" << std::endl;
        }
    }
}
```

## 异步 I/O 示例

### 示例 9: 异步 Echo 服务器

```cpp
#include "acl_cpp/lib_acl.hpp"

// 客户端处理回调
class echo_callback : public acl::aio_callback {
public:
    echo_callback(acl::aio_socket_stream* conn) : conn_(conn) {}
    
    // 读取回调
    bool read_callback(char* data, int len) override {
        std::cout << "收到数据: " << std::string(data, len);
        
        // 回显数据
        conn_->write(data, len);
        
        // 继续读取下一行
        conn_->gets(30);
        return true;
    }
    
    // 写完成回调
    bool write_callback() override {
        std::cout << "数据发送完成" << std::endl;
        return true;
    }
    
    // 超时回调
    bool timeout_callback() override {
        std::cout << "客户端超时: " << conn_->get_peer() << std::endl;
        return false;  // 关闭连接
    }
    
    // 关闭回调
    void close_callback() override {
        std::cout << "客户端关闭: " << conn_->get_peer() << std::endl;
        delete this;
    }
    
private:
    acl::aio_socket_stream* conn_;
};

// 监听回调
class echo_accept : public acl::aio_callback {
public:
    echo_accept(acl::aio_handle* handle) : handle_(handle) {}
    
    bool accept_callback(acl::aio_socket_stream* client) override {
        std::cout << "新客户端: " << client->get_peer() << std::endl;
        
        // 创建客户端回调
        echo_callback* callback = new echo_callback(client);
        client->add_read_callback(callback);
        client->add_write_callback(callback);
        client->add_timeout_callback(callback);
        client->add_close_callback(callback);
        
        // 开始读取
        client->gets(30);  // 30 秒超时
        
        return true;
    }
    
private:
    acl::aio_handle* handle_;
};

void async_echo_server() {
    // 创建异步事件引擎
    acl::aio_handle handle(acl::ENGINE_KERNEL);
    
    // 创建监听流
    acl::aio_listen_stream* listener = new acl::aio_listen_stream(&handle);
    
    if (!listener->open("0.0.0.0:8080")) {
        std::cerr << "监听失败" << std::endl;
        return;
    }
    
    // 设置接受回调
    echo_accept* callback = new echo_accept(&handle);
    listener->add_accept_callback(callback);
    
    std::cout << "异步 Echo 服务器启动在端口 8080" << std::endl;
    
    // 事件循环
    while (!handle.check()) {
        // 处理事件
    }
}
```

### 示例 10: 异步 HTTP 客户端

```cpp
#include "acl_cpp/lib_acl.hpp"

class http_client_callback : public acl::aio_callback {
public:
    http_client_callback(acl::aio_handle* handle, const char* url)
        : handle_(handle), url_(url), conn_(nullptr) {}
    
    // 连接成功
    bool open_callback() override {
        std::cout << "连接成功: " << conn_->get_peer() << std::endl;
        
        // 构建 HTTP 请求
        acl::string request;
        request.format("GET %s HTTP/1.1\r\n", "/");
        request.format("Host: %s\r\n", "www.example.com");
        request.append("Connection: close\r\n");
        request.append("\r\n");
        
        // 发送请求
        conn_->write(request.c_str(), request.size());
        
        // 开始读取响应
        conn_->gets(30);
        
        return true;
    }
    
    // 读取响应
    bool read_callback(char* data, int len) override {
        response_.append(data, len);
        
        // 继续读取
        conn_->gets(30);
        
        return true;
    }
    
    // 关闭连接
    void close_callback() override {
        std::cout << "\n响应完成，共 " << response_.size() << " 字节" << std::endl;
        std::cout << response_.c_str() << std::endl;
        
        // 停止事件循环
        handle_->stop();
        
        delete this;
    }
    
    void set_conn(acl::aio_socket_stream* conn) {
        conn_ = conn;
    }
    
private:
    acl::aio_handle* handle_;
    acl::string url_;
    acl::aio_socket_stream* conn_;
    acl::string response_;
};

void async_http_client() {
    acl::aio_handle handle(acl::ENGINE_KERNEL);
    
    // 连接服务器
    acl::aio_socket_stream* conn = acl::aio_socket_stream::open(
        &handle, "www.example.com:80", 10, 30);
    
    if (conn == nullptr) {
        std::cerr << "连接失败" << std::endl;
        return;
    }
    
    // 设置回调
    http_client_callback* callback = new http_client_callback(&handle, "/");
    callback->set_conn(conn);
    
    conn->add_open_callback(callback);
    conn->add_read_callback(callback);
    conn->add_close_callback(callback);
    
    // 事件循环
    while (!handle.check()) {
        // 处理事件
    }
}
```

### 示例 11: 异步定时任务

```cpp
#include "acl_cpp/lib_acl.hpp"

class periodic_task : public acl::aio_timer_callback {
public:
    periodic_task(acl::aio_handle* handle, int interval_ms)
        : handle_(handle), interval_ms_(interval_ms), count_(0) {}
    
    void timer_callback(unsigned int id) override {
        count_++;
        
        // 获取当前时间
        time_t now = time(nullptr);
        char timebuf[64];
        strftime(timebuf, sizeof(timebuf), "%H:%M:%S", localtime(&now));
        
        std::cout << "[" << timebuf << "] 定时任务执行，计数: " << count_ << std::endl;
        
        // 执行任务...
        do_task();
        
        // 继续下一次定时
        if (count_ < 10) {  // 执行 10 次后停止
            handle_->set_timer(this, interval_ms_ * 1000LL, id);
        }
    }
    
    void destroy() override {
        std::cout << "定时任务结束" << std::endl;
        delete this;
    }
    
private:
    void do_task() {
        // 执行实际任务
        // 例如：检查数据库、清理缓存、发送心跳等
    }
    
    acl::aio_handle* handle_;
    int interval_ms_;
    int count_;
};

void async_timer_example() {
    acl::aio_handle handle(acl::ENGINE_KERNEL);
    
    // 创建定时任务，每 2 秒执行一次
    periodic_task* task = new periodic_task(&handle, 2000);
    handle.set_timer(task, 2000000LL, 1);  // 2000000 微秒 = 2 秒
    
    std::cout << "定时任务启动" << std::endl;
    
    // 事件循环
    while (!handle.check()) {
        // 处理事件
    }
}
```

## SSL/TLS 示例

### 示例 12: HTTPS 客户端

```cpp
#include "acl_cpp/lib_acl.hpp"
#include "acl_cpp/stream/openssl_conf.hpp"
#include "acl_cpp/stream/openssl_io.hpp"

bool https_get(const char* host, const char* path, acl::string& response) {
    // 初始化 SSL 配置
    acl::openssl_conf ssl_conf;
    
    // 连接服务器
    acl::socket_stream conn;
    acl::string addr;
    addr.format("%s:443", host);
    
    if (!conn.open(addr.c_str(), 10, 30)) {
        std::cerr << "连接失败" << std::endl;
        return false;
    }
    
    // 创建 SSL IO
    acl::openssl_io* ssl = new acl::openssl_io(ssl_conf, false);
    ssl->set_sni_host(host);
    
    if (conn.setup_hook(ssl) != ssl) {
        std::cerr << "SSL 绑定失败" << std::endl;
        delete ssl;
        return false;
    }
    
    // SSL 握手
    if (!ssl->handshake()) {
        std::cerr << "SSL 握手失败" << std::endl;
        return false;
    }
    
    std::cout << "SSL 连接建立成功" << std::endl;
    
    // 发送 HTTPS 请求
    acl::string request;
    request.format("GET %s HTTP/1.1\r\n", path);
    request.format("Host: %s\r\n", host);
    request.append("Connection: close\r\n");
    request.append("\r\n");
    
    if (conn.write(request.c_str(), request.size()) == -1) {
        std::cerr << "发送请求失败" << std::endl;
        return false;
    }
    
    // 接收响应
    char buf[4096];
    while (true) {
        int ret = conn.read(buf, sizeof(buf) - 1, false);
        if (ret <= 0) {
            break;
        }
        response.append(buf, ret);
    }
    
    return true;
}

void https_client_example() {
    acl::string response;
    if (https_get("www.baidu.com", "/", response)) {
        std::cout << "响应长度: " << response.size() << " 字节" << std::endl;
        std::cout << response.c_str() << std::endl;
    }
}
```

### 示例 13: HTTPS 服务器

```cpp
#include "acl_cpp/lib_acl.hpp"
#include "acl_cpp/stream/openssl_conf.hpp"
#include "acl_cpp/stream/openssl_io.hpp"
#include <thread>

void handle_https_client(acl::socket_stream* client, acl::openssl_conf& ssl_conf) {
    std::cout << "客户端连接: " << client->get_peer() << std::endl;
    
    // 创建 SSL IO
    acl::openssl_io* ssl = new acl::openssl_io(ssl_conf, true);
    
    if (client->setup_hook(ssl) != ssl) {
        std::cerr << "SSL 绑定失败" << std::endl;
        delete ssl;
        delete client;
        return;
    }
    
    // SSL 握手
    if (!ssl->handshake()) {
        std::cerr << "SSL 握手失败" << std::endl;
        delete client;
        return;
    }
    
    std::cout << "SSL 握手成功" << std::endl;
    
    // 读取 HTTP 请求
    acl::string request_line;
    if (client->gets(request_line)) {
        std::cout << "请求: " << request_line.c_str();
        
        // 读取剩余头部
        acl::string line;
        while (client->gets(line)) {
            if (line.empty() || line == "\r\n") {
                break;
            }
        }
        
        // 发送 HTTPS 响应
        const char* html = "<!DOCTYPE html>\n"
                           "<html>\n"
                           "<head><title>HTTPS Server</title></head>\n"
                           "<body>\n"
                           "<h1>Secure Connection!</h1>\n"
                           "<p>这是一个 HTTPS 服务器示例</p>\n"
                           "</body>\n"
                           "</html>\n";
        
        acl::string response;
        response.append("HTTP/1.1 200 OK\r\n");
        response.append("Content-Type: text/html\r\n");
        response.format("Content-Length: %zu\r\n", strlen(html));
        response.append("Connection: close\r\n");
        response.append("\r\n");
        response.append(html);
        
        client->write(response.c_str(), response.size());
    }
    
    delete client;
}

void https_server_example() {
    // 初始化 SSL 配置
    acl::openssl_conf ssl_conf;
    
    // 加载证书和私钥（需要提前生成）
    if (!ssl_conf.add_cert("server.crt", "server.key")) {
        std::cerr << "加载证书失败" << std::endl;
        return;
    }
    
    // 创建服务器
    acl::server_socket server(0, 128);
    if (!server.open("0.0.0.0:8443")) {
        std::cerr << "监听失败" << std::endl;
        return;
    }
    
    std::cout << "HTTPS 服务器启动在端口 8443" << std::endl;
    
    while (true) {
        acl::socket_stream* client = server.accept();
        if (client == nullptr) {
            continue;
        }
        
        // 为每个客户端创建线程
        std::thread([client, &ssl_conf]() {
            handle_https_client(client, ssl_conf);
        }).detach();
    }
}
```

## 综合应用示例

### 示例 14: 简单代理服务器

```cpp
#include "acl_cpp/lib_acl.hpp"
#include <thread>

void proxy_transfer(acl::socket_stream* from, acl::socket_stream* to, const char* name) {
    char buf[4096];
    
    while (true) {
        int ret = from->read(buf, sizeof(buf), false);
        if (ret <= 0) {
            std::cout << name << " 读取结束" << std::endl;
            break;
        }
        
        if (to->write(buf, ret) != ret) {
            std::cerr << name << " 写入失败" << std::endl;
            break;
        }
    }
    
    // 关闭写端
    to->shutdown_write();
}

void handle_proxy_client(acl::socket_stream* client, const char* backend_addr) {
    std::cout << "客户端连接: " << client->get_peer() << std::endl;
    
    // 连接后端服务器
    acl::socket_stream backend;
    if (!backend.open(backend_addr, 10, 30)) {
        std::cerr << "连接后端失败: " << backend_addr << std::endl;
        delete client;
        return;
    }
    
    std::cout << "连接后端成功: " << backend_addr << std::endl;
    
    // 双向转发
    std::thread t1([client, &backend]() {
        proxy_transfer(client, &backend, "客户端->后端");
    });
    
    std::thread t2([client, &backend]() {
        proxy_transfer(&backend, client, "后端->客户端");
    });
    
    t1.join();
    t2.join();
    
    std::cout << "代理完成" << std::endl;
    delete client;
}

void proxy_server() {
    const char* backend_addr = "127.0.0.1:80";  // 后端服务器地址
    
    acl::server_socket server(0, 128);
    if (!server.open("0.0.0.0:8888")) {
        std::cerr << "监听失败" << std::endl;
        return;
    }
    
    std::cout << "代理服务器启动在端口 8888，转发到 " << backend_addr << std::endl;
    
    while (true) {
        acl::socket_stream* client = server.accept();
        if (client == nullptr) {
            continue;
        }
        
        std::thread([client, backend_addr]() {
            handle_proxy_client(client, backend_addr);
        }).detach();
    }
}
```

### 示例 15: 文件下载服务器

```cpp
#include "acl_cpp/lib_acl.hpp"

void send_file(acl::socket_stream* client, const char* filepath) {
    // 打开文件
    acl::fstream file;
    if (!file.open_read(filepath)) {
        // 发送 404 错误
        const char* error_404 = 
            "HTTP/1.1 404 Not Found\r\n"
            "Content-Type: text/plain\r\n"
            "\r\n"
            "File not found\r\n";
        client->write(error_404, strlen(error_404));
        return;
    }
    
    // 获取文件大小
    long long file_size = file.fsize();
    
    // 发送 HTTP 响应头
    acl::string header;
    header.append("HTTP/1.1 200 OK\r\n");
    header.append("Content-Type: application/octet-stream\r\n");
    header.format("Content-Length: %lld\r\n", file_size);
    header.format("Content-Disposition: attachment; filename=\"%s\"\r\n", filepath);
    header.append("\r\n");
    
    client->write(header.c_str(), header.size());
    
    // 传输文件内容
    char buf[65536];  // 64KB 缓冲
    long long total = 0;
    
    while (true) {
        int ret = file.read(buf, sizeof(buf), false);
        if (ret <= 0) {
            break;
        }
        
        if (client->write(buf, ret) != ret) {
            std::cerr << "发送失败" << std::endl;
            break;
        }
        
        total += ret;
        
        // 显示进度
        if (total % (1024 * 1024) == 0) {
            std::cout << "已发送: " << (total / 1024 / 1024) << " MB / "
                      << (file_size / 1024 / 1024) << " MB\r" << std::flush;
        }
    }
    
    std::cout << "\n文件发送完成: " << total << " 字节" << std::endl;
}

void file_download_server() {
    acl::server_socket server(0, 128);
    if (!server.open("0.0.0.0:8080")) {
        std::cerr << "监听失败" << std::endl;
        return;
    }
    
    std::cout << "文件下载服务器启动在端口 8080" << std::endl;
    
    while (true) {
        acl::socket_stream* client = server.accept();
        if (client == nullptr) {
            continue;
        }
        
        // 读取请求行
        acl::string request_line;
        if (client->gets(request_line)) {
            // 解析文件路径（简化示例）
            acl::string filepath = "test.dat";  // 实际应从请求中解析
            
            std::cout << "请求文件: " << filepath.c_str() << std::endl;
            
            // 读取剩余头部
            acl::string line;
            while (client->gets(line)) {
                if (line.empty() || line == "\r\n") {
                    break;
                }
            }
            
            // 发送文件
            send_file(client, filepath.c_str());
        }
        
        delete client;
    }
}
```

### 示例 16: 聊天室服务器

```cpp
#include "acl_cpp/lib_acl.hpp"
#include <thread>
#include <mutex>
#include <vector>

class ChatRoom {
public:
    void add_client(acl::socket_stream* client, const std::string& nickname) {
        std::lock_guard<std::mutex> lock(mutex_);
        clients_.push_back({client, nickname});
        broadcast(nickname + " 加入了聊天室\n", client);
    }
    
    void remove_client(acl::socket_stream* client) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        for (auto it = clients_.begin(); it != clients_.end(); ++it) {
            if (it->first == client) {
                broadcast(it->second + " 离开了聊天室\n", client);
                clients_.erase(it);
                break;
            }
        }
    }
    
    void broadcast(const std::string& message, acl::socket_stream* exclude = nullptr) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        for (const auto& pair : clients_) {
            if (pair.first != exclude) {
                pair.first->write(message.c_str(), message.size());
            }
        }
    }
    
    std::string get_nickname(acl::socket_stream* client) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        for (const auto& pair : clients_) {
            if (pair.first == client) {
                return pair.second;
            }
        }
        return "Unknown";
    }
    
private:
    std::mutex mutex_;
    std::vector<std::pair<acl::socket_stream*, std::string>> clients_;
};

void handle_chat_client(acl::socket_stream* client, ChatRoom& room) {
    // 获取昵称
    client->write("请输入昵称: ", 18);
    
    acl::string nickname;
    if (!client->gets(nickname) || nickname.empty()) {
        delete client;
        return;
    }
    
    nickname.trim();
    
    // 添加到聊天室
    room.add_client(client, nickname.c_str());
    
    client->write("欢迎加入聊天室！输入 /quit 退出\n", 48);
    
    // 接收消息
    acl::string message;
    while (client->gets(message)) {
        message.trim();
        
        if (message.empty()) {
            continue;
        }
        
        if (message == "/quit") {
            break;
        }
        
        // 广播消息
        std::string broadcast_msg = room.get_nickname(client) + ": " + message.c_str() + "\n";
        room.broadcast(broadcast_msg, client);
    }
    
    // 移除客户端
    room.remove_client(client);
    delete client;
}

void chat_server() {
    ChatRoom room;
    
    acl::server_socket server(0, 128);
    if (!server.open("0.0.0.0:9999")) {
        std::cerr << "监听失败" << std::endl;
        return;
    }
    
    std::cout << "聊天室服务器启动在端口 9999" << std::endl;
    
    while (true) {
        acl::socket_stream* client = server.accept();
        if (client == nullptr) {
            continue;
        }
        
        std::thread([client, &room]() {
            handle_chat_client(client, room);
        }).detach();
    }
}
```

## 编译和运行

### 编译示例

```bash
# 编译文件操作示例
g++ -o file_example file_example.cpp -lacl_cpp -lacl -lpthread -ldl -lz

# 编译网络示例
g++ -o echo_server echo_server.cpp -lacl_cpp -lacl -lpthread -ldl -lz

# 编译 SSL 示例（需要 OpenSSL）
g++ -o https_client https_client.cpp -lacl_cpp -lacl -lssl -lcrypto -lpthread -ldl -lz
```

### 运行示例

```bash
# 运行服务器
./echo_server

# 使用 telnet 测试
telnet localhost 8080

# 使用 curl 测试 HTTP
curl http://localhost:8080/
```

## 注意事项

1. **错误处理**: 所有示例都应该添加完善的错误处理
2. **资源释放**: 注意及时释放 socket、文件等资源
3. **线程安全**: 多线程环境下注意同步
4. **缓冲区大小**: 根据实际需求调整缓冲区大小
5. **超时设置**: 合理设置连接和读写超时
6. **SSL 证书**: HTTPS 示例需要提前生成证书文件

## 更多示例

更多示例代码可以参考：
- ACL 源码中的 `samples` 目录
- ACL 项目的单元测试代码
- ACL 官方文档和教程

