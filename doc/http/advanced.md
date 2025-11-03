# HTTP高级特性

## 1. 内存池管理

ACL HTTP库使用内存池技术优化内存分配，减少频繁的malloc/free调用。

### 1.1 dbuf_pool使用

```cpp
#include "acl_cpp/lib_acl.hpp"

// 创建内存池
acl::dbuf_guard pool;

// HTTP头使用内存池
acl::http_header header(&pool);

// Cookie使用内存池
acl::HttpCookie cookie("name", "value", &pool);

// 内存池会在dbuf_guard析构时自动释放所有内存
```

### 1.2 自定义内存分配器

```cpp
class MyAllocator : public acl::dbuf_pool {
public:
    void* dbuf_alloc(size_t size) override {
        // 自定义内存分配
        return custom_malloc(size);
    }
    
    void* dbuf_calloc(size_t size) override {
        // 自定义清零分配
        return custom_calloc(size);
    }
};
```

## 2. 字符集转换

### 2.1 自动字符集转换

```cpp
// 客户端
acl::http_request req("www.example.com:80");

// 设置本地字符集为GBK
req.set_local_charset("gbk");

// 请求时自动转换
req.request_header().set_url("/api/data");
req.request(NULL, 0);

// 响应自动从UTF-8转换为GBK
acl::string body;
req.get_body(body, "gbk");
```

```cpp
// 服务端
class MyServlet : public acl::HttpServlet {
public:
    MyServlet(acl::socket_stream* stream, acl::session* session)
        : HttpServlet(stream, session) {
        // 设置本地字符集
        setLocalCharset("gbk");
    }

protected:
    bool doPost(acl::HttpServletRequest& req, 
                acl::HttpServletResponse& res) override {
        // 请求体会自动转换为GBK
        acl::string body;
        req.getBody(body);
        
        // 处理GBK数据
        // ...
        
        return true;
    }
};
```

### 2.2 手动字符集转换

```cpp
#include "acl_cpp/lib_acl.hpp"

acl::charset_conv conv;

// UTF-8转GBK
acl::string input = "中文测试";
acl::string output;
conv.convert("utf-8", "gbk", input.c_str(), input.length(), &output);

// GBK转UTF-8
conv.convert("gbk", "utf-8", input.c_str(), input.length(), &output);
```

## 3. HTTP压缩

### 3.1 客户端请求压缩内容

```cpp
acl::http_request req("www.example.com:80");

// 请求GZIP压缩
req.request_header().accept_gzip(true);

// 启用自动解压
req.set_unzip(true);

req.request_header().set_url("/large/data");

if (req.request(NULL, 0)) {
    // 自动解压响应
    acl::string body;
    req.get_body(body);
}
```

### 3.2 服务端压缩响应

```cpp
bool MyServlet::doGet(acl::HttpServletRequest& req, 
                      acl::HttpServletResponse& res) {
    // 检查客户端支持
    std::vector<acl::string> encodings;
    req.getAcceptEncoding(encodings);
    
    for (auto& enc : encodings) {
        if (enc == "gzip") {
            // 启用GZIP压缩
            res.setContentEncoding(true);
            break;
        }
    }
    
    // 发送大数据
    acl::string large_data;
    // ... 填充数据
    res.write(large_data);
    
    return true;
}
```

### 3.3 手动压缩/解压

```cpp
#include "acl_cpp/lib_acl.hpp"

// GZIP压缩
acl::zlib_stream zstream;
zstream.set_out_max_length(8192);

const char* data = "要压缩的数据";
size_t len = strlen(data);

if (zstream.zip_begin(acl::zlib_stream::ZLIB_TYPE_GZIP)) {
    if (zstream.zip_update(data, len)) {
        const char* compressed;
        size_t compressed_len;
        
        if (zstream.zip_finish(&compressed, &compressed_len)) {
            printf("压缩后大小: %zu\n", compressed_len);
        }
    }
}

// GZIP解压
acl::zlib_stream zstream2;
if (zstream2.unzip_begin()) {
    if (zstream2.unzip_update(compressed, compressed_len)) {
        const char* decompressed;
        size_t decompressed_len;
        
        if (zstream2.unzip_finish(&decompressed, &decompressed_len)) {
            printf("解压后: %.*s\n", (int)decompressed_len, decompressed);
        }
    }
}
```

