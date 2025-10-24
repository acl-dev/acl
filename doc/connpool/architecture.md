# ACL 连接池架构设计

## 整体分层架构

```
┌─────────────────────────────────────────────────────────────────────┐
│                         应用层 (Application Layer)                   │
│  ┌────────────┐  ┌────────────┐  ┌────────────┐                    │
│  │  业务代码  │  │  业务代码  │  │  业务代码  │  ...               │
│  │  Thread 1  │  │  Thread 2  │  │  Thread 3  │                    │
│  └─────┬──────┘  └─────┬──────┘  └─────┬──────┘                    │
│        │ peek()         │ peek()         │ peek()                   │
└────────┼────────────────┼────────────────┼───────────────────────────┘
         │                │                │
         ↓                ↓                ↓
┌─────────────────────────────────────────────────────────────────────┐
│               连接池管理层 (Pool Manager Layer)                      │
│                                                                       │
│  ┌────────────────────────────────────────────────────────────────┐ │
│  │              connect_manager (连接池管理器)                     │ │
│  │  - 管理多个服务器的连接池集合                                  │ │
│  │  - 提供负载均衡（轮询/哈希）                                   │ │
│  │  - 支持线程绑定模式                                            │ │
│  │  - 动态增删服务器节点                                          │ │
│  └────────────────────────────────────────────────────────────────┘ │
│         │ 管理                                                        │
│         ↓                                                             │
│  [pool_1] [pool_2] [pool_3] ... [pool_N]                           │
└─────────────────────────────────────────────────────────────────────┘
         │
         ↓
┌─────────────────────────────────────────────────────────────────────┐
│                  连接池层 (Connection Pool Layer)                    │
│                                                                       │
│  ┌────────────────────────────────────────────────────────────────┐ │
│  │              connect_pool (单服务器连接池)                      │ │
│  │  - 管理单个服务器的连接集合                                    │ │
│  │  - 实现连接复用（对象池模式）                                  │ │
│  │  - 连接健康检查                                                │ │
│  │  - 引用计数管理                                                │ │
│  │  - 自动故障恢复                                                │ │
│  └────────────────────────────────────────────────────────────────┘ │
│         │ 管理                                                        │
│         ↓                                                             │
│  空闲连接队列: [conn_1] → [conn_2] → [conn_3] → ...               │
└─────────────────────────────────────────────────────────────────────┘
         │
         ↓
┌─────────────────────────────────────────────────────────────────────┐
│                   连接层 (Connection Layer)                          │
│                                                                       │
│  ┌────────────────────────────────────────────────────────────────┐ │
│  │              connect_client (连接客户端)                        │ │
│  │  - 封装单个网络连接                                            │ │
│  │  - 实现协议通信逻辑                                            │ │
│  │  - 连接建立与关闭                                              │ │
│  │  - 连接存活检测                                                │ │
│  └────────────────────────────────────────────────────────────────┘ │
│         │                                                             │
│         ↓                                                             │
│  [TCP Socket] → [服务器]                                            │
└─────────────────────────────────────────────────────────────────────┘
```

## 核心组件详解

### 1. connect_client - 连接客户端基类

**文件位置**：`lib_acl_cpp/include/acl_cpp/connpool/connect_client.hpp`

#### 核心成员

```cpp
class connect_client : public noncopyable {
protected:
    int   conn_timeout_;      // 连接超时（秒）
    int   rw_timeout_;        // 读写超时（秒）
    time_t when_;             // 最后使用时间
    connect_pool* pool_;      // 所属连接池
    
public:
    // 纯虚函数，必须由子类实现
    virtual bool open() = 0;   // 建立连接
    virtual bool alive();      // 检测连接是否存活（可选）
};
```

#### 设计要点

1. **纯虚基类**：定义连接接口标准
2. **记录使用时间**：用于空闲连接检测
3. **关联连接池**：方便连接归还和管理
4. **不可拷贝**：继承 `noncopyable`，避免意外拷贝

#### 典型实现

```cpp
// TCP 客户端实现
class tcp_client : public connect_client {
private:
    socket_stream* conn_;  // TCP 连接
    char* addr_;           // 服务器地址
    
public:
    virtual bool open() {
        conn_ = new socket_stream();
        return conn_->open(addr_, conn_timeout_, rw_timeout_);
    }
};

// Redis 客户端实现
class redis_client : public connect_client {
private:
    socket_stream conn_;   // TCP 连接
    char* addr_;           // 服务器地址
    char* pass_;           // 认证密码
    int dbnum_;            // 数据库编号
    
public:
    virtual bool open() {
        if (!conn_.open(addr_, conn_timeout_, rw_timeout_)) {
            return false;
        }
        // 认证
        if (pass_ && !auth(pass_)) {
            return false;
        }
        // 选择数据库
        if (dbnum_ > 0 && !select(dbnum_)) {
            return false;
        }
        return true;
    }
};
```

