# mqtt_aclient - MQTT 异步客户端

## 概述

`mqtt_aclient` 是 ACL MQTT 模块提供的异步客户端类，基于事件驱动模型实现 MQTT 通信。该类继承自 `aio_open_callback`，适用于高并发场景，支持 SSL/TLS 加密传输。

## 类定义

```cpp
class ACL_CPP_API mqtt_aclient : public aio_open_callback {
public:
    // 构造函数
    explicit mqtt_aclient(aio_handle& handle, sslbase_conf* ssl_conf = NULL);
    
    // 销毁对象（纯虚函数，必须实现）
    virtual void destroy() = 0;
    
    // 连接服务器
    bool open(const char* addr, int conn_timeout, int rw_timeout);
    bool open(aio_socket_stream* conn);
    
    // 关闭连接
    void close();
    
    // 发送消息
    bool send(mqtt_message& message);
    
    // 获取连接对象
    aio_socket_stream* get_conn() const;
    
    // SSL 相关设置
    sslbase_conf* get_ssl_conf() const;
    mqtt_aclient& set_host(const char* host);
    mqtt_aclient& set_sni_prefix(const char* prefix);
    mqtt_aclient& set_sni_suffix(const char* suffix);
    
    // 获取地址信息
    bool get_ns_addr(string& out) const;
    bool get_server_addr(string& out) const;

protected:
    virtual ~mqtt_aclient();
    
    // 回调方法（需要子类实现）
    virtual bool on_open() = 0;
    virtual bool on_body(const mqtt_message&) = 0;
    
    // 可选的回调方法
    virtual void on_ns_failed() {}
    virtual void on_connect_timeout() {}
    virtual void on_connect_failed() {}
    virtual bool on_read_timeout() { return false; }
    virtual void on_disconnect() {}
    virtual bool on_header(const mqtt_header&) { return true; }
};
```

## 构造函数

```cpp
mqtt_aclient(aio_handle& handle, sslbase_conf* ssl_conf = NULL);
```

**参数**：
- `handle` - AIO 事件句柄，用于事件循环
- `ssl_conf` - SSL 配置对象指针，如果为 NULL 则不使用 SSL

**示例**：
```cpp
// 创建 AIO 事件句柄
acl::aio_handle handle(acl::ENGINE_KERNEL);

// 创建不使用 SSL 的异步客户端
class my_mqtt_client : public acl::mqtt_aclient {
public:
    my_mqtt_client(acl::aio_handle& handle) 
        : mqtt_aclient(handle, NULL) {}
    // ...
};

my_mqtt_client* client = new my_mqtt_client(handle);

// 创建使用 SSL 的异步客户端
acl::sslbase_conf* ssl_conf = new acl::openssl_conf(true);
my_mqtt_client* ssl_client = new my_mqtt_client(handle);
// ...
```

## 核心方法

### open() - 连接服务器

```cpp
bool open(const char* addr, int conn_timeout, int rw_timeout);
```

异步连接到 MQTT 服务器。

**参数**：
- `addr` - MQTT 服务器地址，格式为 `ip|port` 或 `domain|port`
- `conn_timeout` - 连接超时时间（秒）
- `rw_timeout` - 读写超时时间（秒）

**返回值**：
- `true` - 连接操作已启动（实际连接结果通过 `on_open()` 回调通知）
- `false` - 启动连接失败，应该调用 `destroy()` 销毁对象

**示例**：
```cpp
if (!client->open("127.0.0.1|1883", 10, 30)) {
    printf("启动连接失败\n");
    client->destroy();
    return;
}
// 连接结果会通过 on_open() 回调通知
```

### open() - 使用已有连接

```cpp
bool open(aio_socket_stream* conn);
```

使用已建立的异步连接（用于服务器端）。

**参数**：
- `conn` - 已建立的异步 socket 连接

**返回值**：
- `true` - 成功
- `false` - 失败，应该调用 `destroy()` 销毁对象