## 4. SSL/TLS加密

### 4.1 客户端SSL

```cpp
#include "acl_cpp/lib_acl.hpp"

// 使用MbedTLS
acl::mbedtls_conf ssl_conf(false);  // false表示客户端

acl::http_request req("www.example.com:443");
req.set_ssl(&ssl_conf);

// 设置SNI
req.set_ssl_sni("www.example.com");

// 发送HTTPS请求
req.request_header().set_url("/secure/api");
req.request(NULL, 0);
```

```cpp
// 使用OpenSSL
acl::openssl_conf ssl_conf;
ssl_conf.enable_cache(true);

// 加载CA证书
ssl_conf.add_cert_dir("/etc/ssl/certs");

acl::http_request req("www.example.com:443");
req.set_ssl(&ssl_conf);
```

### 4.2 服务端SSL

```cpp
#include "acl_cpp/lib_acl.hpp"

int main() {
    // 创建SSL配置
    acl::mbedtls_conf ssl_conf(true);  // true表示服务端
    
    // 加载证书和私钥
    ssl_conf.add_cert("/path/to/cert.pem");
    ssl_conf.set_key("/path/to/key.pem");
    
    // 创建服务器
    acl::server_socket server;
    server.open("0.0.0.0:443");
    
    while (true) {
        acl::socket_stream* conn = server.accept();
        if (conn == NULL) continue;
        
        // SSL握手
        acl::mbedtls_io ssl_io(&ssl_conf, true);
        if (!ssl_io.handshake(*conn)) {
            printf("SSL握手失败\n");
            delete conn;
            continue;
        }
        
        // 处理HTTPS请求
        acl::memcache_session session("127.0.0.1:11211");
        MyServlet servlet(conn, &session);
        servlet.doRun();
        
        delete conn;
    }
    
    return 0;
}
```

### 4.3 证书验证

```cpp
// 客户端证书验证
acl::openssl_conf ssl_conf;

// 启用证书验证
ssl_conf.set_verify_mode(acl::OPENSSL_VERIFY_PEER);

// 加载CA证书
ssl_conf.add_cert_file("/path/to/ca-bundle.crt");

// 设置验证深度
ssl_conf.set_verify_depth(4);
```

## 5. 连接池

### 5.1 HTTP连接池

```cpp
#include "acl_cpp/lib_acl.hpp"

// 创建连接池管理器
acl::connect_manager manager;

// 创建服务器连接池
acl::http_request_pool* pool = new acl::http_request_pool(
    "api.example.com:80",  // 服务器地址
    100,                    // 最大连接数
    0                       // 连接池索引
);

// 设置连接超时
pool->set_timeout(10, 10);  // 连接超时，IO超时（秒）

// 设置空闲时间
pool->set_idle(60);  // 60秒

// 添加到管理器
manager.set(pool);

// 使用连接
for (int i = 0; i < 1000; i++) {
    acl::http_request* req = (acl::http_request*)pool->peek();
    if (req == NULL) {
        printf("获取连接失败\n");
        continue;
    }
    
    // 重置状态
    req->request_header().reset();
    req->request_header().set_url("/api/test");
    req->request_header().set_host("api.example.com");
    
    if (req->request(NULL, 0)) {
        acl::string body;
        req->get_body(body);
        printf("响应: %s\n", body.c_str());
    }
    
    // 归还连接
    pool->put(req, true);  // true保持连接
}
```

### 5.2 连接池配置

```cpp
// 创建多服务器连接池
acl::connect_manager manager;

manager.init("www.google.com:80|10, www.bing.com:80|10");
// 格式：服务器地址|最大连接数, ...

// 设置全局参数
manager.set_timeout(10, 10);  // 连接超时，IO超时
manager.set_idle(120);         // 空闲超时

// 从连接池获取连接（自动选择服务器）
acl::http_request* req = (acl::http_request*)
    manager.peek("www.google.com:80");
```

### 5.3 连接池统计

```cpp
acl::http_request_pool* pool = /* ... */;

// 获取统计信息
printf("总连接数: %d\n", pool->get_count());
printf("空闲连接: %d\n", pool->get_idle_count());
```

## 6. 异步HTTP

### 6.1 异步客户端