---

### 2. connect_pool - 单服务器连接池

**文件位置**：`lib_acl_cpp/include/acl_cpp/connpool/connect_pool.hpp`

#### 核心成员

```cpp
class connect_pool : public noncopyable {
protected:
    // 连接池状态
    bool  alive_;              // 连接池是否可用
    ssize_t refers_;           // 引用计数
    bool  delay_destroy_;      // 延迟销毁标志
    
    // 服务器信息
    char  addr_[256];          // 服务器地址
    size_t idx_;               // 连接池索引
    
    // 连接数限制
    size_t max_;               // 最大连接数
    size_t min_;               // 最小连接数
    size_t count_;             // 当前连接总数
    
    // 超时配置
    int   conn_timeout_;       // 连接超时（秒）
    int   rw_timeout_;         // 读写超时（秒）
    time_t idle_ttl_;          // 空闲连接TTL
    
    // 故障恢复
    int   retry_inter_;        // 失败重试间隔（秒）
    time_t last_dead_;         // 上次故障时间
    
    // 连接队列
    std::list<connect_client*> pool_;  // 空闲连接队列
    locker lock_;              // 互斥锁
    
    // 统计信息
    unsigned long long total_used_;    // 总使用次数
    unsigned long long current_used_;  // 当前周期使用次数
    
public:
    // 核心方法
    connect_client* peek(bool on = true);
    void put(connect_client* conn, bool keep = true,
             cpool_put_oper_t oper = cpool_put_check_idle);
    size_t check_idle(time_t ttl);
    size_t check_dead(thread_pool* threads = NULL);
    void keep_conns(thread_pool* threads = NULL);
    
protected:
    // 工厂方法
    virtual connect_client* create_connect() = 0;
};
```

#### 连接获取流程 (peek)

```cpp
connect_client* connect_pool::peek(bool on) {
    // 1. 检查连接池是否可用
    if (!aliving()) {
        return NULL;  // 连接池故障且未到重试时间
    }
    
    // 2. 尝试从空闲队列获取连接
    lock_.lock();
    connect_client* conn = NULL;
    if (!pool_.empty()) {
        conn = pool_.back();    // 取最旧的连接
        pool_.pop_back();
        count_--;
    }
    lock_.unlock();
    
    // 3. 没有空闲连接且允许创建新连接
    if (conn == NULL && on) {
        if (max_ > 0 && count_ >= max_) {
            return NULL;  // 达到最大连接数
        }
        
        // 创建新连接
        conn = create_connect();  // 调用子类工厂方法
        conn->set_pool(this);
        conn->set_timeout(conn_timeout_, rw_timeout_);
        count_++;
    }
    
    // 4. 建立连接
    if (conn && !conn->open()) {
        delete conn;
        count_--;
        set_alive(false);  // 标记连接池为故障状态
        return NULL;
    }
    
    // 5. 更新使用时间和统计
    if (conn) {
        conn->set_when(time(NULL));
        total_used_++;
        current_used_++;
    }
    
    return conn;
}
```

#### 连接归还流程 (put)

```cpp
void connect_pool::put(connect_client* conn, bool keep,
                        cpool_put_oper_t oper) {
    if (conn == NULL) return;
    
    // 1. 更新最后使用时间
    conn->set_when(time(NULL));
    
    // 2. 不保持连接则直接销毁
    if (!keep) {
        delete conn;
        lock_.lock();
        count_--;
        lock_.unlock();
        return;
    }
    
    // 3. 根据操作标志执行维护
    if (oper & cpool_put_check_idle) {
        check_idle(idle_ttl_);      // 检查并清理空闲连接
    }
    if (oper & cpool_put_check_dead) {
        check_dead(NULL);           // 检查并清理死连接
    }
    if (oper & cpool_put_keep_conns) {
        keep_conns(NULL);           // 保持最小连接数
    }
    
    // 4. 将连接放回队首
    lock_.lock();
    pool_.push_front(conn);  // 最近使用的连接放队首
    lock_.unlock();
}
```

