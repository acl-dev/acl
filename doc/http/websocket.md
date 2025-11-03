# WebSocket使用指南

## 概述

ACL提供了完整的WebSocket协议实现（RFC 6455），支持同步和异步两种模式，可用于构建实时通信应用。

## 核心类

### 1. websocket - WebSocket协议处理

`websocket`类负责WebSocket协议的底层实现。

## WebSocket帧类型

```cpp
enum {
    FRAME_CONTINUATION = 0x00,  // 延续帧
    FRAME_TEXT         = 0x01,  // 文本帧
    FRAME_BINARY       = 0x02,  // 二进制帧
    FRAME_CLOSE        = 0x08,  // 关闭帧
    FRAME_PING         = 0x09,  // PING帧
    FRAME_PONG         = 0x0A,  // PONG帧
};
```

## 服务端WebSocket

### 1. WebSocketServlet - 服务端基类

继承`WebSocketServlet`实现WebSocket服务。

```cpp
#include "acl_cpp/lib_acl.hpp"

class MyWebSocketServlet : public acl::WebSocketServlet {
public:
    MyWebSocketServlet(acl::socket_stream* stream, 
                       acl::session* session)
        : WebSocketServlet(stream, session) {}
    
    ~MyWebSocketServlet() {}

protected:
    // 接收到消息
    bool onMessage(unsigned long long len, bool text, bool finish) override {
        char buf[8192];
        int ret = readPayload(buf, sizeof(buf) - 1);
        
        if (ret > 0) {
            buf[ret] = '\0';
            printf("收到消息: %s (类型: %s)\n", 
                   buf, text ? "文本" : "二进制");
            
            // 回显消息
            if (text) {
                sendText(buf);
            } else {
                sendBinary(buf, ret);
            }
        }
        
        return ret >= 0;
    }
    
    // 接收到PING
    bool onPing(unsigned long long len, bool finish) override {
        printf("收到PING\n");
        // 自动回复PONG
        sendPong(NULL);
        return true;
    }
    
    // 接收到PONG
    bool onPong(unsigned long long len, bool finish) override {
        printf("收到PONG\n");
        return true;
    }
    
    // 连接关闭
    void onClose() override {
        printf("WebSocket连接已关闭\n");
    }
};

// 服务器主函数
int main() {
    acl::acl_cpp_init();
    
    acl::server_socket server;
    if (!server.open("0.0.0.0:9000")) {
        printf("打开端口失败\n");
        return 1;
    }
    
    printf("WebSocket服务器运行在 9000 端口\n");
    
    while (true) {
        acl::socket_stream* conn = server.accept();
        if (conn == NULL) {
            continue;
        }
        
        // 创建Session
        acl::memcache_session session("127.0.0.1:11211");
        
        // 创建WebSocketServlet
        MyWebSocketServlet servlet(conn, &session);
        
        // 运行
        servlet.doRun();
        
        delete conn;
    }
    
    return 0;
}
```

### 2. 发送消息

```cpp
class MyWebSocketServlet : public acl::WebSocketServlet {
protected:
    bool onMessage(unsigned long long len, bool text, bool finish) override {
        // 读取消息内容
        acl::string msg;
        while (true) {
            char buf[8192];
            int ret = readPayload(buf, sizeof(buf));
            if (ret <= 0) break;
            msg.append(buf, ret);
        }
        
        // 发送文本消息
        sendText("这是文本消息");
        
        // 发送二进制消息
        unsigned char data[] = {0x01, 0x02, 0x03};
        sendBinary((const char*)data, sizeof(data));
        
        // 发送PING
        sendPing("ping data");
        
        // 发送PONG
        sendPong("pong data");
        
        return true;
    }
};
```

### 3. 处理分片消息

WebSocket消息可能被分成多个帧发送。

