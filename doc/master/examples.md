# ACL Master 使用示例

本文档提供了各种 Master 框架服务器模型的完整使用示例。

## 目录

- [master_threads 示例](#master_threads-示例)
- [master_proc 示例](#master_proc-示例)
- [master_aio 示例](#master_aio-示例)
- [master_fiber 示例](#master_fiber-示例)
- [master_trigger 示例](#master_trigger-示例)
- [master_udp 示例](#master_udp-示例)
- [配置文件示例](#配置文件示例)
- [编译和运行](#编译和运行)

---

## master_threads 示例

### 简单的 Echo 服务器（线程池模型）

```cpp
#include <acl_cpp/lib_acl.hpp>

// 配置参数
static int   g_io_timeout = 120;
static char* g_log_file = NULL;

static acl::master_str_tbl var_conf_str_tab[] = {
    { "log_file", "echo.log", &g_log_file },
    { NULL, NULL, NULL }
};

static acl::master_int_tbl var_conf_int_tab[] = {
    { "io_timeout", 120, &g_io_timeout, 0, 0 },
    { NULL, 0, NULL, 0, 0 }
};

// Echo 服务器类
class echo_server : public acl::master_threads {
public:
    echo_server() {}
    ~echo_server() {}

protected:
    // 进程初始化
    void proc_on_init() override {
        // 打开日志
        acl::log::open(g_log_file, "echo_server");
        acl::logger("Echo 服务器启动成功");
    }

    // 进程退出
    void proc_on_exit() override {
        acl::logger("Echo 服务器正在退出");
    }

    // 线程初始化
    void thread_on_init() override {
        acl::logger("工作线程 %lu 启动", acl::thread::self());
    }

    // 线程退出
    void thread_on_exit() override {
        acl::logger("工作线程 %lu 退出", acl::thread::self());
    }

    // 连接到达
    bool thread_on_accept(acl::socket_stream* stream) override {
        acl::logger("新连接来自: %s", stream->get_peer());
        
        // 设置读写超时
        stream->set_rw_timeout(g_io_timeout);
        
        return true;
    }

    // 处理客户端数据
    bool thread_on_read(acl::socket_stream* stream) override {
        char buf[8192];
        
        // 读取数据
        int ret = stream->read(buf, sizeof(buf) - 1, false);
        if (ret == -1) {
            acl::logger_error("读取失败: %s", acl::last_serror());
            return false;
        }
        
        buf[ret] = '\0';
        acl::logger("收到数据: %s", buf);
        
        // 回显数据
        if (stream->write(buf, ret) == -1) {
            acl::logger_error("写入失败: %s", acl::last_serror());
            return false;
        }
        
        // 继续保持连接
        return true;
    }

    // 连接超时
    bool thread_on_timeout(acl::socket_stream* stream) override {
        acl::logger("连接超时: %s", stream->get_peer());
        return false; // 关闭连接
    }

    // 连接关闭
    void thread_on_close(acl::socket_stream* stream) override {
        acl::logger("连接关闭: %s", stream->get_peer());
    }
};

int main(int argc, char* argv[]) {
    echo_server server;
    
    // 设置配置参数
    server.set_cfg_int(var_conf_int_tab);
    server.set_cfg_str(var_conf_str_tab);
    
    // 运行服务器
    server.run_daemon(argc, argv);
    
    return 0;
}
```

### HTTP 服务器示例（线程池模型）

```cpp
#include <acl_cpp/lib_acl.hpp>

class http_server : public acl::master_threads {
protected:
    bool thread_on_read(acl::socket_stream* stream) override {
        acl::http_request req(stream);
        acl::http_response& res = req.get_response();
        
        // 读取 HTTP 请求
        if (!req.read_header()) {
            acl::logger_error("读取 HTTP 头失败");
            return false;
        }
        
        // 获取请求信息
        acl::string method = req.request_method();
        acl::string path = req.request_url();
        
        acl::logger("HTTP 请求: %s %s", method.c_str(), path.c_str());
        
        // 构造响应
        acl::string body;
        body.format("<html><body><h1>Hello World!</h1>"
                   "<p>Method: %s</p><p>Path: %s</p>"
                   "</body></html>",
                   method.c_str(), path.c_str());
        
        // 发送响应
        res.set_status(200);
        res.set_content_type("text/html; charset=utf-8");
        res.set_content_length(body.size());
        
        if (!res.write(body) || !res.write(NULL, 0)) {
            return false;
        }
        
        // HTTP/1.0 或 Connection: close 则关闭连接
        if (!req.keep_alive()) {
            return false;
        }
        
        return true;
    }
};

int main(int argc, char* argv[]) {
    http_server server;
    server.run_daemon(argc, argv);
    return 0;
}
```

---

## master_proc 示例

### 简单的请求处理服务器（多进程模型）

```cpp
#include <acl_cpp/lib_acl.hpp>

class proc_server : public acl::master_proc {
public:
    proc_server() {}
    ~proc_server() {}

protected:
    void proc_on_init() override {
        acl::log::open("proc_server.log", "proc_server");
        acl::logger("进程 %d 初始化完成", getpid());
    }

    void proc_on_exit() override {
        acl::logger("进程 %d 退出", getpid());
    }

    // 处理客户端连接
    void on_accept(acl::socket_stream* stream) override {
        acl::logger("进程 %d 接受连接: %s", getpid(), stream->get_peer());
        
        // 设置超时
        stream->set_rw_timeout(120);
        
        char buf[8192];
        while (true) {
            int ret = stream->read(buf, sizeof(buf) - 1, false);
            if (ret == -1) {
                break;
            }
            
            buf[ret] = '\0';
            acl::logger("收到: %s", buf);
            
            // 处理请求（这里模拟耗时操作）
            acl::string response;
            response.format("进程 %d 处理结果: %s\r\n", getpid(), buf);
            
            if (stream->write(response) == -1) {
                break;
            }
        }
        
        acl::logger("连接处理完成");
        // 注意：函数返回后连接会被自动关闭
    }
};

int main(int argc, char* argv[]) {
    proc_server server;
    server.run_daemon(argc, argv);
    return 0;
}
```

---

## master_aio 示例

### 异步 Echo 服务器

```cpp
#include <acl_cpp/lib_acl.hpp>

// 异步客户端连接处理类
class echo_client : public acl::aio_callback {
public:
    echo_client(acl::aio_socket_stream* conn)
        : conn_(conn) {
        conn_->set_rw_timeout(120);
    }
    
    ~echo_client() {
        delete conn_;
    }
    
    // 开始读取
    void start() {
        // 异步读取数据
        conn_->read(1024, 0, this);
    }

protected:
    // 读取回调
    bool read_callback(char* data, int len) override {
        acl::logger("收到 %d 字节数据", len);
        
        // 异步写回数据
        conn_->write(data, len, this);
        return true;
    }
    
    // 写入回调
    bool write_callback() override {
        acl::logger("数据已发送");
        
        // 继续读取
        conn_->read(1024, 0, this);
        return true;
    }
    
    // 超时回调
    bool timeout_callback() override {
        acl::logger_warn("连接超时");
        return false; // 关闭连接
    }
    
    // 关闭回调
    void close_callback() override {
        acl::logger("连接已关闭");
        delete this; // 自动销毁
    }

private:
    acl::aio_socket_stream* conn_;
};

// 异步服务器类
class aio_echo_server : public acl::master_aio {
protected:
    void proc_on_init() override {
        acl::log::open("aio_echo.log", "aio_echo");
        acl::logger("异步 Echo 服务器启动");
    }

    void proc_on_exit() override {
        acl::logger("异步 Echo 服务器退出");
    }

    // 接受新连接
    bool on_accept(acl::aio_socket_stream* stream) override {
        acl::logger("新连接: %s", stream->get_peer());
        
        // 创建客户端处理对象
        echo_client* client = new echo_client(stream);
        client->start();
        
        return true; // 继续接受连接
    }
};

int main(int argc, char* argv[]) {
    aio_echo_server server;
    server.run_daemon(argc, argv);
    return 0;
}
```

### 异步 HTTP 服务器

```cpp
#include <acl_cpp/lib_acl.hpp>

class http_client : public acl::aio_callback {
public:
    http_client(acl::aio_socket_stream* conn)
        : conn_(conn), req_(conn) {}
    
    ~http_client() {
        delete conn_;
    }
    
    void start() {
        // 异步读取 HTTP 请求头
        req_.read_header(this);
    }

protected:
    bool read_wakeup() override {
        // HTTP 请求头读取完成
        acl::http_response& res = req_.get_response();
        acl::string method = req_.request_method();
        acl::string path = req_.request_url();
        
        acl::logger("HTTP: %s %s", method.c_str(), path.c_str());
        
        // 构造响应
        acl::string body = "Hello from async HTTP server!";
        
        res.set_status(200);
        res.set_content_type("text/plain");
        res.set_content_length(body.size());
        
        // 异步写入响应
        if (!res.write(body) || !res.write(NULL, 0)) {
            return false;
        }
        
        if (!req_.keep_alive()) {
            return false;
        }
        
        // 继续读取下一个请求
        req_.reset();
        req_.read_header(this);
        return true;
    }
    
    void close_callback() override {
        delete this;
    }

private:
    acl::aio_socket_stream* conn_;
    acl::http_request req_;
};

class aio_http_server : public acl::master_aio {
protected:
    bool on_accept(acl::aio_socket_stream* stream) override {
        http_client* client = new http_client(stream);
        client->start();
        return true;
    }
};

int main(int argc, char* argv[]) {
    aio_http_server server;
    server.run_daemon(argc, argv);
    return 0;
}
```

---

## master_fiber 示例

### 协程 Echo 服务器

```cpp
#include <acl_cpp/lib_acl.hpp>
#include <fiber/libfiber.hpp>
#include <fiber/master_fiber.hpp>

class fiber_echo_server : public acl::master_fiber {
public:
    fiber_echo_server() {}
    ~fiber_echo_server() {}

protected:
    // 进程初始化
    void proc_on_init() override {
        acl::log::open("fiber_echo.log", "fiber_echo");
        acl::logger("协程 Echo 服务器启动");
    }

    // 进程退出
    void proc_on_exit() override {
        acl::logger("协程 Echo 服务器退出");
    }

    // 线程初始化
    void thread_on_init() override {
        acl::logger("工作线程 %lu 启动", acl::thread::self());
    }

    // 线程退出
    void thread_on_exit() override {
        acl::logger("工作线程 %lu 退出", acl::thread::self());
    }

    // 处理客户端连接（在协程中运行）
    void on_accept(acl::socket_stream& stream) override {
        acl::logger("新连接: %s", stream.get_peer());
        
        // 增加连接计数
        users_count_add(1);
        
        char buf[8192];
        
        // 以同步方式编程，但实际是异步执行
        // 不会阻塞其他协程
        while (true) {
            int ret = stream.read(buf, sizeof(buf) - 1, false);
            if (ret <= 0) {
                break;
            }
            
            buf[ret] = '\0';
            acl::logger("收到数据: %s", buf);
            
            // 回显数据
            if (stream.write(buf, ret) <= 0) {
                break;
            }
        }
        
        acl::logger("连接关闭: %s", stream.get_peer());
        
        // 减少连接计数
        users_count_add(-1);
        
        // 函数返回后，框架会自动关闭连接
    }
};

int main(int argc, char* argv[]) {
    fiber_echo_server server;
    server.run_daemon(argc, argv);
    return 0;
}
```

### 协程 HTTP 服务器

```cpp
#include <acl_cpp/lib_acl.hpp>
#include <fiber/libfiber.hpp>
#include <fiber/master_fiber.hpp>

class fiber_http_server : public acl::master_fiber {
protected:
    void proc_on_init() override {
        acl::log::open("fiber_http.log", "fiber_http");
        acl::logger("协程 HTTP 服务器启动");
    }

    void on_accept(acl::socket_stream& stream) override {
        users_count_add(1);
        
        // HTTP Keep-Alive 支持
        while (true) {
            if (!handle_request(stream)) {
                break;
            }
        }
        
        users_count_add(-1);
    }

private:
    bool handle_request(acl::socket_stream& stream) {
        acl::http_request req(&stream);
        acl::http_response& res = req.get_response();
        
        // 读取 HTTP 请求头
        if (!req.read_header()) {
            return false;
        }
        
        // 获取请求信息
        acl::string method = req.request_method();
        acl::string path = req.request_url();
        
        acl::logger("HTTP 请求: %s %s from %s", 
                   method.c_str(), path.c_str(), stream.get_peer());
        
        // 路由处理
        acl::string body;
        int status = 200;
        
        if (path == "/") {
            body = "<html><body><h1>Welcome to Fiber HTTP Server!</h1></body></html>";
        } else if (path == "/api/info") {
            // 返回 JSON
            body.format("{\"server\":\"fiber_http\",\"connections\":%lld}", 
                       users_count());
            res.set_content_type("application/json");
        } else if (path.begin_with("/sleep/")) {
            // 演示协程睡眠（不会阻塞其他协程）
            int seconds = atoi(path.c_str() + 7);
            acl::fiber::delay(seconds * 1000);  // 毫秒
            body.format("<html><body><h1>Slept for %d seconds</h1></body></html>", 
                       seconds);
        } else {
            status = 404;
            body = "<html><body><h1>404 Not Found</h1></body></html>";
        }
        
        // 发送响应
        res.set_status(status);
        if (!res.get_content_type()) {
            res.set_content_type("text/html; charset=utf-8");
        }
        res.set_content_length(body.size());
        
        if (!res.write(body) || !res.write(NULL, 0)) {
            return false;
        }
        
        // 检查是否保持连接
        return req.keep_alive();
    }
};

int main(int argc, char* argv[]) {
    fiber_http_server server;
    server.run_daemon(argc, argv);
    return 0;
}
```

### 协程 Redis 代理服务器

```cpp
#include <acl_cpp/lib_acl.hpp>
#include <fiber/libfiber.hpp>
#include <fiber/master_fiber.hpp>

class redis_proxy : public acl::master_fiber {
public:
    redis_proxy() : redis_addr_("127.0.0.1:6379") {}

protected:
    void proc_on_init() override {
        acl::log::open("redis_proxy.log", "redis_proxy");
        acl::logger("Redis 代理服务器启动");
    }

    void on_accept(acl::socket_stream& client) override {
        users_count_add(1);
        
        // 连接到 Redis 服务器
        acl::socket_stream redis_conn;
        if (!redis_conn.open(redis_addr_, 10, 10)) {
            acl::logger_error("连接 Redis 失败: %s", redis_addr_.c_str());
            users_count_add(-1);
            return;
        }
        
        acl::logger("代理连接: %s -> %s", 
                   client.get_peer(), redis_addr_.c_str());
        
        // 创建两个协程进行双向数据转发
        acl::fiber* fb1 = acl::fiber::create([&client, &redis_conn]() {
            forward_data(client, redis_conn);
        });
        
        acl::fiber* fb2 = acl::fiber::create([&client, &redis_conn]() {
            forward_data(redis_conn, client);
        });
        
        // 等待两个协程结束
        fb1->join();
        fb2->join();
        
        acl::logger("代理连接关闭");
        users_count_add(-1);
    }

private:
    acl::string redis_addr_;
    
    static void forward_data(acl::socket_stream& from, acl::socket_stream& to) {
        char buf[8192];
        while (true) {
            int ret = from.read(buf, sizeof(buf), false);
            if (ret <= 0) {
                break;
            }
            
            if (to.write(buf, ret) <= 0) {
                break;
            }
        }
    }
};

int main(int argc, char* argv[]) {
    redis_proxy server;
    server.run_daemon(argc, argv);
    return 0;
}
```

### 协程 WebSocket 服务器

```cpp
#include <acl_cpp/lib_acl.hpp>
#include <fiber/libfiber.hpp>
#include <fiber/master_fiber.hpp>

class websocket_server : public acl::master_fiber {
protected:
    void on_accept(acl::socket_stream& stream) override {
        users_count_add(1);
        
        acl::websocket ws(stream);
        
        // WebSocket 握手
        if (!ws.handshake()) {
            acl::logger_error("WebSocket 握手失败");
            users_count_add(-1);
            return;
        }
        
        acl::logger("WebSocket 连接建立: %s", stream.get_peer());
        
        // 发送欢迎消息
        acl::string welcome;
        welcome.format("Welcome! Total users: %lld", users_count());
        ws.send_text(welcome);
        
        // 接收和处理消息
        char buf[8192];
        while (true) {
            bool ret = ws.read_frame_data(buf, sizeof(buf) - 1);
            if (!ret) {
                break;
            }
            
            unsigned long long len = ws.get_frame_payload_len();
            buf[len] = '\0';
            
            acl::logger("收到消息: %s", buf);
            
            // 广播给所有客户端（这里简化为回显）
            acl::string response;
            response.format("Echo: %s", buf);
            
            if (!ws.send_text(response)) {
                break;
            }
            
            // 演示定期发送心跳
            static time_t last_ping = time(NULL);
            time_t now = time(NULL);
            if (now - last_ping >= 30) {
                ws.send_ping();
                last_ping = now;
            }
        }
        
        acl::logger("WebSocket 连接关闭");
        users_count_add(-1);
    }
};

int main(int argc, char* argv[]) {
    websocket_server server;
    server.run_daemon(argc, argv);
    return 0;
}
```

### 协程并发 HTTP 客户端示例

```cpp
#include <acl_cpp/lib_acl.hpp>
#include <fiber/libfiber.hpp>
#include <fiber/master_fiber.hpp>

class http_client_server : public acl::master_fiber {
protected:
    void on_accept(acl::socket_stream& stream) override {
        // 接收客户端请求
        acl::http_request req(&stream);
        if (!req.read_header()) {
            return;
        }
        
        // 并发请求多个后端服务
        std::vector<acl::fiber*> fibers;
        std::vector<acl::string> results;
        results.resize(3);
        
        // 创建3个协程并发请求
        fibers.push_back(acl::fiber::create([&results]() {
            results[0] = http_get("http://api1.example.com/data");
        }));
        
        fibers.push_back(acl::fiber::create([&results]() {
            results[1] = http_get("http://api2.example.com/data");
        }));
        
        fibers.push_back(acl::fiber::create([&results]() {
            results[2] = http_get("http://api3.example.com/data");
        }));
        
        // 等待所有请求完成
        for (auto fb : fibers) {
            fb->join();
        }
        
        // 合并结果
        acl::string body;
        body.format("{\"api1\":%s,\"api2\":%s,\"api3\":%s}",
                   results[0].c_str(), results[1].c_str(), results[2].c_str());
        
        // 返回响应
        acl::http_response& res = req.get_response();
        res.set_status(200);
        res.set_content_type("application/json");
        res.set_content_length(body.size());
        res.write(body);
        res.write(NULL, 0);
    }

private:
    static acl::string http_get(const char* url) {
        acl::http_request req(url);
        acl::http_response* res = req.get_response();
        
        if (res == NULL) {
            return "\"error\"";
        }
        
        acl::string body;
        if (!res->get_body(body)) {
            return "\"error\"";
        }
        
        return body;
    }
};

int main(int argc, char* argv[]) {
    http_client_server server;
    server.run_daemon(argc, argv);
    return 0;
}
```

---

## master_trigger 示例

### 定时任务服务器

```cpp
#include <acl_cpp/lib_acl.hpp>

static int g_count = 0;

class trigger_server : public acl::master_trigger {
protected:
    void proc_on_init() override {
        acl::log::open("trigger.log", "trigger");
        acl::logger("定时任务服务器启动");
    }

    void proc_on_exit() override {
        acl::logger("定时任务服务器退出，共执行 %d 次", g_count);
    }

    // 定时触发
    void on_trigger() override {
        g_count++;
        acl::logger("定时任务触发 - 第 %d 次", g_count);
        
        // 执行定时任务
        do_cleanup();
        do_statistics();
        do_backup();
    }

private:
    void do_cleanup() {
        // 清理过期数据
        acl::logger("清理过期数据...");
    }
    
    void do_statistics() {
        // 统计分析
        acl::logger("统计分析...");
    }
    
    void do_backup() {
        // 数据备份
        acl::logger("数据备份...");
    }
};

int main(int argc, char* argv[]) {
    trigger_server server;
    server.run_daemon(argc, argv);
    return 0;
}
```

### 健康检查服务

```cpp
#include <acl_cpp/lib_acl.hpp>

class health_checker : public acl::master_trigger {
protected:
    void proc_on_init() override {
        acl::log::open("health.log", "health_checker");
        load_config();
    }

    void on_trigger() override {
        check_database();
        check_redis();
        check_disk_space();
        check_memory();
        
        send_report();
    }

private:
    void load_config() {
        // 加载要检查的服务列表
        acl::logger("加载配置...");
    }
    
    void check_database() {
        acl::logger("检查数据库连接...");
        // 尝试连接数据库
    }
    
    void check_redis() {
        acl::logger("检查 Redis 连接...");
        // 尝试连接 Redis
    }
    
    void check_disk_space() {
        acl::logger("检查磁盘空间...");
        // 检查磁盘使用率
    }
    
    void check_memory() {
        acl::logger("检查内存使用...");
        // 检查内存使用率
    }
    
    void send_report() {
        // 发送健康检查报告
        acl::logger("发送健康检查报告");
    }
};

int main(int argc, char* argv[]) {
    health_checker server;
    server.run_daemon(argc, argv);
    return 0;
}
```

---

## master_udp 示例

### UDP Echo 服务器

```cpp
#include <acl_cpp/lib_acl.hpp>

class udp_echo_server : public acl::master_udp {
protected:
    void proc_on_init() override {
        acl::log::open("udp_echo.log", "udp_echo");
        acl::logger("UDP Echo 服务器启动");
    }

    void proc_on_exit() override {
        acl::logger("UDP Echo 服务器退出");
    }

    void proc_on_bind(acl::socket_stream& stream) override {
        acl::logger("绑定 UDP 地址: %s", stream.get_local());
    }

    void thread_on_init() override {
        acl::logger("UDP 工作线程 %lu 启动", acl::thread::self());
    }

    // 处理 UDP 数据
    void on_read(acl::socket_stream* stream) override {
        char buf[8192];
        char peer_addr[64];
        
        // 接收数据报
        int ret = stream->recvfrom(buf, sizeof(buf) - 1, peer_addr, sizeof(peer_addr));
        if (ret <= 0) {
            acl::logger_error("接收失败: %s", acl::last_serror());
            return;
        }
        
        buf[ret] = '\0';
        acl::logger("收到来自 %s 的数据: %s", peer_addr, buf);
        
        // 回显数据
        if (stream->sendto(buf, ret, peer_addr) == -1) {
            acl::logger_error("发送失败: %s", acl::last_serror());
        }
    }
};

int main(int argc, char* argv[]) {
    udp_echo_server server;
    server.run_daemon(argc, argv);
    return 0;
}
```

### UDP Syslog 服务器

```cpp
#include <acl_cpp/lib_acl.hpp>

class syslog_server : public acl::master_udp {
protected:
    void proc_on_init() override {
        acl::log::open("syslog.log", "syslog_server");
        acl::logger("Syslog 服务器启动");
        
        // 打开日志文件
        log_file_.open("messages.log", O_WRONLY | O_CREAT | O_APPEND, 0644);
    }

    void on_read(acl::socket_stream* stream) override {
        char buf[8192];
        char peer_addr[64];
        
        int ret = stream->recvfrom(buf, sizeof(buf) - 1, peer_addr, sizeof(peer_addr));
        if (ret <= 0) {
            return;
        }
        
        buf[ret] = '\0';
        
        // 解析 syslog 消息
        parse_and_log(buf, ret, peer_addr);
    }

private:
    acl::ofstream log_file_;
    
    void parse_and_log(const char* data, int len, const char* peer) {
        // 获取当前时间
        time_t now = time(NULL);
        struct tm* tm_now = localtime(&now);
        char time_buf[64];
        strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_now);
        
        // 写入日志文件
        acl::string log_line;
        log_line.format("[%s] [%s] %s\n", time_buf, peer, data);
        
        lock();
        log_file_.write(log_line);
        log_file_.flush();
        unlock();
        
        acl::logger("记录日志: %s", data);
    }
};

int main(int argc, char* argv[]) {
    syslog_server server;
    server.run_daemon(argc, argv);
    return 0;
}
```

---

## 配置文件示例

### 线程池服务器配置 (echo.cf)

```bash
# ACL 服务器配置文件

# 服务类型: inet(TCP), udp, unix, trigger
service_type = inet

# 监听地址
# 多个地址用逗号或分号分隔
# 0.0.0.0:8080 表示监听所有接口的 8080 端口
service_addr = 0.0.0.0:8080, :::8080

# 进程数
# 对于线程池模型，通常设为 1
process_limit = 1

# 每个进程的最大线程数
# 通常设为 CPU 核心数的 2-4 倍
ioctl_max_threads = 250

# 每个连接最大处理次数
# 0 表示无限制（长连接）
ioctl_use_limit = 0

# I/O 超时时间（秒）
# 超过此时间无数据传输则关闭连接
io_timeout = 120

# 进程空闲退出时间（秒）
# master_threads 通常不设置
# ioctl_idle_limit = 120

# 日志文件路径
log_file = /var/log/echo_server.log

# 自定义配置参数
# 这些参数可以在代码中通过配置表读取
max_buffer_size = 8192
enable_compression = 1
```

### 多进程服务器配置 (proc.cf)

```bash
service_type = inet
service_addr = 0.0.0.0:9090

# 最小进程数
# acl_master 启动时预创建的进程数
process_limit = 10

# 最大进程数
# 超过此数量不再创建新进程
# process_max_limit = 50

# 每个进程最大处理连接数
# 处理完这么多连接后进程退出，master 会重新创建
ioctl_use_limit = 100

# 进程空闲超时（秒）
# 进程空闲超过此时间将退出
ioctl_idle_limit = 60

# 快速退出
# 非0时，退出信号到达时立即退出，不等待连接处理完成
ioctl_quick_abort = 1

io_timeout = 120
log_file = /var/log/proc_server.log
```

### 异步服务器配置 (aio.cf)

```bash
service_type = inet
service_addr = 0.0.0.0:7070

# 异步模型通常单进程
process_limit = 1

# 最大并发连接数
ioctl_max_threads = 10000

# 每个连接最大请求数
# 对于 HTTP 短连接，可以设为 0 表示无限制
ioctl_use_limit = 0

io_timeout = 30

# 事件引擎类型
# 自动选择: auto
# Linux: epoll
# FreeBSD/MacOS: kqueue
# Windows: iocp
# event_mode = epoll

log_file = /var/log/aio_server.log
```

### 协程服务器配置 (fiber.cf)

```bash
service_type = inet
service_addr = 0.0.0.0:8080

# 协程模型通常单进程
process_limit = 1

# 工作线程数
# 建议设置为 CPU 核心数
fiber_worker_threads = 4

# 协程栈大小（字节）
# 默认 128KB，可以根据需要调整
fiber_stack_size = 131072

# 最大并发连接数
# 协程模型可以支持很高的并发
ioctl_max_threads = 1000000

# 每个连接最大请求数（0 表示无限制）
ioctl_use_limit = 0

io_timeout = 30

log_file = /var/log/fiber_server.log

# 自定义配置
enable_websocket = 1
max_body_size = 10485760
```

### 定时触发服务器配置 (trigger.cf)

```bash
service_type = trigger

# 触发间隔（秒）
# 每隔这么多秒触发一次 on_trigger
trigger_interval = 60

# 进程数（通常为1）
process_limit = 1

log_file = /var/log/trigger_server.log

# 自定义配置
cleanup_days = 7
backup_path = /var/backup
```

### UDP 服务器配置 (udp.cf)

```bash
service_type = udp
service_addr = 0.0.0.0:514

# 进程数
process_limit = 1

# UDP 处理线程数
ioctl_max_threads = 10

io_timeout = 30

log_file = /var/log/udp_server.log
```

---

## 编译和运行

### 编译

#### 单文件编译

```bash
g++ -o echo_server echo_server.cpp \
    -I/usr/local/include \
    -L/usr/local/lib \
    -l_acl_cpp \
    -l_protocol \
    -l_acl \
    -lpthread \
    -ldl \
    -lz \
    -std=c++11
```

#### Makefile 示例

```makefile
CXX = g++
CXXFLAGS = -Wall -O2 -std=c++11
INCLUDES = -I/usr/local/include
LDFLAGS = -L/usr/local/lib
LIBS = -l_acl_cpp -l_protocol -l_acl -lpthread -ldl -lz

TARGET = echo_server
SOURCES = echo_server.cpp
OBJECTS = $(SOURCES:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: all clean
```

### 独立运行（调试模式）

```bash
# 线程池模型
./echo_server -a 127.0.0.1:8080 -c echo.cf

# 多进程模型
./proc_server -a 127.0.0.1:9090 -c proc.cf

# 异步模型
./aio_server -a 127.0.0.1:7070 -c aio.cf

# 定时触发（不需要监听地址）
./trigger_server -c trigger.cf

# UDP 服务器
./udp_server -a 127.0.0.1:514 -c udp.cf
```

### Daemon 模式运行

#### 1. 创建 acl_master 配置

在 `/opt/acl-master/conf/` 目录下创建服务配置文件 `echo.cf`:

```bash
# /opt/acl-master/conf/echo.cf
service echo
{
    service_type = inet
    service_addr = 0.0.0.0:8080
    master_class = threads
    
    process_limit = 1
    ioctl_max_threads = 250
    ioctl_use_limit = 0
    
    master_dispatch = sock
    master_command = /opt/servers/echo_server
    master_args = -c /opt/servers/echo_server.cf
    
    io_timeout = 120
}
```

#### 2. 启动 acl_master

```bash
# 启动 acl_master
/opt/soft/acl-master/bin/acl_master -c /opt/acl-master/conf

# 重新加载配置
/opt/soft/acl-master/bin/acl_master -r

# 停止服务
/opt/soft/acl-master/bin/acl_master -s stop

# 停止 acl_master
/opt/soft/acl-master/bin/acl_master -s quit
```

### 测试服务器

#### 使用 telnet 测试 Echo 服务器

```bash
telnet localhost 8080
# 输入任意文本，服务器会回显
```

#### 使用 nc 测试

```bash
echo "Hello Server" | nc localhost 8080
```

#### 使用 curl 测试 HTTP 服务器

```bash
curl http://localhost:7070/
```

#### 使用 dig 测试 UDP 服务器

```bash
# 发送 UDP 数据包
echo "test message" | nc -u localhost 514
```

---

## 高级示例

### 使用定时器

```cpp
class timer_example : public acl::master_threads {
protected:
    void proc_on_init() override {
        // 创建定时器，5 秒后触发
        acl::event_timer* timer = new my_timer;
        proc_set_timer(timer);
    }
    
    bool thread_on_read(acl::socket_stream* stream) override {
        // 处理逻辑
        return true;
    }
};

class my_timer : public acl::event_timer {
public:
    my_timer() : event_timer(5000) {} // 5000 毫秒
    
protected:
    void timer_callback(unsigned int) override {
        acl::logger("定时器触发");
        // 执行定时任务
    }
};
```

### 流量控制

```cpp
bool thread_on_accept(acl::socket_stream* stream) override {
    // 检查任务队列长度
    size_t qlen = task_qlen();
    
    if (qlen > 1000) {
        // 队列太长，拒绝新连接
        acl::logger_warn("任务队列过长 (%d)，拒绝连接", (int)qlen);
        return false;
    }
    
    return true;
}
```

### 连接池管理

```cpp
class pooled_server : public acl::master_threads {
protected:
    void proc_on_init() override {
        // 初始化 Redis 连接池
        redis_pool_ = new acl::redis_client_pool("127.0.0.1:6379", 100);
        redis_pool_->set_timeout(10, 10);
    }
    
    void proc_on_exit() override {
        delete redis_pool_;
    }
    
    bool thread_on_read(acl::socket_stream* stream) override {
        // 从连接池获取连接
        acl::redis_client* redis = (acl::redis_client*)redis_pool_->peek();
        if (redis == NULL) {
            acl::logger_error("获取 Redis 连接失败");
            return false;
        }
        
        // 使用 Redis
        acl::redis cmd(redis);
        acl::string value;
        if (cmd.get("key", value)) {
            // 处理结果
        }
        
        // 归还连接
        redis_pool_->put(redis, true);
        
        return true;
    }

private:
    acl::redis_client_pool* redis_pool_;
};
```

---

## 总结

本文档提供了 ACL Master 框架各种服务器模型的完整示例，包括：

- **master_threads**: 适用于中等并发场景
- **master_proc**: 适用于高稳定性要求场景
- **master_aio**: 适用于高并发场景（异步事件驱动，回调编程）
- **master_fiber**: 适用于高并发场景（协程，同步编程方式）
- **master_trigger**: 适用于定时任务场景
- **master_udp**: 适用于 UDP 协议场景

### 模型对比总结

| 特性 | threads | proc | aio | fiber | trigger | udp |
|------|---------|------|-----|-------|---------|-----|
| 编程模型 | 同步 | 同步 | 异步回调 | 同步 | 定时触发 | 同步 |
| 并发能力 | 中等 | 低 | 高 | 高 | N/A | 中等 |
| 资源占用 | 中 | 高 | 低 | 低 | 低 | 中 |
| 编程难度 | 简单 | 简单 | 较复杂 | 简单 | 简单 | 简单 |
| 连接类型 | 长短均可 | 长短均可 | 长短均可 | 长短均可 | N/A | UDP |
| 适用场景 | 中等并发 | 稳定性优先 | 高并发 | 高并发 | 定时任务 | UDP协议 |
| 主要优势 | 编程简单 | 进程隔离 | 事件驱动 | 同步写法 | 定时触发 | UDP支持 |

### master_aio vs master_fiber 选择建议

**master_aio** 和 **master_fiber** 都支持高并发（长连接和短连接均可），主要区别在于编程方式：

- **选择 master_aio**：
  - 熟悉事件驱动、异步回调编程模式
  - 需要精确控制事件处理流程
  - 团队有异步编程经验

- **选择 master_fiber**：
  - 偏好同步方式编写代码（逻辑更清晰）
  - 需要复杂的业务逻辑处理
  - 希望降低异步编程的心智负担
  - 需要在一个连接处理中创建多个子协程

**性能对比**：两者在高并发场景下性能相当，资源占用也相当。

根据实际需求和团队技术栈选择合适的模型，参考示例代码快速开发高性能网络服务。

更多示例代码可以参考 ACL 源码中的 `app/master` 和 `app/master_fiber` 目录。