#### 连接复用策略

采用 **混合策略**：
- **获取**：从队尾取出（最旧的连接）
- **归还**：放到队首（最近使用的）

**优点**：
1. 优先使用旧连接，新连接作为后备
2. 长期未用的连接会被 `check_idle()` 清理
3. 保持活跃连接的热度，减少失效概率

```
队列状态演示：

初始状态:
[conn_A(旧)] → [conn_B] → [conn_C] → [conn_D(新)]
 队首                                    队尾

peek() - 从队尾取出最旧的连接:
[conn_A] → [conn_B] → [conn_C]          conn_D ← 被取出使用

put(conn_D) - 放回队首:
[conn_D(刚用)] → [conn_A] → [conn_B] → [conn_C]
```

---

### 3. connect_manager - 连接池管理器

**文件位置**：`lib_acl_cpp/include/acl_cpp/connpool/connect_manager.hpp`

#### 核心成员

```cpp
class connect_manager : public noncopyable {
protected:
    // 线程绑定开关
    bool thread_binding_;
    
    // 服务器配置
    std::map<string, conn_config> addrs_;  // 地址 → 配置
    connect_pool* default_pool_;            // 默认连接池
    
    // 连接池集合（按线程ID组织）
    manager_t manager_;  // map<thread_id, conns_pools*>
    
    // 全局配置
    int  retry_inter_;   // 失败重试间隔
    time_t idle_ttl_;    // 空闲连接TTL
    int  check_inter_;   // 检查间隔
    
    // 监控器
    connect_monitor* monitor_;
    
    // 互斥锁
    locker lock_;
    
public:
    // 初始化
    void init(const char* default_addr, const char* addr_list, ...);
    void set(const char* addr, size_t max, ...);
    void remove(const char* addr);
    
    // 获取连接池
    connect_pool* peek();                  // 轮询
    connect_pool* peek(const char* key);   // 哈希
    connect_pool* get(const char* addr);   // 指定地址
    
    // 批量检查
    size_t check_idle_conns(size_t step);
    size_t check_dead_conns(size_t step);
    size_t keep_min_conns(size_t step);
    
    // 监控管理
    bool start_monitor(connect_monitor* monitor);
    connect_monitor* stop_monitor(bool graceful);
    
protected:
    // 工厂方法
    virtual connect_pool* create_pool(const char* addr,
                                      size_t count, size_t idx) = 0;
};
```

#### 数据结构

```cpp
// 连接池集合
struct conns_pools {
    std::vector<connect_pool*> pools;  // 连接池数组
    size_t check_next;                 // 下次检查的索引
    size_t conns_next;                 // 下次获取连接的索引
};

// 服务器配置
struct conn_config {
    string addr;           // 服务器地址
    size_t max;            // 最大连接数
    size_t min;            // 最小连接数
    int conn_timeout;      // 连接超时
    int rw_timeout;        // 读写超时
    bool sockopt_timeo;    // 是否使用 setsockopt 设置超时
};
```

#### 线程绑定机制

**共享模式** (`thread_binding_ = false`)：
```
manager_[0] → conns_pools {
    pools: [pool_A, pool_B, pool_C]  ← 所有线程共享
}
```

**绑定模式** (`thread_binding_ = true`)：
```
manager_[thread_1] → conns_pools {
    pools: [pool_A1, pool_B1, pool_C1]  ← 线程1独享
}

manager_[thread_2] → conns_pools {
    pools: [pool_A2, pool_B2, pool_C2]  ← 线程2独享
}

manager_[thread_3] → conns_pools {
    pools: [pool_A3, pool_B3, pool_C3]  ← 线程3独享
}
```

**优点**：
- 减少锁竞争
- 提高缓存局部性
- 适合协程环境

#### 负载均衡策略

**1. 轮询方式** (Round Robin)

```cpp
connect_pool* connect_manager::peek() {
    conns_pools& pools = get_pools_by_id(get_thread_id());
    
    lock_.lock();
    size_t idx = pools.conns_next++ % pools.pools.size();
    connect_pool* pool = pools.pools[idx];
    lock_.unlock();
    
    return pool;
}
```

示例：
```
pools: [A, B, C, D]
       ↑
第1次: conns_next=0 → 返回 A
第2次: conns_next=1 → 返回 B
第3次: conns_next=2 → 返回 C
第4次: conns_next=3 → 返回 D
第5次: conns_next=4 % 4 = 0 → 返回 A  (循环)
```