```cpp
class MyWebSocketServlet : public acl::WebSocketServlet {
private:
    acl::string buffer_;  // 缓存分片数据

protected:
    bool onMessage(unsigned long long len, bool text, bool finish) override {
        // 读取当前分片
        char buf[8192];
        int ret = readPayload(buf, sizeof(buf));
        
        if (ret > 0) {
            buffer_.append(buf, ret);
        }
        
        // 如果是最后一个分片
        if (finish) {
            printf("完整消息: %s\n", buffer_.c_str());
            
            // 处理完整消息
            processMessage(buffer_);
            
            // 清空缓存
            buffer_.clear();
        }
        
        return ret >= 0;
    }
    
    void processMessage(const acl::string& msg) {
        // 处理完整的消息
    }
};
```

### 4. 主动关闭连接

```cpp
class MyWebSocketServlet : public acl::WebSocketServlet {
protected:
    bool onMessage(unsigned long long len, bool text, bool finish) override {
        char buf[8192];
        int ret = readPayload(buf, sizeof(buf));
        
        if (ret > 0) {
            buf[ret] = '\0';
            
            // 如果收到"quit"则关闭连接
            if (strcmp(buf, "quit") == 0) {
                printf("客户端请求关闭连接\n");
                return false;  // 返回false关闭连接
            }
        }
        
        return true;
    }
};
```

## 客户端WebSocket

### 1. 同步客户端

使用`websocket`类实现同步WebSocket客户端。

```cpp
#include "acl_cpp/lib_acl.hpp"

int main() {
    acl::acl_cpp_init();
    
    // 1. 建立TCP连接
    acl::socket_stream conn;
    if (!conn.open("echo.websocket.org", 80, 10, 10)) {
        printf("连接失败\n");
        return 1;
    }
    
    // 2. 发送WebSocket握手请求
    acl::http_request req(&conn);
    
    // 设置WebSocket握手头
    acl::http_header& hdr = req.request_header();
    hdr.set_url("/");
    hdr.set_host("echo.websocket.org");
    hdr.set_upgrade("websocket");
    hdr.add_entry("Connection", "Upgrade");
    hdr.set_ws_key("123456789xxx");  // WebSocket密钥
    hdr.set_ws_version(13);
    
    // 发送握手请求
    if (!req.request(NULL, 0)) {
        printf("握手失败\n");
        return 1;
    }
    
    // 3. 检查握手响应
    int status = req.http_status();
    if (status != 101) {
        printf("握手失败，状态码: %d\n", status);
        return 1;
    }
    
    printf("WebSocket连接已建立\n");
    
    // 4. 创建WebSocket对象
    acl::websocket ws(conn);
    
    // 5. 发送文本消息
    const char* msg = "Hello, WebSocket!";
    ws.set_frame_fin(true);
    ws.set_frame_opcode(FRAME_TEXT);
    ws.set_frame_payload_len(strlen(msg));
    ws.send_frame_data(msg, strlen(msg));
    
    // 6. 接收消息
    if (ws.read_frame_head()) {
        unsigned long long payload_len = ws.get_frame_payload_len();
        unsigned char opcode = ws.get_frame_opcode();
        
        printf("收到帧: opcode=%d, len=%llu\n", opcode, payload_len);
        
        // 读取数据
        char buf[8192];
        int ret = ws.read_frame_data(buf, sizeof(buf) - 1);
        if (ret > 0) {
            buf[ret] = '\0';
            printf("收到消息: %s\n", buf);
        }
    }
    
    // 7. 关闭连接
    conn.close();
    
    return 0;
}
```

### 2. 使用便捷方法

