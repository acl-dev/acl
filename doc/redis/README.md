# ACL Redis 客户端库文档

## 简介

ACL Redis 是 ACL 项目中用于与 Redis 服务器进行交互的 C++ 客户端库，它是 `lib_acl_cpp` 的重要组成部分。该库提供了完整的 Redis 命令支持，包含 12 个主要的操作类和 150+ Redis 命令，支持 Redis 单机模式、集群模式和 Pipeline 模式。

## 主要特性

### 1. 完整的命令支持
- **String（字符串）**：SET/GET/INCR/DECR 等 30+ 命令
- **Hash（哈希表）**：HSET/HGET/HMSET/HMGET 等 15+ 命令
- **List（列表）**：LPUSH/RPUSH/LPOP/LRANGE 等 20+ 命令
- **Set（集合）**：SADD/SREM/SMEMBERS/SINTER 等 15+ 命令
- **Sorted Set（有序集合）**：ZADD/ZRANGE/ZREM 等 20+ 命令
- **Key（键操作）**：DEL/EXISTS/EXPIRE/TTL 等 10+ 命令
- **PubSub（发布订阅）**：PUBLISH/SUBSCRIBE/UNSUBSCRIBE
- **Transaction（事务）**：MULTI/EXEC/DISCARD/WATCH
- **Script（Lua脚本）**：EVAL/EVALSHA/SCRIPT
- **Server（服务器）**：INFO/CONFIG/DBSIZE 等
- **Geo（地理位置）**：GEOADD/GEORADIUS 等
- **Stream（流）**：XADD/XREAD/XGROUP 等
- **HyperLogLog**：PFADD/PFCOUNT/PFMERGE

### 2. 多种连接模式
- **单机模式**：直接连接单个 Redis 实例
- **集群模式**：完整支持 Redis 3.0+ 集群，自动哈希槽路由
- **Pipeline 模式**：批量发送命令，提高吞吐量
- **连接池**：内置连接池管理，支持多线程并发

### 3. 高性能设计
- **内存池管理**：使用 `dbuf_pool` 减少内存分配开销
- **零拷贝**：高效的数据传输机制
- **批量操作**：MGET/MSET/Pipeline 等批量命令支持
- **连接复用**：连接池自动管理连接生命周期

### 4. 企业级特性
- **集群自动重定向**：支持 MOVE/ASK 自动重定向
- **故障切换**：连接失败自动重试
- **密码认证**：支持 Redis 密码认证
- **SSL/TLS**：支持加密连接
- **数据库选择**：支持多数据库切换（单机模式）
- **线程安全**：集群模式下线程安全

## 架构设计

ACL Redis 采用面向对象的分层设计：

```
redis (统一接口类)
  ├── redis_command (基类)
  │     ├── 连接管理
  │     ├── 命令构建
  │     ├── 结果解析
  │     └── 内存管理
  ├── redis_string (字符串操作)
  ├── redis_hash (哈希表操作)
  ├── redis_list (列表操作)
  ├── redis_set (集合操作)
  ├── redis_zset (有序集合操作)
  ├── redis_key (键操作)
  ├── redis_pubsub (发布订阅)
  ├── redis_transaction (事务)
  ├── redis_script (Lua脚本)
  ├── redis_server (服务器命令)
  ├── redis_geo (地理位置)
  ├── redis_stream (流)
  └── redis_hyperloglog (HyperLogLog)
```

详细架构设计请参阅 [architecture.md](architecture.md)

## 快速开始

### 1. 编译安装

```bash
# 编译基础库
cd lib_acl && make
cd ../lib_protocol && make
cd ../lib_acl_cpp && make
```

### 2. 单机模式示例

```cpp
#include "acl_cpp/lib_acl.hpp"

int main() {
    // 初始化（Windows 必需）
    acl::acl_cpp_init();
    
    // 创建 Redis 客户端
    acl::redis_client client("127.0.0.1:6379", 10, 10);
    
    // 创建命令对象
    acl::redis_string cmd(&client);
    
    // SET 命令
    if (cmd.set("name", "Alice")) {
        printf("SET 成功\n");
    }
    
    // GET 命令
    acl::string value;
    cmd.clear();  // 重置命令对象
    if (cmd.get("name", value)) {
        printf("GET: %s\n", value.c_str());
    }
    
    return 0;
}
```

### 3. 集群模式示例

```cpp
#include "acl_cpp/lib_acl.hpp"

int main() {
    acl::acl_cpp_init();
    
    // 创建集群对象
    acl::redis_client_cluster cluster;
    
    // 设置集群节点（自动发现所有节点和槽位）
    cluster.set_all_slot("127.0.0.1:7000", 100, 10, 10);
    
    // 创建统一命令对象
    acl::redis cmd;
    cmd.set_cluster(&cluster);
    
    // 使用方式与单机模式相同
    cmd.set("user:1001", "Alice");
    
    acl::string name;
    cmd.clear();
    cmd.get("user:1001", name);
    
    return 0;
}
```

更多示例请参阅 [examples.md](examples.md)

## 文档导航

| 文档 | 说明 |
|------|------|
| [架构设计](architecture.md) | 详细的架构设计和模块说明 |
| [API 文档](api.md) | 完整的 API 接口文档 |
| [示例代码](examples.md) | 各种使用场景的示例代码 |
| [最佳实践](best-practices.md) | 性能优化和使用建议 |

## 源码位置

- **头文件**：`lib_acl_cpp/include/acl_cpp/redis/`
- **源码**：`lib_acl_cpp/src/redis/`
- **示例**：`lib_acl_cpp/samples/redis/`

## 编译选项

在 Makefile 中添加以下编译标志：

```makefile
# Linux
CFLAGS += -DLINUX2 -I./lib_acl_cpp/include
LDFLAGS += -L./lib_acl_cpp/lib -l_acl_cpp \
           -L./lib_protocol/lib -l_protocol \
           -L./lib_acl/lib -l_acl \
           -lpthread

# macOS
CFLAGS += -DMACOSX -I./lib_acl_cpp/include
LDFLAGS += -L./lib_acl_cpp/lib -l_acl_cpp \
           -L./lib_protocol/lib -l_protocol \
           -L./lib_acl/lib -l_acl \
           -lpthread

# FreeBSD
CFLAGS += -DFREEBSD -I./lib_acl_cpp/include
```

## 支持的平台

- **Linux**（CentOS/Ubuntu/Debian 等）
- **macOS**
- **FreeBSD**
- **Solaris**
- **Windows**（VC2003/2008/2010/2012/2015/2017/2019/2022）
- **Android**（NDK 12b/16b/28c）
- **iOS**

## 性能特点

- **高并发**：支持多线程并发访问
- **高吞吐**：Pipeline 模式批量处理
- **低延迟**：连接池减少连接建立开销
- **内存高效**：内存池管理，减少碎片

## 技术支持

- **项目主页**：https://github.com/acl-dev/acl
- **问题反馈**：https://github.com/acl-dev/acl/issues
- **作者**：郑树新（zsxxsz）

## 许可协议

Apache License 2.0

## 版本历史

- **v3.5+**：支持 Redis Stream、Geo 等新特性
- **v3.0+**：完整支持 Redis 3.0+ 集群模式
- **v2.x**：基础 Redis 命令支持

---

**注意**：本文档基于 ACL 最新版本编写，具体功能可能因版本而异。