**2. 哈希方式** (Key-based Hash)

```cpp
connect_pool* connect_manager::peek(const char* key) {
    if (key == NULL) {
        return peek();  // 退化为轮询
    }
    
    conns_pools& pools = get_pools_by_id(get_thread_id());
    
    // 使用 CRC32 哈希算法
    unsigned hash = acl_hash_crc32(key, strlen(key));
    size_t idx = hash % pools.pools.size();
    
    return pools.pools[idx];
}
```

示例：
```
key = "user:1000"
hash = CRC32("user:1000") = 0x12345678
idx = 0x12345678 % 4 = 0
返回 pools[0]

key = "user:1001"
hash = CRC32("user:1001") = 0xABCDEF00
idx = 0xABCDEF00 % 4 = 2
返回 pools[2]
```

**优点**：相同 key 总是路由到同一节点，有利于缓存

**3. 指定地址**

```cpp
connect_pool* connect_manager::get(const char* addr) {
    conns_pools& pools = get_pools_by_id(get_thread_id());
    
    // 遍历查找匹配地址的连接池
    for (size_t i = 0; i < pools.pools.size(); i++) {
        if (strcmp(pools.pools[i]->get_addr(), addr) == 0) {
            return pools.pools[i];
        }
    }
    
    return NULL;
}
```

---

### 4. connect_monitor - 连接池监控器

**文件位置**：`lib_acl_cpp/include/acl_cpp/connpool/connect_monitor.hpp`

#### 核心成员

```cpp
class connect_monitor : public thread {
private:
    connect_manager& manager_;  // 管理的连接池管理器
    
    // 监控配置
    bool  check_server_;        // 是否检测服务器连通性
    int   check_inter_;         // 检查间隔（秒）
    int   conn_timeout_;        // 连接超时
    
    // 检查开关
    bool  check_idle_;          // 是否清理空闲连接
    bool  kick_dead_;           // 是否踢除死连接
    bool  keep_conns_;          // 是否保持最小连接数
    size_t check_step_;         // 每次检查的连接池数量
    
    // 线程池（用于并发检测）
    thread_pool* threads_;
    
    // 控制标志
    bool stop_;                 // 是否停止
    bool stop_graceful_;        // 是否优雅停止
    
protected:
    // 线程主函数
    virtual void* run();
    
    // 检测回调（可重写）
    virtual void nio_check(check_client& checker, aio_socket_stream& conn);
    virtual void sio_check(check_client& checker, socket_stream& conn);
    
public:
    // 配置方法
    connect_monitor& set_check_inter(int n);
    connect_monitor& set_check_conns(bool check_idle, bool kick_dead,
                                      bool keep_conns, ...);
    
    // 控制方法
    void stop(bool graceful);
};
```

#### 工作流程

```cpp
void* connect_monitor::run() {
    while (!stop_) {
        // 1. 休眠到下次检查时间
        sleep(check_inter_);
        
        if (stop_) break;
        
        // 2. 批量检查连接池
        size_t left = manager_.size();
        while (left > 0 && !stop_) {
            size_t step = check_step_ > 0 ? check_step_ : left;
            
            // 检查空闲连接
            if (check_idle_) {
                manager_.check_idle_conns(step, &left);
            }
            
            // 检查死连接
            if (kick_dead_) {
                manager_.check_dead_conns(step, &left);
            }
            
            // 保持最小连接数
            if (keep_conns_) {
                manager_.keep_min_conns(step);
            }
        }
        
        // 3. 检测服务器连通性（可选）
        if (check_server_) {
            std::vector<string> addrs;
            // 获取所有服务器地址
            for each addr {
                // 异步连接测试
                aio_connect(addr, conn_timeout_);
            }
        }
    }
    
    return NULL;
}
```

#### 分批检查的意义

```
假设有 100 个连接池，check_step_ = 10

第1轮: 检查 pools[0..9]     耗时约 100ms
      sleep(check_inter_)
      
第2轮: 检查 pools[10..19]   耗时约 100ms
      sleep(check_inter_)
      
...

第10轮: 检查 pools[90..99]  耗时约 100ms

总共分 10 轮完成，每轮只占用很少CPU时间
避免一次性检查 100 个连接池造成长时间停顿
```

---

## Redis 模块继承示例

### 继承关系图

