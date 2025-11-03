# Go 风格协程启动 (go_fiber)

## 概述

ACL Fiber 提供了类似 Go 语言的协程启动语法，通过 `go` 关键字可以方便地启动协程。这是 C++11 及以上版本才支持的特性。

## 宏定义

```cpp
#define go                  acl::go_fiber()>
#define go_stack(size)      acl::go_fiber(size, false)>
#define go_share(size)      acl::go_fiber(size, true)>

#define go_wait_fiber       acl::go_fiber()<
#define go_wait_thread      acl::go_fiber()<<
#define go_wait             go_wait_thread
```

## 核心类

```cpp
class go_fiber {
public:
    go_fiber() = default;
    go_fiber(size_t stack_size, bool on);
    
    std::shared_ptr<fiber> operator > (std::function<void()> fn) const;
    void operator < (std::function<void()> fn);
    void operator << (std::function<void()> fn);
};
```

## 基本用法

### go - 启动协程

最简单的协程启动方式，使用默认栈大小（320KB）。

```cpp
#include <fiber/libfiber.hpp>

void simple_func() {
    printf("Hello from fiber!\n");
}

int main() {
    // 启动普通函数
    go simple_func;
    
    // 启动 lambda 表达式
    go[] {
        printf("Lambda fiber\n");
    };
    
    acl::fiber::schedule();
    return 0;
}
```

### go_stack - 指定栈大小

指定协程的栈大小（非共享栈模式）。

```cpp
#include <fiber/libfiber.hpp>

int main() {
    // 使用 128KB 栈大小
    go_stack(128000) [] {
        printf("Fiber with 128KB stack\n");
    };
    
    // 使用 512KB 栈大小
    go_stack(512000) [] {
        printf("Fiber with 512KB stack\n");
    };
    
    acl::fiber::schedule();
    return 0;
}
```

### go_share - 共享栈模式

使用共享栈模式启动协程，可以节省内存。

```cpp
#include <fiber/libfiber.hpp>

int main() {
    // 使用共享栈模式
    go_share(320000) [] {
        printf("Fiber with shared stack\n");
    };
    
    acl::fiber::schedule();
    return 0;
}
```

### go_wait / go_wait_thread - 等待协程完成

在另一个线程中执行函数并等待完成。

```cpp
#include <fiber/libfiber.hpp>

void incr(int& n) {
    n++;
}

int main() {
    int n = 100;
    
    // 在新线程中运行并等待
    go_wait_thread[&] { incr(n); };
    printf("n = %d\n", n);  // n 应该是 101
    
    return 0;
}
```

### go_wait_fiber - 在协程中等待

在协程中执行函数并等待完成。

```cpp
#include <fiber/libfiber.hpp>

void incr(int& n) {
    n++;
}

int main() {
    int n = 200;
    
    // 在新协程中运行并等待
    go_wait_fiber[&] { incr(n); };
    printf("n = %d\n", n);  // n 应该是 201
    
    return 0;
}
```

## Lambda 表达式捕获

### 值捕获

```cpp
#include <fiber/libfiber.hpp>

int main() {
    int value = 42;
    std::string msg = "Hello";
    
    // 按值捕获
    go[=] {
        printf("value = %d, msg = %s\n", value, msg.c_str());
    };
    
    acl::fiber::schedule();
    return 0;
}
```

### 引用捕获

```cpp
#include <fiber/libfiber.hpp>

int main() {
    int counter = 0;
    
    // 按引用捕获
    for (int i = 0; i < 5; i++) {
        go[&counter, i] {
            counter++;
            printf("Fiber %d: counter = %d\n", i, counter);
        };
    }
    
    acl::fiber::schedule();
    printf("Final counter: %d\n", counter);
    return 0;
}
```

### 混合捕获

```cpp
#include <fiber/libfiber.hpp>

int main() {
    int counter = 0;
    std::string prefix = "Fiber";
    
    // 引用捕获 counter，值捕获 prefix
    go[&counter, prefix] {
        counter++;
        printf("%s: counter = %d\n", prefix.c_str(), counter);
    };
    
    acl::fiber::schedule();
    return 0;
}
```

## 获取协程对象

使用 `go` 启动协程会返回 `shared_ptr<fiber>`，可以用于控制协程。

```cpp
#include <fiber/libfiber.hpp>

int main() {
    // 获取协程对象
    auto fb = go[] {
        for (int i = 0; i < 10; i++) {
            printf("Running: %d\n", i);
            acl::fiber::delay(100);
        }
    };
    
    // 在另一个协程中控制它
    go[fb] {
        acl::fiber::delay(500);
        printf("Killing fiber %u\n", fb->get_id());
        fb->kill();
    };
    
    acl::fiber::schedule();
    return 0;
}
```

