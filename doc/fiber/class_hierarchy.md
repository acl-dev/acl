# ACL Fiber 类层次结构

## 概述

本文档描述了 ACL Fiber C++ 库的完整类层次结构和组织关系。

## 核心组件架构

```
ACL Fiber C++ Library
│
├── 协程核心
│   ├── fiber                      // 协程基类
│   ├── fiber_timer                // 定时器协程
│   ├── fiber_trigger<T>           // 定时器管理协程
│   └── go_fiber                   // Go 风格协程启动器
│
├── 同步原语
│   ├── 互斥锁
│   │   ├── fiber_mutex            // 跨线程互斥锁
│   │   ├── fiber_mutex_guard      // 互斥锁 RAII 守卫
│   │   ├── fiber_lock             // 同线程轻量级锁
│   │   └── fiber_rwlock           // 读写锁
│   ├── 条件变量
│   │   └── fiber_cond             // 条件变量
│   ├── 信号量
│   │   ├── fiber_sem              // 信号量
│   │   └── fiber_sem_guard        // 信号量 RAII 守卫
│   └── 事件锁
│       └── fiber_event            // 事件锁
│
├── 通信机制
│   ├── channel<T>                 // 协程间通道
│   ├── fiber_tbox<T*>             // 指针消息队列（条件变量）
│   ├── fiber_tbox2<T>             // 值消息队列（条件变量）
│   ├── fiber_sbox<T*>             // 指针消息队列（信号量）
│   └── fiber_sbox2<T>             // 值消息队列（信号量）
│
├── 协程管理
│   ├── wait_group                 // 协程等待组
│   └── fiber_pool                 // 协程池
│
├── 网络服务
│   ├── master_fiber               // 网络服务主框架
│   ├── http_server                // HTTP 服务器
│   ├── http_servlet               // HTTP Servlet
│   └── tcp_keeper                 // TCP 连接保持器
│
└── 辅助类
    ├── fiber_frame                // 协程栈帧
    ├── fiber_mutex_stat           // 互斥锁状态
    ├── fiber_mutex_stats          // 互斥锁状态集合
    └── task_box<task_fn>          // 任务盒子（fiber_pool 使用）
```

## 类继承关系

### fiber 类族

```
fiber (abstract base class)
├── 用户自定义协程类 (继承并实现 run())
└── fiber_timer (定时器协程)
```

### 同步原语类族

```
互斥锁类
├── fiber_mutex (跨线程互斥锁)
│   └── fiber_mutex_guard (RAII 守卫)
├── fiber_lock (同线程轻量级锁)
└── fiber_rwlock (读写锁)

条件变量类
└── fiber_cond

信号量类
├── fiber_sem
└── fiber_sem_guard (RAII 守卫)

事件类
└── fiber_event
```

### 消息队列类族

```
box<T> (抽象基类)
├── fiber_tbox<T*>
├── fiber_sbox<T*>
└── channel<T> (底层使用 C API)

box2<T> (抽象基类)
├── fiber_tbox2<T>
└── fiber_sbox2<T>
```

### 网络服务类族

```
master_base
└── master_fiber

http_server_impl
└── http_server

http_servlet_impl
└── http_servlet

thread
└── tcp_keeper
```

## 文件组织结构

