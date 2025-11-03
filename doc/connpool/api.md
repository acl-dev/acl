# ACL 连接池 API 参考

本文档提供 ACL 连接池模块的完整 API 参考。

## 目录

- [connect_client](#connect_client)
- [connect_pool](#connect_pool)
- [connect_manager](#connect_manager)
- [connect_monitor](#connect_monitor)
- [辅助类](#辅助类)

---

## connect_client

**文件位置**：`lib_acl_cpp/include/acl_cpp/connpool/connect_client.hpp`

连接客户端基类，封装单个网络连接。

### 构造函数

```cpp
connect_client();
```

### 纯虚函数（必须实现）

#### open

```cpp
virtual bool open() = 0;
```

建立到服务器的连接。

**返回值**：
- `true` - 连接成功
- `false` - 连接失败

**子类实现示例**：
```cpp
bool redis_client::open() {
    if (!conn_.open(addr_, conn_timeout_, rw_timeout_)) {
        return false;
    }
    if (pass_ && !auth(pass_)) {
        return false;
    }
    return true;
}
```

### 虚函数（可选实现）

#### alive

```cpp
virtual bool alive();
```

检测连接是否存活。默认返回 `true`。

**返回值**：
- `true` - 连接正常
- `false` - 连接已断开

**建议实现**：发送心跳包检测

#### set_timeout

```cpp
virtual void set_timeout(int conn_timeout, int rw_timeout);
```

设置连接和读写超时时间。

**参数**：
- `conn_timeout` - 连接超时（秒）
- `rw_timeout` - 读写超时（秒）

### 成员函数

#### get_when

```cpp
time_t get_when() const;
```

获取连接最后使用时间。

**返回值**：Unix 时间戳

#### set_when

```cpp
void set_when(time_t when);
```

设置连接使用时间。

**参数**：
- `when` - Unix 时间戳

#### get_pool

```cpp
connect_pool* get_pool() const;
```

获取所属连接池。

**返回值**：连接池指针

---

## connect_pool

**文件位置**：`lib_acl_cpp/include/acl_cpp/connpool/connect_pool.hpp`

单服务器连接池基类。

### 构造函数

```cpp
connect_pool(const char* addr, size_t max, size_t idx = 0);
```

**参数**：
- `addr` - 服务器地址，格式 `ip:port`
- `max` - 最大连接数，0 表示无限制
- `idx` - 连接池索引（在集合中的位置）

### 配置方法

#### set_timeout

```cpp
connect_pool& set_timeout(int conn_timeout, int rw_timeout,
                          bool sockopt_timo = false);
```

设置连接和读写超时。

**参数**：
- `conn_timeout` - 连接超时（秒）
- `rw_timeout` - 读写超时（秒）
- `sockopt_timo` - 是否使用 `setsockopt` 设置超时

**返回值**：`*this` 支持链式调用

#### set_conns_min

```cpp
connect_pool& set_conns_min(size_t min);
```

设置最小连接数。

**参数**：
- `min` - 最小连接数

#### set_retry_inter

```cpp
connect_pool& set_retry_inter(int retry_inter);
```

设置故障后重试间隔。

**参数**：
- `retry_inter` - 重试间隔（秒）
  - `<= 0` - 不自动重试
  - `> 0` - 指定秒数后重试

#### set_idle_ttl

```cpp
connect_pool& set_idle_ttl(time_t ttl);
```

设置空闲连接存活时间。

**参数**：
- `ttl` - 存活时间（秒）
  - `< 0` - 永不过期
  - `== 0` - 立即过期
  - `> 0` - 指定秒数后过期

#### set_check_inter

```cpp
connect_pool& set_check_inter(int n);
```

设置连接检查间隔。

**参数**：
- `n` - 检查间隔（秒）

### 连接管理

#### peek

```cpp
connect_client* peek(bool on = true, double* tc = NULL, bool* old = NULL);
```

从连接池获取一个连接。

**参数**：
- `on` - 无空闲连接时是否创建新连接
- `tc` - 输出参数，获取连接耗时（毫秒）
- `old` - 输出参数，是否为旧连接（复用）

**返回值**：
- 成功 - 连接指针
- 失败 - `NULL`

**示例**：
```cpp
double elapsed;
bool is_old;
connect_client* conn = pool.peek(true, &elapsed, &is_old);
if (conn) {
    printf("获取连接耗时: %.2f ms, 是否复用: %s\n",
           elapsed, is_old ? "是" : "否");
}
```

#### put

```cpp
void put(connect_client* conn, bool keep = true,
         cpool_put_oper_t oper = cpool_put_check_idle);
```

归还连接到连接池。

**参数**：
- `conn` - 连接指针
- `keep` - 是否保持连接
  - `true` - 放回连接池复用
  - `false` - 直接销毁
- `oper` - 维护操作标志位
  - `cpool_put_oper_none` - 不执行任何操作
  - `cpool_put_check_idle` - 检查并清理空闲连接
  - `cpool_put_check_dead` - 检查并清理死连接
  - `cpool_put_keep_conns` - 保持最小连接数

**示例**：
```cpp
// 正常归还
pool.put(conn, true);

// 出错时不保持
pool.put(conn, false);

// 归还时执行多种检查
pool.put(conn, true,
    cpool_put_check_idle | cpool_put_check_dead | cpool_put_keep_conns);
```

#### bind_one

```cpp
void bind_one(connect_client* conn);
```

将连接池绑定到独立线程，线程之间不共享连池。

**参数**：
- `conn` - 连接指针

### 连接检查

#### check_idle

```cpp
size_t check_idle(time_t ttl, bool exclusive = true);
size_t check_idle(bool exclusive = true);
```

检查并清理空闲连接。

**参数**：
- `ttl` - 超时时间（秒），使用 `idle_ttl_` 默认值
- `exclusive` - 是否需要加锁

**返回值**：清理的连接数

#### check_dead

```cpp
size_t check_dead(thread_pool* threads = NULL);
```

检查并清理死连接。

**参数**：
- `threads` - 线程池，用于并发检测（可选）

**返回值**：清理的连接数

#### keep_conns

```cpp
void keep_conns(thread_pool* threads = NULL);
```

保持最小连接数。

**参数**：
- `threads` - 线程池，用于并发创建（可选）

### 状态查询

#### get_addr

```cpp
const char* get_addr() const;
```

获取服务器地址。

#### get_max

```cpp
size_t get_max() const;
```

获取最大连接数。

#### get_count

```cpp
size_t get_count() const;
```

获取当前连接总数。

#### get_idx

```cpp
size_t get_idx() const;
```

获取连接池索引。

#### aliving

```cpp
bool aliving();
```

检查连接池是否可用。

**返回值**：
- `true` - 连接池正常
- `false` - 连接池故障中

#### get_total_used

```cpp
unsigned long long get_total_used() const;
```

获取连接池总使用次数。

#### get_current_used

```cpp
unsigned long long get_current_used() const;
```

获取当前周期使用次数。

### 状态管理

#### set_alive

```cpp
void set_alive(bool ok);
```

设置连接池存活状态。

**参数**：
- `ok` - `true` 正常，`false` 故障

#### refer / unrefer

```cpp
void refer();
void unrefer();
```

增加/减少引用计数。用于延迟销毁。

### 纯虚函数（必须实现）

#### create_connect

```cpp
virtual connect_client* create_connect() = 0;
```

创建新连接。工厂方法。

**返回值**：连接指针

**示例**：
```cpp
connect_client* redis_client_pool::create_connect() {
    redis_client* conn = new redis_client(
        addr_, conn_timeout_, rw_timeout_);
    conn->set_password(pass_);
    return conn;
}
```

---

## connect_manager

**文件位置**：`lib_acl_cpp/include/acl_cpp/connpool/connect_manager.hpp`

连接池管理器基类，管理多个服务器的连接池。

### 构造函数

```cpp
connect_manager();
```

### 配置方法

#### bind_thread

```cpp
void bind_thread(bool yes);
```

设置是否启用线程绑定模式，设定绑定模式后，每个线程将创建各自的连接池，该连接池仅属于创建线程，不与其它线程共享。

**参数**：
- `yes` - `true` 启用，`false` 禁用

**说明**：协程环境建议启用

#### init

```cpp
void init(const char* default_addr, const char* addr_list,
          size_t count, int conn_timeout = 30, int rw_timeout = 30,
          bool sockopt_timeo = false);
```

初始化服务器列表。

**参数**：
- `default_addr` - 默认服务器地址
- `addr_list` - 服务器列表，格式：
  - `IP:PORT:COUNT;IP:PORT:COUNT;...`
  - `IP:PORT:COUNT,IP:PORT:COUNT,IP:PORT;...`
- `count` - 默认最大连接数
- `conn_timeout` - 连接超时（秒）
- `rw_timeout` - 读写超时（秒）
- `sockopt_timeo` - 是否使用 `setsockopt` 设置超时

**示例**：
```cpp
manager.init("127.0.0.1:6379",
             "127.0.0.1:6379:100;127.0.0.1:6380:100",
             50, 10, 10);
```

#### set

```cpp
void set(const char* addr, size_t max, int conn_timeout = 30,
         int rw_timeout = 30, bool sockopt_timeo = false, size_t min = 0);
```

动态添加或更新服务器。

**参数**：
- `addr` - 服务器地址
- `max` - 最大连接数
- `conn_timeout` - 连接超时（秒）
- `rw_timeout` - 读写超时（秒）
- `sockopt_timeo` - 是否使用 `setsockopt`
- `min` - 最小连接数

#### remove

```cpp
void remove(const char* addr);
```

移除服务器。

**参数**：
- `addr` - 服务器地址

#### set_retry_inter

```cpp
void set_retry_inter(int n);
```

设置所有连接池的重试间隔。

#### set_idle_ttl

```cpp
void set_idle_ttl(time_t ttl);
```

设置所有连接池的空闲TTL。

#### set_check_inter

```cpp
void set_check_inter(int n);
```

设置所有连接池的检查间隔。

### 获取连接池

#### peek (轮询)

```cpp
virtual connect_pool* peek();
```

轮询方式获取连接池。

**返回值**：连接池指针

#### peek (哈希)

```cpp
virtual connect_pool* peek(const char* key, bool exclusive = true);
```

哈希方式获取连接池。

**参数**：
- `key` - 键值，用于计算哈希
- `exclusive` - 是否需要加锁

**返回值**：连接池指针

#### get

```cpp
connect_pool* get(const char* addr, bool exclusive = true,
                  bool restore = false);
```

根据地址获取连接池。

**参数**：
- `addr` - 服务器地址
- `exclusive` - 是否需要加锁
- `restore` - 故障时是否自动恢复

**返回值**：连接池指针

#### get_pools

```cpp
std::vector<connect_pool*>& get_pools();
```

获取所有连接池。

**返回值**：连接池数组引用

### 批量检查

#### check_idle_conns

```cpp
size_t check_idle_conns(size_t step, size_t* left = NULL);
```

分批检查空闲连接。

**参数**：
- `step` - 每批检查的连接池数量
- `left` - 输出剩余待检查数量

**返回值**：清理的连接数

#### check_dead_conns

```cpp
size_t check_dead_conns(size_t step, size_t* left = NULL);
```

分批检查死连接。

**参数**：
- `step` - 每批检查的连接池数量
- `left` - 输出剩余待检查数量

**返回值**：清理的连接数

#### keep_min_conns

```cpp
size_t keep_min_conns(size_t step);
```

分批保持最小连接数。

**参数**：
- `step` - 每批处理的连接池数量

**返回值**：剩余待处理数量

#### check_conns

```cpp
size_t check_conns(size_t step, bool check_idle, bool kick_dead,
                   bool keep_conns, thread_pool* threads, size_t* left = NULL);
```

综合检查维护。

**参数**：
- `step` - 每批处理数量
- `check_idle` - 是否检查空闲
- `kick_dead` - 是否踢除死连接
- `keep_conns` - 是否保持最小连接数
- `threads` - 线程池（可选）
- `left` - 输出剩余数量

### 监控管理

#### start_monitor

```cpp
bool start_monitor(connect_monitor* monitor);
```

启动后台监控线程。

**参数**：
- `monitor` - 监控器对象

**返回值**：
- `true` - 启动成功
- `false` - 已有监控在运行

#### stop_monitor

```cpp
connect_monitor* stop_monitor(bool graceful = true);
```

停止监控线程。

**参数**：
- `graceful` - 是否优雅关闭

**返回值**：监控器对象指针

### 纯虚函数（必须实现）

#### create_pool

```cpp
virtual connect_pool* create_pool(const char* addr,
                                  size_t count, size_t idx) = 0;
```

创建连接池。工厂方法。

**参数**：
- `addr` - 服务器地址
- `count` - 最大连接数
- `idx` - 连接池索引

**返回值**：连接池指针

---

## connect_monitor

**文件位置**：`lib_acl_cpp/include/acl_cpp/connpool/connect_monitor.hpp`

连接池后台监控线程。

### 构造函数

```cpp
connect_monitor(connect_manager& manager, bool check_server = false);
```

**参数**：
- `manager` - 连接池管理器引用
- `check_server` - 是否主动检测服务器

### 配置方法

#### set_check_inter

```cpp
connect_monitor& set_check_inter(int n);
```

设置检查间隔。

**参数**：
- `n` - 间隔时间（秒）

#### set_conn_timeout

```cpp
connect_monitor& set_conn_timeout(int n);
```

设置连接超时。

**参数**：
- `n` - 超时时间（秒）

#### set_check_conns

```cpp
connect_monitor& set_check_conns(bool check_idle, bool kick_dead,
    bool keep_conns, thread_pool* threads = NULL, size_t step = 0);
```

配置检查选项。

**参数**：
- `check_idle` - 是否检查空闲连接
- `kick_dead` - 是否踢除死连接
- `keep_conns` - 是否保持最小连接数
- `threads` - 线程池（可选）
- `step` - 每次检查数量

### 控制方法

#### stop

```cpp
void stop(bool graceful);
```

停止监控线程。

**参数**：
- `graceful` - 是否优雅停止

### 虚函数（可重写）

#### nio_check

```cpp
virtual void nio_check(check_client& checker, aio_socket_stream& conn);
```

异步 IO 检测回调。

#### sio_check

```cpp
virtual void sio_check(check_client& checker, socket_stream& conn);
```

同步 IO 检测回调。

#### on_connected

```cpp
virtual void on_connected(const check_client&, double cost);
```

连接成功回调。

#### on_timeout

```cpp
virtual void on_timeout(const char* addr, double cost);
```

连接超时回调。

#### on_refused

```cpp
virtual void on_refused(const char* addr, double cost);
```

连接拒绝回调。

---

## 辅助类

### connect_guard

RAII 方式管理连接。

```cpp
class connect_guard : public noncopyable {
public:
    connect_guard(connect_pool& pool);
    ~connect_guard();
    
    void set_keep(bool keep);
    connect_client* peek();
};
```

**使用示例**：
```cpp
acl::connect_guard guard(pool);
acl::redis_client* conn = (acl::redis_client*) guard.peek();
if (conn) {
    // 使用连接...
}
// guard 析构时自动归还连接
```

### conns_pools

连接池集合结构（内部使用）。

```cpp
struct conns_pools {
    std::vector<connect_pool*> pools;  // 连接池数组
    size_t check_next;                 // 检查索引
    size_t conns_next;                 // 获取索引
};
```

### conn_config

连接配置结构。

```cpp
struct conn_config {
    string addr;           // 服务器地址
    size_t max;            // 最大连接数
    size_t min;            // 最小连接数
    int conn_timeout;      // 连接超时
    int rw_timeout;        // 读写超时
    bool sockopt_timeo;    // 是否使用 setsockopt
};
```

---

## 枚举类型

### cpool_put_oper_t

连接归还时的操作标志。

```cpp
typedef enum {
    cpool_put_oper_none  = 0,          // 不执行任何操作
    cpool_put_check_idle = 1,          // 检查空闲连接
    cpool_put_check_dead = (1 << 1),   // 检查死连接
    cpool_put_keep_conns = (1 << 2),   // 保持最小连接数
} cpool_put_oper_t;
```

**使用示例**：
```cpp
// 组合多个操作
pool.put(conn, true,
    cpool_put_check_idle | cpool_put_check_dead | cpool_put_keep_conns);
```

---

## 完整示例

### 基本使用流程

```cpp
#include "acl_cpp/lib_acl.hpp"

int main() {
    // 1. 初始化
    acl::acl_cpp_init();
    
    // 2. 创建连接池
    acl::redis_client_pool pool("127.0.0.1:6379", 100);
    
    // 3. 配置
    pool.set_timeout(10, 10)
        .set_conns_min(10)
        .set_idle_ttl(300)
        .set_retry_inter(5);
    
    // 4. 获取连接
    acl::redis_client* conn = (acl::redis_client*) pool.peek();
    if (conn == NULL) {
        printf("获取连接失败\n");
        return 1;
    }
    
    // 5. 使用连接
    acl::redis_string cmd(conn);
    if (!cmd.set("key", "value")) {
        printf("执行失败: %s\n", cmd.result_error());
        pool.put(conn, false);
        return 1;
    }
    
    // 6. 归还连接
    pool.put(conn, true);
    
    return 0;
}
```

---

## 参考链接

- [架构设计](architecture.md) - 详细架构说明
- [使用示例](examples.md) - 更多代码示例
- [最佳实践](best-practices.md) - 生产环境建议

---

## 版本历史

- v3.5.0 - 初始版本
- 持续更新中...

