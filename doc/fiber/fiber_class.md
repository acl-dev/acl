# fiber 类详解

## 概述

`fiber` 是 ACL Fiber 库的核心类，代表一个协程（coroutine）。它是一个纯虚类，需要继承并实现 `run()` 方法。

## 类定义

```cpp
class FIBER_CPP_API fiber {
public:
    fiber();
    explicit fiber(ACL_FIBER *fb);
    explicit fiber(bool running);
    virtual ~fiber();
    
protected:
    virtual void run();
    
private:
    ACL_FIBER* f_;
};
```

## 构造函数

### fiber()

默认构造函数，创建一个未启动的协程对象。

```cpp
acl::fiber fb;
```

### fiber(ACL_FIBER *fb)

使用已存在的 C 语言协程对象创建 C++ 协程对象。

**参数：**
- `fb`：C 语言的协程对象指针

### fiber(bool running)

带运行状态参数的构造函数。

**参数：**
- `running`：
  - `true` - 表示当前协程已启动，仅用于绑定现有协程，禁止调用 `start()` 方法
  - `false` - 需要调用 `start()` 方法启动协程

**示例：**

```cpp
class MyFiber : public acl::fiber {
protected:
    void run() override {
        printf("Fiber is running\n");
    }
};

// 创建未启动的协程
MyFiber fb(false);
fb.start();  // 启动协程
```

## 核心方法

### start()

启动协程。

```cpp
void start(size_t stack_size = 320000, bool share_stack = false);
```

**参数：**
- `stack_size`：协程的初始栈大小（字节），默认 320KB
- `share_stack`：是否使用共享栈模式

**示例：**

```cpp
class MyFiber : public acl::fiber {
protected:
    void run() override {
        // 协程逻辑
    }
};

MyFiber fb;
fb.start();           // 使用默认栈大小
// 或
fb.start(128000);     // 使用 128KB 栈
// 或
fb.start(320000, true);  // 使用共享栈模式
```

### reset()

在协程结束后，将协程句柄设置为 NULL。此方法只能在协程结束后调用。

```cpp
void reset();
```

### kill()

停止当前运行的协程。

```cpp
bool kill(bool sync = false);
```

**参数：**
- `sync`：是否使用同步模式，即是否等待被杀死的协程返回

**返回值：**
- `true` - 成功
- `false` - 协程未启动或已退出

**示例：**

```cpp
auto fb = go[] {
    while (true) {
        acl::fiber::delay(1000);
    }
};

// 异步杀死协程
fb->kill();

// 或同步等待协程退出
fb->kill(true);
```

### killed()

检查当前协程是否已被通知退出。

```cpp
bool killed() const;
```

### self_killed()

检查当前运行的协程是否已被通知退出（静态方法）。

```cpp
static bool self_killed();
```

**示例：**

```cpp
go[] {
    while (!acl::fiber::self_killed()) {
        // 执行任务
        acl::fiber::delay(100);
    }
    printf("Fiber is exiting...\n");
};
```

## 协程信息

### get_id()

获取协程的 ID 号。

```cpp
unsigned int get_id() const;
```

### self()

获取当前运行协程的 ID 号（静态方法）。

```cpp
static unsigned int self();
```

**示例：**

```cpp
go[] {
    printf("Current fiber ID: %u\n", acl::fiber::self());
};
```

### fiber_id()

获取指定协程的 ID 号（静态方法）。

```cpp
static unsigned int fiber_id(const fiber& fb);
```

## 错误处理

### get_errno()

获取协程在执行系统 API 出错后的错误号。

```cpp
int get_errno() const;
```

### set_errno()

设置协程的错误号。

```cpp
void set_errno(int errnum);
```

### last_error()

获取当前协程调用系统 API 后的错误号（静态方法）。

```cpp
static int last_error();
```

### last_serror()

获取当前协程调用系统 API 后的错误消息（静态方法）。

```cpp
static const char* last_serror();
```

### strerror()

将给定的错误号转换为描述信息（静态方法）。

```cpp
static const char* strerror(int errnum, char* buf, size_t size);
```

**示例：**

```cpp
go[] {
    int fd = open("nonexistent.txt", O_RDONLY);
    if (fd < 0) {
        printf("Error: %s\n", acl::fiber::last_serror());
        printf("Error code: %d\n", acl::fiber::last_error());
    }
};
```

## 状态查询

### is_ready()

检查协程是否在等待调度队列中。

```cpp
bool is_ready() const;
```

### is_suspended()

检查协程是否处于挂起状态。