```
lib_fiber/cpp/include/fiber/
├── fiber.hpp                      // 核心协程类
├── fiber_cpp_define.hpp           // 宏定义和平台相关定义
├── go_fiber.hpp                   // Go 风格协程启动
├── wait_group.hpp                 // 等待组
│
├── 同步原语
│   ├── fiber_mutex.hpp            // 互斥锁
│   ├── fiber_mutex_stat.hpp       // 互斥锁状态
│   ├── fiber_lock.hpp             // 轻量级锁和读写锁
│   ├── fiber_cond.hpp             // 条件变量
│   ├── fiber_sem.hpp              // 信号量和 sbox
│   └── fiber_event.hpp            // 事件锁
│
├── 通信机制
│   ├── channel.hpp                // 通道
│   ├── fiber_tbox.hpp             // tbox 消息队列
│   └── fiber_tbox2.hpp            // tbox2 消息队列
│
├── 协程管理
│   └── fiber_pool.hpp             // 协程池
│
├── 网络服务
│   ├── master_fiber.hpp           // 网络服务主框架
│   ├── http_server.hpp            // HTTP 服务器
│   ├── http_servlet.hpp           // HTTP Servlet
│   ├── tcp_keeper.hpp             // TCP 连接保持器
│   ├── fiber_redis_pipeline.hpp   // Redis 管道
│   └── detail/
│       ├── http_server_impl.hpp   // HTTP 服务器实现
│       └── http_servlet_impl.hpp  // HTTP Servlet 实现
│
└── 主头文件
    ├── lib_fiber.hpp              // 主头文件（别名）
    └── libfiber.hpp               // 主头文件（包含所有）
```

## 核心组件关系图

### 协程生命周期

```
创建协程
    ↓
[fiber 构造] → [start() 启动] → [run() 执行] → [协程结束]
    ↓                                              ↓
[go 宏启动] ─────────────────────────────────→ [自动管理]
    ↓
[fiber_pool]
```

### 同步机制

```
fiber_mutex + fiber_cond → fiber_tbox/fiber_tbox2
fiber_sem → fiber_sbox/fiber_sbox2
fiber_lock/fiber_rwlock → 同线程协程同步
fiber_event → 事件通知
```

### 协程通信

```
生产者协程 → [channel/tbox/sbox] → 消费者协程
     ↓                                    ↓
  push()                               pop()
```

### 网络服务架构

```
master_fiber
    ↓
[on_accept] → socket_stream
    ↓
http_server → http_servlet → 处理请求
```

## 依赖关系

### 核心依赖

```
fiber
├── 依赖: ACL_FIBER (C API)
└── 被依赖: 所有其他类

go_fiber
├── 依赖: fiber, fiber_tbox
└── 要求: C++11

wait_group
├── 依赖: fiber_tbox, atomic_long
└── 被依赖: fiber_pool

fiber_pool
├── 依赖: fiber, fiber_sem, wait_group
└── 要求: C++11
```

### 同步原语依赖

```
fiber_mutex
├── 依赖: ACL_FIBER_MUTEX (C API)
└── 被依赖: fiber_cond, fiber_tbox

fiber_cond
├── 依赖: fiber_mutex, ACL_FIBER_COND (C API)
└── 被依赖: fiber_tbox, fiber_tbox2

fiber_sem
├── 依赖: ACL_FIBER_SEM (C API)
└── 被依赖: fiber_sbox, fiber_sbox2, fiber_pool
```

## 类型定义

### 枚举类型

```cpp
// 事件引擎类型
enum fiber_event_t {
    FIBER_EVENT_T_KERNEL,   // epoll/kqueue/iocp
    FIBER_EVENT_T_POLL,     // poll
    FIBER_EVENT_T_SELECT,   // select
    FIBER_EVENT_T_WMSG,     // Windows 消息
    FIBER_EVENT_T_IO_URING  // io_uring
};

// 信号量属性
enum fiber_sem_attr_t {
    fiber_sem_t_sync,       // 同步模式
    fiber_sem_t_async       // 异步模式
};
```

### 函数类型

```cpp
// 任务函数类型 (fiber_pool)
using task_fn = std::function<void(void)>;

// 协程集合类型
using fibers_set = std::set<std::shared_ptr<fiber>, 
                           std::owner_less<std::shared_ptr<fiber>>>;
```

### 结构体

```cpp
// 协程栈帧
struct fiber_frame {
    std::string func;
    long pc;
    long off;
};

// 互斥锁状态
struct fiber_mutex_stat {
    fiber* fb;
    ACL_FIBER_MUTEX* waiting;
    std::vector<ACL_FIBER_MUTEX*> holding;
};

struct fiber_mutex_stats {
    std::vector<fiber_mutex_stat> stats;
};
```

## 使用层次

### 基础层（必须了解）

