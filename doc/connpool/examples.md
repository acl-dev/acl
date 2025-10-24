# ACL 连接池使用示例

本文档提供了 ACL 连接池在各种场景下的使用示例。

## 目录

- [基础示例](#基础示例)
- [Redis 连接池](#redis-连接池)
- [MySQL 连接池](#mysql-连接池)
- [TCP 连接池](#tcp-连接池)
- [多线程使用](#多线程使用)
- [集群模式](#集群模式)
- [自定义连接池](#自定义连接池)
- [高级用法](#高级用法)

---

## 基础示例

### 最简单的使用

```cpp
#include "acl_cpp/lib_acl.hpp"

int main() {
    acl::acl_cpp_init();
    
    // 创建 Redis 连接池
    acl::redis_client_pool pool("127.0.0.1:6379", 100);
    
    // 获取连接
    acl::redis_client* conn = (acl::redis_client*) pool.peek();
    if (conn == NULL) {
        printf("获取连接失败\n");
        return 1;
    }
    
    // 使用连接执行 Redis 命令
    acl::redis_string cmd(conn);
    if (!cmd.set("hello", "world")) {
        printf("执行命令失败: %s\n", cmd.result_error());
    }
    
    // 归还连接
    pool.put(conn, true);
    
    return 0;
}
```

### 使用 RAII 管理连接

```cpp
#include "acl_cpp/lib_acl.hpp"

void raii_example() {
    acl::redis_client_pool pool("127.0.0.1:6379", 100);
    
    {
        // 使用 connect_guard 自动管理连接
        acl::connect_guard guard(pool);
        acl::redis_client* conn = (acl::redis_client*) guard.peek();
        
        if (conn) {
            acl::redis_string cmd(conn);
            cmd.set("key", "value");
        }
        
        // guard 析构时自动归还连接到池
    }
}
```

---

## Redis 连接池

### 单机 Redis

```cpp
#include "acl_cpp/lib_acl.hpp"

void redis_single_example() {
    // 1. 创建连接池
    const char* redis_addr = "127.0.0.1:6379";
    size_t max_conns = 100;
    
    acl::redis_client_pool pool(redis_addr, max_conns);
    
    // 2. 配置连接池
    pool.set_timeout(10, 10)           // 连接超时10秒，IO超时10秒
        .set_password("mypassword")     // 设置密码
        .set_db(2)                      // 选择数据库2
        .set_conns_min(10)              // 最小连接数10
        .set_idle_ttl(300)              // 空闲300秒后关闭
        .set_retry_inter(5);            // 失败后5秒重试
    
    // 3. String 操作
    {
        acl::redis_client* conn = (acl::redis_client*) pool.peek();
        acl::redis_string cmd(conn);
        
        // SET
        cmd.set("name", "张三");
        
        // GET
        acl::string value;
        if (cmd.get("name", value)) {
            printf("name = %s\n", value.c_str());
        }
        
        // MSET
        std::map<acl::string, acl::string> kvs;
        kvs["key1"] = "value1";
        kvs["key2"] = "value2";
        cmd.mset(kvs);
        
        pool.put(conn, true);
    }
    
    // 4. Hash 操作
    {
        acl::redis_client* conn = (acl::redis_client*) pool.peek();
        acl::redis_hash cmd(conn);
        
        // HSET
        cmd.hset("user:1000", "name", "张三");
        cmd.hset("user:1000", "age", "25");
        
        // HGET
        acl::string name;
        cmd.hget("user:1000", "name", name);
        
        // HGETALL
        std::map<acl::string, acl::string> result;
        cmd.hgetall("user:1000", result);
        
        pool.put(conn, true);
    }
    
    // 5. List 操作
    {
        acl::redis_client* conn = (acl::redis_client*) pool.peek();
        acl::redis_list cmd(conn);
        
        // LPUSH
        cmd.lpush("mylist", "item1");
        cmd.lpush("mylist", "item2");
        
        // LRANGE
        std::vector<acl::string> result;
        cmd.lrange("mylist", 0, -1, result);
        
        pool.put(conn, true);
    }
}
```

### Redis 集群

```cpp
#include "acl_cpp/lib_acl.hpp"

void redis_cluster_example() {
    // 1. 创建集群管理器
    acl::redis_client_cluster cluster;
    
    // 2. 配置集群参数
    cluster.set_redirect_max(15)                // 最大重定向15次
           .set_redirect_sleep(100)             // 重定向休眠100微秒
           .set_password("default", "mypass");  // 设置默认密码
    
    // 3. 自动发现集群节点（连接任意一个节点即可）
    const char* one_node = "127.0.0.1:7000";
    size_t max_conns = 100;
    int conn_timeout = 10;
    int rw_timeout = 10;
    
    cluster.set_all_slot(one_node, max_conns, conn_timeout, rw_timeout);
    
    // 4. 使用集群（自动路由到正确节点）
    acl::redis_string cmd(&cluster);
    
    // SET 命令（自动计算槽位并路由）
    if (!cmd.set("user:1000", "张三")) {
        printf("SET 失败: %s\n", cmd.result_error());
    }
    
    // GET 命令
    acl::string value;
    if (cmd.get("user:1000", value)) {
        printf("user:1000 = %s\n", value.c_str());
    }
    
    // 5. 批量操作（注意：集群模式下批量操作可能跨节点）
    std::vector<acl::string> keys;
    keys.push_back("key1");
    keys.push_back("key2");
    keys.push_back("key3");
    
    std::vector<acl::string> values;
    if (cmd.mget(keys, &values) > 0) {
        for (size_t i = 0; i < values.size(); i++) {
            printf("%s = %s\n", keys[i].c_str(), values[i].c_str());
        }
    }
}
```

### 手动配置集群节点

```cpp
void redis_cluster_manual() {
    acl::redis_client_cluster cluster;
    
    // 手动设置每个槽位对应的节点
    // Redis 集群有 16384 个槽位
    
    // 节点1: 槽位 0-5460
    for (int i = 0; i <= 5460; i++) {
        cluster.set_slot(i, "127.0.0.1:7000");
    }
    
    // 节点2: 槽位 5461-10922
    for (int i = 5461; i <= 10922; i++) {
        cluster.set_slot(i, "127.0.0.1:7001");
    }
    
    // 节点3: 槽位 10923-16383
    for (int i = 10923; i <= 16383; i++) {
        cluster.set_slot(i, "127.0.0.1:7002");
    }
    
    // 为不同节点设置不同密码
    cluster.set_password("127.0.0.1:7000", "pass1");
    cluster.set_password("127.0.0.1:7001", "pass2");
    cluster.set_password("127.0.0.1:7002", "pass3");
    
    // 使用集群
    acl::redis_string cmd(&cluster);
    cmd.set("mykey", "myvalue");
}
```

---

## MySQL 连接池

### 基础使用

```cpp
#include "acl_cpp/lib_acl.hpp"
#include "acl_cpp/db/db_mysql.hpp"

void mysql_pool_example() {
    // 1. 创建 MySQL 连接池
    acl::mysql_pool pool("127.0.0.1:3306", 50);
    
    // 2. 配置连接池
    pool.set_timeout(10, 10)
        .set_dbname("test_db")
        .set_dbuser("root")
        .set_dbpass("password")
        .set_charset("utf8mb4");
    
    // 3. 获取连接并执行查询
    acl::db_handle* conn = pool.peek();
    if (conn == NULL) {
        printf("获取连接失败\n");
        return;
    }
    
    // 转换为 MySQL 连接对象
    acl::db_mysql* mysql = (acl::db_mysql*) conn;
    
    // 4. 执行 SELECT 查询
    const char* sql = "SELECT id, name, age FROM users WHERE age > 18";
    if (!mysql->sql_select(sql)) {
        printf("查询失败: %s\n", mysql->get_error());
        pool.put(conn, false);  // 出错时不保持连接
        return;
    }
    
    // 5. 遍历结果集
    const acl::db_rows& rows = mysql->get_result();
    for (size_t i = 0; i < rows.length(); i++) {
        const acl::db_row* row = rows[i];
        
        int id = atoi((*row)["id"]);
        const char* name = (*row)["name"];
        int age = atoi((*row)["age"]);
        
        printf("id=%d, name=%s, age=%d\n", id, name, age);
    }
    
    // 6. 执行 INSERT
    acl::string insert_sql;
    insert_sql.format("INSERT INTO users (name, age) VALUES ('%s', %d)",
                      "张三", 25);
    
    if (!mysql->sql_update(insert_sql.c_str())) {
        printf("插入失败: %s\n", mysql->get_error());
    } else {
        printf("插入成功，受影响行数: %d\n", mysql->affect_count());
    }
    
    // 7. 归还连接
    pool.put(conn, true);
}
```

### 事务处理

```cpp
void mysql_transaction_example() {
    acl::mysql_pool pool("127.0.0.1:3306", 50);
    pool.set_dbname("test_db")
        .set_dbuser("root")
        .set_dbpass("password");
    
    acl::db_mysql* mysql = (acl::db_mysql*) pool.peek();
    
    // 开启事务
    if (!mysql->sql_update("BEGIN")) {
        printf("开启事务失败\n");
        pool.put(mysql, false);
        return;
    }
    
    bool success = true;
    
    // 执行多个 SQL
    if (!mysql->sql_update("UPDATE accounts SET balance = balance - 100 WHERE id = 1")) {
        printf("扣款失败\n");
        success = false;
    }
    
    if (success && !mysql->sql_update("UPDATE accounts SET balance = balance + 100 WHERE id = 2")) {
        printf("转账失败\n");
        success = false;
    }
    
    // 提交或回滚
    if (success) {
        mysql->sql_update("COMMIT");
        printf("事务提交成功\n");
    } else {
        mysql->sql_update("ROLLBACK");
        printf("事务已回滚\n");
    }
    
    pool.put(mysql, success);
}
```

---

## TCP 连接池

### 自定义协议连接池

```cpp
#include "acl_cpp/lib_acl.hpp"

// 简单的回显协议示例
void tcp_pool_example() {
    // 1. 创建 TCP 连接池
    acl::tcp_pool pool("127.0.0.1:8888", 50);
    pool.set_timeout(10, 10);
    
    // 2. 获取连接
    acl::tcp_client* conn = (acl::tcp_client*) pool.peek();
    if (conn == NULL) {
        printf("获取连接失败\n");
        return;
    }
    
    // 3. 发送数据并接收响应
    const char* request = "Hello, Server!";
    acl::string response;
    
    if (!conn->send(request, strlen(request), &response)) {
        printf("通信失败\n");
        pool.put(conn, false);
        return;
    }
    
    printf("收到响应: %s\n", response.c_str());
    
    // 4. 归还连接
    pool.put(conn, true);
}
```

### 使用 tcp_ipc 高级封装

```cpp
#include "acl_cpp/lib_acl.hpp"

void tcp_ipc_example() {
    // tcp_ipc 封装了 tcp_manager，提供更简单的接口
    acl::tcp_ipc ipc;
    
    // 配置
    ipc.set_limit(100)          // 每个服务器最多100个连接
       .set_idle(300)           // 空闲300秒超时
       .set_conn_timeout(10)    // 连接超时10秒
       .set_rw_timeout(10);     // 读写超时10秒
    
    // 添加服务器地址
    ipc.add_addr("127.0.0.1:8888");
    ipc.add_addr("127.0.0.1:8889");
    
    // 发送数据到指定服务器
    const char* data = "Hello";
    acl::string response;
    
    if (ipc.send("127.0.0.1:8888", data, strlen(data), &response)) {
        printf("响应: %s\n", response.c_str());
    }
    
    // 广播到所有服务器
    size_t success_count = ipc.broadcast(data, strlen(data));
    printf("成功发送到 %lu 个服务器\n", (unsigned long)success_count);
}
```

---

## 多线程使用

### 线程池 + 连接池

```cpp
#include "acl_cpp/lib_acl.hpp"
#include <vector>

class WorkerThread : public acl::thread {
public:
    WorkerThread(acl::redis_client_pool& pool, int count)
        : pool_(pool), count_(count), success_(0), failed_(0) {}
    
    int get_success() const { return success_; }
    int get_failed() const { return failed_; }
    
protected:
    void* run() override {
        for (int i = 0; i < count_; i++) {
            // 从连接池获取连接
            acl::redis_client* conn = (acl::redis_client*) pool_.peek();
            if (conn == NULL) {
                failed_++;
                continue;
            }
            
            // 执行业务逻辑
            acl::redis_string cmd(conn);
            acl::string key, value;
            key.format("thread_%lu_key_%d", acl_pthread_self(), i);
            value.format("value_%d", i);
            
            bool result = cmd.set(key, value);
            
            // 归还连接
            pool_.put(conn, result);
            
            if (result) {
                success_++;
            } else {
                failed_++;
            }
        }
        
        return NULL;
    }
    
private:
    acl::redis_client_pool& pool_;
    int count_;
    int success_;
    int failed_;
};

void multithread_example() {
    // 创建连接池
    acl::redis_client_pool pool("127.0.0.1:6379", 100);
    pool.set_timeout(10, 10);
    
    // 启动多个线程
    int thread_count = 10;
    int operations_per_thread = 1000;
    
    std::vector<WorkerThread*> threads;
    
    // 创建并启动线程
    for (int i = 0; i < thread_count; i++) {
        WorkerThread* thread = new WorkerThread(pool, operations_per_thread);
        thread->set_detachable(false);
        thread->start();
        threads.push_back(thread);
    }
    
    // 等待所有线程完成
    int total_success = 0;
    int total_failed = 0;
    
    for (size_t i = 0; i < threads.size(); i++) {
        threads[i]->wait();
        total_success += threads[i]->get_success();
        total_failed += threads[i]->get_failed();
        delete threads[i];
    }
    
    printf("总操作数: %d\n", total_success + total_failed);
    printf("成功: %d\n", total_success);
    printf("失败: %d\n", total_failed);
}
```

### 协程环境（线程绑定模式）

```cpp
#include "acl_cpp/lib_acl.hpp"

void fiber_example() {
    // 创建集群管理器并启用线程绑定
    acl::redis_client_cluster cluster;
    cluster.bind_thread(true);  // 关键：启用线程绑定
    
    // 初始化集群
    cluster.set_all_slot("127.0.0.1:7000", 100, 10, 10);
    
    // 在协程环境中使用
    // 每个协程线程都有独立的连接池集合
    // 避免了跨线程使用连接的问题
    
    acl::redis_string cmd(&cluster);
    cmd.set("key", "value");
}
```

---

## 集群模式

### 多服务器负载均衡

```cpp
#include "acl_cpp/lib_acl.hpp"

// 自定义 Redis 管理器
class MyRedisManager : public acl::connect_manager {
protected:
    acl::connect_pool* create_pool(const char* addr,
                                    size_t count, size_t idx) override {
        acl::redis_client_pool* pool =
            new acl::redis_client_pool(addr, count, idx);
        pool->set_password("mypass");
        pool->set_timeout(10, 10);
        return pool;
    }
};

void load_balance_example() {
    MyRedisManager manager;
    
    // 初始化多个 Redis 服务器
    // 格式: IP:PORT:MAX_CONNS;IP:PORT:MAX_CONNS;...
    const char* servers = "127.0.0.1:6379:100;127.0.0.1:6380:100;127.0.0.1:6381:100";
    manager.init(NULL, servers, 100, 10, 10);
    
    // 配置全局参数
    manager.set_idle_ttl(300);
    manager.set_retry_inter(5);
    manager.set_check_inter(60);
    
    // 1. 轮询方式获取连接池
    for (int i = 0; i < 10; i++) {
        acl::connect_pool* pool = manager.peek();  // 轮询
        printf("第%d次获取: %s\n", i, pool->get_addr());
    }
    // 输出: 6379, 6380, 6381, 6379, 6380, 6381, ...
    
    // 2. 哈希方式获取连接池（根据 key 路由）
    const char* keys[] = {"user:1000", "user:1001", "user:1002"};
    for (int i = 0; i < 3; i++) {
        acl::connect_pool* pool = manager.peek(keys[i]);
        printf("key=%s 路由到: %s\n", keys[i], pool->get_addr());
    }
    // 相同 key 总是路由到同一节点
    
    // 3. 指定地址获取连接池
    acl::connect_pool* pool = manager.get("127.0.0.1:6379");
    if (pool) {
        acl::redis_client* conn = (acl::redis_client*) pool->peek();
        // ... 使用连接 ...
        pool->put(conn, true);
    }
}
```

---

## 自定义连接池

### 实现自定义协议

```cpp
#include "acl_cpp/lib_acl.hpp"

// 1. 自定义连接客户端
class my_client : public acl::connect_client {
public:
    my_client(const char* addr, int conn_timeout, int rw_timeout)
        : addr_(acl_mystrdup(addr))
        , conn_(NULL) {
        conn_timeout_ = conn_timeout;
        rw_timeout_ = rw_timeout;
    }
    
    ~my_client() {
        if (conn_) {
            delete conn_;
        }
        acl_myfree(addr_);
    }
    
    // 实现 open() 方法
    bool open() override {
        conn_ = new acl::socket_stream();
        if (!conn_->open(addr_, conn_timeout_, rw_timeout_)) {
            delete conn_;
            conn_ = NULL;
            return false;
        }
        
        // 发送协议握手
        if (!send_handshake()) {
            conn_->close();
            return false;
        }
        
        return true;
    }
    
    // 实现 alive() 方法（可选）
    bool alive() override {
        if (conn_ == NULL || !conn_->alive()) {
            return false;
        }
        
        // 发送心跳检测
        return send_ping();
    }
    
    // 业务方法
    bool send_request(const char* data, size_t len, acl::string& response) {
        if (conn_ == NULL) {
            return false;
        }
        
        // 发送请求
        if (conn_->write(data, len) == -1) {
            return false;
        }
        
        // 接收响应
        if (conn_->gets(response) == false) {
            return false;
        }
        
        return true;
    }
    
private:
    char* addr_;
    acl::socket_stream* conn_;
    
    bool send_handshake() {
        const char* handshake = "HELLO 1.0\r\n";
        return conn_->write(handshake, strlen(handshake)) != -1;
    }
    
    bool send_ping() {
        const char* ping = "PING\r\n";
        if (conn_->write(ping, strlen(ping)) == -1) {
            return false;
        }
        
        acl::string response;
        if (!conn_->gets(response)) {
            return false;
        }
        
        return response == "PONG\r\n";
    }
};

// 2. 自定义连接池
class my_pool : public acl::connect_pool {
public:
    my_pool(const char* addr, size_t max, size_t idx = 0)
        : connect_pool(addr, max, idx) {}
    
protected:
    // 实现工厂方法
    acl::connect_client* create_connect() override {
        return new my_client(addr_, conn_timeout_, rw_timeout_);
    }
};

// 3. 自定义管理器
class my_manager : public acl::connect_manager {
protected:
    acl::connect_pool* create_pool(const char* addr,
                                    size_t count, size_t idx) override {
        return new my_pool(addr, count, idx);
    }
};

// 4. 使用自定义连接池
void custom_pool_example() {
    my_pool pool("127.0.0.1:9999", 50);
    pool.set_timeout(10, 10);
    
    my_client* conn = (my_client*) pool.peek();
    if (conn) {
        acl::string response;
        conn->send_request("GET /data", 9, response);
        printf("响应: %s\n", response.c_str());
        
        pool.put(conn, true);
    }
}
```

---

## 高级用法

### 连接池监控

```cpp
#include "acl_cpp/lib_acl.hpp"

void monitor_example() {
    acl::redis_client_cluster cluster;
    cluster.set_all_slot("127.0.0.1:7000", 100, 10, 10);
    
    // 创建监控器
    acl::connect_monitor monitor(cluster, true);
    
    // 配置监控参数
    monitor.set_check_inter(30)              // 每30秒检查一次
           .set_conn_timeout(5)              // 连接超时5秒
           .set_check_conns(
               true,   // 检查并关闭空闲连接
               true,   // 踢除死连接
               true,   // 保持最小连接数
               NULL,   // 不使用线程池
               10      // 每次检查10个连接池
           );
    
    // 启动监控线程
    if (!cluster.start_monitor(&monitor)) {
        printf("启动监控失败\n");
        return;
    }
    
    // ... 应用运行 ...
    
    // 停止监控（优雅关闭）
    cluster.stop_monitor(true);
}
```

### 动态管理服务器节点

```cpp
void dynamic_server_example() {
    MyRedisManager manager;
    
    // 初始服务器列表
    manager.set("127.0.0.1:6379", 100, 10, 10);
    manager.set("127.0.0.1:6380", 100, 10, 10);
    
    // ... 使用连接池 ...
    
    // 动态添加新服务器
    manager.set("127.0.0.1:6381", 100, 10, 10);
    printf("添加服务器: 127.0.0.1:6381\n");
    
    // 移除故障服务器
    manager.remove("127.0.0.1:6380");
    printf("移除服务器: 127.0.0.1:6380\n");
    
    // 获取所有连接池
    std::vector<acl::connect_pool*>& pools = manager.get_pools();
    printf("当前连接池数量: %lu\n", (unsigned long)pools.size());
    
    // 打印统计信息
    manager.statistics();
}
```

### 批量检查和维护

```cpp
void maintenance_example() {
    MyRedisManager manager;
    manager.init(NULL, "127.0.0.1:6379:100;127.0.0.1:6380:100", 100, 10, 10);
    
    manager.set_idle_ttl(300);      // 空闲300秒超时
    manager.set_check_inter(60);    // 每60秒检查
    
    // 手动触发维护
    while (true) {
        sleep(60);
        
        // 1. 清理空闲连接
        size_t idle_left = 0;
        size_t idle_closed = manager.check_idle_conns(10, &idle_left);
        printf("清理了 %lu 个空闲连接，剩余 %lu 个待检查\n",
               (unsigned long)idle_closed, (unsigned long)idle_left);
        
        // 2. 检查死连接
        size_t dead_left = 0;
        size_t dead_closed = manager.check_dead_conns(10, &dead_left);
        printf("清理了 %lu 个死连接，剩余 %lu 个待检查\n",
               (unsigned long)dead_closed, (unsigned long)dead_left);
        
        // 3. 保持最小连接数
        size_t min_left = manager.keep_min_conns(10);
        printf("预热连接，剩余 %lu 个连接池待处理\n", (unsigned long)min_left);
    }
}
```

### SSL/TLS 加密连接

```cpp
#include "acl_cpp/lib_acl.hpp"

void ssl_example() {
    // 1. 创建 SSL 配置
    acl::sslbase_conf* ssl_conf = new acl::sslbase_conf();
    
    // 2. 配置 SSL 参数
    ssl_conf->enable_cache(true);           // 启用会话缓存
    ssl_conf->set_verify_depth(10);         // 验证深度
    
    // 可选：设置证书
    // ssl_conf->add_cert("client.crt");
    // ssl_conf->set_key("client.key");
    // ssl_conf->set_ca_path("/path/to/ca/");
    
    // 3. 创建 Redis 连接池并应用 SSL
    acl::redis_client_pool pool("127.0.0.1:6379", 50);
    pool.set_ssl_conf(ssl_conf);
    pool.set_password("mypass");
    
    // 4. 正常使用（底层自动使用 SSL）
    acl::redis_client* conn = (acl::redis_client*) pool.peek();
    if (conn) {
        acl::redis_string cmd(conn);
        cmd.set("secure_key", "secure_value");
        pool.put(conn, true);
    }
}
```

---

## 完整应用示例

### Web 服务器中使用连接池

```cpp
#include "acl_cpp/lib_acl.hpp"

// 全局连接池管理器
static acl::redis_client_cluster* g_redis_cluster = NULL;

// 初始化函数（程序启动时调用）
void init_pools() {
    // 初始化 ACL 库
    acl::acl_cpp_init();
    
    // 创建 Redis 集群
    g_redis_cluster = new acl::redis_client_cluster();
    g_redis_cluster->set_all_slot("127.0.0.1:7000", 200, 10, 10);
    
    // 启动监控
    acl::connect_monitor* monitor = new acl::connect_monitor(*g_redis_cluster);
    monitor->set_check_inter(60)
           ->set_check_conns(true, true, true);
    g_redis_cluster->start_monitor(monitor);
    
    printf("连接池初始化完成\n");
}

// 清理函数（程序退出时调用）
void cleanup_pools() {
    if (g_redis_cluster) {
        g_redis_cluster->stop_monitor(true);
        delete g_redis_cluster;
        g_redis_cluster = NULL;
    }
    printf("连接池已清理\n");
}

// HTTP 请求处理函数
void handle_request(acl::HttpServletRequest& req,
                    acl::HttpServletResponse& resp) {
    // 从 Redis 获取用户信息
    acl::redis_string cmd(g_redis_cluster);
    
    const char* user_id = req.getParameter("user_id");
    acl::string key;
    key.format("user:%s", user_id);
    
    acl::string user_data;
    if (cmd.get(key, user_data)) {
        // 返回用户数据
        resp.setStatus(200);
        resp.setContentType("application/json");
        resp.write(user_data);
    } else {
        // 用户不存在
        resp.setStatus(404);
        resp.write("{\"error\": \"User not found\"}");
    }
}

int main() {
    // 初始化连接池
    init_pools();
    
    // ... 启动 Web 服务器 ...
    
    // 程序退出时清理
    cleanup_pools();
    return 0;
}
```

---

## 总结

ACL 连接池提供了灵活而强大的功能：

1. **易用性**：简洁的 API，快速上手
2. **通用性**：支持任意协议的连接池实现
3. **高性能**：连接复用、批量操作、异步检测
4. **高可用**：自动故障恢复、健康检查、负载均衡
5. **可扩展**：清晰的继承体系，易于定制

更多示例代码请参考：
- `lib_acl_cpp/samples/redis/` - Redis 示例
- `lib_acl_cpp/samples/db/` - 数据库示例
- `lib_acl_cpp/samples/connect_pool/` - 通用连接池示例