```cpp
bool is_suspended() const;
```

## 协程调度

### init()

显式设置协程调度事件引擎类型并设置自动启动模式。

```cpp
static void init(fiber_event_t type, bool schedule_auto = false);
```

**参数：**
- `type`：事件引擎类型（FIBER_EVENT_T_KERNEL、FIBER_EVENT_T_POLL 等）
- `schedule_auto`：是否自动启动调度器

**示例：**

```cpp
// 使用 epoll/kqueue/iocp 并启用自动调度
acl::fiber::init(acl::FIBER_EVENT_T_KERNEL, true);

go[] {
    printf("Hello\n");
};
// 无需调用 schedule()，会自动调度
```

### schedule()

启动协程调度过程。

```cpp
static void schedule(fiber_event_t type = FIBER_EVENT_T_KERNEL);
```

**参数：**
- `type`：事件引擎类型

**示例：**

```cpp
go[] { printf("Fiber 1\n"); };
go[] { printf("Fiber 2\n"); };

// 启动调度器（会阻塞直到所有协程完成）
acl::fiber::schedule();
```

### schedule_with()

使用指定的事件类型启动协程调度。

```cpp
static void schedule_with(fiber_event_t type);
```

### scheduled()

检查当前线程是否处于协程调度状态。

```cpp
static bool scheduled();
```

### schedule_stop()

停止协程调度过程。

```cpp
static void schedule_stop();
```

**示例：**

```cpp
go[] {
    acl::fiber::delay(5000);
    acl::fiber::schedule_stop();  // 5秒后停止调度
};

acl::fiber::schedule();
```

### schedule_gui()

在 Windows 平台上启用 GUI 协程模式（仅 Windows）。

```cpp
static void schedule_gui();
```

## 协程控制

### yield()

主动放弃当前运行协程的调度权。

```cpp
static void yield();
```

**示例：**

```cpp
go[] {
    for (int i = 0; i < 100; i++) {
        printf("Task %d\n", i);
        if (i % 10 == 0) {
            acl::fiber::yield();  // 让出 CPU
        }
    }
};
```

### switch_to_next()

挂起当前协程并执行等待队列中的下一个协程。

```cpp
static void switch_to_next();
```

### ready()

将指定协程添加到待执行队列。

```cpp
static void ready(fiber& f);
```

### delay()

让当前运行的协程休眠一段时间（毫秒）。

```cpp
static size_t delay(size_t milliseconds);
```

**参数：**
- `milliseconds`：休眠的毫秒数

**返回值：**
- 协程休眠后唤醒时剩余的毫秒数

**示例：**

```cpp
go[] {
    printf("Start\n");
    acl::fiber::delay(1000);  // 休眠 1 秒
    printf("After 1 second\n");
};
```

## 统计信息

### alive_number()

获取活跃协程的数量。

```cpp
static unsigned alive_number();
```

### dead_number()

获取处于退出状态的协程数量。

```cpp
static unsigned dead_number();
```

**示例：**

```cpp
for (int i = 0; i < 100; i++) {
    go[] { acl::fiber::delay(1000); };
}

printf("Alive fibers: %u\n", acl::fiber::alive_number());
```

## 共享栈配置

### set_shared_stack_size()

设置共享栈模式下的共享栈大小。

```cpp
static void set_shared_stack_size(size_t size);
```

**参数：**
- `size`：共享栈大小（字节），内部默认 1024000 字节

### get_shared_stack_size()

获取共享栈模式下的共享栈大小。

```cpp
static size_t get_shared_stack_size();
```

**返回值：**
- 共享栈大小，返回 0 表示未启用共享栈模式

**示例：**

```cpp
// 设置共享栈大小为 2MB
acl::fiber::set_shared_stack_size(2 * 1024 * 1024);

// 启动使用共享栈的协程
MyFiber fb;
fb.start(320000, true);  // 启用共享栈
```

## 高级配置

### set_fdlimit()

设置进程可以创建的最大文件句柄数。

```cpp
static int set_fdlimit(int max);
```

**参数：**
- `max`：最大文件句柄数（仅当 max >= 0 时有效）

**返回值：**
- 当前可用的最大句柄数

### stdout_open()

设置是否将错误消息输出到标准输出。

```cpp
static void stdout_open(bool on);
```

### set_non_blocking()

设置所有协程在连接服务器时使用带超时的非阻塞方式（仅 Windows）。

```cpp
static void set_non_blocking(bool yes);
```

### set_max_cache()

设置缓存的最大数量。

```cpp
static void set_max_cache(int max);
static int get_max_cache();
```