1. `fiber` - 协程基类
2. `go_fiber` - 协程启动
3. `fiber::schedule()` - 调度器
4. `fiber_mutex` - 互斥锁

### 通信层

1. `channel` - 简单通道
2. `fiber_tbox` - 功能丰富的消息队列
3. `fiber_cond` - 条件变量

### 管理层

1. `wait_group` - 等待组
2. `fiber_pool` - 协程池

### 应用层

1. `http_server` - HTTP 服务器
2. `master_fiber` - 网络服务框架

## 平台相关

### 跨平台支持

```
Linux
├── fiber_event_t: KERNEL (epoll), POLL, SELECT, IO_URING
└── 所有功能

FreeBSD/macOS
├── fiber_event_t: KERNEL (kqueue), POLL, SELECT
└── 所有功能（除 tcp_keeper）

Windows
├── fiber_event_t: KERNEL (iocp), POLL, SELECT, WMSG
└── 部分功能受限
```

### 编译宏

```cpp
#define FIBER_CPP_API          // DLL 导出/导入
#define ACL_CPP_API            // ACL C++ API 可用标志
#define USE_CPP11              // 使用 C++11 特性
```

## C API 映射

### C 结构体映射

```cpp
// C++ 类 → C 结构体
fiber → ACL_FIBER
fiber_mutex → ACL_FIBER_MUTEX
fiber_cond → ACL_FIBER_COND
fiber_event → ACL_FIBER_EVENT
fiber_sem → ACL_FIBER_SEM
fiber_lock → ACL_FIBER_LOCK
fiber_rwlock → ACL_FIBER_RWLOCK
channel → ACL_CHANNEL
```

### 获取 C 对象

```cpp
// 所有主要类都提供获取底层 C 对象的方法
fiber::get_fiber() → ACL_FIBER*
fiber_mutex::get_mutex() → ACL_FIBER_MUTEX*
fiber_cond::get_cond() → ACL_FIBER_COND*
fiber_event::get_event() → ACL_FIBER_EVENT*
```

## 设计模式

### RAII 模式

```cpp
fiber_mutex_guard    // 互斥锁守卫
fiber_sem_guard      // 信号量守卫
```

### 模板模式

```cpp
fiber                // 抽象基类，子类实现 run()
master_fiber         // 抽象基类，子类实现 on_accept()
```

### 工厂模式

```cpp
fiber::fiber_create()      // 创建 C 协程对象
channel_create()           // 创建 C channel 对象
```

### 观察者模式

```cpp
master_fiber 回调
├── thread_on_init()
├── thread_on_exit()
└── on_accept()

http_server 回调
├── before_proc_jail()
├── on_proc_init()
├── on_proc_exit()
├── on_proc_sighup()
├── on_thread_init()
└── on_thread_accept()
```

## 命名约定

### 类命名

- 小写下划线：`fiber_mutex`, `wait_group`
- 驼峰命名：`go_fiber`, `HttpServer`

### 方法命名

- 小写下划线：`add()`, `get_id()`, `schedule_stop()`
- 静态方法：`fiber::self()`, `fiber::schedule()`

### 宏定义

- 大写：`FIBER_EVENT_T_KERNEL`
- 小写关键字：`go`, `go_stack`, `go_share`

## 最佳实践建议

### 初学者路径

1. 学习 `fiber` 基类和 `go` 宏
2. 了解 `fiber::schedule()` 调度器
3. 使用 `fiber_mutex` 和 `wait_group`
4. 尝试 `channel` 或 `fiber_tbox` 通信

### 进阶路径

1. 使用 `fiber_pool` 管理协程
2. 掌握各种同步原语的使用场景
3. 学习 `master_fiber` 和 `http_server`
4. 优化性能（共享栈、事件引擎选择等）

## 相关文档

- [fiber 类](fiber_class.md)
- [go_fiber](go_fiber.md)
- [同步原语](synchronization.md)
- [channel](channel.md)
- [fiber_tbox](fiber_tbox.md)
- [wait_group](wait_group.md)
- [fiber_pool](fiber_pool.md)
- [README](README.md)

