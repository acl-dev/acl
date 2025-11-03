# ACL Fiber 快速参考

## 协程创建和启动

### 使用 go 宏（推荐）

```cpp
// 启动简单协程
go[] { printf("Hello\n"); };

// 指定栈大小
go_stack(128000) [] { /* ... */ };

// 共享栈模式
go_share(320000) [] { /* ... */ };

// 获取协程对象
auto fb = go[] { /* ... */ };

// 等待协程完成（在协程中）
go_wait_fiber[&] { /* ... */ };

// 等待协程完成（在线程中）
go_wait_thread[&] { /* ... */ };
```

### 继承 fiber 类

```cpp
class MyFiber : public acl::fiber {
protected:
    void run() override {
        printf("Running\n");
    }
};

MyFiber* fb = new MyFiber();
fb->start();  // 启动协程
```

## 调度器

```cpp
// 启动调度器（阻塞直到所有协程完成）
acl::fiber::schedule();

// 指定事件引擎
acl::fiber::schedule(acl::FIBER_EVENT_T_KERNEL);

// 设置自动调度模式
acl::fiber::init(acl::FIBER_EVENT_T_KERNEL, true);

// 停止调度器
acl::fiber::schedule_stop();

// 检查是否在调度中
bool running = acl::fiber::scheduled();
```

## 协程控制

```cpp
// 当前协程休眠（毫秒）
acl::fiber::delay(1000);

// 让出 CPU
acl::fiber::yield();

// 获取当前协程 ID
unsigned int id = acl::fiber::self();

// 检查是否被杀死
if (acl::fiber::self_killed()) { /* ... */ }

// 杀死协程
fb->kill();         // 异步
fb->kill(true);     // 同步等待
```

## 同步原语

### fiber_mutex（互斥锁）

```cpp
acl::fiber_mutex mutex;

mutex.lock();
// 临界区
mutex.unlock();

// 或使用 RAII
acl::fiber_mutex_guard guard(mutex);
```

### fiber_lock（轻量级锁，仅同线程）

```cpp
acl::fiber_lock lock;

lock.lock();
// 临界区
lock.unlock();
```

### fiber_rwlock（读写锁）

```cpp
acl::fiber_rwlock rwlock;

// 读锁
rwlock.rlock();
// 读操作
rwlock.runlock();

// 写锁
rwlock.wlock();
// 写操作
rwlock.wunlock();
```

### fiber_cond（条件变量）

```cpp
acl::fiber_mutex mutex;
acl::fiber_cond cond;

// 等待
mutex.lock();
while (!ready) {
    cond.wait(mutex);  // 或 cond.wait(mutex, timeout_ms)
}
mutex.unlock();

// 通知
mutex.lock();
ready = true;
mutex.unlock();
cond.notify();
```

### fiber_sem（信号量，仅限同线程）

```cpp
acl::fiber_sem sem(5);  // 最大计数 5

sem.wait();      // P 操作，或 sem.wait(timeout_ms)
// 临界区
sem.post();      // V 操作

// 或使用 RAII
acl::fiber_sem_guard guard(sem);
```

**注意**: `fiber_sem` 只能用于同一线程内的协程间同步，不支持跨线程使用。

### fiber_event（事件锁）

```cpp
acl::fiber_event event;

// 等待
event.wait();

// 通知
event.notify();
```

## 协程通信

### channel（通道）

```cpp
acl::channel<int> ch;

// 发送
int value = 42;
ch.put(value);
// 或
ch << value;

// 接收
int result;
ch.pop(result);
```

### fiber_tbox（指针消息队列）

```cpp
acl::fiber_tbox<Message> tbox;

// 发送
Message* msg = new Message();
tbox.push(msg);

// 接收
Message* msg = tbox.pop();           // 永久等待
Message* msg = tbox.pop(1000);       // 超时 1 秒

// 批量接收
std::vector<Message*> batch;
size_t n = tbox.pop(batch, 10, 1000);

// 使用完记得 delete
delete msg;
```

### fiber_tbox2（值消息队列）

```cpp
acl::fiber_tbox2<std::shared_ptr<Data>> tbox;

// 发送
auto data = std::make_shared<Data>();
tbox.push(data);

// 接收
std::shared_ptr<Data> data;
if (tbox.pop(data, 1000)) {
    // 使用 data
}
```

## wait_group（等待组）

```cpp
acl::wait_group wg;

// 添加计数
wg.add(5);

// 启动协程
for (int i = 0; i < 5; i++) {
    go[&wg, i] {
        // 任务代码
        wg.done();  // 完成时调用
    };
}

// 等待所有协程完成
wg.wait();
```

## fiber_pool（协程池）

```cpp
// 创建协程池：最小1个，最大10个
acl::fiber_pool pool(1, 10);

// 提交任务
pool.exec([](int id) {
    printf("Task %d\n", id);
}, 42);

// 或
pool.exec([]() {
    printf("Task\n");
});

// 停止协程池
pool.stop();

// 查询状态
size_t count = pool.get_box_count();
size_t idle = pool.get_box_idle();
```

## 错误处理

```cpp
// 获取错误码
int err = acl::fiber::last_error();

// 获取错误消息
const char* errmsg = acl::fiber::last_serror();

// 设置错误码
acl::fiber::set_sys_errno(EINVAL);

// 清除错误
acl::fiber::clear();
```

## 统计信息

```cpp
// 活跃协程数
unsigned int alive = acl::fiber::alive_number();

// 已结束协程数
unsigned int dead = acl::fiber::dead_number();
```

## 调试