### share_epoll()

设置多个协程是否可以在同一线程中共享一个 epoll 句柄。

```cpp
static void share_epoll(bool yes);
```

### Hook 相关

```cpp
static void acl_io_hook();
static void acl_io_unlock();
static bool winapi_hook();  // Windows only
```

### 事件配置

```cpp
static void set_event_directly(bool yes);
static void set_event_keepio(bool yes);
static void set_event_oneshot(bool yes);
```

## 系统错误

### get_sys_errno()

获取系统错误号。

```cpp
static int get_sys_errno();
```

### set_sys_errno()

设置系统错误号。

```cpp
static void set_sys_errno(int errnum);
```

## 调试功能

### stacktrace()

获取协程的调用栈。

```cpp
static void stacktrace(const fiber& fb, std::vector<fiber_frame>& out, size_t max = 50);
```

**参数：**
- `fb`：目标协程
- `out`：保存结果的向量
- `max`：栈的最大深度

### stackshow()

将协程的调用栈输出到标准输出。

```cpp
static void stackshow(const fiber& fb, size_t max = 50);
```

**示例：**

```cpp
auto fb = go[] {
    // 一些操作
    acl::fiber::delay(1000);
};

// 显示协程调用栈
acl::fiber::stackshow(*fb);
```

## C API 接口

### get_fiber()

返回对应的 C 语言协程对象。

```cpp
ACL_FIBER* get_fiber() const;
```

### fiber_create()

调用 C API 创建一个协程。

```cpp
static ACL_FIBER* fiber_create(
    void (*fn)(ACL_FIBER*, void*),
    void* ctx,
    size_t size,
    bool share_stack = false
);
```

**参数：**
- `fn`：协程的执行入口
- `ctx`：执行函数的上下文
- `size`：协程栈大小
- `share_stack`：是否使用共享栈创建

## 完整示例

### 基础协程

```cpp
#include <fiber/libfiber.hpp>

class WorkerFiber : public acl::fiber {
public:
    WorkerFiber(int id) : id_(id) {}
    
protected:
    void run() override {
        printf("Worker %d started\n", id_);
        
        for (int i = 0; i < 5; i++) {
            printf("Worker %d: step %d\n", id_, i);
            acl::fiber::delay(100);
        }
        
        printf("Worker %d finished\n", id_);
    }
    
private:
    int id_;
};

int main() {
    for (int i = 0; i < 3; i++) {
        WorkerFiber* fb = new WorkerFiber(i);
        fb->start();
    }
    
    acl::fiber::schedule();
    return 0;
}
```

### 错误处理

```cpp
#include <fiber/libfiber.hpp>
#include <fcntl.h>

go[] {
    int fd = open("test.txt", O_RDONLY);
    if (fd < 0) {
        int errnum = acl::fiber::last_error();
        const char* errmsg = acl::fiber::last_serror();
        printf("Open failed: [%d] %s\n", errnum, errmsg);
        return;
    }
    
    // 使用文件描述符
    close(fd);
};

acl::fiber::schedule();
```

### 协程生命周期控制

```cpp
#include <fiber/libfiber.hpp>

int main() {
    auto worker = go[] {
        while (!acl::fiber::self_killed()) {
            printf("Working...\n");
            acl::fiber::delay(500);
        }
        printf("Worker is shutting down\n");
    };
    
    // 主协程
    go[worker] {
        acl::fiber::delay(3000);
        printf("Killing worker\n");
        worker->kill(true);  // 同步等待worker退出
        printf("Worker killed\n");
        
        acl::fiber::schedule_stop();
    };
    
    acl::fiber::schedule();
    return 0;
}
```

## 注意事项

1. **必须实现 run() 方法**：继承 `fiber` 类时必须实现 `run()` 虚函数
2. **调用 schedule()**：创建协程后必须调用 `schedule()` 启动调度器，除非使用了 `init(..., true)` 启用自动调度
3. **栈大小选择**：根据协程的调用深度和局部变量大小选择合适的栈大小
4. **共享栈使用场景**：适合大量短生命周期的协程，可以显著节省内存
5. **阻塞调用**：避免在协程中调用会阻塞线程的系统调用
6. **跨平台差异**：某些功能（如 GUI 模式）仅在特定平台可用

## 相关类

- [fiber_timer](fiber_timer.md) - 定时器协程类
- [go_fiber](go_fiber.md) - Go 风格协程启动
- [wait_group](wait_group.md) - 协程等待组
- [fiber_pool](fiber_pool.md) - 协程池