### send() - 发送消息

```cpp
bool send(mqtt_message& message);
```

异步发送 MQTT 消息。

**参数**：
- `message` - 要发送的 MQTT 消息对象

**返回值**：
- `true` - 发送成功
- `false` - 发送失败

**示例**：
```cpp
// 发送 CONNECT 消息
acl::mqtt_connect conn_msg;
conn_msg.set_cid("async_client_001")
        .set_username("user")
        .set_passwd("pass")
        .set_keep_alive(60);

if (!send(conn_msg)) {
    printf("发送失败\n");
}
```

### close() - 关闭连接

```cpp
void close();
```

异步关闭与 MQTT 服务器的连接。关闭完成后会调用 `on_disconnect()` 回调。

### destroy() - 销毁对象

```cpp
virtual void destroy() = 0;
```

纯虚函数，必须由子类实现。当对象不再需要时调用此方法销毁对象。

**典型实现**：
```cpp
void destroy() override {
    delete this;
}
```

## 回调方法

### on_open() - 连接成功回调

```cpp
virtual bool on_open() = 0;
```

当连接成功建立时被调用（无论是客户端连接还是服务器端接受连接）。

**返回值**：
- `true` - 继续处理
- `false` - 关闭连接，应该调用 `destroy()` 销毁对象

**示例**：
```cpp
bool on_open() override {
    printf("连接成功\n");
    
    // 发送 CONNECT 消息
    acl::mqtt_connect conn_msg;
    conn_msg.set_cid("client_001")
            .set_keep_alive(60);
    
    if (!send(conn_msg)) {
        printf("发送 CONNECT 失败\n");
        return false;
    }
    
    return true;
}
```

### on_body() - 接收消息回调

```cpp
virtual bool on_body(const mqtt_message& msg) = 0;
```

当接收到完整的 MQTT 消息时被调用。

**参数**：
- `msg` - 接收到的 MQTT 消息

**返回值**：
- `true` - 继续处理
- `false` - 关闭连接

**示例**：
```cpp
bool on_body(const acl::mqtt_message& msg) override {
    switch (msg.get_header().get_type()) {
    case acl::MQTT_CONNACK: {
        const acl::mqtt_connack& connack = (const acl::mqtt_connack&) msg;
        if (connack.get_connack_code() == acl::MQTT_CONNACK_OK) {
            printf("连接确认成功\n");
            // 订阅主题
            subscribe_topics();
        } else {
            printf("连接被拒绝: %d\n", connack.get_connack_code());
            return false;
        }
        break;
    }
    case acl::MQTT_PUBLISH: {
        const acl::mqtt_publish& pub = (const acl::mqtt_publish&) msg;
        printf("收到消息: 主题=%s, 内容=%s\n", 
               pub.get_topic(), 
               pub.get_payload().c_str());
        
        // 如果是 QoS 1，发送 PUBACK
        if (pub.get_header().get_qos() == acl::MQTT_QOS1) {
            acl::mqtt_puback puback;
            puback.set_pkt_id(pub.get_pkt_id());
            send(puback);
        }
        break;
    }
    case acl::MQTT_SUBACK: {
        const acl::mqtt_suback& suback = (const acl::mqtt_suback&) msg;
        printf("订阅成功，消息ID: %d\n", suback.get_pkt_id());
        break;
    }
    case acl::MQTT_PINGRESP:
        printf("收到心跳响应\n");
        break;
    default:
        printf("收到消息类型: %d\n", msg.get_header().get_type());
        break;
    }
    
    return true;
}
```

### on_header() - 接收消息头回调

```cpp
virtual bool on_header(const mqtt_header& header) { return true; }
```

当接收到 MQTT 消息头时被调用（可选实现）。

**参数**：
- `header` - 接收到的消息头

**返回值**：
- `true` - 继续接收消息体
- `false` - 关闭连接