## 实际应用示例

### 并发任务处理

```cpp
#include <fiber/libfiber.hpp>
#include <vector>

void process_task(int id) {
    printf("Processing task %d...\n", id);
    acl::fiber::delay(1000);
    printf("Task %d completed\n", id);
}

int main() {
    std::vector<std::shared_ptr<acl::fiber>> fibers;
    
    // 启动 10 个并发任务
    for (int i = 0; i < 10; i++) {
        auto fb = go[i] { process_task(i); };
        fibers.push_back(fb);
    }
    
    printf("Started %zu fibers\n", fibers.size());
    
    acl::fiber::schedule();
    return 0;
}
```

### 使用 wait_group

```cpp
#include <fiber/libfiber.hpp>

void worker(acl::wait_group& wg, int id) {
    printf("Worker %d started\n", id);
    acl::fiber::delay(500);
    printf("Worker %d done\n", id);
    wg.done();
}

int main() {
    acl::wait_group wg;
    
    // 启动 5 个工作协程
    for (int i = 0; i < 5; i++) {
        wg.add(1);
        go[&wg, i] { worker(wg, i); };
    }
    
    // 等待所有协程完成
    wg.wait();
    printf("All workers completed\n");
    
    return 0;
}
```

### 协程间通信

```cpp
#include <fiber/libfiber.hpp>

int main() {
    acl::channel<int> ch;
    
    // 生产者
    go[&ch] {
        for (int i = 0; i < 5; i++) {
            ch.put(i);
            printf("Sent: %d\n", i);
        }
    };
    
    // 消费者
    go[&ch] {
        for (int i = 0; i < 5; i++) {
            int value;
            ch.pop(value);
            printf("Received: %d\n", value);
        }
        acl::fiber::schedule_stop();
    };
    
    acl::fiber::schedule();
    return 0;
}
```

### 定时任务

```cpp
#include <fiber/libfiber.hpp>

int main() {
    // 定时打印
    go[] {
        for (int i = 0; i < 10; i++) {
            printf("Tick %d\n", i);
            acl::fiber::delay(1000);  // 每秒一次
        }
        acl::fiber::schedule_stop();
    };
    
    acl::fiber::schedule();
    return 0;
}
```

### 超时控制

```cpp
#include <fiber/libfiber.hpp>

void long_running_task(bool& finished) {
    printf("Task started\n");
    acl::fiber::delay(5000);  // 模拟耗时任务
    finished = true;
    printf("Task finished\n");
}

int main() {
    bool finished = false;
    
    // 启动任务
    go[&finished] { long_running_task(finished); };
    
    // 超时监控
    go[&finished] {
        acl::fiber::delay(3000);  // 3秒超时
        if (!finished) {
            printf("Task timeout!\n");
            acl::fiber::schedule_stop();
        }
    };
    
    acl::fiber::schedule();
    return 0;
}
```

### 并行计算

```cpp
#include <fiber/libfiber.hpp>
#include <vector>

long long sum_range(long long start, long long end) {
    long long sum = 0;
    for (long long i = start; i < end; i++) {
        sum += i;
    }
    return sum;
}

int main() {
    const long long N = 10000000;
    const int NUM_FIBERS = 4;
    const long long CHUNK = N / NUM_FIBERS;
    
    std::vector<long long> results(NUM_FIBERS);
    acl::wait_group wg;
    
    // 并行计算
    for (int i = 0; i < NUM_FIBERS; i++) {
        wg.add(1);
        go[i, &results, &wg] {
            long long start = i * CHUNK;
            long long end = (i == NUM_FIBERS - 1) ? N : (i + 1) * CHUNK;
            results[i] = sum_range(start, end);
            printf("Fiber %d: sum[%lld, %lld) = %lld\n", 
                   i, start, end, results[i]);
            wg.done();
        };
    }
    
    // 等待并汇总
    go[&wg, &results] {
        wg.wait();
        
        long long total = 0;
        for (long long result : results) {
            total += result;
        }
        printf("Total sum: %lld\n", total);
        
        acl::fiber::schedule_stop();
    };
    
    acl::fiber::schedule();
    return 0;
}
```

### HTTP 请求并发