```cpp
// 打印协程栈
auto fb = go[] { /* ... */ };
acl::fiber::stackshow(*fb);

// 获取协程栈
std::vector<acl::fiber_frame> frames;
acl::fiber::stacktrace(*fb, frames);

// 死锁检测
acl::fiber_mutex::deadlock_show();
```

## 配置

```cpp
// 设置共享栈大小
acl::fiber::set_shared_stack_size(2 * 1024 * 1024);

// 设置文件描述符限制
acl::fiber::set_fdlimit(10000);

// 启用错误输出
acl::fiber::stdout_open(true);

// 共享 epoll（Linux）
acl::fiber::share_epoll(true);
```

## HTTP 服务器

```cpp
acl::http_server server("0.0.0.0:8080");

// 注册路由
server.Get("/hello", [](acl::HttpRequest& req, acl::HttpResponse& res) {
    res.setStatus(200)
       .setContentType("text/plain")
       .setBody("Hello");
    return true;
});

server.Post("/data", [](acl::HttpRequest& req, acl::HttpResponse& res) {
    // 处理 POST 请求
    return true;
});

// 启动服务器
server.run_alone("0.0.0.0:8080");
```

## master_fiber（网络服务）

```cpp
class MyService : public acl::master_fiber {
protected:
    void on_accept(acl::socket_stream& stream) override {
        // 处理客户端连接
        char buf[4096];
        int n = stream.read(buf, sizeof(buf));
        if (n > 0) {
            stream.write(buf, n);  // 回显
        }
    }
};

int main(int argc, char* argv[]) {
    MyService service;
    service.run_daemon(argc, argv);  // 守护进程模式
    // 或
    service.run_alone("0.0.0.0:8080");  // 单独运行
    return 0;
}
```

## 常用模式

### 生产者-消费者

```cpp
acl::channel<Task> ch;

// 生产者
go[&ch] {
    for (int i = 0; i < 10; i++) {
        Task task{i};
        ch.put(task);
    }
};

// 消费者
go[&ch] {
    for (int i = 0; i < 10; i++) {
        Task task;
        ch.pop(task);
        process(task);
    }
};
```

### 并发限制

```cpp
acl::fiber_sem sem(5);  // 最多5个并发

for (int i = 0; i < 100; i++) {
    go[&sem, i] {
        acl::fiber_sem_guard guard(sem);
        // 任务代码
    };
}
```

### 超时控制

```cpp
acl::fiber_tbox<Result> tbox;

go[&tbox] {
    Result* res = tbox.pop(5000);  // 5秒超时
    if (!res) {
        printf("Timeout!\n");
    }
};
```

### 批处理

```cpp
acl::wait_group wg;
acl::fiber_pool pool(2, 10);

for (int i = 0; i < 100; i++) {
    wg.add(1);
    pool.exec([&wg, i]() {
        process(i);
        wg.done();
    });
}

wg.wait();
```

## 编译和链接

### GCC/Clang

```bash
g++ -std=c++11 -o myapp myapp.cpp \
    -I/path/to/acl/include \
    -L/path/to/acl/lib \
    -lacl_fiber -lacl_cpp -lacl \
    -lpthread -ldl -lz
```

### CMake

```cmake
find_package(ACL REQUIRED)

add_executable(myapp myapp.cpp)
target_link_libraries(myapp 
    acl_fiber
    acl_cpp
    acl
    pthread
    dl
    z
)
target_compile_features(myapp PRIVATE cxx_std_11)
```

## 性能技巧

```cpp
// 1. 使用共享栈（大量短协程）
fb->start(320000, true);

// 2. 调整栈大小（深度递归）
go_stack(512000) [] { /* ... */ };

// 3. 使用协程池（避免频繁创建）
acl::fiber_pool pool(2, 20);

// 4. 批量操作
std::vector<T*> batch;
tbox.pop(batch, 100, 0);

// 5. 选择合适的事件引擎
acl::fiber::schedule(acl::FIBER_EVENT_T_IO_URING);
```

## 常见陷阱

```cpp
// ❌ 错误：先启动后 add
go[&wg] { /*...*/ wg.done(); };
wg.add(1);  // 竞态条件

// ✅ 正确：先 add 后启动
wg.add(1);
go[&wg] { /*...*/ wg.done(); };

// ❌ 错误：引用捕获临时变量
for (int i = 0; i < 10; i++) {
    go[&i] { printf("%d\n", i); };  // i 可能已改变
}

// ✅ 正确：值捕获
for (int i = 0; i < 10; i++) {
    go[i] { printf("%d\n", i); };
}

// ❌ 错误：忘记调用 schedule()
go[] { printf("Hello\n"); };
// 程序结束，协程没运行

// ✅ 正确：调用 schedule()
go[] { printf("Hello\n"); };
acl::fiber::schedule();
```

## 调试检查清单

1. ✅ 是否调用了 `fiber::schedule()`？
2. ✅ `wait_group` 的 `add()` 和 `done()` 是否配对？
3. ✅ 是否有死锁（使用 `deadlock_show()` 检查）？
4. ✅ Lambda 捕获的变量是否仍然有效？
5. ✅ 指针消息是否正确 delete？
6. ✅ 协程池是否在任务提交前创建？
7. ✅ 是否在协程中调用了阻塞的系统调用？
8. ✅ 栈大小是否足够？

## 相关文档

- [README](README.md) - 总览和快速入门
- [fiber 类](fiber_class.md) - 核心协程类详解
- [同步原语](synchronization.md) - 锁和同步机制
- [通信机制](channel.md) - 协程间通信
- [协程管理](fiber_pool.md) - 协程池和等待组
- [类层次结构](class_hierarchy.md) - 完整架构