### on_disconnect() - 断开连接回调

```cpp
virtual void on_disconnect() {}
```

当连接断开时被调用（可选实现）。

**示例**：
```cpp
void on_disconnect() override {
    printf("连接已断开\n");
    // 可以在这里实现重连逻辑
}
```

### on_ns_failed() - DNS 解析失败回调

```cpp
virtual void on_ns_failed() {}
```

当 DNS 解析失败时被调用（可选实现）。

### on_connect_timeout() - 连接超时回调

```cpp
virtual void on_connect_timeout() {}
```

当连接超时时被调用（可选实现）。

### on_connect_failed() - 连接失败回调

```cpp
virtual void on_connect_failed() {}
```

当连接失败时被调用（可选实现）。

### on_read_timeout() - 读超时回调

```cpp
virtual bool on_read_timeout() { return false; }
```

当读操作超时时被调用（可选实现）。

**返回值**：
- `true` - 继续等待
- `false` - 关闭连接（默认）

## SSL/TLS 支持

### set_host() - 设置主机名

```cpp
mqtt_aclient& set_host(const char* host);
```

设置远程主机名，用于 SSL SNI（Server Name Indication）扩展。

**参数**：
- `host` - 主机名

**返回值**：
- 返回自身引用，支持链式调用

### set_sni_prefix() - 设置 SNI 前缀

```cpp
mqtt_aclient& set_sni_prefix(const char* prefix);
```

### set_sni_suffix() - 设置 SNI 后缀

```cpp
mqtt_aclient& set_sni_suffix(const char* suffix);
```

## 完整使用示例

### 示例 1：异步 MQTT 客户端

