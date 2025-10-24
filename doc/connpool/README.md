# ACL 连接池设计文档

## 目录

- [概述](#概述)
- [整体架构](architecture.md)
- [使用示例](examples.md)
- [API 参考](api.md)
- [最佳实践](best-practices.md)

## 概述

ACL 连接池 (connpool) 是一个通用的、高性能的网络连接池管理框架，提供了完整的连接池管理功能，包括连接复用、健康检查、故障恢复、负载均衡等特性。

### 主要特性

- **通用性强**：基于抽象基类设计，支持任意类型的网络连接
- **高性能**：连接复用、线程绑定、批量检查等优化手段
- **高可用**：自动故障检测与恢复、连接健康检查、引用计数管理
- **易扩展**：清晰的继承体系，已实现 TCP、Redis、MySQL、Memcache 等多种连接池
- **线程安全**：完善的锁机制保护，支持多线程和协程环境
- **自动化管理**：后台监控线程自动维护连接池健康状态

### 核心组件

```
connect_client      连接客户端基类（封装单个连接）
      ↑
      ├── tcp_client       TCP 连接实现
      ├── redis_client     Redis 连接实现
      └── mysql_client     MySQL 连接实现

connect_pool        连接池基类（管理单个服务器的连接）
      ↑
      ├── tcp_pool         TCP 连接池
      ├── redis_client_pool    Redis 连接池
      └── mysql_pool       MySQL 连接池

connect_manager     连接池管理器基类（管理多个服务器）
      ↑
      ├── tcp_manager      TCP 连接池管理器
      ├── redis_client_cluster   Redis 集群管理器
      └── mysql_manager    MySQL 连接池管理器

connect_monitor     连接池监控器（后台健康检查）
```

## 快速开始

### 单机连接池示例

```cpp
#include "acl_cpp/lib_acl.hpp"

int main() {
    // 1. 创建连接池
    acl::redis_client_pool pool("127.0.0.1:6379", 100);
    
    // 2. 配置参数
    pool.set_timeout(10, 10)        // 连接和读写超时
        .set_password("mypass")      // 认证密码
        .set_idle_ttl(300);          // 空闲连接TTL
    
    // 3. 获取连接
    acl::redis_client* conn = (acl::redis_client*) pool.peek();
    
    // 4. 使用连接
    acl::redis_string cmd(conn);
    cmd.set("key", "value");
    
    // 5. 归还连接
    pool.put(conn, true);
    
    return 0;
}
```

### 集群连接池示例

```cpp
#include "acl_cpp/lib_acl.hpp"

int main() {
    // 1. 创建集群管理器
    acl::redis_client_cluster cluster;
    
    // 2. 自动发现集群拓扑
    cluster.set_all_slot("127.0.0.1:7000", 50, 10, 10);
    
    // 3. 使用集群（自动路由）
    acl::redis_string cmd(&cluster);
    cmd.set("user:1000", "张三");
    
    return 0;
}
```

## 设计原则

### 1. 分层架构

连接池采用四层架构设计：
- **应用层**：业务代码，调用连接池 API
- **管理层**：`connect_manager`，管理多个连接池，提供负载均衡
- **连接池层**：`connect_pool`，管理单服务器的连接，实现连接复用
- **连接层**：`connect_client`，封装单个网络连接

### 2. 工厂模式

每层通过工厂方法创建下一层对象：
```cpp
// connect_manager 创建 connect_pool
virtual connect_pool* create_pool(const char* addr, size_t count, size_t idx) = 0;

// connect_pool 创建 connect_client
virtual connect_client* create_connect() = 0;
```

### 3. 模板方法模式

基类定义算法框架，子类实现具体细节：
```cpp
class connect_pool {
    // 框架方法（已实现）
    connect_client* peek();
    void put(connect_client* conn, bool keep);
    
    // 模板方法（子类实现）
    virtual connect_client* create_connect() = 0;
};
```

### 4. 对象池模式

连接复用，避免频繁创建销毁：
```cpp
// 从池中获取连接（复用旧连接）
connect_client* conn = pool.peek();

// 使用完毕归还到池中
pool.put(conn, true);
```

## 核心机制

### 1. 连接复用

采用 **LIFO + LRU** 混合策略：
- `peek()` 从队尾取出最旧的连接
- `put()` 将连接放回队首
- 优先使用旧连接，保持连接活跃

### 2. 故障恢复

连接池状态机：
```
ALIVE (正常) --[连接失败]--> DEAD (故障)
      ↑                          |
      |--[到达重试间隔]------------|
```

- 连接失败时标记连接池为 `DEAD` 状态
- 定期检查是否到达重试时间
- 自动恢复到 `ALIVE` 状态重新尝试

### 3. 健康检查

三种检查方式：
- **被动检查**：`put()` 时根据参数检查
- **主动检查**：`connect_monitor` 定时扫描
- **分批检查**：避免一次性检查大量连接

### 4. 引用计数

延迟销毁机制：
```cpp
pool->refer();    // 增加引用
// ... 使用连接池 ...
pool->unrefer();  // 减少引用，引用计数为0时才销毁
```

### 5. 线程绑定

支持两种模式：
- **共享模式**：所有线程共享连接池集合
- **绑定模式**：每个线程独立的连接池集合（适合协程）

## 性能优化

### 1. 连接预热

```cpp
pool.set_conns_min(20);  // 预创建20个连接
pool.keep_conns();        // 立即预热
```

### 2. 批量检查

```cpp
// 每次检查10个连接池，避免停顿
manager.check_idle_conns(10);
```

### 3. 异步检查

```cpp
// 使用线程池异步检查连接健康状态
pool.check_dead(thread_pool);
```

### 4. 减少锁竞争

- 线程绑定模式避免跨线程竞争
- 无锁快速路径（状态检查）
- 细粒度锁（每个连接池独立锁）

## 适用场景

- **数据库连接池**：MySQL、PostgreSQL、SQLite
- **缓存连接池**：Redis、Memcache
- **RPC 客户端**：HTTP、gRPC、Thrift
- **消息队列**：Kafka、RabbitMQ
- **任何需要长连接复用的场景**

## 文档导航

- [详细架构设计](architecture.md) - 完整的架构图和组件说明
- [使用示例](examples.md) - 各种使用场景的代码示例
- [API 参考](api.md) - 完整的 API 文档
- [最佳实践](best-practices.md) - 生产环境配置建议

## 相关模块

- `lib_acl_cpp/include/acl_cpp/connpool/` - 连接池头文件
- `lib_acl_cpp/src/connpool/` - 连接池实现
- `lib_acl_cpp/samples/redis/redis_pool/` - Redis 连接池示例
- `lib_acl_cpp/samples/redis/redis_client_cluster/` - Redis 集群示例

## 作者与贡献

ACL 库由 zhengshuxin 开发和维护。

## 许可证

Apache License 2.0