```cpp
#include "acl_cpp/lib_acl.hpp"

class MyAsyncClient : public acl::http_aclient {
public:
    MyAsyncClient(acl::aio_handle& handle) 
        : http_aclient(handle) {}
    
    void destroy() override { delete this; }

protected:
    bool on_connect() override {
        // 设置请求
        request_header().set_url("/api/test");
        request_header().set_host("www.example.com");
        
        // 发送请求
        send_request(NULL, 0);
        return true;
    }
    
    bool on_http_res_hdr(const acl::http_header& header) override {
        printf("状态码: %d\n", header.get_status());
        return true;
    }
    
    bool on_http_res_body(char* data, size_t dlen) override {
        printf("数据: %.*s\n", (int)dlen, data);
        return true;
    }
    
    bool on_http_res_finish(bool success) override {
        printf("完成: %s\n", success ? "成功" : "失败");
        return false;
    }
};

int main() {
    acl::aio_handle handle;
    
    // 创建多个异步请求
    for (int i = 0; i < 100; i++) {
        MyAsyncClient* client = new MyAsyncClient(handle);
        client->open("www.example.com:80", 10, 60);
    }
    
    // 事件循环
    while (true) {
        handle.check();
    }
    
    return 0;
}
```

### 6.2 异步POST请求

```cpp
class MyAsyncClient : public acl::http_aclient {
protected:
    bool on_connect() override {
        // 构建POST数据
        acl::json json;
        json.add_text("username", "test");
        json.add_text("password", "123456");
        acl::string body = json.to_string();
        
        // 设置请求头
        request_header().set_url("/api/login");
        request_header().set_host("api.example.com");
        request_header().set_content_type("application/json");
        
        // 发送POST请求
        send_request(body.c_str(), body.length());
        
        return true;
    }
};
```

## 7. 流式处理

### 7.1 流式发送

```cpp
acl::http_request req("upload.example.com:80");

// 设置Content-Length
req.request_header().set_url("/upload");
req.request_header().set_content_length(file_size);

// 发送请求头
if (!req.write_head()) {
    return false;
}

// 流式发送文件
FILE* fp = fopen("/path/to/large/file", "rb");
char buf[8192];
while (true) {
    size_t n = fread(buf, 1, sizeof(buf), fp);
    if (n == 0) break;
    
    if (!req.write_body(buf, n)) {
        break;
    }
}
fclose(fp);

// 结束发送
req.write_body(NULL, 0);

// 读取响应
acl::string response;
req.get_body(response);
```

### 7.2 流式接收

```cpp
acl::http_request req("www.example.com:80");

req.request_header().set_url("/large/data");

if (req.request(NULL, 0)) {
    // 流式读取
    FILE* fp = fopen("/tmp/output.dat", "wb");
    
    char buf[8192];
    while (true) {
        int n = req.read_body(buf, sizeof(buf));
        if (n <= 0) break;
        
        fwrite(buf, 1, n, fp);
    }
    
    fclose(fp);
}
```

## 8. 多线程/多进程

### 8.1 多线程服务器

```cpp
#include <thread>
#include <vector>

void worker_thread(acl::server_socket* server) {
    while (true) {
        acl::socket_stream* conn = server->accept();
        if (conn == NULL) continue;
        
        acl::memcache_session session("127.0.0.1:11211");
        MyServlet servlet(conn, &session);
        servlet.doRun();
        
        delete conn;
    }
}

int main() {
    acl::server_socket server;
    server.open("0.0.0.0:8080");
    
    // 创建工作线程
    std::vector<std::thread> threads;
    for (int i = 0; i < 4; i++) {
        threads.emplace_back(worker_thread, &server);
    }
    
    // 等待线程结束
    for (auto& t : threads) {
        t.join();
    }
    
    return 0;
}
```

### 8.2 与acl_master集成

```cpp
#include "acl_cpp/lib_acl.hpp"

// 服务初始化
static void service_init(void* ctx) {
    (void) ctx;
    printf("服务启动\n");
}

// 服务退出
static void service_exit(void* ctx) {
    (void) ctx;
    printf("服务停止\n");
}

// 处理客户端连接
static void service_main(acl::socket_stream* conn, void* ctx) {
    (void) ctx;
    
    acl::memcache_session session("127.0.0.1:11211");
    MyServlet servlet(conn, &session);
    servlet.doRun();
}

int main(int argc, char* argv[]) {
    // 配置服务器参数
    acl::master_service_threaded service;
    
    // 设置回调
    service.set_cfg_int(/* 配置 */);
    service.set_cfg_str(/* 配置 */);
    
    // 运行服务
    service.run_daemon(argc, argv, service_main, 
                      service_init, service_exit);
    
    return 0;
}
```