```cpp
#include "acl_cpp/lib_acl.hpp"
#include "acl_cpp/mqtt/mqtt_aclient.hpp"
#include "acl_cpp/mqtt/mqtt_connect.hpp"
#include "acl_cpp/mqtt/mqtt_connack.hpp"
#include "acl_cpp/mqtt/mqtt_subscribe.hpp"
#include "acl_cpp/mqtt/mqtt_publish.hpp"

class my_mqtt_client : public acl::mqtt_aclient {
public:
    my_mqtt_client(acl::aio_handle& handle, const char* addr)
        : mqtt_aclient(handle, NULL)
        , addr_(addr)
        , pkt_id_(1) {
    }

protected:
    void destroy() override {
        delete this;
    }

    // 连接成功
    bool on_open() override {
        printf("连接成功，发送 CONNECT 消息\n");
        
        acl::mqtt_connect conn_msg;
        conn_msg.set_cid("async_client_001")
                .set_username("admin")
                .set_passwd("123456")
                .set_keep_alive(60);
        
        if (!send(conn_msg)) {
            printf("发送 CONNECT 失败\n");
            return false;
        }
        
        return true;
    }

    // 接收消息
    bool on_body(const acl::mqtt_message& msg) override {
        switch (msg.get_header().get_type()) {
        case acl::MQTT_CONNACK:
            return handle_connack((const acl::mqtt_connack&) msg);
        case acl::MQTT_SUBACK:
            return handle_suback((const acl::mqtt_suback&) msg);
        case acl::MQTT_PUBLISH:
            return handle_publish((const acl::mqtt_publish&) msg);
        case acl::MQTT_PINGRESP:
            printf("收到心跳响应\n");
            break;
        default:
            printf("收到消息类型: %d\n", msg.get_header().get_type());
            break;
        }
        return true;
    }

    // 连接断开
    void on_disconnect() override {
        printf("连接断开\n");
    }

    // 连接超时
    void on_connect_timeout() override {
        printf("连接超时\n");
    }

    // 连接失败
    void on_connect_failed() override {
        printf("连接失败\n");
    }

private:
    bool handle_connack(const acl::mqtt_connack& connack) {
        if (connack.get_connack_code() != acl::MQTT_CONNACK_OK) {
            printf("连接被拒绝: %d\n", connack.get_connack_code());
            return false;
        }
        
        printf("连接确认成功，开始订阅主题\n");
        
        // 订阅主题
        acl::mqtt_subscribe sub_msg;
        sub_msg.set_pkt_id(pkt_id_++)
               .add_topic("sensor/temperature", acl::MQTT_QOS1)
               .add_topic("sensor/humidity", acl::MQTT_QOS1);
        
        if (!send(sub_msg)) {
            printf("发送 SUBSCRIBE 失败\n");
            return false;
        }
        
        return true;
    }

    bool handle_suback(const acl::mqtt_suback& suback) {
        printf("订阅成功，消息ID: %d\n", suback.get_pkt_id());
        
        const std::vector<acl::mqtt_qos_t>& qoses = suback.get_qoses();
        for (size_t i = 0; i < qoses.size(); i++) {
            printf("  主题 %lu QoS: %d\n", i, qoses[i]);
        }
        
        return true;
    }

    bool handle_publish(const acl::mqtt_publish& pub) {
        printf("收到发布消息:\n");
        printf("  主题: %s\n", pub.get_topic());
        printf("  QoS: %d\n", pub.get_header().get_qos());
        printf("  内容: %.*s\n", 
               (int)pub.get_payload_len(), 
               pub.get_payload().c_str());
        
        // 如果是 QoS 1，发送 PUBACK
        if (pub.get_header().get_qos() == acl::MQTT_QOS1) {
            acl::mqtt_puback puback;
            puback.set_pkt_id(pub.get_pkt_id());
            send(puback);
        }
        
        return true;
    }

private:
    std::string addr_;
    unsigned short pkt_id_;
};

int main() {
    acl::log::stdout_open(true);
    
    // 创建 AIO 事件句柄
    acl::aio_handle handle(acl::ENGINE_KERNEL);
    
    // 创建异步客户端
    my_mqtt_client* client = new my_mqtt_client(handle, "127.0.0.1|1883");
    
    // 连接服务器
    if (!client->open("127.0.0.1|1883", 10, 30)) {
        printf("启动连接失败\n");
        client->destroy();
        return 1;
    }
    
    // 进入事件循环
    while (true) {
        handle.check();
    }
    
    return 0;
}
```

### 示例 2：异步 MQTT 发布客户端

```cpp
#include "acl_cpp/lib_acl.hpp"
#include "acl_cpp/mqtt/mqtt_aclient.hpp"

class mqtt_publisher : public acl::mqtt_aclient {
public:
    mqtt_publisher(acl::aio_handle& handle)
        : mqtt_aclient(handle, NULL)
        , pkt_id_(1)
        , connected_(false) {
    }

    // 发布消息（外部调用）
    void publish(const char* topic, const char* payload) {
        if (!connected_) {
            printf("尚未连接\n");
            return;
        }
        
        acl::mqtt_publish pub_msg;
        pub_msg.set_topic(topic)
               .set_pkt_id(pkt_id_++)
               .set_payload(strlen(payload), payload);
        pub_msg.get_header().set_qos(acl::MQTT_QOS1);
        
        if (!send(pub_msg)) {
            printf("发送失败\n");
        } else {
            printf("发布消息: %s -> %s\n", topic, payload);
        }
    }

protected:
    void destroy() override {
        delete this;
    }

    bool on_open() override {
        printf("连接建立\n");
        
        acl::mqtt_connect conn_msg;
        conn_msg.set_cid("publisher_001")
                .set_keep_alive(60);
        
        return send(conn_msg);
    }

    bool on_body(const acl::mqtt_message& msg) override {
        switch (msg.get_header().get_type()) {
        case acl::MQTT_CONNACK: {
            const acl::mqtt_connack& connack = (const acl::mqtt_connack&) msg;
            if (connack.get_connack_code() == acl::MQTT_CONNACK_OK) {
                printf("连接成功，可以开始发布\n");
                connected_ = true;
            }
            break;
        }
        case acl::MQTT_PUBACK: {
            const acl::mqtt_puback& puback = (const acl::mqtt_puback&) msg;
            printf("发布确认，消息ID: %d\n", puback.get_pkt_id());
            break;
        }
        default:
            break;
        }
        return true;
    }

    void on_disconnect() override {
        connected_ = false;
        printf("连接断开\n");
    }

private:
    unsigned short pkt_id_;
    bool connected_;
};

int main() {
    acl::aio_handle handle(acl::ENGINE_KERNEL);
    
    mqtt_publisher* publisher = new mqtt_publisher(handle);
    
    if (!publisher->open("127.0.0.1|1883", 10, 30)) {
        printf("连接失败\n");
        publisher->destroy();
        return 1;
    }
    
    // 模拟定时发布消息
    int count = 0;
    time_t last_publish = time(NULL);
    
    while (true) {
        handle.check();
        
        // 每 5 秒发布一次
        time_t now = time(NULL);
        if (now - last_publish >= 5) {
            char payload[128];
            snprintf(payload, sizeof(payload), "message_%d", count++);
            publisher->publish("test/topic", payload);
            last_publish = now;
        }
        
        usleep(10000);  // 10ms
    }
    
    return 0;
}
```