```cpp
int main() {
    acl::socket_stream conn;
    conn.open("echo.websocket.org", 80, 10, 10);
    
    // WebSocket握手
    acl::http_request req(&conn);
    acl::http_header& hdr = req.request_header();
    hdr.set_url("/");
    hdr.set_host("echo.websocket.org");
    hdr.set_upgrade("websocket");
    hdr.add_entry("Connection", "Upgrade");
    hdr.set_ws_key("123456789xxx");
    hdr.set_ws_version(13);
    
    if (!req.request(NULL, 0) || req.http_status() != 101) {
        printf("握手失败\n");
        return 1;
    }
    
    acl::websocket ws(conn);
    
    // 使用peek_frame_head()和peek_frame_data()简化读取
    while (true) {
        // 读取帧头
        if (!ws.peek_frame_head()) {
            if (ws.eof()) {
                printf("连接已关闭\n");
                break;
            }
            printf("读取帧头失败\n");
            break;
        }
        
        unsigned char opcode = ws.get_frame_opcode();
        
        if (opcode == FRAME_TEXT || opcode == FRAME_BINARY) {
            // 读取数据
            acl::string buf;
            int ret = ws.peek_frame_data(buf, 8192);
            
            if (ret == 0) {
                // 当前帧数据读取完毕
                printf("收到消息: %s\n", buf.c_str());
            } else if (ret < 0) {
                // 错误
                break;
            }
        } else if (opcode == FRAME_PING) {
            // 回复PONG
            ws.send_frame_pong(NULL, 0);
        } else if (opcode == FRAME_CLOSE) {
            printf("收到关闭帧\n");
            break;
        }
    }
    
    return 0;
}
```

### 3. 异步客户端

使用`http_aclient`实现异步WebSocket客户端。

```cpp
#include "acl_cpp/lib_acl.hpp"

class MyWebSocketClient : public acl::http_aclient {
public:
    MyWebSocketClient(acl::aio_handle& handle)
        : http_aclient(handle) {}
    
    ~MyWebSocketClient() {}
    
    void destroy() override {
        delete this;
    }

protected:
    // 连接成功
    bool on_connect() override {
        printf("连接成功，开始握手\n");
        
        // 设置WebSocket握手头
        request_header().set_url("/");
        request_header().set_host("echo.websocket.org");
        
        // 发起WebSocket握手
        ws_handshake("123456789xxx");
        
        return true;
    }
    
    // WebSocket握手成功
    bool on_ws_handshake() override {
        printf("WebSocket握手成功\n");
        
        // 发送消息
        const char* msg = "Hello from async client!";
        ws_send_text((char*)msg, strlen(msg));
        
        // 开始异步读取
        ws_read_wait(60);  // 60秒超时
        
        return true;
    }
    
    // WebSocket握手失败
    void on_ws_handshake_failed(int status) override {
        printf("握手失败，状态码: %d\n", status);
    }
    
    // 收到TEXT帧
    bool on_ws_frame_text() override {
        printf("收到TEXT帧\n");
        return true;
    }
    
    // 收到BINARY帧
    bool on_ws_frame_binary() override {
        printf("收到BINARY帧\n");
        return true;
    }
    
    // 收到帧数据
    bool on_ws_frame_data(char* data, size_t dlen) override {
        printf("收到数据: %.*s\n", (int)dlen, data);
        return true;
    }
    
    // 帧接收完成
    bool on_ws_frame_finish() override {
        printf("帧接收完成\n");
        
        // 继续读取下一帧
        ws_read_wait(60);
        
        return true;
    }
    
    // 收到PING
    void on_ws_frame_ping(acl::string& data) override {
        printf("收到PING: %s\n", data.c_str());
        // 自动回复PONG
    }
    
    // 收到PONG
    void on_ws_frame_pong(acl::string& data) override {
        printf("收到PONG: %s\n", data.c_str());
    }
    
    // 收到关闭帧
    void on_ws_frame_closed() override {
        printf("收到关闭帧\n");
    }
    
    // 连接断开
    void on_disconnect() override {
        printf("连接已断开\n");
    }
};

// 使用示例
int main() {
    acl::acl_cpp_init();
    
    acl::aio_handle handle;
    
    MyWebSocketClient* client = new MyWebSocketClient(handle);
    
    if (!client->open("echo.websocket.org:80", 10, 60)) {
        printf("连接失败\n");
        delete client;
        return 1;
    }
    
    // 事件循环
    while (true) {
        handle.check();
    }
    
    return 0;
}
```

## WebSocket高级特性

### 1. 心跳检测

