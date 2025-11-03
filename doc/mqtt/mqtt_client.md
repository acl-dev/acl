# mqtt_client - MQTT 同步客户端

## 概述

`mqtt_client` 是 ACL MQTT 模块提供的同步客户端类，用于以同步阻塞方式与 MQTT 服务器通信。该类继承自 `connect_client`，支持连接池管理。

## 类定义

```cpp
class ACL_CPP_API mqtt_client : public connect_client {
public:
    // 构造函数
    explicit mqtt_client(const char* addr, int conn_timeout = 10, int rw_timeout = 10);
    explicit mqtt_client(acl::socket_stream& conn, int conn_timeout = 10);
    
    ~mqtt_client();
    
    // 发送 MQTT 消息
    bool send(mqtt_message& message);
    
    // 接收 MQTT 消息
    mqtt_message* get_message(size_t max = 128000);
    
    // 读取消息头
    bool read_header(mqtt_header& header);
    
    // 读取消息体
    bool read_message(const mqtt_header& header, mqtt_message& body);
    
    // 获取底层 socket 流
    socket_stream* sock_stream() const;
};
```

## 构造函数

### 1. 通过地址构造

```cpp
mqtt_client(const char* addr, int conn_timeout = 10, int rw_timeout = 10);
```

**参数**：
- `addr` - MQTT 服务器地址，格式为 `ip|port` 或 `domain|port`，例如 `"127.0.0.1|1883"`
- `conn_timeout` - 连接超时时间（秒），默认 10 秒
- `rw_timeout` - 读写超时时间（秒），默认 10 秒

**示例**：
```cpp
// 连接到本地 MQTT 服务器
acl::mqtt_client client("127.0.0.1|1883", 10, 30);

// 连接到远程 MQTT 服务器
acl::mqtt_client client2("mqtt.example.com|1883", 5, 60);
```

### 2. 通过已有连接构造

```cpp
mqtt_client(acl::socket_stream& conn, int conn_timeout = 10);
```

**参数**：
- `conn` - 已建立的 socket 连接
- `conn_timeout` - 连接超时时间（秒）

**使用场景**：用于 MQTT 服务器端，接收客户端连接后创建 mqtt_client 对象。

**示例**：
```cpp
// 服务器端接收连接
acl::socket_stream* conn = server.accept();
if (conn) {
    acl::mqtt_client client(*conn, 10);
    // 处理客户端请求...
}
```

## 主要方法

### send() - 发送消息

```cpp
bool send(mqtt_message& message);
```

发送一条 MQTT 消息到对端。

**参数**：
- `message` - 要发送的 MQTT 消息对象（会调用对象的可变方法，因此不能声明为 const）

**返回值**：
- `true` - 发送成功
- `false` - 发送失败

**示例**：
```cpp
// 发送 CONNECT 消息
acl::mqtt_connect conn_msg;
conn_msg.set_cid("client123")
        .set_username("user")
        .set_passwd("password")
        .set_keep_alive(60);

if (!client.send(conn_msg)) {
    printf("发送 CONNECT 失败\n");
    return false;
}

// 发送 PUBLISH 消息
acl::mqtt_publish pub_msg;
pub_msg.set_topic("sensor/temperature")
       .set_pkt_id(1)
       .set_payload(strlen("25.5"), "25.5");
pub_msg.get_header().set_qos(acl::MQTT_QOS1);

if (!client.send(pub_msg)) {
    printf("发送 PUBLISH 失败\n");
    return false;
}
```

### get_message() - 接收消息

```cpp
mqtt_message* get_message(size_t max = 128000);
```

从 MQTT 连接读取数据并创建对应的消息对象。

**参数**：
- `max` - 限制消息体的最大长度（字节），默认 128000

**返回值**：
- 成功返回消息对象指针（需要调用者释放）
- 失败返回 `NULL`

**注意**：
- 返回的消息对象需要调用者负责释放内存
- 可以通过 `get_header().get_type()` 获取消息类型
- 需要将返回的基类指针转换为具体的消息类型

**示例**：
```cpp
acl::mqtt_message* msg = client.get_message();
if (msg == NULL) {
    printf("接收消息失败\n");
    return false;
}

// 根据消息类型处理
switch (msg->get_header().get_type()) {
case acl::MQTT_CONNACK: {
    acl::mqtt_connack* connack = (acl::mqtt_connack*) msg;
    if (connack->get_connack_code() == acl::MQTT_CONNACK_OK) {
        printf("连接成功\n");
    } else {
        printf("连接失败: %d\n", connack->get_connack_code());
    }
    break;
}
case acl::MQTT_PUBLISH: {
    acl::mqtt_publish* pub = (acl::mqtt_publish*) msg;
    printf("收到消息，主题: %s, 内容: %s\n", 
           pub->get_topic(), 
           pub->get_payload().c_str());
    break;
}
case acl::MQTT_SUBACK: {
    acl::mqtt_suback* suback = (acl::mqtt_suback*) msg;
    printf("订阅成功，消息ID: %d\n", suback->get_pkt_id());
    break;
}
default:
    printf("收到其他类型消息: %d\n", msg->get_header().get_type());
    break;
}

delete msg;  // 记得释放内存
```