### 示例 3：SSL/TLS 加密连接

```cpp
#include "acl_cpp/lib_acl.hpp"
#include "acl_cpp/mqtt/mqtt_aclient.hpp"

class secure_mqtt_client : public acl::mqtt_aclient {
public:
    secure_mqtt_client(acl::aio_handle& handle, acl::sslbase_conf* ssl_conf)
        : mqtt_aclient(handle, ssl_conf) {
    }

protected:
    void destroy() override {
        delete this;
    }

    bool on_open() override {
        printf("SSL 连接建立\n");
        
        acl::mqtt_connect conn_msg;
        conn_msg.set_cid("ssl_client_001")
                .set_keep_alive(60);
        
        return send(conn_msg);
    }

    bool on_body(const acl::mqtt_message& msg) override {
        // 处理消息...
        return true;
    }
};

int main() {
    acl::aio_handle handle(acl::ENGINE_KERNEL);
    
    // 创建 SSL 配置
    acl::openssl_conf* ssl_conf = new acl::openssl_conf(true);
    // 可选：设置证书
    // ssl_conf->set_ca_path("/path/to/ca.crt");
    
    secure_mqtt_client* client = new secure_mqtt_client(handle, ssl_conf);
    
    // 设置 SNI 主机名
    client->set_host("mqtt.example.com");
    
    // 连接服务器（使用 8883 SSL 端口）
    if (!client->open("mqtt.example.com|8883", 10, 30)) {
        printf("连接失败\n");
        client->destroy();
        delete ssl_conf;
        return 1;
    }
    
    while (true) {
        handle.check();
    }
    
    delete ssl_conf;
    return 0;
}
```

## 注意事项

1. **对象生命周期**：异步客户端对象必须动态分配（使用 new），并在 `destroy()` 方法中释放
2. **线程模型**：所有回调都在 AIO 事件循环线程中执行，不要在回调中执行阻塞操作
3. **事件循环**：必须定期调用 `aio_handle::check()` 驱动事件循环
4. **错误处理**：连接失败时会调用相应的错误回调，需要在回调中调用 `destroy()` 清理资源
5. **SSL 配置**：SSL 配置对象的生命周期需要由调用者管理
6. **消息 ID**：发送需要确认的消息时必须设置唯一的消息 ID

## 相关类

- [mqtt_client](mqtt_client.md) - MQTT 同步客户端
- [mqtt_message](mqtt_messages.md) - MQTT 消息类型
- [aio_handle](../stream/aio_handle.md) - 异步事件句柄

