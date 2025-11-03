# ACL MQTT 模块文档

## 概述

ACL C++ MQTT 模块是一个完整的 MQTT 协议实现，支持 MQTT 客户端和服务器开发。该模块提供了同步和异步两种通信模式，支持 MQTT 3.1.1 协议的所有消息类型。

## 主要特性

- **完整的 MQTT 协议支持**：实现了 MQTT 3.1.1 协议的所有消息类型
- **同步/异步模式**：提供 `mqtt_client`（同步）和 `mqtt_aclient`（异步）两种客户端
- **流式解析**：支持流式解析 MQTT 消息，适用于网络编程
- **QoS 支持**：支持 QoS 0、QoS 1、QoS 2 三种服务质量等级
- **SSL/TLS 支持**：异步客户端支持 SSL/TLS 加密通信
- **连接池支持**：同步客户端继承自 `connect_client`，支持连接池管理

## 目录结构

```
lib_acl_cpp/include/acl_cpp/mqtt/
├── mqtt_header.hpp          # MQTT 消息头
├── mqtt_message.hpp         # MQTT 消息基类
├── mqtt_client.hpp          # MQTT 同步客户端
├── mqtt_aclient.hpp         # MQTT 异步客户端
├── mqtt_connect.hpp         # CONNECT 消息
├── mqtt_connack.hpp         # CONNACK 消息
├── mqtt_publish.hpp         # PUBLISH 消息
├── mqtt_puback.hpp          # PUBACK 消息
├── mqtt_pubrec.hpp          # PUBREC 消息
├── mqtt_pubrel.hpp          # PUBREL 消息
├── mqtt_pubcomp.hpp         # PUBCOMP 消息
├── mqtt_subscribe.hpp       # SUBSCRIBE 消息
├── mqtt_suback.hpp          # SUBACK 消息
├── mqtt_unsubscribe.hpp     # UNSUBSCRIBE 消息
├── mqtt_unsuback.hpp        # UNSUBACK 消息
├── mqtt_pingreq.hpp         # PINGREQ 消息
├── mqtt_pingresp.hpp        # PINGRESP 消息
├── mqtt_disconnect.hpp      # DISCONNECT 消息
└── mqtt_ack.hpp             # ACK 消息基类
```

## 核心类

### 1. mqtt_header
MQTT 消息头类，用于构建和解析 MQTT 消息头。

**主要功能**：
- 设置/获取消息类型
- 设置/获取 QoS 等级
- 设置/获取消息体长度
- 流式解析消息头

### 2. mqtt_message
MQTT 消息基类，所有具体的 MQTT 消息类型都继承自此类。

**主要方法**：
- `to_string()` - 序列化为字节流
- `update()` - 从字节流反序列化
- `finished()` - 检查消息是否解析完成

### 3. mqtt_client
MQTT 同步客户端，用于同步方式的 MQTT 通信。

**主要功能**：
- 发送/接收 MQTT 消息
- 支持连接池
- 适用于传统的同步编程模型

### 4. mqtt_aclient
MQTT 异步客户端，用于异步方式的 MQTT 通信。

**主要功能**：
- 异步发送/接收 MQTT 消息
- 支持 SSL/TLS
- 基于事件驱动的编程模型
- 适用于高并发场景

## MQTT 消息类型

| 消息类型 | 类名 | 说明 |
|---------|------|------|
| CONNECT | `mqtt_connect` | 客户端连接请求 |
| CONNACK | `mqtt_connack` | 服务器连接确认 |
| PUBLISH | `mqtt_publish` | 发布消息 |
| PUBACK | `mqtt_puback` | 发布确认（QoS 1） |
| PUBREC | `mqtt_pubrec` | 发布接收（QoS 2） |
| PUBREL | `mqtt_pubrel` | 发布释放（QoS 2） |
| PUBCOMP | `mqtt_pubcomp` | 发布完成（QoS 2） |
| SUBSCRIBE | `mqtt_subscribe` | 订阅主题 |
| SUBACK | `mqtt_suback` | 订阅确认 |
| UNSUBSCRIBE | `mqtt_unsubscribe` | 取消订阅 |
| UNSUBACK | `mqtt_unsuback` | 取消订阅确认 |
| PINGREQ | `mqtt_pingreq` | 心跳请求 |
| PINGRESP | `mqtt_pingresp` | 心跳响应 |
| DISCONNECT | `mqtt_disconnect` | 断开连接 |