### read_header() - 读取消息头

```cpp
bool read_header(mqtt_header& header);
```

从 MQTT 连接读取消息头信息。

**参数**：
- `header` - 用于存储消息头的对象

**返回值**：
- `true` - 读取成功
- `false` - 读取失败

**使用场景**：需要分步解析消息时使用。

### read_message() - 读取消息体

```cpp
bool read_message(const mqtt_header& header, mqtt_message& body);
```

根据消息头读取消息体。

**参数**：
- `header` - 已解析的消息头
- `body` - 用于存储消息体的对象

**返回值**：
- `true` - 读取成功
- `false` - 读取失败

**示例**：
```cpp
// 分步解析消息
acl::mqtt_header header(acl::MQTT_RESERVED_MIN);
if (!client.read_header(header)) {
    printf("读取消息头失败\n");
    return false;
}

acl::mqtt_message* body = acl::mqtt_message::create_message(header);
if (body == NULL) {
    printf("创建消息对象失败\n");
    return false;
}

if (!client.read_message(header, *body)) {
    printf("读取消息体失败\n");
    delete body;
    return false;
}

// 处理消息...
delete body;
```

### sock_stream() - 获取底层连接

```cpp
socket_stream* sock_stream() const;
```

获取当前 mqtt_client 对象使用的底层 socket_stream 连接。

**返回值**：
- 返回 socket_stream 指针
- 如果没有打开连接则返回 `NULL`

## 完整使用示例

### 示例 1：MQTT 客户端连接和发布

```cpp
#include "acl_cpp/lib_acl.hpp"
#include "acl_cpp/mqtt/mqtt_client.hpp"
#include "acl_cpp/mqtt/mqtt_connect.hpp"
#include "acl_cpp/mqtt/mqtt_connack.hpp"
#include "acl_cpp/mqtt/mqtt_publish.hpp"
#include "acl_cpp/mqtt/mqtt_puback.hpp"

int main() {
    acl::log::stdout_open(true);
    
    // 1. 创建客户端
    acl::mqtt_client client("127.0.0.1|1883", 10, 30);
    
    // 2. 打开连接
    if (!client.open()) {
        printf("连接服务器失败\n");
        return 1;
    }
    
    // 3. 发送 CONNECT 消息
    acl::mqtt_connect conn_msg;
    conn_msg.set_cid("test_client")
            .set_username("admin")
            .set_passwd("123456")
            .set_keep_alive(60);
    
    if (!client.send(conn_msg)) {
        printf("发送 CONNECT 失败\n");
        return 1;
    }
    
    // 4. 接收 CONNACK
    acl::mqtt_message* msg = client.get_message();
    if (msg == NULL || msg->get_header().get_type() != acl::MQTT_CONNACK) {
        printf("接收 CONNACK 失败\n");
        return 1;
    }
    
    acl::mqtt_connack* connack = (acl::mqtt_connack*) msg;
    if (connack->get_connack_code() != acl::MQTT_CONNACK_OK) {
        printf("连接被拒绝: %d\n", connack->get_connack_code());
        delete msg;
        return 1;
    }
    printf("连接成功\n");
    delete msg;
    
    // 5. 发布消息
    acl::mqtt_publish pub_msg;
    pub_msg.set_topic("test/topic")
           .set_pkt_id(1)
           .set_payload(11, "Hello MQTT!");
    pub_msg.get_header().set_qos(acl::MQTT_QOS1);
    
    if (!client.send(pub_msg)) {
        printf("发送 PUBLISH 失败\n");
        return 1;
    }
    
    // 6. 接收 PUBACK (QoS 1)
    msg = client.get_message();
    if (msg && msg->get_header().get_type() == acl::MQTT_PUBACK) {
        acl::mqtt_puback* puback = (acl::mqtt_puback*) msg;
        printf("发布成功，消息ID: %d\n", puback->get_pkt_id());
        delete msg;
    }
    
    return 0;
}
```

### 示例 2：MQTT 客户端订阅