```
connpool 基础框架                Redis 模块实现
─────────────────                ───────────────

connect_client                   redis_client
      ↑                                ↑
      └────────────────────────────────┘
                 继承
                 
connect_pool                     redis_client_pool
      ↑                                ↑
      └────────────────────────────────┘
                 继承
                 
connect_manager                  redis_client_cluster
      ↑                                ↑
      └────────────────────────────────┘
                 继承
```

### 实现示例

#### 1. redis_client

```cpp
class redis_client : public connect_client {
private:
    socket_stream conn_;   // TCP 连接
    char* addr_;           // 服务器地址
    char* pass_;           // 认证密码
    int dbnum_;            // 数据库编号
    
protected:
    // 实现 open() 方法
    virtual bool open() {
        // 1. 建立 TCP 连接
        if (!conn_.open(addr_, conn_timeout_, rw_timeout_)) {
            return false;
        }
        
        // 2. 认证（如果有密码）
        if (pass_ && *pass_) {
            if (!auth(pass_)) {
                conn_.close();
                return false;
            }
        }
        
        // 3. 选择数据库（非集群模式）
        if (dbnum_ > 0) {
            if (!select_db(dbnum_)) {
                conn_.close();
                return false;
            }
        }
        
        return true;
    }
    
public:
    // Redis 专用方法
    const redis_result* run(const string& req, size_t nchildren);
};
```

#### 2. redis_client_pool

```cpp
class redis_client_pool : public connect_pool {
private:
    char* pass_;           // 密码（传递给连接）
    int dbnum_;            // 数据库编号
    
protected:
    // 实现工厂方法
    virtual connect_client* create_connect() {
        // 创建 redis_client 实例
        redis_client* conn = new redis_client(
            addr_,           // 继承自 connect_pool
            conn_timeout_,   // 继承自 connect_pool
            rw_timeout_      // 继承自 connect_pool
        );
        
        // 设置 Redis 特定参数
        if (pass_) {
            conn->set_password(pass_);
        }
        if (dbnum_ > 0) {
            conn->set_db(dbnum_);
        }
        
        return conn;
    }
    
public:
    // 配置方法
    redis_client_pool& set_password(const char* pass);
    redis_client_pool& set_db(int dbnum);
};
```

#### 3. redis_client_cluster

```cpp
class redis_client_cluster : public connect_manager {
private:
    int max_slot_;                      // 最大槽位数（16384）
    const char** slot_addrs_;           // 槽位到地址映射
    std::map<string, string> passwds_;  // 节点密码
    int redirect_max_;                  // 最大重定向次数
    
protected:
    // 实现工厂方法
    virtual connect_pool* create_pool(const char* addr,
                                      size_t count, size_t idx) {
        // 创建 redis_client_pool 实例
        redis_client_pool* pool = new redis_client_pool(addr, count, idx);
        
        // 设置密码（如果有）
        const char* pass = get_password(addr);
        if (pass) {
            pool->set_password(pass);
        }
        
        return pool;
    }
    
public:
    // 集群专用方法
    void set_all_slot(const char* addr, size_t max_conns, ...);
    void set_slot(int slot, const char* addr);
    redis_client_pool* peek_slot(int slot);
    redis_client* redirect(const char* addr, size_t max_conns);
};
```

---

## 线程安全设计

### 锁的层次

```
connect_manager
  └─ lock_ (全局锁)
      - 保护 manager_ 映射表
      - 保护 addrs_ 配置表
      - 动态增删服务器时使用
      
      ├─ connect_pool (多个实例，每个有独立锁)
      │   └─ lock_ (连接池锁)
      │       - 保护 pool_ 连接队列
      │       - 保护 count_, alive_ 等状态
      │       - peek()/put() 时使用
      │
      └─ connect_pool (另一个实例)
          └─ lock_ (独立的连接池锁)
```

### 锁的使用原则

1. **最小化锁范围**
   ```cpp
   // 只在必要时加锁
   lock_.lock();
   conn = pool_.back();
   pool_.pop_back();
   lock_.unlock();  // 尽快释放
   
   // 不在锁内执行耗时操作
   conn->open();    // 在锁外进行网络IO
   ```

2. **避免嵌套锁**
   ```cpp
   // 不要在持有一个锁时去获取另一个锁
   // 可能导致死锁
   ```

3. **线程绑定优化**
   ```cpp
   // 每个线程独立的连接池集合
   // 大部分时候不需要加锁
   ```

4. **无锁快速路径**
   ```cpp
   // 状态检查不加锁（volatile 读取）
   if (!alive_) {
       return NULL;
   }
   ```