```cpp
#include <fiber/libfiber.hpp>
#include <vector>
#include <string>

struct Response {
    std::string url;
    int status_code;
    std::string body;
};

Response fetch_url(const std::string& url) {
    // 模拟 HTTP 请求
    printf("Fetching: %s\n", url.c_str());
    acl::fiber::delay(500);
    
    Response resp;
    resp.url = url;
    resp.status_code = 200;
    resp.body = "Content from " + url;
    return resp;
}

int main() {
    std::vector<std::string> urls = {
        "http://example.com/page1",
        "http://example.com/page2",
        "http://example.com/page3",
        "http://example.com/page4",
        "http://example.com/page5"
    };
    
    acl::fiber_tbox<Response> results;
    acl::wait_group wg;
    
    // 并发请求
    for (const auto& url : urls) {
        wg.add(1);
        go[url, &results, &wg] {
            Response resp = fetch_url(url);
            results.push(new Response(resp));
            wg.done();
        };
    }
    
    // 收集结果
    go[&wg, &results, &urls] {
        wg.wait();
        
        for (size_t i = 0; i < urls.size(); i++) {
            Response* resp = results.pop();
            printf("Response from %s: %d, %s\n",
                   resp->url.c_str(), resp->status_code, resp->body.c_str());
            delete resp;
        }
        
        acl::fiber::schedule_stop();
    };
    
    acl::fiber::schedule();
    return 0;
}
```

## 与传统方式对比

### 传统继承方式

```cpp
class MyFiber : public acl::fiber {
protected:
    void run() override {
        printf("Hello from fiber\n");
    }
};

int main() {
    MyFiber* fb = new MyFiber();
    fb->start();
    acl::fiber::schedule();
    return 0;
}
```

### go 方式

```cpp
int main() {
    go[] {
        printf("Hello from fiber\n");
    };
    acl::fiber::schedule();
    return 0;
}
```

### 对比

| 特性 | 传统继承方式 | go 方式 |
|------|------------|---------|
| 代码量 | 多 | 少 |
| 灵活性 | 低 | 高 |
| 可读性 | 中 | 高 |
| Lambda 支持 | 否 | 是 |
| 闭包捕获 | 手动 | 自动 |
| 适用场景 | 复杂逻辑 | 简单任务 |

## 内部实现

`go` 宏实际上是 `go_fiber` 类的便捷封装：

```cpp
class go_fiber {
public:
    std::shared_ptr<fiber> operator > (std::function<void()> fn) const {
        auto* ctx = new fiber_ctx(std::move(fn));
        auto fb = fiber::fiber_create(fiber_main, (void*) ctx, 
                                      stack_size_, stack_share_);
        return std::make_shared<fiber>(fb);
    }
    
private:
    static void fiber_main(ACL_FIBER*, void* ctx) {
        auto* fc = (fiber_ctx *) ctx;
        std::function<void()> fn = fc->fn_;
        delete fc;
        fn();
    }
};
```

## 编译要求

使用 `go` 语法需要 C++11 或更高版本：

```bash
# GCC/Clang
g++ -std=c++11 -o myapp myapp.cpp -lacl_fiber -lacl_cpp -lacl

# CMake
set(CMAKE_CXX_STANDARD 11)
```

## 注意事项

1. **C++11 要求**：
   - `go` 语法需要 C++11 支持
   - 使用 lambda 表达式和 `std::function`

2. **变量捕获**：
   - 按引用捕获时注意变量生命周期
   - 确保被捕获的变量在协程运行期间有效

3. **栈大小选择**：
   - 默认 320KB 适合大多数场景
   - 深度递归或大量局部变量需要更大栈
   - 简单任务可以使用更小栈

4. **共享栈模式**：
   - 适合大量短生命周期协程
   - 可以显著节省内存
   - 对性能有一定影响

5. **协程对象生命周期**：
   - `go` 返回 `shared_ptr<fiber>`
   - 可以手动管理协程生命周期
   - 不保存返回值时自动管理

6. **调度器启动**：
   - 记得调用 `fiber::schedule()`
   - 或使用 `fiber::init(..., true)` 启用自动调度

## 最佳实践

1. **优先使用 go 语法**：简洁易读，适合大多数场景

2. **合理捕获变量**：
   ```cpp
   // 好的做法
   go[&wg, id] { worker(wg, id); };
   
   // 避免全部捕获
   go[&] { /* ... */ };  // 除非确实需要
   ```

3. **使用 shared_ptr 管理资源**：
   ```cpp
   auto data = std::make_shared<Data>();
   go[data] { process(data); };
   ```

4. **配合 wait_group 使用**：
   ```cpp
   acl::wait_group wg;
   for (int i = 0; i < N; i++) {
       wg.add(1);
       go[&wg, i] {
           do_work(i);
           wg.done();
       };
   }
   wg.wait();
   ```

5. **异常处理**：
   ```cpp
   go[] {
       try {
           risky_operation();
       } catch (const std::exception& e) {
           printf("Error: %s\n", e.what());
       }
   };
   ```

## 相关文档

- [fiber 类](fiber_class.md) - 协程基类
- [wait_group](wait_group.md) - 协程同步
- [fiber_pool](fiber_pool.md) - 协程池
- [channel](channel.md) - 协程通信

