# ACL Redis 最佳实践

## 目录
- [连接管理](#连接管理)
- [性能优化](#性能优化)
- [错误处理](#错误处理)
- [内存管理](#内存管理)
- [集群使用](#集群使用)
- [安全建议](#安全建议)
- [调试技巧](#调试技巧)

## 连接管理

### 1. 使用连接池

**推荐做法：**

```cpp
// 创建连接池（单例模式）
class RedisPool {
private:
    static acl::redis_client_pool* pool_;
    
public:
    static acl::redis_client_pool* get_instance() {
        if (pool_ == NULL) {
            pool_ = new acl::redis_client_pool(
                "127.0.0.1:6379",
                100,    // 最大连接数
                10,     // 连接超时
                10      // 读写超时
            );
            pool_->set_idle_ttl(60);  // 空闲连接60秒超时
        }
        return pool_;
    }
};

// 使用连接池
void worker() {
    acl::redis_client_pool* pool = RedisPool::get_instance();
    acl::redis_client* conn = (acl::redis_client*)pool->peek();
    
    if (conn) {
        acl::redis_string cmd(conn);
        cmd.set("key", "value");
        pool->put(conn, true);  // 归还连接
    }
}
```

**避免：**

```cpp
// 不要每次都创建新连接
void bad_practice() {
    for (int i = 0; i < 1000; i++) {
        // 每次创建连接，开销巨大
        acl::redis_client client("127.0.0.1:6379", 10, 10);
        acl::redis_string cmd(&client);
        cmd.set("key", "value");
        // 连接在作用域结束时关闭
    }
}
```

### 2. 正确设置超时时间

```cpp
// 根据场景设置合理的超时
acl::redis_client_pool pool(
    "127.0.0.1:6379",
    100,
    5,      // 连接超时5秒（网络较好）
    3       // 读写超时3秒（Redis操作通常很快）
);

// 对于慢操作，可以单独设置超时
acl::redis_client* conn = (acl::redis_client*)pool.peek();
if (conn) {
    int timeout = 30;  // 30秒超时
    // 注意：需要在命令执行时传递超时参数
}
```

### 3. 命令对象复用

```cpp
// 推荐：复用命令对象
void recommended() {
    acl::redis_client client("127.0.0.1:6379", 10, 10);
    acl::redis_string cmd(&client);
    
    for (int i = 0; i < 1000; i++) {
        cmd.set("key", "value");
        cmd.clear();  // 重置命令对象
    }
}

// 避免：每次创建新对象
void not_recommended() {
    acl::redis_client client("127.0.0.1:6379", 10, 10);
    
    for (int i = 0; i < 1000; i++) {
        acl::redis_string cmd(&client);  // 重复创建
        cmd.set("key", "value");
    }
}
```

## 性能优化

### 1. 使用 Pipeline 批量操作

```cpp
// Pipeline 模式：批量发送命令
void pipeline_batch_write(const std::vector<std::pair<string, string>>& data) {
    acl::redis_client_pipeline pipeline("127.0.0.1:6379");
    acl::redis_string cmd;
    cmd.set_pipeline(&pipeline);
    
    // 批量构建命令
    for (const auto& pair : data) {
        cmd.set(pair.first.c_str(), pair.second.c_str());
        cmd.clear(true);  // 保留 pipeline 状态
    }
    
    // 一次性发送并接收
    pipeline.flush();
}

// 对比：单个发送（慢）
void single_write(const std::vector<std::pair<string, string>>& data) {
    acl::redis_client client("127.0.0.1:6379", 10, 10);
    acl::redis_string cmd(&client);
    
    // 每个命令一次网络往返
    for (const auto& pair : data) {
        cmd.set(pair.first.c_str(), pair.second.c_str());
        cmd.clear();
    }
}
```

### 2. 使用 MGET/MSET 代替多次 GET/SET

```cpp
// 推荐：批量操作
void batch_get_set() {
    acl::redis_client client("127.0.0.1:6379", 10, 10);
    acl::redis_string cmd(&client);
    
    // 一次性获取多个键
    std::vector<acl::string> keys = {"key1", "key2", "key3"};
    std::vector<acl::string> values;
    cmd.mget(keys, &values);
    
    // 一次性设置多个键
    std::map<acl::string, acl::string> kvs;
    kvs["key1"] = "value1";
    kvs["key2"] = "value2";
    cmd.clear();
    cmd.mset(kvs);
}

// 避免：多次单个操作
void single_get_set() {
    acl::redis_client client("127.0.0.1:6379", 10, 10);
    acl::redis_string cmd(&client);
    
    acl::string value;
    cmd.get("key1", value);
    cmd.clear();
    cmd.get("key2", value);
    cmd.clear();
    cmd.get("key3", value);
}
```

### 3. 合理使用数据结构

```cpp
// 推荐：使用 Hash 存储对象
void use_hash_for_object() {
    acl::redis_client client("127.0.0.1:6379", 10, 10);
    acl::redis_hash cmd(&client);
    
    std::map<acl::string, acl::string> user;
    user["name"] = "Alice";
    user["age"] = "25";
    user["email"] = "alice@example.com";
    
    // 一个 Hash 键存储整个对象
    cmd.hmset("user:1001", user);
}

// 不推荐：使用多个 String 键
void use_multiple_strings() {
    acl::redis_client client("127.0.0.1:6379", 10, 10);
    acl::redis_string cmd(&client);
    
    // 多个键，浪费内存
    cmd.set("user:1001:name", "Alice");
    cmd.clear();
    cmd.set("user:1001:age", "25");
    cmd.clear();
    cmd.set("user:1001:email", "alice@example.com");
}
```

### 4. 避免大 Key

```cpp
// 推荐：限制集合大小
void limit_collection_size() {
    acl::redis_client client("127.0.0.1:6379", 10, 10);
    acl::redis_list cmd(&client);
    
    const char* key = "recent_logs";
    const int MAX_SIZE = 1000;
    
    // 添加新元素
    cmd.lpush(key, "new log entry");
    
    // 限制列表大小
    cmd.clear();
    cmd.ltrim(key, 0, MAX_SIZE - 1);
}

// 推荐：分片存储大数据
void shard_large_data() {
    acl::redis_client client("127.0.0.1:6379", 10, 10);
    acl::redis_hash cmd(&client);
    
    // 将大 Hash 分片
    for (int shard = 0; shard < 10; shard++) {
        acl::string key;
        key.format("large_hash:shard:%d", shard);
        
        std::map<acl::string, acl::string> data;
        // ... 填充数据
        cmd.hmset(key.c_str(), data);
        cmd.clear();
    }
}
```

### 5. 使用 SCAN 代替 KEYS

```cpp
// 推荐：使用 SCAN 遍历
void scan_keys_safely(const char* pattern) {
    acl::redis_client client("127.0.0.1:6379", 10, 10);
    acl::redis_key cmd(&client);
    
    int cursor = 0;
    size_t count = 100;
    
    do {
        const acl::redis_result* result = cmd.scan(cursor, pattern, &count);
        if (result == NULL) break;
        
        // 处理结果
        const acl::redis_result* cursor_r = result->get_child(0);
        const acl::redis_result* keys_r = result->get_child(1);
        
        if (cursor_r) {
            acl::string cursor_str;
            cursor_str = cursor_r->get(0);
            cursor = atoi(cursor_str.c_str());
        }
        
        if (keys_r) {
            // 处理键
        }
        
        cmd.clear();
    } while (cursor != 0);
}

// 避免：KEYS 会阻塞 Redis
void dangerous_keys(const char* pattern) {
    acl::redis_client client("127.0.0.1:6379", 10, 10);
    acl::redis_key cmd(&client);
    
    // 在生产环境中危险！
    std::vector<acl::string> keys;
    cmd.keys_pattern(pattern, keys);
}
```

## 错误处理

### 1. 检查返回值

```cpp
void proper_error_handling() {
    acl::redis_client client("127.0.0.1:6379", 10, 10);
    acl::redis_string cmd(&client);
    
    // 检查操作是否成功
    if (!cmd.set("key", "value")) {
        // 获取详细错误信息
        const char* error = cmd.result_error();
        printf("SET 失败: %s\n", error);
        
        // 根据错误类型处理
        if (strstr(error, "READONLY")) {
            printf("Redis 处于只读模式\n");
        } else if (strstr(error, "MOVED")) {
            printf("集群槽位已移动\n");
        }
        return;
    }
    
    // 对于返回整数的方法
    int ret = cmd.append("key", "more");
    if (ret < 0) {
        printf("APPEND 失败: %s\n", cmd.result_error());
    } else {
        printf("字符串新长度: %d\n", ret);
    }
}
```

### 2. 连接错误处理

```cpp
void handle_connection_errors() {
    acl::redis_client client("127.0.0.1:6379", 5, 5);
    
    // 检查连接状态
    if (client.eof()) {
        printf("连接已关闭\n");
        // 尝试重连或使用备用连接
        return;
    }
    
    acl::redis_string cmd(&client);
    
    for (int retry = 0; retry < 3; retry++) {
        if (cmd.set("key", "value")) {
            break;  // 成功
        }
        
        if (client.eof()) {
            printf("连接断开，重试 %d/3\n", retry + 1);
            sleep(1);
            cmd.clear();
        } else {
            // 其他错误，不重试
            printf("命令执行失败: %s\n", cmd.result_error());
            break;
        }
    }
}
```

### 3. 集群重定向处理

```cpp
void cluster_redirect_handling() {
    acl::redis_client_cluster cluster;
    cluster.set_all_slot("127.0.0.1:7000", 100, 10, 10);
    
    // 配置重定向参数
    cluster.set_redirect_max(10);       // 最大重定向次数
    cluster.set_redirect_sleep(100);    // 重定向等待时间（微秒）
    
    acl::redis cmd;
    cmd.set_cluster(&cluster);
    
    // 集群模式会自动处理 MOVE/ASK 重定向
    if (!cmd.set("key", "value")) {
        const char* error = cmd.result_error();
        
        if (strstr(error, "CLUSTERDOWN")) {
            printf("集群不可用\n");
        } else if (strstr(error, "TOO MANY REDIRECTS")) {
            printf("重定向次数超过限制\n");
        }
    }
}
```

## 内存管理

### 1. 及时调用 clear()

```cpp
void memory_efficient_loop() {
    acl::redis_client client("127.0.0.1:6379", 10, 10);
    acl::redis_string cmd(&client);
    
    for (int i = 0; i < 1000000; i++) {
        acl::string key;
        key.format("key_%d", i);
        
        cmd.set(key.c_str(), "value");
        
        // 重要：清理内存池
        cmd.clear();
        
        // 不清理会导致内存持续增长
    }
}
```

### 2. 处理大数据时使用分片

```cpp
void process_large_response() {
    acl::redis_client client("127.0.0.1:6379", 10, 10);
    acl::redis_string cmd(&client);
    
    // 启用响应分片（避免一次性分配大内存）
    client.set_slice_respond(true);
    
    // 获取大字符串
    const acl::redis_result* result = cmd.get("large_key");
    
    if (result && result->get_type() == acl::REDIS_RESULT_STRING) {
        // 数据被分片存储，逐片处理
        size_t size = result->get_size();
        for (size_t i = 0; i < size; i++) {
            size_t len;
            const char* data = result->get(i, &len);
            // 处理数据片段
        }
    }
}
```

### 3. 设置过期时间

```cpp
void set_expiration() {
    acl::redis_client client("127.0.0.1:6379", 10, 10);
    acl::redis_string cmd(&client);
    acl::redis_key key_cmd(&client);
    
    // 方法1：SET 时设置过期
    cmd.setex("session:123", "token_abc", 3600);  // 1小时
    
    // 方法2：SET 后设置过期
    cmd.clear();
    cmd.set("temp:data", "value");
    key_cmd.clear();
    key_cmd.expire("temp:data", 300);  // 5分钟
    
    // 方法3：使用 SET 的 EX 选项
    cmd.clear();
    cmd.set("cache:key", "value", 600, SETFLAG_EX);  // 10分钟
}
```

## 集群使用

### 1. 预热槽位映射

```cpp
void preheat_cluster() {
    acl::redis_client_cluster cluster;
    
    // 在应用启动时预先发现所有节点
    cluster.set_all_slot("127.0.0.1:7000", 100, 10, 10);
    
    // 设置密码
    cluster.set_password("default", "your_password");
    
    printf("集群初始化完成，槽位映射已缓存\n");
}
```

### 2. 使用 Hash Tag 保证键在同一槽位

```cpp
void use_hash_tags() {
    acl::redis_client_cluster cluster;
    cluster.set_all_slot("127.0.0.1:7000", 100, 10, 10);
    
    acl::redis cmd;
    cmd.set_cluster(&cluster);
    
    // 使用 {user:1001} 作为 hash tag
    // 这些键会被路由到同一个槽位
    cmd.set("{user:1001}:name", "Alice");
    cmd.clear();
    cmd.set("{user:1001}:age", "25");
    cmd.clear();
    cmd.set("{user:1001}:email", "alice@example.com");
    
    // 可以在事务中使用
    acl::redis_transaction trans;
    trans.set_cluster(&cluster);
    trans.multi();
    
    // 这些命令会在同一个节点执行
    cmd.set("{user:1001}:status", "active");
    cmd.incr("{user:1001}:login_count");
    
    trans.exec();
}
```

### 3. 监控集群状态

```cpp
void monitor_cluster() {
    acl::redis_client_cluster cluster;
    cluster.set_all_slot("127.0.0.1:7000", 100, 10, 10);
    
    // 定期检查槽位映射
    auto check_thread = [&cluster]() {
        while (true) {
            sleep(60);  // 每分钟检查一次
            
            // 重新发现槽位（处理节点变更）
            cluster.set_all_slot("127.0.0.1:7000", 100, 10, 10);
            printf("槽位映射已更新\n");
        }
    };
    
    std::thread monitor(check_thread);
    monitor.detach();
}
```

## 安全建议

### 1. 使用密码认证

```cpp
void secure_connection() {
    // 单机模式
    acl::redis_client client("127.0.0.1:6379", 10, 10);
    client.set_password("strong_password_here");
    
    // 集群模式
    acl::redis_client_cluster cluster;
    cluster.set_password("default", "strong_password_here");
    cluster.set_all_slot("127.0.0.1:7000", 100, 10, 10);
}
```

### 2. 使用 SSL/TLS

```cpp
void secure_ssl_connection() {
    acl::polarssl_conf ssl_conf;
    
    acl::redis_client client("127.0.0.1:6379", 10, 10);
    client.set_ssl_conf(&ssl_conf);
    
    // 或者集群模式
    acl::redis_client_cluster cluster;
    cluster.set_ssl_conf(&ssl_conf);
}
```

### 3. 输入验证

```cpp
void validate_input(const char* user_input) {
    // 验证输入，防止注入
    if (strlen(user_input) > 1024) {
        printf("输入过长\n");
        return;
    }
    
    // 转义特殊字符（如果需要）
    acl::string safe_input;
    safe_input.format("%s", user_input);
    
    acl::redis_client client("127.0.0.1:6379", 10, 10);
    acl::redis_string cmd(&client);
    
    cmd.set("user_data", safe_input.c_str());
}
```

## 调试技巧

### 1. 启用详细日志

```cpp
void enable_logging() {
    // 启用 ACL 日志
    acl::log::stdout_open(true);
    
    // 设置日志级别
    acl::log::open("redis_debug.log", "redis_app");
    
    acl::redis_client client("127.0.0.1:6379", 10, 10);
    acl::redis_string cmd(&client);
    
    // 操作会被记录
    cmd.set("key", "value");
}
```

### 2. 检查连接地址

```cpp
void check_connection_addr() {
    acl::redis_client client("127.0.0.1:6379", 10, 10);
    
    // 启用地址检查（调试模式）
    client.set_check_addr(true);
    
    acl::redis_string cmd(&client);
    cmd.set_check_addr(true);
    
    // 每次操作前会验证连接地址
    cmd.set("key", "value");
}
```

### 3. 打印命令和结果

```cpp
void debug_commands() {
    acl::redis_client client("127.0.0.1:6379", 10, 10);
    acl::redis_string cmd(&client);
    
    // 执行命令
    if (!cmd.set("debug_key", "debug_value")) {
        printf("命令失败\n");
        printf("错误类型: %d\n", cmd.result_type());
        printf("错误信息: %s\n", cmd.result_error());
        return;
    }
    
    // 打印结果
    printf("结果类型: %d\n", cmd.result_type());
    printf("结果状态: %s\n", cmd.result_status());
    
    // 获取值
    acl::string value;
    cmd.clear();
    if (cmd.get("debug_key", value)) {
        printf("值: %s\n", value.c_str());
        printf("长度: %zu\n", value.length());
    }
}
```

### 4. 性能分析

```cpp
void performance_analysis() {
    struct timeval start, end;
    
    acl::redis_client client("127.0.0.1:6379", 10, 10);
    acl::redis_string cmd(&client);
    
    // 测量单次操作
    gettimeofday(&start, NULL);
    cmd.set("key", "value");
    gettimeofday(&end, NULL);
    
    double elapsed = (end.tv_sec - start.tv_sec) * 1000.0 +
                    (end.tv_usec - start.tv_usec) / 1000.0;
    printf("SET 耗时: %.3f ms\n", elapsed);
    
    // 测量批量操作
    const int COUNT = 10000;
    gettimeofday(&start, NULL);
    
    for (int i = 0; i < COUNT; i++) {
        acl::string key;
        key.format("key_%d", i);
        cmd.set(key.c_str(), "value");
        cmd.clear();
    }
    
    gettimeofday(&end, NULL);
    elapsed = (end.tv_sec - start.tv_sec) * 1000.0 +
             (end.tv_usec - start.tv_usec) / 1000.0;
    
    printf("批量操作:\n");
    printf("  总数: %d\n", COUNT);
    printf("  耗时: %.2f ms\n", elapsed);
    printf("  QPS: %.2f\n", COUNT * 1000.0 / elapsed);
}
```

## 常见陷阱

### 1. 忘记调用 clear()

```cpp
// 错误示例
void memory_leak() {
    acl::redis_client client("127.0.0.1:6379", 10, 10);
    acl::redis_string cmd(&client);
    
    for (int i = 0; i < 1000000; i++) {
        cmd.set("key", "value");
        // 忘记调用 clear()，内存持续增长
    }
}

// 正确示例
void no_memory_leak() {
    acl::redis_client client("127.0.0.1:6379", 10, 10);
    acl::redis_string cmd(&client);
    
    for (int i = 0; i < 1000000; i++) {
        cmd.set("key", "value");
        cmd.clear();  // 释放内存
    }
}
```

### 2. 集群模式下使用多 key 命令

```cpp
// 错误：集群模式下不同 key 可能在不同节点
void cluster_multi_key_error() {
    acl::redis_client_cluster cluster;
    cluster.set_all_slot("127.0.0.1:7000", 100, 10, 10);
    
    acl::redis cmd;
    cmd.set_cluster(&cluster);
    
    // 错误：key1 和 key2 可能在不同节点
    std::vector<acl::string> keys = {"key1", "key2"};
    std::vector<acl::string> values;
    cmd.mget(keys, &values);  // 可能失败
}

// 正确：使用 hash tag
void cluster_multi_key_correct() {
    acl::redis_client_cluster cluster;
    cluster.set_all_slot("127.0.0.1:7000", 100, 10, 10);
    
    acl::redis cmd;
    cmd.set_cluster(&cluster);
    
    // 使用 hash tag 保证在同一节点
    std::vector<acl::string> keys = {"{user}:key1", "{user}:key2"};
    std::vector<acl::string> values;
    cmd.mget(keys, &values);  // 成功
}
```

### 3. 不检查返回值

```cpp
// 危险
void ignore_errors() {
    acl::redis_client client("127.0.0.1:6379", 10, 10);
    acl::redis_string cmd(&client);
    
    cmd.set("key", "value");  // 不检查返回值
    
    acl::string value;
    cmd.get("key", value);    // 不检查返回值
    printf("Value: %s\n", value.c_str());  // 可能是空字符串
}

// 安全
void check_errors() {
    acl::redis_client client("127.0.0.1:6379", 10, 10);
    acl::redis_string cmd(&client);
    
    if (!cmd.set("key", "value")) {
        printf("SET 失败: %s\n", cmd.result_error());
        return;
    }
    
    acl::string value;
    cmd.clear();
    if (!cmd.get("key", value)) {
        printf("GET 失败: %s\n", cmd.result_error());
        return;
    }
    
    printf("Value: %s\n", value.c_str());
}
```

## 性能检查清单

- [ ] 使用连接池，避免频繁创建连接
- [ ] 使用 Pipeline 或 MGET/MSET 批量操作
- [ ] 使用 SCAN 代替 KEYS
- [ ] 避免大 Key，使用分片
- [ ] 设置合理的过期时间
- [ ] 及时调用 clear() 释放内存
- [ ] 使用合适的数据结构（Hash vs String）
- [ ] 启用响应分片处理大数据
- [ ] 集群模式下使用 hash tag
- [ ] 设置合理的超时时间

## 安全检查清单

- [ ] 使用密码认证
- [ ] 考虑使用 SSL/TLS
- [ ] 验证用户输入
- [ ] 限制命令权限（Redis 配置）
- [ ] 使用防火墙限制访问
- [ ] 定期更新 ACL 库
- [ ] 记录和监控异常操作
- [ ] 不在日志中输出敏感信息

---

遵循这些最佳实践可以帮助你构建高性能、安全、可靠的 Redis 应用。

