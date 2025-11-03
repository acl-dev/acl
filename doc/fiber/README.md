# ACL Fiber C++ 库文档

## 概述

ACL Fiber 是一个高性能的 C++ 协程库，提供了类似 Golang 风格的协程编程体验。它支持多种操作系统（Linux、FreeBSD、MacOS、Windows）和多种事件引擎（epoll、kqueue、iocp、poll、select、io_uring）。

## 主要特性

- **高性能协程调度**：支持多种事件引擎，可根据平台自动选择最优方案
- **完整的同步原语**：提供互斥锁、条件变量、信号量、事件等同步机制
- **Go 风格的语法**：支持 `go` 关键字启动协程，语法简洁优雅
- **通道通信**：支持协程间的 Channel 通信
- **协程池**：提供高效的协程池管理
- **网络服务框架**：内置 HTTP 服务器和主服务框架
- **跨平台支持**：支持 Linux、FreeBSD、MacOS、Windows 等主流操作系统
- **共享栈模式**：支持共享栈模式以节省内存

## 核心组件

### 基础组件

- [fiber](fiber_class.md) - 核心协程类
- [go_fiber](go_fiber.md) - Go 风格的协程启动
- [wait_group](wait_group.md) - 协程等待组

### 同步原语

- [fiber_mutex](synchronization.md#fiber_mutex) - 互斥锁（支持跨线程）
- [fiber_lock](synchronization.md#fiber_lock) - 轻量级锁（仅限同线程）
- [fiber_rwlock](synchronization.md#fiber_rwlock) - 读写锁（仅限同线程）
- [fiber_sem](synchronization.md#fiber_sem) - 信号量（仅限同线程）
- [fiber_cond](synchronization.md#fiber_cond) - 条件变量（支持跨线程）
- [fiber_event](synchronization.md#fiber_event) - 事件锁（支持跨线程）

### 通信机制

- [channel](channel.md) - 协程间通道通信
- [fiber_tbox](fiber_tbox.md) - 指针类型消息队列
- [fiber_tbox2](fiber_tbox.md#fiber_tbox2) - 值类型消息队列
- [fiber_sbox](fiber_tbox.md#fiber_sbox) - 基于信号量的消息队列
- [fiber_sbox2](fiber_tbox.md#fiber_sbox2) - 基于信号量的值类型消息队列

### 高级功能

- [fiber_pool](fiber_pool.md) - 协程池
- [http_server](http_server.md) - HTTP 服务器
- [master_fiber](master_fiber.md) - 网络服务主框架
- [tcp_keeper](tcp_keeper.md) - TCP 连接池管理器

## 快速开始

### 基本示例

```cpp
#include <acl_cpp/lib_acl.hpp>
#include <fiber/libfiber.hpp>

// 简单的协程函数
static void fiber_func() {
    printf("Hello from fiber %u\n", acl::fiber::self());
}

int main() {
    // 使用 go 关键字启动协程
    go fiber_func;
    
    // 启动协程调度器
    acl::fiber::schedule();
    
    return 0;
}
```

### Lambda 表达式示例

```cpp
#include <fiber/libfiber.hpp>

int main() {
    int count = 0;
    
    // 使用 lambda 启动协程
    go[&] {
        printf("Count: %d\n", ++count);
    };
    
    acl::fiber::schedule();
    return 0;
}
```

### 使用 wait_group

```cpp
#include <fiber/libfiber.hpp>

void worker(acl::wait_group& wg, int id) {
    printf("Worker %d running\n", id);
    wg.done();
}

int main() {
    acl::wait_group wg;
    
    for (int i = 0; i < 10; i++) {
        wg.add(1);
        go[&wg, i] {
            worker(wg, i);
        };
    }
    
    wg.wait();
    printf("All workers done\n");
    
    return 0;
}
```

### 使用协程池

```cpp
#include <fiber/libfiber.hpp>

int main() {
    // 创建协程池：最小1个，最大20个协程
    acl::fiber_pool pool(1, 20, 60, 500, 64000, false);
    acl::wait_group wg;
    
    for (int i = 0; i < 100; i++) {
        wg.add(1);
        pool.exec([&wg, i]() {
            printf("Task %d is running\n", i);
            wg.done();
        });
    }
    
    go[&wg, &pool] {
        wg.wait();
        pool.stop();
    };
    
    acl::fiber::schedule();
    return 0;
}
```

### HTTP 服务器示例

```cpp
#include <fiber/libfiber.hpp>

int main() {
    acl::http_server server("0.0.0.0:8080");
    
    // 注册 GET 路由
    server.Get("/hello", [](acl::HttpRequest&, acl::HttpResponse& res) {
        res.setStatus(200)
           .setContentType("text/plain")
           .setBody("Hello, World!");
        return true;
    });
    
    // 启动服务器
    server.run_alone("0.0.0.0:8080");
    
    return 0;
}
```

## 事件引擎类型

ACL Fiber 支持多种事件引擎：

| 类型 | 说明 | 支持平台 |
|------|------|----------|
| `FIBER_EVENT_T_KERNEL` | 内核级事件引擎（epoll/kqueue/iocp） | Linux/FreeBSD/MacOS/Windows |
| `FIBER_EVENT_T_POLL` | poll 引擎 | Linux/FreeBSD/MacOS/Windows |
| `FIBER_EVENT_T_SELECT` | select 引擎 | Linux/FreeBSD/MacOS/Windows |
| `FIBER_EVENT_T_WMSG` | Windows 消息引擎 | Windows |
| `FIBER_EVENT_T_IO_URING` | io_uring 引擎 | Linux 5.1+ |

### 设置事件引擎

```cpp
// 显式设置事件引擎类型并启用自动调度
acl::fiber::init(acl::FIBER_EVENT_T_KERNEL, true);

// 或者在启动调度器时指定
acl::fiber::schedule(acl::FIBER_EVENT_T_KERNEL);
```

## 编译和链接

### 编译选项

```bash
# Linux
g++ -o myapp myapp.cpp -I/path/to/acl/include \
    -L/path/to/acl/lib -lacl_fiber -lacl_cpp -lacl \
    -lpthread -ldl -lz -std=c++11

# macOS
clang++ -o myapp myapp.cpp -I/path/to/acl/include \
    -L/path/to/acl/lib -lacl_fiber -lacl_cpp -lacl \
    -lpthread -lz -std=c++11
```

### CMake 配置

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

set_property(TARGET myapp PROPERTY CXX_STANDARD 11)
```

## 性能优化建议

1. **共享栈模式**：对于大量短生命周期协程，考虑使用共享栈模式
   ```cpp
   fiber.start(320000, true);  // 启用共享栈
   ```

2. **调整栈大小**：根据实际需求调整协程栈大小
   ```cpp
   go_stack(128000) my_function;  // 指定 128KB 栈大小
   ```

3. **使用协程池**：避免频繁创建销毁协程，使用协程池复用

4. **选择合适的事件引擎**：
   - Linux：优先使用 `FIBER_EVENT_T_IO_URING`（需要内核 5.1+）
   - FreeBSD/MacOS：使用 `FIBER_EVENT_T_KERNEL`（kqueue）
   - Windows：使用 `FIBER_EVENT_T_KERNEL`（iocp）

5. **避免阻塞调用**：使用协程提供的异步 API，避免阻塞整个调度器

## 注意事项

1. **线程安全**：
   - `fiber_mutex`、`fiber_cond`、`fiber_event` 支持跨线程使用
   - `fiber_lock`、`fiber_rwlock`、`fiber_sem` 仅支持同线程内的协程间同步

2. **调度器启动**：
   - 调用 `fiber::schedule()` 会阻塞当前线程直到所有协程完成
   - 可以使用 `fiber::init(..., true)` 启用自动调度模式

3. **资源管理**：
   - 协程对象会自动管理生命周期
   - 使用 `shared_ptr<fiber>` 可以手动管理协程对象

4. **错误处理**：
   - 使用 `fiber::last_error()` 获取系统调用错误码
   - 使用 `fiber::last_serror()` 获取错误描述

## 相关文档

- [类层次结构](class_hierarchy.md) - 完整的类继承关系
- [API 参考](api_reference.md) - 详细的 API 文档
- [示例代码](examples.md) - 更多示例代码
- [常见问题](faq.md) - 常见问题解答

## 相关链接

- [ACL 项目主页](https://github.com/acl-dev/acl)
- [在线文档](https://acl-dev.github.io/acl/)
- [问题反馈](https://github.com/acl-dev/acl/issues)

## 许可证

ACL 库采用 Apache License 2.0 许可证。