---

## 性能优化技术

### 1. 对象复用

避免频繁 `new/delete`：
```cpp
// 不好的做法
for (int i = 0; i < 10000; i++) {
    conn = new redis_client();
    conn->open();
    // ... 使用 ...
    delete conn;  // 频繁创建销毁
}

// 好的做法
for (int i = 0; i < 10000; i++) {
    conn = pool.peek();  // 复用连接
    // ... 使用 ...
    pool.put(conn);      // 归还复用
}
```

### 2. 连接预热

提前创建连接，避免冷启动：
```cpp
pool.set_conns_min(20);   // 设置最小连接数
pool.keep_conns();         // 立即预热

// 内部会创建 20 个连接放入池中
// 后续 peek() 直接从池中获取，无需等待连接建立
```

### 3. 批量操作

分批检查避免停顿：
```cpp
// 假设有 1000 个连接池
// 一次性检查会阻塞很长时间

// 分10批检查，每批100个
for (int i = 0; i < 10; i++) {
    manager.check_idle_conns(100);
    // 让出CPU，避免长时间占用
}
```

### 4. 异步检测

使用线程池并发检测：
```cpp
thread_pool pool(10);  // 10个工作线程

// 将检测任务分配到线程池
pool.check_dead(&thread_pool);

// 内部实现
for (auto conn : conns) {
    check_job* job = new check_job(conn);
    thread_pool.execute(job);  // 并发执行
}
```

### 5. 减少内存分配

使用内存池：
```cpp
// redis_result 使用 dbuf_pool 内存池
dbuf_pool pool;
const redis_result* result = client.run(&pool, request);

// result 对象从 pool 分配
// pool 析构时统一释放，避免频繁 malloc/free
```

---

## 配置参数详解

| 参数名 | 类型 | 默认值 | 说明 | 推荐值 |
|--------|------|--------|------|--------|
| **连接池大小** |||||
| `max_` | size_t | 0 (无限) | 单个连接池最大连接数 | 并发数×1.5 |
| `min_` | size_t | 0 | 单个连接池最小连接数 | max÷10 |
| **超时时间** |||||
| `conn_timeout_` | int | 30秒 | TCP连接建立超时 | 3-10秒 |
| `rw_timeout_` | int | 30秒 | 读写操作超时 | 1-5秒 |
| `idle_ttl_` | time_t | -1 (永不) | 空闲连接存活时间 | 300秒 |
| **故障恢复** |||||
| `retry_inter_` | int | 1秒 | 连接失败后重试间隔 | 5-60秒 |
| **健康检查** |||||
| `check_inter_` | int | 30秒 | 连接检查时间间隔 | 30-300秒 |
| `check_step_` | size_t | 0 (全部) | 每次检查连接池数量 | 总数÷10 |
| **监控开关** |||||
| `check_server_` | bool | false | 是否主动检测服务器 | false |
| `check_idle_` | bool | false | 是否清理空闲连接 | true |
| `kick_dead_` | bool | false | 是否踢除死连接 | true |
| `keep_conns_` | bool | false | 是否保持最小连接数 | true |

### 典型场景配置

#### 高并发短连接场景
```cpp
pool.set_timeout(3, 3)        // 快速超时
    .set_conns_min(50)         // 大量预热
    .set_idle_ttl(60)          // 短TTL，快速释放
    .set_check_inter(30);      // 频繁检查
```

#### 低并发长连接场景
```cpp
pool.set_timeout(10, 30)      // 宽松超时
    .set_conns_min(5)          // 少量预热
    .set_idle_ttl(3600)        // 长TTL，保持连接
    .set_check_inter(300);     // 较少检查
```

#### 关键业务场景
```cpp
pool.set_retry_inter(-1);     // 不自动恢复，人工介入
    .set_conns_min(100);       // 大量预热，确保可用
```

---

## 总结

ACL 连接池的架构设计体现了以下特点：

1. **清晰的分层**：应用层 → 管理层 → 连接池层 → 连接层
2. **灵活的扩展**：基于抽象类和工厂模式，易于实现新类型
3. **完善的管理**：连接复用、健康检查、故障恢复、负载均衡
4. **高性能优化**：对象复用、连接预热、批量操作、异步检测
5. **线程安全**：细粒度锁、线程绑定、无锁快速路径
6. **易于使用**：丰富的配置选项、RAII管理、友好的API

这是一个成熟的工业级连接池实现，适用于各种网络服务场景。

