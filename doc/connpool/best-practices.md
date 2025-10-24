# ACL 连接池最佳实践

本文档总结了在生产环境中使用 ACL 连接池的最佳实践和常见问题。

## 目录

- [连接池大小设置](#连接池大小设置)
- [超时配置](#超时配置)
- [连接保活策略](#连接保活策略)
- [错误处理](#错误处理)
- [性能优化](#性能优化)
- [监控和诊断](#监控和诊断)
- [常见问题](#常见问题)

---

## 连接池大小设置

### 基本原则

```cpp
// 经验公式
max_conns = 并发请求数 × 平均响应时间(秒) × 安全系数(1.5-2.0)

// 示例计算
// 场景: 1000 QPS, 平均响应时间 100ms
// max_conns = 1000 × 0.1 × 1.5 = 150
```

### 不同场景的推荐配置

#### 高并发短连接

```cpp
acl::redis_client_pool pool("127.0.0.1:6379", 200);
pool.set_conns_min(50)         // 预热50个连接
    .set_idle_ttl(60)           // 60秒空闲超时，快速释放
    .set_check_inter(30);       // 30秒检查一次
```

**适用场景**：
- Web API 服务
- 高峰期流量大，低峰期流量小
- 需要快速释放资源

#### 低并发长连接

```cpp
acl::redis_client_pool pool("127.0.0.1:6379", 50);
pool.set_conns_min(10)         // 预热10个连接
    .set_idle_ttl(3600)         // 1小时空闲超时，长期保持
    .set_check_inter(300);      // 5分钟检查一次
```

**适用场景**：
- 后台任务处理
- 定时任务
- 内部服务调用

#### 关键业务系统

```cpp
acl::redis_client_pool pool("127.0.0.1:6379", 500);
pool.set_conns_min(100)        // 预热大量连接
    .set_idle_ttl(-1)           // 永不过期
    .set_retry_inter(-1);       // 不自动恢复，需人工介入
```

**适用场景**：
- 金融交易系统
- 核心业务系统
- 要求高可用性的服务

### 动态调整

```cpp
// 根据当前负载动态调整
void adjust_pool_size() {
    size_t current_count = pool.get_count();
    unsigned long long usage = pool.get_current_used();
    
    if (usage > current_count * 0.8) {
        // 使用率超过80%，考虑增加最大连接数
        printf("连接池使用率高，建议增加最大连接数\n");
    }
    
    if (usage < current_count * 0.2) {
        // 使用率低于20%，可以减少最小连接数
        printf("连接池使用率低，可以减少预热连接数\n");
    }
}
```

---

## 超时配置

### 超时时间的选择

| 超时类型 | 推荐值 | 说明 |
|---------|--------|------|
| `conn_timeout` | 3-10秒 | 连接建立超时，不宜过长 |
| `rw_timeout` | 1-30秒 | 读写超时，根据业务需求设置 |
| `idle_ttl` | 300-3600秒 | 空闲连接超时，根据流量特征 |
| `retry_inter` | 5-60秒 | 失败重试间隔，避免雪崩 |

### 典型配置示例

#### API 服务配置

```cpp
pool.set_timeout(5, 3);        // 快速超时，避免阻塞
```

#### 批处理任务配置

```cpp
pool.set_timeout(10, 60);      // 较长超时，容忍慢查询
```

#### 实时性要求高的场景

```cpp
pool.set_timeout(1, 1);        // 极短超时，快速失败
```

### 超时处理最佳实践

```cpp
bool execute_with_timeout(acl::redis_client_pool& pool,
                          const char* key, const char* value) {
    int original_timeout = 10;  // 原始超时
    int retry_count = 3;        // 重试次数
    
    for (int i = 0; i < retry_count; i++) {
        acl::redis_client* conn = (acl::redis_client*) pool.peek();
        if (conn == NULL) {
            continue;
        }
        
        // 每次重试增加超时时间
        int timeout = original_timeout * (i + 1);
        conn->set_timeout(timeout, timeout);
        
        acl::redis_string cmd(conn);
        bool result = cmd.set(key, value);
        
        pool.put(conn, result);
        
        if (result) {
            return true;  // 成功
        }
        
        // 失败，等待后重试
        sleep(1);
    }
    
    return false;  // 重试失败
}
```

---

## 连接保活策略

### 空闲连接管理

```cpp
// 配置空闲连接TTL
pool.set_idle_ttl(300);  // 5分钟

// 手动触发空闲连接清理
size_t closed = pool.check_idle(300);
printf("清理了 %lu 个空闲连接\n", (unsigned long)closed);
```

### 最小连接数保持

```cpp
// 设置最小连接数
pool.set_conns_min(20);

// 预热连接池
pool.keep_conns();

// 定期保持最小连接数
while (true) {
    sleep(60);
    pool.keep_conns();  // 确保至少有20个连接
}
```

### 连接健康检查

```cpp
// 实现自定义的 alive() 检测
class my_redis_client : public acl::redis_client {
public:
    bool alive() override {
        if (!acl::redis_client::alive()) {
            return false;
        }
        
        // 发送 PING 命令检测
        acl::redis_string cmd(this);
        acl::string result;
        if (!cmd.ping(result)) {
            return false;
        }
        
        return result == "PONG";
    }
};
```

### 定期检查死连接

```cpp
// 使用线程池并发检测
acl::thread_pool thread_pool;
thread_pool.set_limit(10);  // 10个工作线程

// 定期检查
while (true) {
    sleep(60);
    
    // 使用线程池并发检测
    size_t dead = pool.check_dead(&thread_pool);
    printf("检测到 %lu 个死连接\n", (unsigned long)dead);
}
```

---

## 错误处理

### 连接获取失败处理

```cpp
acl::redis_client* conn = (acl::redis_client*) pool.peek();

if (conn == NULL) {
    // 连接池可能处于以下状态：
    // 1. 连接数达到上限
    // 2. 服务器故障
    // 3. 网络问题
    
    // 检查连接池状态
    if (!pool.aliving()) {
        // 连接池处于故障状态
        logger_error("连接池故障: %s", pool.get_addr());
        
        // 可以尝试其他连接池或降级处理
        return handle_pool_failure();
    }
    
    // 连接数达到上限，等待后重试
    sleep(1);
    conn = (acl::redis_client*) pool.peek();
    if (conn == NULL) {
        logger_error("连接池繁忙，无法获取连接");
        return handle_pool_busy();
    }
}
```

### 命令执行失败处理

```cpp
bool safe_execute(acl::redis_client* conn, const char* key, const char* value) {
    acl::redis_string cmd(conn);
    
    if (!cmd.set(key, value)) {
        const char* error = cmd.result_error();
        
        // 根据错误类型决定是否保持连接
        if (strstr(error, "READONLY") != NULL) {
            // Redis 只读错误，连接正常
            return false;
        } else if (strstr(error, "MOVED") != NULL ||
                   strstr(error, "ASK") != NULL) {
            // 集群重定向，连接正常
            return false;
        } else if (strstr(error, "timeout") != NULL ||
                   strstr(error, "connection") != NULL) {
            // 连接异常，应该关闭
            pool.put(conn, false);
            return false;
        }
        
        // 其他错误，保持连接
        return false;
    }
    
    return true;
}
```

### 异常安全保证

```cpp
void exception_safe_example(acl::redis_client_pool& pool) {
    acl::redis_client* conn = NULL;
    
    try {
        conn = (acl::redis_client*) pool.peek();
        if (conn == NULL) {
            throw std::runtime_error("无法获取连接");
        }
        
        // 执行业务逻辑
        acl::redis_string cmd(conn);
        cmd.set("key", "value");
        
        // 正常归还
        pool.put(conn, true);
        
    } catch (const std::exception& e) {
        // 异常时确保连接被归还或关闭
        if (conn) {
            pool.put(conn, false);  // 不保持连接
        }
        
        logger_error("执行异常: %s", e.what());
        throw;  // 重新抛出
    }
}
```

---

## 性能优化

### 1. 连接预热

```cpp
// 启动时预热连接池
void warmup_pools(acl::redis_client_pool& pool) {
    printf("开始预热连接池...\n");
    
    size_t min_conns = 50;
    pool.set_conns_min(min_conns);
    
    // 立即创建连接
    std::vector<acl::redis_client*> conns;
    for (size_t i = 0; i < min_conns; i++) {
        acl::redis_client* conn = (acl::redis_client*) pool.peek();
        if (conn) {
            conns.push_back(conn);
        }
    }
    
    // 归还所有连接
    for (auto conn : conns) {
        pool.put(conn, true);
    }
    
    printf("连接池预热完成，当前连接数: %lu\n",
           (unsigned long)pool.get_count());
}
```

### 2. 批量操作

```cpp
// 使用 Pipeline 批量执行
void batch_operation(acl::redis_client* conn) {
    acl::redis_pipeline pipeline(conn);
    
    // 批量添加命令
    for (int i = 0; i < 1000; i++) {
        acl::string key, value;
        key.format("key_%d", i);
        value.format("value_%d", i);
        
        pipeline.set(key, value);
    }
    
    // 一次性执行
    std::vector<acl::redis_result*> results;
    pipeline.flush(results);
    
    printf("批量执行了 %lu 个命令\n", (unsigned long)results.size());
}
```

### 3. 线程绑定模式

```cpp
// 在协程环境中启用线程绑定
void fiber_optimized() {
    acl::redis_client_cluster cluster;
    cluster.bind_thread(true);  // 关键优化
    
    // 每个协程线程独立的连接池
    // 减少锁竞争，提高性能
    
    cluster.set_all_slot("127.0.0.1:7000", 100, 10, 10);
    
    // 正常使用，性能更好
    acl::redis_string cmd(&cluster);
    cmd.set("key", "value");
}
```

### 4. 减少连接创建销毁

```cpp
// 不好的做法：频繁创建销毁连接池
void bad_practice() {
    for (int i = 0; i < 1000; i++) {
        acl::redis_client_pool pool("127.0.0.1:6379", 10);
        acl::redis_client* conn = (acl::redis_client*) pool.peek();
        // ... 使用 ...
        pool.put(conn, true);
        // pool 析构，连接全部销毁
    }
}

// 好的做法：复用连接池
void good_practice() {
    static acl::redis_client_pool pool("127.0.0.1:6379", 100);
    
    for (int i = 0; i < 1000; i++) {
        acl::redis_client* conn = (acl::redis_client*) pool.peek();
        // ... 使用 ...
        pool.put(conn, true);
        // 连接被复用
    }
}
```

### 5. 使用内存池

```cpp
// Redis 操作使用内存池
void use_memory_pool() {
    acl::redis_client* conn = get_connection();
    
    // 创建内存池
    acl::dbuf_pool pool;
    
    for (int i = 0; i < 10000; i++) {
        acl::string request;
        request.format("GET key_%d\r\n", i);
        
        // 使用内存池，避免频繁 malloc/free
        const acl::redis_result* result = conn->run(&pool, request, 1);
        
        // 处理结果...
    }
    
    // pool 析构时统一释放内存
}
```

---

## 监控和诊断

### 连接池状态监控

```cpp
class PoolMonitor {
public:
    void monitor(acl::connect_manager& manager) {
        std::vector<acl::connect_pool*>& pools = manager.get_pools();
        
        for (auto pool : pools) {
            printf("========================================\n");
            printf("连接池: %s\n", pool->get_addr());
            printf("  最大连接数: %lu\n", (unsigned long)pool->get_max());
            printf("  当前连接数: %lu\n", (unsigned long)pool->get_count());
            printf("  总使用次数: %llu\n", pool->get_total_used());
            printf("  当前周期使用: %llu\n", pool->get_current_used());
            printf("  连接池状态: %s\n", pool->aliving() ? "正常" : "故障");
            
            // 计算使用率
            double usage_rate = pool->get_max() > 0 ?
                (double)pool->get_count() / pool->get_max() * 100 : 0;
            printf("  使用率: %.2f%%\n", usage_rate);
            
            if (usage_rate > 80) {
                printf("  [警告] 使用率过高！\n");
            }
        }
    }
};

// 定期监控
void start_monitoring(acl::connect_manager& manager) {
    PoolMonitor monitor;
    
    while (true) {
        sleep(60);  // 每分钟监控一次
        monitor.monitor(manager);
    }
}
```

### 性能指标统计

```cpp
class PerformanceStats {
public:
    void record_operation(bool success, double latency_ms) {
        total_ops_++;
        
        if (success) {
            success_ops_++;
            total_latency_ += latency_ms;
            
            if (latency_ms > max_latency_) {
                max_latency_ = latency_ms;
            }
            if (latency_ms < min_latency_ || min_latency_ == 0) {
                min_latency_ = latency_ms;
            }
        } else {
            failed_ops_++;
        }
    }
    
    void print_stats() {
        printf("总操作数: %lu\n", total_ops_);
        printf("成功: %lu (%.2f%%)\n", success_ops_,
               (double)success_ops_ / total_ops_ * 100);
        printf("失败: %lu (%.2f%%)\n", failed_ops_,
               (double)failed_ops_ / total_ops_ * 100);
        
        if (success_ops_ > 0) {
            printf("平均延迟: %.2f ms\n", total_latency_ / success_ops_);
            printf("最大延迟: %.2f ms\n", max_latency_);
            printf("最小延迟: %.2f ms\n", min_latency_);
        }
    }
    
private:
    size_t total_ops_ = 0;
    size_t success_ops_ = 0;
    size_t failed_ops_ = 0;
    double total_latency_ = 0;
    double max_latency_ = 0;
    double min_latency_ = 0;
};
```

### 日志记录

```cpp
// 记录连接获取和归还
class LoggedPool : public acl::redis_client_pool {
public:
    LoggedPool(const char* addr, size_t max)
        : redis_client_pool(addr, max) {}
    
    acl::connect_client* peek(bool on = true) {
        struct timeval start;
        gettimeofday(&start, NULL);
        
        acl::connect_client* conn = redis_client_pool::peek(on);
        
        struct timeval end;
        gettimeofday(&end, NULL);
        
        double elapsed = (end.tv_sec - start.tv_sec) * 1000.0 +
                        (end.tv_usec - start.tv_usec) / 1000.0;
        
        if (elapsed > 100) {  // 超过100ms记录警告
            logger_warn("获取连接耗时过长: %.2f ms", elapsed);
        }
        
        return conn;
    }
};
```

---

## 常见问题

### Q1: 为什么获取连接返回 NULL？

**可能原因**：
1. 连接数达到上限 (`max_` 限制)
2. 连接池处于故障状态 (`alive_ = false`)
3. 服务器无法连接

**解决方法**：
```cpp
conn = pool.peek();
if (conn == NULL) {
    // 检查原因
    if (!pool.aliving()) {
        // 连接池故障，检查网络和服务器
        logger_error("连接池故障: %s", pool.get_addr());
    } else if (pool.get_count() >= pool.get_max()) {
        // 连接数达到上限，增加最大连接数
        logger_warn("连接池已满，当前: %lu, 最大: %lu",
                    pool.get_count(), pool.get_max());
    }
}
```

### Q2: 连接池出现内存泄漏？

**可能原因**：
1. 获取连接后忘记归还
2. 异常时未正确释放连接

**解决方法**：
```cpp
// 使用 RAII 保证连接归还
acl::connect_guard guard(pool);
acl::redis_client* conn = (acl::redis_client*) guard.peek();
// guard 析构时自动归还

// 或者使用 try-catch 确保归还
try {
    conn = pool.peek();
    // ... 使用 ...
    pool.put(conn, true);
} catch (...) {
    if (conn) pool.put(conn, false);
    throw;
}
```

### Q3: 连接池性能不如预期？

**检查清单**：
```cpp
// 1. 连接数是否足够
size_t count = pool.get_count();
size_t max = pool.get_max();
if (count >= max * 0.8) {
    // 使用率过高，增加最大连接数
}

// 2. 是否启用线程绑定（协程环境）
cluster.bind_thread(true);

// 3. 是否进行连接预热
pool.set_conns_min(50);
pool.keep_conns();

// 4. 超时时间是否合理
pool.set_timeout(5, 3);  // 不要设置太长

// 5. 是否批量操作
// 使用 Pipeline 而不是逐个执行
```

### Q4: 集群重定向过多？

**可能原因**：
1. 集群拓扑未正确配置
2. 数据迁移中
3. 节点故障

**解决方法**：
```cpp
cluster.set_redirect_max(20);      // 增加重定向次数
cluster.set_redirect_sleep(1000);  // 增加重定向休眠时间

// 定期更新集群拓扑
void update_cluster_topology() {
    cluster.set_all_slot("127.0.0.1:7000", 100, 10, 10);
}
```

### Q5: 连接频繁超时？

**检查和优化**：
```cpp
// 1. 检查网络延迟
// ping 服务器查看延迟

// 2. 增加超时时间
pool.set_timeout(10, 10);

// 3. 检查服务器负载
// 是否 Redis 服务器过载

// 4. 使用慢查询日志
// 分析是否有慢查询

// 5. 启用连接健康检查
monitor.set_check_conns(true, true, true);
```

### Q6: 如何实现连接池的优雅重启？

```cpp
void graceful_restart() {
    // 1. 停止接受新请求
    stop_accepting_requests();
    
    // 2. 等待现有请求完成
    wait_for_active_requests();
    
    // 3. 停止监控线程
    manager.stop_monitor(true);  // 优雅停止
    
    // 4. 等待所有连接归还
    while (pool.get_count() > 0) {
        sleep(1);
        printf("等待连接归还: %lu\n", pool.get_count());
    }
    
    // 5. 重新初始化连接池
    init_pools();
    
    // 6. 恢复服务
    start_accepting_requests();
}
```

---

## 生产环境检查清单

### 启动前检查

- [ ] 连接池大小配置合理
- [ ] 超时时间根据业务设置
- [ ] 启用连接预热
- [ ] 配置监控和日志
- [ ] 测试故障恢复机制

### 运行时监控

- [ ] 监控连接池使用率
- [ ] 监控平均响应时间
- [ ] 监控错误率
- [ ] 监控连接创建销毁频率
- [ ] 监控服务器健康状态

### 告警规则

```cpp
// 使用率告警
if (pool.get_count() >= pool.get_max() * 0.9) {
    send_alert("连接池使用率超过90%");
}

// 错误率告警
if (failed_ops > total_ops * 0.05) {
    send_alert("连接池错误率超过5%");
}

// 响应时间告警
if (avg_latency > 100) {
    send_alert("连接池平均响应时间超过100ms");
}
```

---

## 总结

遵循以下原则可以最大化 ACL 连接池的效能：

1. **合理配置**：根据业务特点配置连接数和超时
2. **连接预热**：提前创建连接，避免冷启动
3. **及时归还**：使用 RAII 或 try-catch 确保连接归还
4. **健康检查**：启用监控，及时发现和处理问题
5. **性能优化**：批量操作、线程绑定、内存池
6. **异常处理**：完善的错误处理和降级方案
7. **监控告警**：实时监控关键指标，及时发现异常

通过遵循这些最佳实践，可以构建高性能、高可用的连接池系统。

