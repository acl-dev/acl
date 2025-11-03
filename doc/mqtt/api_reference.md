# MQTT API 参考

本文档提供 ACL MQTT 模块的完整 API 参考。

## 目录

- [mqtt_header](#mqtt_header)
- [mqtt_message](#mqtt_message)
- [mqtt_client](#mqtt_client)
- [mqtt_aclient](#mqtt_aclient)
- [消息类型](#消息类型)
- [枚举和常量](#枚举和常量)
- [工具函数](#工具函数)

## mqtt_header

MQTT 消息头类，用于构建和解析 MQTT 消息头部。

### 构造函数

```cpp
// 用于构建 MQTT 消息
explicit mqtt_header(mqtt_type_t type);

// 用于复制 MQTT 消息头
mqtt_header(const mqtt_header& header);
```

### 构建相关方法

```cpp
// 构建消息头数据
bool build_header(string& out);

// 设置消息类型
mqtt_header& set_type(mqtt_type_t type);

// 设置消息头标志位
mqtt_header& set_header_flags(char flags);

// 设置剩余长度（消息体长度）
mqtt_header& set_remaing_length(unsigned len);

// 设置 QoS 等级
mqtt_header& set_qos(mqtt_qos_t qos);

// 设置重发标志
mqtt_header& set_dup(bool yes);

// 设置保留标志
mqtt_header& set_remain(bool yes);
```

### 解析相关方法

```cpp
// 流式解析消息头数据
// 返回值：
//   > 0: 消息头已完成，返回值为剩余未消费数据的长度
//     0: 数据已消费完，调用 finished() 检查是否完成
//    -1: 解析错误
int update(const char* data, int dlen);

// 检查消息头是否解析完成
bool finished() const;

// 重置消息头状态以便重用
void reset();
```

### 获取信息方法

```cpp
// 获取消息类型
mqtt_type_t get_type() const;

// 获取消息头标志位
unsigned char get_header_flags() const;

// 获取消息体长度
unsigned get_remaining_length() const;

// 获取 QoS 等级
mqtt_qos_t get_qos() const;

// 检查是否为重发消息
bool is_dup() const;

// 检查保留标志
bool is_remain() const;
```

### 示例

```cpp
// 构建消息头
acl::mqtt_header header(acl::MQTT_PUBLISH);
header.set_qos(acl::MQTT_QOS1)
      .set_remaing_length(100);

acl::string buf;
header.build_header(buf);

// 解析消息头
acl::mqtt_header parsed_header(acl::MQTT_RESERVED_MIN);
int ret = parsed_header.update(buf.c_str(), buf.size());
if (parsed_header.finished()) {
    printf("消息类型: %d\n", parsed_header.get_type());
    printf("消息体长度: %u\n", parsed_header.get_remaining_length());
}
```

## mqtt_message

MQTT 消息基类，所有具体消息类型的父类。

### 构造函数

```cpp
// 用于构建消息
explicit mqtt_message(mqtt_type_t type);

// 用于解析消息
explicit mqtt_message(const mqtt_header& header);
```

### 纯虚函数（子类必须实现）

```cpp
// 序列化为字节流
virtual bool to_string(string& out) = 0;

// 从字节流反序列化
// 返回值：
//   > 0: 消息已完成，返回值为剩余未消费数据的长度
//     0: 数据已消费完，调用 finished() 检查是否完成
//    -1: 解析错误
virtual int update(const char* data, int dlen) = 0;

// 检查消息是否解析完成
virtual bool finished() const { return false; }
```

### 公共方法

```cpp
// 获取消息头（可变）
mqtt_header& get_header();

// 获取消息头（不可变）
const mqtt_header& get_header() const;

// 静态工厂方法：根据消息头创建对应的消息对象
static mqtt_message* create_message(const mqtt_header& header);
```

### 示例

```cpp
// 创建消息对象
acl::mqtt_header header(acl::MQTT_PUBLISH);
acl::mqtt_message* msg = acl::mqtt_message::create_message(header);

if (msg) {
    // 解析消息体
    msg->update(data, len);
    
    if (msg->finished()) {
        // 处理消息
        switch (msg->get_header().get_type()) {
        case acl::MQTT_PUBLISH:
            // 转换为具体类型
            acl::mqtt_publish* pub = (acl::mqtt_publish*)msg;
            // ...
            break;
        }
    }
    
    delete msg;
}
```

## mqtt_client

MQTT 同步客户端类，用于同步阻塞方式的 MQTT 通信。

### 构造函数

```cpp
// 通过地址构造（用于客户端）
explicit mqtt_client(const char* addr, int conn_timeout = 10, int rw_timeout = 10);

// 通过已有连接构造（用于服务器端）
explicit mqtt_client(acl::socket_stream& conn, int conn_timeout = 10);
```

**参数说明**：
- `addr`: 服务器地址，格式为 "ip|port" 或 "domain|port"
- `conn_timeout`: 连接超时时间（秒）
- `rw_timeout`: 读写超时时间（秒）
- `conn`: 已建立的 socket 连接

### 核心方法

```cpp
// 发送 MQTT 消息
bool send(mqtt_message& message);

// 接收 MQTT 消息（自动解析）
// max: 限制消息体最大长度，默认 128000 字节
mqtt_message* get_message(size_t max = 128000);

// 读取消息头
bool read_header(mqtt_header& header);

// 读取消息体
bool read_message(const mqtt_header& header, mqtt_message& body);

// 获取底层 socket 流
socket_stream* sock_stream() const;
```

### 继承方法

```cpp
// 从 connect_client 继承

// 打开连接
bool open();

// 关闭连接
void close();
```

### 示例

```cpp
// 创建客户端并连接
acl::mqtt_client client("127.0.0.1|1883", 10, 30);
if (!client.open()) {
    printf("连接失败\n");
    return;
}

// 发送消息
acl::mqtt_connect conn;
conn.set_cid("client_001");
client.send(conn);

// 接收消息
acl::mqtt_message* msg = client.get_message();
if (msg) {
    // 处理消息
    delete msg;
}

client.close();
```

## mqtt_aclient

MQTT 异步客户端类，用于异步事件驱动方式的 MQTT 通信。

### 构造函数

```cpp
explicit mqtt_aclient(aio_handle& handle, sslbase_conf* ssl_conf = NULL);
```

**参数说明**：
- `handle`: AIO 事件句柄
- `ssl_conf`: SSL 配置对象，NULL 表示不使用 SSL

### 核心方法

```cpp
// 连接服务器（异步）
bool open(const char* addr, int conn_timeout, int rw_timeout);

// 使用已有连接（用于服务器端）
bool open(aio_socket_stream* conn);

// 关闭连接（异步）
void close();

// 发送消息（异步）
bool send(mqtt_message& message);

// 获取连接对象
aio_socket_stream* get_conn() const;

// 获取 SSL 配置
sslbase_conf* get_ssl_conf() const;
```

### SSL 相关方法

```cpp
// 设置主机名（用于 SSL SNI）
mqtt_aclient& set_host(const char* host);

// 设置 SNI 前缀
mqtt_aclient& set_sni_prefix(const char* prefix);

// 设置 SNI 后缀
mqtt_aclient& set_sni_suffix(const char* suffix);
```

### 地址信息方法

```cpp
// 获取 DNS 服务器地址
bool get_ns_addr(string& out) const;

// 获取 MQTT 服务器地址
bool get_server_addr(string& out) const;
```

### 纯虚函数（必须实现）

```cpp
// 销毁对象
virtual void destroy() = 0;

// 连接成功回调
virtual bool on_open() = 0;

// 接收消息回调
virtual bool on_body(const mqtt_message& msg) = 0;
```

### 虚函数（可选实现）

```cpp
// DNS 解析失败
virtual void on_ns_failed() {}

// 连接超时
virtual void on_connect_timeout() {}

// 连接失败
virtual void on_connect_failed() {}

// 读超时
virtual bool on_read_timeout() { return false; }

// 连接断开
virtual void on_disconnect() {}

// 接收消息头
virtual bool on_header(const mqtt_header& header) { return true; }
```

### 示例

```cpp
class MyMqttClient : public acl::mqtt_aclient {
public:
    MyMqttClient(acl::aio_handle& handle) 
        : mqtt_aclient(handle, NULL) {}

protected:
    void destroy() override {
        delete this;
    }

    bool on_open() override {
        // 连接成功，发送 CONNECT
        acl::mqtt_connect conn;
        conn.set_cid("async_client");
        return send(conn);
    }

    bool on_body(const acl::mqtt_message& msg) override {
        // 处理接收到的消息
        return true;
    }
};

// 使用
acl::aio_handle handle(acl::ENGINE_KERNEL);
MyMqttClient* client = new MyMqttClient(handle);
client->open("127.0.0.1|1883", 10, 30);

while (true) {
    handle.check();
}
```

## 消息类型

### mqtt_connect

连接请求消息。

```cpp
class mqtt_connect : public mqtt_message {
public:
    mqtt_connect();
    mqtt_connect(const mqtt_header& header);
    
    // 设置方法
    void set_keep_alive(unsigned short keep_alive);
    void set_cid(const char* cid);
    void set_username(const char* name);
    void set_passwd(const char* passwd);
    void set_will_qos(mqtt_qos_t qos);
    void set_will_topic(const char* topic);
    void set_will_msg(const char* msg);
    void clean_session();
    
    // 获取方法
    unsigned short get_keep_alive() const;
    const char* get_cid() const;
    const char* get_username() const;
    const char* get_passwd() const;
    mqtt_qos_t get_will_qos() const;
    const char* get_will_topic() const;
    const char* get_will_msg() const;
    bool session_cleaned() const;
};
```

### mqtt_connack

连接确认消息。

```cpp
class mqtt_connack : public mqtt_message {
public:
    mqtt_connack();
    mqtt_connack(const mqtt_header& header);
    
    mqtt_connack& set_session(bool on);
    mqtt_connack& set_connack_code(unsigned char code);
    
    bool get_session() const;
    unsigned char get_connack_code() const;
};
```

### mqtt_publish

发布消息。

```cpp
class mqtt_publish : public mqtt_message {
public:
    mqtt_publish();
    mqtt_publish(const mqtt_header& header);
    
    mqtt_publish& set_topic(const char* topic);
    mqtt_publish& set_pkt_id(unsigned short id);
    mqtt_publish& set_payload(unsigned len, const char* data = NULL);
    
    const char* get_topic() const;
    unsigned short get_pkt_id() const;
    unsigned get_payload_len() const;
    const string& get_payload() const;
};
```

### mqtt_subscribe

订阅消息。

```cpp
class mqtt_subscribe : public mqtt_message {
public:
    mqtt_subscribe();
    mqtt_subscribe(const mqtt_header& header);
    
    mqtt_subscribe& set_pkt_id(unsigned short id);
    mqtt_subscribe& add_topic(const char* topic, mqtt_qos_t qos);
    
    unsigned short get_pkt_id() const;
    const std::vector<std::string>& get_topics() const;
    const std::vector<mqtt_qos_t>& get_qoses() const;
};
```

### mqtt_suback

订阅确认消息。

```cpp
class mqtt_suback : public mqtt_message {
public:
    mqtt_suback();
    mqtt_suback(const mqtt_header& header);
    
    mqtt_suback& set_pkt_id(unsigned short id);
    mqtt_suback& add_topic_qos(mqtt_qos_t qos);
    mqtt_suback& add_topic_qos(const std::vector<mqtt_qos_t>& qoses);
    
    unsigned short get_pkt_id() const;
    const std::vector<mqtt_qos_t>& get_qoses() const;
};
```

### mqtt_unsubscribe

取消订阅消息。

```cpp
class mqtt_unsubscribe : public mqtt_message {
public:
    mqtt_unsubscribe();
    mqtt_unsubscribe(const mqtt_header& header);
    
    mqtt_unsubscribe& set_pkt_id(unsigned short id);
    mqtt_unsubscribe& add_topic(const char* topic);
    
    unsigned short get_pkt_id() const;
    const std::vector<std::string>& get_topics() const;
};
```

### mqtt_ack

确认消息基类（用于 PUBACK、PUBREC、PUBREL、PUBCOMP、UNSUBACK）。

```cpp
class mqtt_ack : public mqtt_message {
public:
    mqtt_ack(mqtt_type_t type);
    mqtt_ack(const mqtt_header& header);
    
    void set_pkt_id(unsigned short id);
    unsigned short get_pkt_id() const;
};
```

### mqtt_puback, mqtt_pubrec, mqtt_pubrel, mqtt_pubcomp

这些类都继承自 `mqtt_ack`，提供相同的接口。

```cpp
class mqtt_puback : public mqtt_ack {
public:
    mqtt_puback();
    mqtt_puback(const mqtt_header& header);
};

class mqtt_pubrec : public mqtt_ack {
public:
    mqtt_pubrec();
    mqtt_pubrec(const mqtt_header& header);
};

class mqtt_pubrel : public mqtt_ack {
public:
    mqtt_pubrel();
    mqtt_pubrel(const mqtt_header& header);
};

class mqtt_pubcomp : public mqtt_ack {
public:
    mqtt_pubcomp();
    mqtt_pubcomp(const mqtt_header& header);
};

class mqtt_unsuback : public mqtt_ack {
public:
    mqtt_unsuback();
    mqtt_unsuback(const mqtt_header& header);
};
```

### mqtt_pingreq, mqtt_pingresp, mqtt_disconnect

简单消息类型，无额外方法。

```cpp
class mqtt_pingreq : public mqtt_message {
public:
    mqtt_pingreq();
    mqtt_pingreq(const mqtt_header& header);
};

class mqtt_pingresp : public mqtt_message {
public:
    mqtt_pingresp();
    mqtt_pingresp(const mqtt_header& header);
};

class mqtt_disconnect : public mqtt_message {
public:
    mqtt_disconnect();
    mqtt_disconnect(const mqtt_header& header);
};
```

## 枚举和常量

### mqtt_type_t - 消息类型

```cpp
typedef enum {
    MQTT_RESERVED_MIN   = 0,   // 保留
    MQTT_CONNECT        = 1,   // 客户端请求连接
    MQTT_CONNACK        = 2,   // 连接确认
    MQTT_PUBLISH        = 3,   // 发布消息
    MQTT_PUBACK         = 4,   // 发布确认（QoS 1）
    MQTT_PUBREC         = 5,   // 发布接收（QoS 2）
    MQTT_PUBREL         = 6,   // 发布释放（QoS 2）
    MQTT_PUBCOMP        = 7,   // 发布完成（QoS 2）
    MQTT_SUBSCRIBE      = 8,   // 订阅主题
    MQTT_SUBACK         = 9,   // 订阅确认
    MQTT_UNSUBSCRIBE    = 10,  // 取消订阅
    MQTT_UNSUBACK       = 11,  // 取消订阅确认
    MQTT_PINGREQ        = 12,  // 心跳请求
    MQTT_PINGRESP       = 13,  // 心跳响应
    MQTT_DISCONNECT     = 14,  // 断开连接
    MQTT_RESERVED_MAX   = 15,  // 保留
} mqtt_type_t;
```

### mqtt_qos_t - QoS 等级

```cpp
typedef enum {
    MQTT_QOS0 = 0x0,  // 最多一次交付
    MQTT_QOS1 = 0x1,  // 至少一次交付
    MQTT_QOS2 = 0x2,  // 恰好一次交付
} mqtt_qos_t;
```

### mqtt_option_t - 选项类型

```cpp
typedef enum {
    MQTT_NONE,   // 无
    MQTT_NEED,   // 必需
    MQTT_MAYBE,  // 可选
} mqtt_option_t;
```

### CONNACK 返回码

```cpp
enum {
    MQTT_CONNACK_OK       = 0x00,  // 连接成功
    MQTT_CONNACK_ERR_VER  = 0x01,  // 不可接受的协议版本
    MQTT_CONNACK_ERR_CID  = 0x02,  // 客户端标识符被拒绝
    MQTT_CONNACK_ERR_SVR  = 0x03,  // 服务器不可用
    MQTT_CONNACK_ERR_AUTH = 0x04,  // 用户名或密码错误
    MQTT_CONNACK_ERR_DENY = 0x05,  // 未授权
};
```

### CONNECT 状态（旧定义）

```cpp
typedef enum {
    CONNECT_ACCEPTED         = 0x00,  // 连接接受
    CONNECT_INVALID_VERSION  = 0x01,  // 无效版本
    CONNECT_INVALID_CID      = 0x02,  // 无效客户端 ID
    CONNECT_NOT_AVAIL        = 0x03,  // 不可用
    CONNECT_LOGIN_FAILED     = 0x04,  // 登录失败
    CONNECT_NO_AUTHORITY     = 0x05,  // 无权限
} mqtt_conn_status_t;
```

## 工具函数

### mqtt_type_desc

获取消息类型的描述字符串。

```cpp
const char* mqtt_type_desc(mqtt_type_t type);
```

**示例**：
```cpp
printf("消息类型: %s\n", acl::mqtt_type_desc(acl::MQTT_PUBLISH));
// 输出: 消息类型: PUBLISH
```

### mqtt_qos_desc

获取 QoS 等级的描述字符串。

```cpp
const char* mqtt_qos_desc(mqtt_qos_t qos);
```

**示例**：
```cpp
printf("QoS: %s\n", acl::mqtt_qos_desc(acl::MQTT_QOS1));
// 输出: QoS: QoS1
```

## 数据结构

### mqtt_constrain

MQTT 消息约束结构（内部使用）。

```cpp
struct mqtt_constrain {
    mqtt_type_t type;        // 消息类型
    unsigned char flags:4;   // 标志位
    mqtt_option_t id;        // 消息 ID 选项
    mqtt_option_t payload;   // 载荷选项
    const char* desc;        // 描述
};
```

## 命名空间

所有 MQTT 相关的类、函数和类型都在 `acl` 命名空间中。

```cpp
namespace acl {
    // MQTT 相关定义
}
```

## 头文件包含关系

```
mqtt_header.hpp          - 基础头文件
  ├── mqtt_message.hpp   - 消息基类
  │    ├── mqtt_connect.hpp
  │    ├── mqtt_connack.hpp
  │    ├── mqtt_publish.hpp
  │    ├── mqtt_subscribe.hpp
  │    ├── mqtt_suback.hpp
  │    ├── mqtt_unsubscribe.hpp
  │    ├── mqtt_pingreq.hpp
  │    ├── mqtt_pingresp.hpp
  │    ├── mqtt_disconnect.hpp
  │    └── mqtt_ack.hpp
  │         ├── mqtt_puback.hpp
  │         ├── mqtt_pubrec.hpp
  │         ├── mqtt_pubrel.hpp
  │         ├── mqtt_pubcomp.hpp
  │         └── mqtt_unsuback.hpp
  ├── mqtt_client.hpp    - 同步客户端
  └── mqtt_aclient.hpp   - 异步客户端
```

## 注意事项

### 内存管理

1. **消息对象**：`get_message()` 返回的对象需要调用者释放
2. **异步客户端**：必须动态分配，在 `destroy()` 中释放
3. **SSL 配置**：SSL 配置对象的生命周期由调用者管理

### 线程安全

1. **同步客户端**：不是线程安全的，需要外部同步
2. **异步客户端**：所有回调在同一线程中执行

### 消息 ID

1. QoS > 0 的消息必须设置有效的消息 ID（1-65535）
2. 消息 ID 为 0 表示无效
3. 同一时刻的未确认消息应使用不同的 ID

### 错误处理

1. 所有 I/O 操作都可能失败，需要检查返回值
2. 连接失败时应该调用 `destroy()` 清理资源
3. 异步客户端的错误通过回调通知

## 相关文档

- [MQTT 同步客户端](mqtt_client.md)
- [MQTT 异步客户端](mqtt_aclient.md)
- [MQTT 消息类型](mqtt_messages.md)
- [使用示例](examples.md)

