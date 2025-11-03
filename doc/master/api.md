# ACL Master API 参考文档

## 目录
- [master_base](#master_base)
- [master_threads](#master_threads)
- [master_proc](#master_proc)
- [master_aio](#master_aio)
- [master_trigger](#master_trigger)
- [master_udp](#master_udp)
- [master_fiber](#master_fiber)
- [master_conf](#master_conf)

---

## master_base

基类，为所有服务器模型提供公共功能。

### 头文件
```cpp
#include <acl_cpp/master/master_base.hpp>
```

### 公共方法

#### 配置设置方法

##### set_cfg_bool
```cpp
master_base& set_cfg_bool(master_bool_tbl* table);
```
设置布尔型配置参数表。

**参数**:
- `table`: 布尔型配置参数表

**返回值**: 返回自身引用，支持链式调用

---

##### set_cfg_int
```cpp
master_base& set_cfg_int(master_int_tbl* table);
```
设置整型配置参数表。

**参数**:
- `table`: 整型配置参数表

**返回值**: 返回自身引用，支持链式调用

---

##### set_cfg_int64
```cpp
master_base& set_cfg_int64(master_int64_tbl* table);
```
设置64位整型配置参数表。

**参数**:
- `table`: 64位整型配置参数表

**返回值**: 返回自身引用，支持链式调用

---

##### set_cfg_str
```cpp
master_base& set_cfg_str(master_str_tbl* table);
```
设置字符串配置参数表。

**参数**:
- `table`: 字符串配置参数表

**返回值**: 返回自身引用，支持链式调用

---

#### 状态查询方法

##### daemon_mode
```cpp
bool daemon_mode() const;
```
判断是否运行在 acl_master 控制的 daemon 模式。

**返回值**: 
- `true`: daemon 模式
- `false`: 独立运行模式

---

#### 定时器管理方法

##### proc_set_timer
```cpp
bool proc_set_timer(event_timer* timer);
```
设置进程级别的定时器。只能在进程或线程空间（如 `proc_on_init`）中被调用。定时器执行完后会自动销毁。

**参数**:
- `timer`: 定时器对象

**返回值**:
- `true`: 设置成功
- `false`: 设置失败

---

##### proc_del_timer
```cpp
void proc_del_timer(event_timer* timer);
```
删除进程级定时器。

**参数**:
- `timer`: 通过 `proc_set_timer` 设置的定时器对象

---

### 虚函数（回调方法）

##### proc_on_listen
```cpp
virtual void proc_on_listen(server_socket& ss);
```
在进程监听成功时调用，每成功绑定一个监听地址都会调用本方法。

**参数**:
- `ss`: 服务器套接字对象

---

##### proc_pre_jail
```cpp
virtual void proc_pre_jail();
```
在进程切换用户之前调用的回调函数。在此函数中可以进行一些需要 root 权限的操作。

---

##### proc_on_init
```cpp
virtual void proc_on_init();
```
在进程切换用户完成后调用的回调函数。此函数被调用时，进程权限为普通用户级别。

---

##### proc_pre_exit
```cpp
virtual bool proc_pre_exit();
```
在进程退出前先调用此虚方法。如果实现返回 `false`，则不会立即退出，会再次调用该虚方法直到实现返回 `true` 为止。

**返回值**:
- `true`: 允许退出
- `false`: 延迟退出

---

##### proc_on_exit
```cpp
virtual void proc_on_exit();
```
在进程退出前调用的回调函数。调用此函数后进程将按正常方式退出。

**注意**: 此方法在 `proc_pre_exit` 调用之后。

---

##### proc_on_sighup
```cpp
virtual bool proc_on_sighup(string& msg);
```
收到 SIGHUP 信号时的回调虚方法。

**参数**:
- `msg`: 输出消息字符串

**返回值**:
- `true`: 处理成功
- `false`: 处理失败

---

## master_threads

多线程服务器模型类，基于线程池处理客户端连接。

### 头文件
```cpp
#include <acl_cpp/master/master_threads.hpp>
```

### 继承关系
```cpp
class master_threads : public master_base
```

### 公共方法

#### run_daemon
```cpp
void run_daemon(int argc, char** argv);
```
在 daemon 模式下运行，由 acl_master 进程管理。

**参数**:
- `argc`: main 函数的参数个数
- `argv`: main 函数的参数数组

**使用示例**:
```cpp
int main(int argc, char* argv[]) {
    my_server server;
    server.run_daemon(argc, argv);
    return 0;
}
```

---

#### run_alone
```cpp
bool run_alone(const char* addrs, const char* path = NULL,
    unsigned int count = 1, int threads_count = 1);
```
在独立模式下运行，用于调试和测试。

**参数**:
- `addrs`: 监听地址列表，格式: "IP:PORT, IP:PORT..."
- `path`: 配置文件全路径（可选）
- `count`: 循环处理的次数上限，0 表示永久循环（默认值: 1）
- `threads_count`: 线程数，大于 1 时自动启动线程池模式（默认值: 1）

**返回值**:
- `true`: 启动成功
- `false`: 启动失败

**注意**: 当 `count` != 1 时，`threads_count` 参数才有效。实际参数值会使用配置文件中的 `ioctl_use_limit` 和 `ioctl_max_threads`。

---

#### thread_enable_read
```cpp
void thread_enable_read(socket_stream* stream);
```
激活连接流的可读状态。

**参数**:
- `stream`: 套接字流对象

---

#### thread_disable_read
```cpp
void thread_disable_read(socket_stream* stream);
```
禁止连接流的可读状态。

**参数**:
- `stream`: 套接字流对象

---

#### get_conf_path
```cpp
const char* get_conf_path() const;
```
获取配置文件路径。

**返回值**: 配置文件路径，NULL 表示没有配置文件

---

#### task_qlen
```cpp
size_t task_qlen() const;
```
获得当前线程池对象中挤压的待处理任务数量。API 调用方可以根据此数值判断是否需要进行关于流控的操作，如拒绝新连接等。

**返回值**: 待处理任务数量

---

#### threads_pool
```cpp
acl_pthread_pool_t* threads_pool() const;
```
获取 lib_acl C 库中的线程池句柄。

**返回值**: 线程池句柄指针

---

### 纯虚函数（必须实现）

#### thread_on_read
```cpp
virtual bool thread_on_read(socket_stream* stream) = 0;
```
当某个客户端连接有数据可读或连接关闭时调用此函数。

**参数**:
- `stream`: 客户端连接流对象

**返回值**:
- `true`: 保持连接，继续等待数据
- `false`: 关闭连接

**注意**: 长连接应用应该返回 `false`

---

### 虚函数（可选实现）

#### keep_read
```cpp
virtual bool keep_read(socket_stream* stream);
```
在调用 `thread_on_read` 返回 `true` 后，会自动调用本方法，用于判断连接是否继续可读。

**参数**:
- `stream`: 客户端连接流对象

**返回值**:
- `true`: 继续保持可读（默认）
- `false`: 不再继续监听

**默认实现**: 返回 `true`

---

#### thread_on_accept
```cpp
virtual bool thread_on_accept(socket_stream* stream);
```
当线程池中的某个线程获得一个连接时的回调函数。可以进行一些初始化操作。

**参数**:
- `stream`: 客户端连接流对象

**返回值**:
- `true`: 继续处理（默认）
- `false`: 关闭连接，不再调用 `thread_main` 过程

**默认实现**: 返回 `true`

---

#### thread_on_handshake
```cpp
virtual bool thread_on_handshake(socket_stream* stream);
```
在收到一个客户端连接后，服务器回调此函数与客户端进行认证等握手操作。此函数在 `thread_on_accept` 之后被调用。

**参数**:
- `stream`: 客户端连接流对象

**返回值**:
- `true`: 握手成功（默认）
- `false`: 关闭连接

**默认实现**: 返回 `true`

---

#### thread_on_timeout
```cpp
virtual bool thread_on_timeout(socket_stream* stream);
```
当某个连接的 IO 读写超时时的回调函数。

**参数**:
- `stream`: 客户端连接流对象

**返回值**:
- `true`: 等待下一次读写操作（给连接一次机会）
- `false`: 关闭连接（默认）

**默认实现**: 返回 `false`

---

#### thread_on_close
```cpp
virtual void thread_on_close(socket_stream* stream);
```
当某个线程绑定的连接关闭时的回调函数。

**参数**:
- `stream`: 客户端连接流对象

**注意**: 如果 `thread_on_accept` 返回 `false` 导致关闭时，此函数不会被调用。

---

#### thread_on_init
```cpp
virtual void thread_on_init();
```
当线程池中一个线程被创建时的回调函数。

---

#### thread_on_exit
```cpp
virtual void thread_on_exit();
```
当线程池中一个线程退出时的回调函数。

---

#### proc_exit_timer
```cpp
virtual bool proc_exit_timer(size_t nclients, size_t nthreads);
```
当子进程需要退出时会被调用此函数，可以决定子进程是否退出。退出取决于：
1. 如果此函数返回 `true`，子进程将会退出流程
2. 如果子进程当前所有客户端连接都已关闭，子进程将会退出流程
3. 查看配置文件中的参数 (`ioctl_quick_abort`)，如果该参数不为 0，子进程将会退出流程
4. 等所有客户端连接关闭后再退出

**参数**:
- `nclients`: 当前连接的客户端个数
- `nthreads`: 当前线程池中非忙的工作线程数

**返回值**:
- `true`: 当前子进程可以退出
- `false`: 当前子进程继续运行（默认）

**默认实现**: 返回 `true`

---

## master_proc

多进程服务器模型类，每个连接由一个独立进程处理。

### 头文件
```cpp
#include <acl_cpp/master/master_proc.hpp>
```

### 继承关系
```cpp
class master_proc : public master_base
```

### 公共方法

#### run_daemon
```cpp
void run_daemon(int argc, char** argv);
```
在 daemon 模式下运行，由 acl_master 进程管理。

**参数**:
- `argc`: main 函数的参数个数
- `argv`: main 函数的参数数组

---

#### run_alone
```cpp
bool run_alone(const char* addrs, const char* path = NULL, int count = 1);
```
在独立模式下运行。

**参数**:
- `addrs`: 监听地址列表，格式: "IP:PORT, IP:PORT..."
- `path`: 配置文件全路径（可选）
- `count`: 当值 > 0 时，累计收到连接次数达到该值后函数返回；否则一直循环，永不返回（默认值: 1）

**返回值**:
- `true`: 启动成功
- `false`: 启动失败

---

#### get_conf_path
```cpp
const char* get_conf_path() const;
```
获取配置文件路径。

**返回值**: 配置文件路径，NULL 表示没有配置文件

---

### 纯虚函数（必须实现）

#### on_accept
```cpp
virtual void on_accept(socket_stream* stream) = 0;
```
当收到一个客户端连接时调用此函数。

**参数**:
- `stream`: 客户端连接流对象

**注意**: 此函数返回后连接将被关闭，用户不应该关闭该连接。

---

## master_aio

异步 I/O 服务器模型类，基于事件驱动的异步处理。

### 头文件
```cpp
#include <acl_cpp/master/master_aio.hpp>
```

### 继承关系
```cpp
class master_aio : public master_base, public aio_accept_callback
```

### 公共方法

#### run_daemon
```cpp
void run_daemon(int argc, char** argv);
```
在 daemon 模式下运行，由 acl_master 进程管理。

**参数**:
- `argc`: main 函数的参数个数
- `argv`: main 函数的参数数组

---

#### run_alone
```cpp
bool run_alone(const char* addrs, const char* path = NULL,
    aio_handle_type ht = ENGINE_SELECT);
```
在独立模式下运行。

**参数**:
- `addrs`: 监听地址列表，格式: "IP:PORT, IP:PORT..."
- `path`: 配置文件全路径（可选）
- `ht`: 事件引擎类型（默认值: ENGINE_SELECT）

**返回值**:
- `true`: 启动成功
- `false`: 启动失败

**事件引擎类型**:
- `ENGINE_SELECT`: select
- `ENGINE_POLL`: poll
- `ENGINE_EPOLL`: epoll (Linux)
- `ENGINE_KQUEUE`: kqueue (FreeBSD/MacOS)
- `ENGINE_IOCP`: IOCP (Windows)

---

#### get_handle
```cpp
aio_handle* get_handle() const;
```
获取异步IO的事件句柄。通过此句柄用户可以设置定时器等功能。

**返回值**: 异步事件句柄指针

---

#### stop
```cpp
void stop();
```
在 `run_alone` 模式下，通知服务器可以关闭监听，退出主循环。

---

#### get_conf_path
```cpp
const char* get_conf_path() const;
```
获取配置文件路径。

**返回值**: 配置文件路径，NULL 表示没有配置文件

---

### 纯虚函数（必须实现）

#### on_accept
```cpp
virtual bool on_accept(aio_socket_stream* stream) = 0;
```
当收到一个客户端连接时调用此函数。

**参数**:
- `stream`: 新接收到的客户端异步流对象

**返回值**:
- `true`: 继续接收客户端连接（通常返回 true）
- `false`: 通知服务器不再接受远程客户端连接，并关闭监听套接字

---

## master_trigger

定时触发服务器模型类，按固定间隔执行任务。

### 头文件
```cpp
#include <acl_cpp/master/master_trigger.hpp>
```

### 继承关系
```cpp
class master_trigger : public master_base
```

### 公共方法

#### run_daemon
```cpp
void run_daemon(int argc, char** argv);
```
在 daemon 模式下运行，由 acl_master 进程管理。

**参数**:
- `argc`: main 函数的参数个数
- `argv`: main 函数的参数数组

---

#### run_alone
```cpp
void run_alone(const char* path = NULL, int count = 1, int interval = 1);
```
在独立模式下运行。

**参数**:
- `path`: 配置文件全路径（可选）
- `count`: 当值 > 0 时，触发次数达到该值后函数返回；否则一直循环，永不返回（默认值: 1）
- `interval`: 触发间隔时间（秒）（默认值: 1）

---

#### get_conf_path
```cpp
const char* get_conf_path() const;
```
获取配置文件路径。

**返回值**: 配置文件路径，NULL 表示没有配置文件

---

### 纯虚函数（必须实现）

#### on_trigger
```cpp
virtual void on_trigger() = 0;
```
当定时时间到时调用此函数。

---

## master_udp

UDP 服务器模型类，处理 UDP 数据包。

### 头文件
```cpp
#include <acl_cpp/master/master_udp.hpp>
```

### 继承关系
```cpp
class master_udp : public master_base
```

### 公共方法

#### run_daemon
```cpp
void run_daemon(int argc, char** argv);
```
在 daemon 模式下运行，由 acl_master 进程管理。

**参数**:
- `argc`: main 函数的参数个数
- `argv`: main 函数的参数数组

---

#### run_alone
```cpp
bool run_alone(const char* addrs, const char* path = NULL,
    unsigned int count = 1);
```
在独立模式下运行。

**参数**:
- `addrs`: 监听地址列表，格式: "IP:PORT, IP:PORT..."
- `path`: 配置文件全路径（可选）
- `count`: 循环处理的次数上限，0 表示永久循环（默认值: 1）

**返回值**:
- `true`: 启动成功
- `false`: 启动失败

---

#### get_sstreams
```cpp
const std::vector<socket_stream*>& get_sstreams() const;
```
获得本进程监听的套接字流对象集合。

**返回值**: 套接字流对象的 vector 引用

---

#### get_conf_path
```cpp
const char* get_conf_path() const;
```
获取配置文件路径。

**返回值**: 配置文件路径，NULL 表示没有配置文件

---

#### lock / unlock
```cpp
void lock();
void unlock();
```
锁定/解锁内部互斥锁。

---

### 纯虚函数（必须实现）

#### on_read
```cpp
virtual void on_read(socket_stream* stream) = 0;
```
当 UDP 套接字数据可读时回调此虚函数。此方法在工作线程中调用。

**参数**:
- `stream`: UDP 套接字流对象

---

### 虚函数（可选实现）

#### proc_on_bind
```cpp
virtual void proc_on_bind(socket_stream& stream);
```
当绑定 UDP 地址成功后调用此虚方法。此方法在主进程中被调用。

**参数**:
- `stream`: UDP 套接字流对象引用

---

#### proc_on_unbind
```cpp
virtual void proc_on_unbind(socket_stream& stream);
```
当解绑 UDP 地址时回调此虚方法。此方法在主进程中被调用。

**参数**:
- `stream`: UDP 套接字流对象引用

---

#### thread_on_init
```cpp
virtual void thread_on_init();
```
当线程初始化时调虚方法。

---

## master_fiber

协程服务器模型类，基于协程处理客户端连接。

### 头文件
```cpp
#include <fiber/master_fiber.hpp>
```

### 继承关系
```cpp
class master_fiber : public master_base
```

### 公共方法

#### run_daemon
```cpp
void run_daemon(int argc, char** argv);
```
在 daemon 模式下运行，由 acl_master 进程管理。

**参数**:
- `argc`: main 函数的参数个数
- `argv`: main 函数的参数数组

**使用示例**:
```cpp
int main(int argc, char* argv[]) {
    my_fiber_server server;
    server.run_daemon(argc, argv);
    return 0;
}
```

---

#### run_alone
```cpp
bool run_alone(const char* addrs, const char* path = NULL);
```
在独立模式下运行。

**参数**:
- `addrs`: 监听地址列表，格式: "IP:PORT, IP:PORT..."
- `path`: 配置文件全路径（可选）

**返回值**:
- `true`: 启动成功
- `false`: 启动失败

**使用示例**:
```cpp
int main() {
    my_fiber_server server;
    server.run_alone("127.0.0.1:8080", "server.cf");
    return 0;
}
```

---

#### get_conf_path
```cpp
const char* get_conf_path() const;
```
获取配置文件路径。

**返回值**: 配置文件路径，NULL 表示没有配置文件

---

#### users_count
```cpp
long long users_count();
```
获取当前服务的总连接数。

**返回值**: 当前连接数

---

#### users_count_add
```cpp
long long users_count_add(int n);
```
增加当前连接数的计数。

**参数**:
- `n`: 增加或减少（可以为负数）连接数的值

**返回值**: 修改后的连接数

**使用示例**:
```cpp
// 增加连接计数
long long count = users_count_add(1);

// 减少连接计数
count = users_count_add(-1);
```

---

### 纯虚函数（必须实现）

#### on_accept
```cpp
virtual void on_accept(socket_stream& stream) = 0;
```
当协程服务器接收到客户端连接时调用。

**参数**:
- `stream`: 客户端连接对象

**注意**: 
- 此函数返回后，协程服务框架会关闭连接对象
- 在函数内部可以使用同步方式编程，但实际执行是异步的
- 每个连接在独立的协程中运行

**使用示例**:
```cpp
void on_accept(acl::socket_stream& stream) override {
    char buf[8192];
    
    // 以同步方式读写，但不会阻塞其他协程
    while (true) {
        int ret = stream.read(buf, sizeof(buf), false);
        if (ret <= 0) {
            break;
        }
        
        // 处理数据
        if (stream.write(buf, ret) <= 0) {
            break;
        }
    }
}
```

---

### 虚函数（可选实现）

#### thread_on_init
```cpp
virtual void thread_on_init();
```
当线程初始化时调用的虚函数。

**注意**: 在协程模型中，多个协程可能运行在同一个线程中。

---

#### thread_on_exit
```cpp
virtual void thread_on_exit();
```
当线程退出前调用的虚函数。

---

### 协程特性

#### 协程调度
- **自动调度**: 协程在 I/O 阻塞时自动切换，无需手动管理
- **高并发**: 支持数万到数十万并发连接
- **栈空间**: 每个协程默认占用 128KB 栈空间

#### 同步编程，异步执行
```cpp
void on_accept(acl::socket_stream& stream) override {
    // 看起来是同步代码，实际是异步执行
    stream.write("Hello\r\n", 7);  // 不会阻塞其他协程
    
    char buf[1024];
    int ret = stream.read(buf, sizeof(buf), false);  // 不会阻塞其他协程
    
    // 可以直接使用各种同步 API，协程会自动处理调度
}
```

---

## master_conf

配置管理类，用于加载和管理服务器配置。

### 头文件
```cpp
#include <acl_cpp/master/master_conf.hpp>
```

### 配置表结构

#### master_bool_tbl
布尔型配置参数表结构：
```cpp
typedef struct master_bool_tbl {
    const char *name;    // 配置名称
    int   defval;        // 默认值
    int  *target;        // 目标变量指针
} master_bool_tbl;
```

---

#### master_int_tbl
整型配置参数表结构：
```cpp
typedef struct master_int_tbl {
    const char *name;    // 配置名称
    int  defval;         // 默认值
    int *target;         // 目标变量指针
    int  min;            // 最小值
    int  max;            // 最大值
} master_int_tbl;
```

---

#### master_int64_tbl
64位整型配置参数表结构：
```cpp
typedef struct master_int64_tbl {
    const char *name;         // 配置名称
    long long int  defval;    // 默认值
    long long int *target;    // 目标变量指针
    long long int  min;       // 最小值
    long long int  max;       // 最大值
} master_int64_tbl;
```

---

#### master_str_tbl
字符串配置参数表结构：
```cpp
typedef struct master_str_tbl {
    const char *name;     // 配置名称
    const char *defval;   // 默认值
    char **target;        // 目标变量指针
} master_str_tbl;
```

---

### 公共方法

#### set_cfg_bool
```cpp
void set_cfg_bool(master_bool_tbl* table);
```
设置布尔型配置参数表。

**参数**:
- `table`: 布尔型配置参数表

---

#### set_cfg_int
```cpp
void set_cfg_int(master_int_tbl* table);
```
设置整型配置参数表。

**参数**:
- `table`: 整型配置参数表

---

#### set_cfg_int64
```cpp
void set_cfg_int64(master_int64_tbl* table);
```
设置64位整型配置参数表。

**参数**:
- `table`: 64位整型配置参数表

---

#### set_cfg_str
```cpp
void set_cfg_str(master_str_tbl* table);
```
设置字符串配置参数表。

**参数**:
- `table`: 字符串配置参数表

---

#### load
```cpp
void load(const char* path);
```
加载配置文件。

**参数**:
- `path`: 配置文件全路径

---

#### get_path
```cpp
const char* get_path() const;
```
返回通过 `load` 设置的配置文件路径。

**返回值**: 配置文件路径，NULL 表示没有设置配置文件路径

---

#### reset
```cpp
void reset();
```
重置配置解析状态，释放之前申请的资源。调用此函数后，之前获取的字符串类型的配置内存将被释放，禁止继续访问。调用该函数后如需配置解析对象，需要再次使用解析对象加载配置文件。

---

## 全局函数

### master_log_enable
```cpp
void master_log_enable(bool yes);
```
启用或禁用 master 日志。

**参数**:
- `yes`: true 表示启用，false 表示禁用

---

### master_log_enabled
```cpp
bool master_log_enabled(void);
```
查询 master 日志是否已启用。

**返回值**:
- `true`: 已启用
- `false`: 已禁用

---

## 使用注意事项

1. **继承关系**: 使用时需要从相应的 master 类继承并实现纯虚函数
2. **单例模式**: 每个服务器类只应该有一个实例对象
3. **配置管理**: 配置参数表必须以 NULL 结尾
4. **线程安全**: 注意回调函数的线程安全性，特别是在多线程模型中
5. **资源管理**: 框架会自动管理连接的生命周期，用户不需要手动关闭
6. **编译条件**: 仅在非 `ACL_CLIENT_ONLY` 模式下可用