## QoS 级别

```cpp
typedef enum {
    MQTT_QOS0 = 0x0,  // 最多一次（不保证送达）
    MQTT_QOS1 = 0x1,  // 至少一次（确保送达，可能重复）
    MQTT_QOS2 = 0x2,  // 恰好一次（确保送达且不重复）
} mqtt_qos_t;
```

## 连接状态码

```cpp
enum {
    MQTT_CONNACK_OK       = 0x00,  // 连接成功
    MQTT_CONNACK_ERR_VER  = 0x01,  // 协议版本错误
    MQTT_CONNACK_ERR_CID  = 0x02,  // 客户端 ID 错误
    MQTT_CONNACK_ERR_SVR  = 0x03,  // 服务器不可用
    MQTT_CONNACK_ERR_AUTH = 0x04,  // 认证失败
    MQTT_CONNACK_ERR_DENY = 0x05,  // 未授权
};
```

## 快速开始

### 同步客户端示例

```cpp
#include "acl_cpp/mqtt/mqtt_client.hpp"
#include "acl_cpp/mqtt/mqtt_connect.hpp"
#include "acl_cpp/mqtt/mqtt_publish.hpp"

// 连接到 MQTT 服务器
acl::mqtt_client client("127.0.0.1|1883", 10, 30);
client.open();

// 发送 CONNECT 消息
acl::mqtt_connect conn;
conn.set_cid("client_id")
    .set_username("user")
    .set_passwd("pass")
    .set_keep_alive(60);

if (client.send(conn)) {
    // 接收 CONNACK
    acl::mqtt_message* msg = client.get_message();
    // 处理消息...
}
```

### 异步客户端示例

```cpp
#include "acl_cpp/mqtt/mqtt_aclient.hpp"

class my_mqtt_client : public acl::mqtt_aclient {
public:
    my_mqtt_client(acl::aio_handle& handle) 
        : mqtt_aclient(handle) {}

protected:
    void destroy() override {
        delete this;
    }

    bool on_open() override {
        // 连接成功
        return true;
    }

    bool on_body(const acl::mqtt_message& msg) override {
        // 处理接收到的消息
        return true;
    }
};
```

## 文档导航

- [MQTT 同步客户端](mqtt_client.md) - mqtt_client 详细使用说明
- [MQTT 异步客户端](mqtt_aclient.md) - mqtt_aclient 详细使用说明
- [MQTT 消息类型](mqtt_messages.md) - 所有消息类型的详细说明
- [使用示例](examples.md) - 完整的使用示例代码
- [API 参考](api_reference.md) - 完整的 API 参考文档

## 相关链接

- [MQTT 3.1.1 协议规范](http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html)
- [ACL 项目主页](https://github.com/acl-dev/acl)

## 注意事项

1. **线程安全**：同步客户端不是线程安全的，多线程环境下需要外部同步
2. **内存管理**：异步客户端对象应该动态分配，并在 `destroy()` 方法中释放
3. **消息 ID**：需要发送确认的消息（QoS > 0）必须设置有效的消息 ID（1-65535）
4. **Keep Alive**：建议设置合适的心跳间隔，防止连接超时

## 版本历史

- 支持 MQTT 3.1.1 协议
- 实现同步和异步两种客户端模式
- 支持 SSL/TLS 加密传输
- 支持流式解析，适应各种网络环境