```cpp
class MyWebSocketServlet : public acl::WebSocketServlet {
private:
    time_t last_ping_time_;

protected:
    bool onMessage(unsigned long long len, bool text, bool finish) override {
        // 处理消息
        // ...
        
        // 检查是否需要发送PING
        time_t now = time(NULL);
        if (now - last_ping_time_ > 30) {  // 30秒
            sendPing(NULL);
            last_ping_time_ = now;
        }
        
        return true;
    }
    
    bool onPong(unsigned long long len, bool finish) override {
        printf("收到PONG，连接正常\n");
        return true;
    }
};
```

### 2. 广播消息

```cpp
class ChatServer {
private:
    std::vector<acl::websocket*> clients_;
    std::mutex mutex_;

public:
    void addClient(acl::websocket* ws) {
        std::lock_guard<std::mutex> lock(mutex_);
        clients_.push_back(ws);
    }
    
    void removeClient(acl::websocket* ws) {
        std::lock_guard<std::mutex> lock(mutex_);
        clients_.erase(
            std::remove(clients_.begin(), clients_.end(), ws),
            clients_.end()
        );
    }
    
    void broadcast(const char* msg) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        for (auto ws : clients_) {
            ws->set_frame_fin(true);
            ws->set_frame_opcode(FRAME_TEXT);
            ws->set_frame_payload_len(strlen(msg));
            ws->send_frame_data(msg, strlen(msg));
        }
    }
};
```

### 3. 消息压缩

WebSocket支持permessage-deflate扩展进行消息压缩。

```cpp
// 在握手时启用压缩
acl::http_header& hdr = req.request_header();
hdr.add_entry("Sec-WebSocket-Extensions", 
              "permessage-deflate; client_max_window_bits");
```

### 4. 子协议

```cpp
// 客户端请求子协议
acl::http_header& hdr = req.request_header();
hdr.add_entry("Sec-WebSocket-Protocol", "chat, superchat");

// 服务端选择子协议
bool MyServlet::doWebSocket(acl::HttpServletRequest& req,
                            acl::HttpServletResponse& res) {
    // 在握手响应中指定子协议
    res.setHeader("Sec-WebSocket-Protocol", "chat");
    
    return HttpServlet::doWebSocket(req, res);
}
```

## SSL/TLS支持

```cpp
#include "acl_cpp/lib_acl.hpp"

int main() {
    acl::acl_cpp_init();
    
    // 创建SSL配置
    acl::mbedtls_conf ssl_conf(false);
    
    // 建立SSL连接
    acl::socket_stream conn;
    conn.open("echo.websocket.org", 443, 10, 10);
    
    // 设置SSL
    acl::mbedtls_io ssl_io(&ssl_conf, false);
    if (!ssl_io.handshake(conn)) {
        printf("SSL握手失败\n");
        return 1;
    }
    
    // 后续WebSocket通信会通过SSL加密
    // ...
    
    return 0;
}
```

## 最佳实践

1. **错误处理**: 完善的异常处理和连接状态检查
2. **心跳机制**: 定期发送PING/PONG保持连接活跃
3. **消息分片**: 大消息分片发送，避免阻塞
4. **缓冲管理**: 合理设置缓冲区大小
5. **资源清理**: 及时关闭不活跃的连接
6. **安全性**: 使用WSS（WebSocket Secure）加密通信
7. **负载均衡**: 使用多进程/多线程处理高并发

## 性能优化

1. 使用异步模式处理高并发
2. 启用消息压缩减少带宽
3. 合理设置帧大小
4. 使用内存池优化内存分配
5. 减少不必要的内存拷贝

## 常见问题

### Q: WebSocket握手失败?
A: 检查握手请求头是否正确，特别是Upgrade、Connection、Sec-WebSocket-Key等字段。

### Q: 如何实现房间/频道功能?
A: 维护客户端连接池，按房间ID分组管理。

### Q: 如何处理连接异常?
A: 实现心跳机制，定期检测连接状态。

### Q: 客户端如何重连?
A: 实现自动重连逻辑，使用指数退避策略。

## 示例代码

完整的WebSocket示例代码请参考ACL源码的`app/wizard_demo/httpd_websocket`目录。