## 9. 性能优化技巧

### 9.1 Keep-Alive

```cpp
// 客户端
acl::http_request req("www.example.com:80");
req.request_header().set_keep_alive(true);

// 服务端
bool MyServlet::doGet(acl::HttpServletRequest& req,
                      acl::HttpServletResponse& res) {
    res.setKeepAlive(true);
    // ...
    return true;
}
```

### 9.2 缓冲区优化

```cpp
// 设置socket缓冲区
acl::socket_stream conn;
conn.set_tcp_sendbuf(256 * 1024);  // 256KB
conn.set_tcp_recvbuf(256 * 1024);  // 256KB
```

### 9.3 批量操作

```cpp
// 批量发送请求
std::vector<acl::http_request*> requests;

for (int i = 0; i < 100; i++) {
    acl::http_request* req = /* 从连接池获取 */;
    req->request_header().set_url("/api/test");
    requests.push_back(req);
}

// 并发发送
for (auto req : requests) {
    req->request(NULL, 0);
}

// 收集结果
for (auto req : requests) {
    acl::string body;
    req->get_body(body);
}
```

## 10. 调试技巧

### 10.1 打印HTTP头

```cpp
// 客户端
acl::http_request req("www.example.com:80");
req.request(NULL, 0);

// 打印响应头
acl::http_client* client = req.get_client();
client->print_header("响应头");

// 服务端
bool MyServlet::doGet(acl::HttpServletRequest& req,
                      acl::HttpServletResponse& res) {
    // 打印请求头
    acl::string header_str;
    req.sprint_header(header_str, "请求头");
    printf("%s\n", header_str.c_str());
    
    return true;
}
```

### 10.2 日志记录

```cpp
#include "acl_cpp/lib_acl.hpp"

// 设置日志
acl::log::open("app.log", "myapp");

// 记录日志
acl::log::msg1("收到请求: %s", req.getRequestUri());
acl::log::error("错误: %s", error_msg);
```

### 10.3 性能分析

```cpp
#include <chrono>

auto start = std::chrono::high_resolution_clock::now();

// 执行HTTP请求
req.request(NULL, 0);

auto end = std::chrono::high_resolution_clock::now();
auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
    end - start).count();

printf("请求耗时: %ld ms\n", duration);
```

## 11. 常见陷阱

### 11.1 连接复用

```cpp
// 错误：没有重置状态
acl::http_request req("www.example.com:80");

for (int i = 0; i < 10; i++) {
    req.request_header().set_url("/api/test");
    req.request(NULL, 0);  // 第2次会失败
}

// 正确：重置状态
for (int i = 0; i < 10; i++) {
    req.reset();  // 重置
    req.request_header().set_url("/api/test");
    req.request(NULL, 0);
}
```

### 11.2 内存泄漏

```cpp
// 错误：没有释放连接
acl::http_request* req = (acl::http_request*)pool->peek();
req->request(NULL, 0);
// 忘记调用 pool->put(req, true);

// 正确：使用RAII
class HttpGuard {
public:
    HttpGuard(acl::http_request_pool& pool) 
        : pool_(pool), req_(NULL) {
        req_ = (acl::http_request*)pool_.peek();
    }
    
    ~HttpGuard() {
        if (req_) {
            pool_.put(req_, true);
        }
    }
    
    acl::http_request* get() { return req_; }

private:
    acl::http_request_pool& pool_;
    acl::http_request* req_;
};
```

### 11.3 超时设置

```cpp
// 错误：超时时间太短
acl::http_request req("slow-server.com:80", 1, 1);  // 1秒超时
req.request(NULL, 0);  // 可能超时

// 正确：合理的超时时间
acl::http_request req("slow-server.com:80", 10, 30);  // 10秒连接，30秒读写
```

## 最佳实践总结

1. 使用连接池复用连接
2. 合理设置超时时间
3. 启用Keep-Alive
4. 使用GZIP压缩大数据
5. 异步处理高并发
6. 正确管理资源生命周期
7. 完善的错误处理
8. 使用SSL保护敏感数据
9. 日志记录关键操作
10. 性能监控和调优