```cpp
#include "acl_cpp/lib_acl.hpp"
#include "acl_cpp/mqtt/mqtt_client.hpp"
#include "acl_cpp/mqtt/mqtt_subscribe.hpp"
#include "acl_cpp/mqtt/mqtt_suback.hpp"

int main() {
    acl::mqtt_client client("127.0.0.1|1883", 10, 30);
    
    // ... 连接过程省略 ...
    
    // 订阅主题
    acl::mqtt_subscribe sub_msg;
    sub_msg.set_pkt_id(1)
           .add_topic("sensor/+", acl::MQTT_QOS1)
           .add_topic("control/#", acl::MQTT_QOS2);
    
    if (!client.send(sub_msg)) {
        printf("发送 SUBSCRIBE 失败\n");
        return 1;
    }
    
    // 接收 SUBACK
    acl::mqtt_message* msg = client.get_message();
    if (msg && msg->get_header().get_type() == acl::MQTT_SUBACK) {
        acl::mqtt_suback* suback = (acl::mqtt_suback*) msg;
        printf("订阅成功，消息ID: %d\n", suback->get_pkt_id());
        
        const std::vector<acl::mqtt_qos_t>& qoses = suback->get_qoses();
        for (size_t i = 0; i < qoses.size(); i++) {
            printf("主题 %lu 的 QoS: %d\n", i, qoses[i]);
        }
        delete msg;
    }
    
    // 接收发布的消息
    while (true) {
        msg = client.get_message();
        if (msg == NULL) {
            break;
        }
        
        if (msg->get_header().get_type() == acl::MQTT_PUBLISH) {
            acl::mqtt_publish* pub = (acl::mqtt_publish*) msg;
            printf("收到消息:\n");
            printf("  主题: %s\n", pub->get_topic());
            printf("  QoS: %d\n", pub->get_header().get_qos());
            printf("  内容: %s\n", pub->get_payload().c_str());
            
            // 如果是 QoS 1，需要发送 PUBACK
            if (pub->get_header().get_qos() == acl::MQTT_QOS1) {
                acl::mqtt_puback puback;
                puback.set_pkt_id(pub->get_pkt_id());
                client.send(puback);
            }
        }
        
        delete msg;
    }
    
    return 0;
}
```

### 示例 3：MQTT 服务器端

```cpp
#include "acl_cpp/lib_acl.hpp"
#include "acl_cpp/mqtt/mqtt_client.hpp"

void handle_client(acl::socket_stream* conn) {
    acl::mqtt_client client(*conn, 10);
    
    while (true) {
        acl::mqtt_message* msg = client.get_message();
        if (msg == NULL) {
            printf("客户端断开连接\n");
            break;
        }
        
        switch (msg->get_header().get_type()) {
        case acl::MQTT_CONNECT: {
            acl::mqtt_connect* conn_msg = (acl::mqtt_connect*) msg;
            printf("客户端连接: %s\n", conn_msg->get_cid());
            
            // 发送 CONNACK
            acl::mqtt_connack connack;
            connack.set_connack_code(acl::MQTT_CONNACK_OK);
            client.send(connack);
            break;
        }
        case acl::MQTT_PUBLISH: {
            acl::mqtt_publish* pub = (acl::mqtt_publish*) msg;
            printf("收到发布: 主题=%s, 内容=%s\n", 
                   pub->get_topic(), pub->get_payload().c_str());
            
            // 如果是 QoS 1，发送 PUBACK
            if (pub->get_header().get_qos() == acl::MQTT_QOS1) {
                acl::mqtt_puback puback;
                puback.set_pkt_id(pub->get_pkt_id());
                client.send(puback);
            }
            break;
        }
        case acl::MQTT_PINGREQ: {
            printf("收到心跳请求\n");
            acl::mqtt_pingresp resp;
            client.send(resp);
            break;
        }
        case acl::MQTT_DISCONNECT:
            printf("客户端主动断开\n");
            delete msg;
            return;
        default:
            break;
        }
        
        delete msg;
    }
}

int main() {
    acl::server_socket server;
    if (!server.open("127.0.0.1|1883")) {
        printf("监听失败\n");
        return 1;
    }
    
    printf("MQTT 服务器启动，监听 1883 端口\n");
    
    while (true) {
        acl::socket_stream* conn = server.accept();
        if (conn == NULL) {
            printf("accept 失败\n");
            break;
        }
        
        handle_client(conn);
        delete conn;
    }
    
    return 0;
}
```

## 注意事项

1. **线程安全**：`mqtt_client` 不是线程安全的，多线程环境需要外部同步
2. **内存管理**：`get_message()` 返回的对象需要调用者释放
3. **连接管理**：需要先调用 `open()` 打开连接，然后才能发送/接收消息
4. **超时设置**：合理设置连接超时和读写超时，避免长时间阻塞
5. **错误处理**：每次 I/O 操作都应该检查返回值
6. **消息 ID**：QoS > 0 的消息必须设置有效的消息 ID（1-65535）

## 相关类

- [mqtt_aclient](mqtt_aclient.md) - MQTT 异步客户端
- [mqtt_message](mqtt_messages.md) - MQTT 消息类型
- [mqtt_header](api_reference.md#mqtt_header) - MQTT 消息头

