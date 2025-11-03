# fiber_pool 协程池

## 概述

`fiber_pool` 是一个高性能的协程池实现，用于管理和复用协程，避免频繁创建和销毁协程的开销。它支持动态调整池大小、任务队列缓冲以及协程空闲超时等特性。

## 类定义

```cpp
class fiber_pool {
public:
    fiber_pool(size_t min, size_t max, int idle_ms = -1, 
               size_t box_buf = 500, size_t stack_size = 128000, 
               bool stack_share = false);
    ~fiber_pool();
    
    template<class Fn, class ...Args>
    void exec(Fn&& fn, Args&&... args);
    
    void stop();
    
    size_t get_box_min() const;
    size_t get_box_max() const;
    size_t get_box_count() const;
    size_t get_box_idle() const;
    size_t get_box_buf() const;
};
```

## 构造函数

```cpp
fiber_pool(size_t min, size_t max, int idle_ms = -1, 
           size_t box_buf = 500, size_t stack_size = 128000, 
           bool stack_share = false);
```

**参数：**
- `min`：池中最少协程数，可以为 0
- `max`：池中最多协程数
  - 当 `min` = 0 时，`max` 必须 > `min`
  - 当 `min` > 0 时，`max` 可以 >= `min`
- `idle_ms`：协程空闲超时时间（毫秒），-1 表示不超时
- `box_buf`：任务队列缓冲区大小，默认 500
- `stack_size`：每个协程的栈大小，默认 128KB
- `stack_share`：是否使用共享栈模式

## 核心方法

### exec()

在协程池中执行任务。

```cpp
template<class Fn, class ...Args>
void exec(Fn&& fn, Args&&... args);
```

**参数：**
- `fn`：可调用对象（函数、lambda、函数对象等）
- `args`：传递给函数的参数

**特性：**
- 支持完美转发
- 自动选择空闲协程或创建新协程
- 任务队列满时会让出 CPU

### stop()

停止协程池。

```cpp
void stop();
```

**说明：**
- 停止所有协程
- 等待当前正在执行的任务完成

### 查询方法

```cpp
size_t get_box_min() const;    // 获取最小协程数
size_t get_box_max() const;    // 获取最大协程数
size_t get_box_count() const;  // 获取当前协程数
size_t get_box_idle() const;   // 获取空闲协程数
size_t get_box_buf() const;    // 获取任务缓冲区大小
```

## 使用示例

### 基本用法

```cpp
#include <fiber/libfiber.hpp>

void task(int id) {
    printf("Task %d is running in fiber %u\n", id, acl::fiber::self());
    acl::fiber::delay(100);
}

int main() {
    // 创建协程池：最小1个，最大10个协程
    acl::fiber_pool pool(1, 10);
    
    // 提交任务
    for (int i = 0; i < 20; i++) {
        pool.exec(task, i);
    }
    
    // 等待完成后停止
    go[&pool] {
        acl::fiber::delay(5000);
        pool.stop();
    };
    
    acl::fiber::schedule();
    return 0;
}
```

### 使用 Lambda 表达式

```cpp
#include <fiber/libfiber.hpp>

int main() {
    acl::fiber_pool pool(2, 20, 60000, 500, 64000);
    
    // 提交 lambda 任务
    for (int i = 0; i < 50; i++) {
        pool.exec([i]() {
            printf("Task %d executing\n", i);
            acl::fiber::delay(100);
            printf("Task %d done\n", i);
        });
    }
    
    // 等待所有任务完成
    acl::fiber::delay(10000);
    pool.stop();
    
    return 0;
}
```

### 配合 wait_group 使用

```cpp
#include <fiber/libfiber.hpp>

void worker(acl::wait_group& wg, int id) {
    printf("Worker %d started\n", id);
    acl::fiber::delay(500);
    printf("Worker %d finished\n", id);
    wg.done();
}

int main() {
    acl::fiber_pool pool(1, 20, 60000, 500, 64000, false);
    acl::wait_group wg;
    
    const int NUM_TASKS = 100;
    wg.add(NUM_TASKS);
    
    // 提交任务到协程池
    for (int i = 0; i < NUM_TASKS; i++) {
        pool.exec(worker, std::ref(wg), i);
    }
    
    // 等待所有任务完成
    go[&wg, &pool] {
        wg.wait();
        printf("All tasks completed\n");
        pool.stop();
    };
    
    acl::fiber::schedule();
    return 0;
}
```

### 任务返回结果

```cpp
#include <fiber/libfiber.hpp>
#include <vector>

int main() {
    acl::fiber_pool pool(2, 10);
    acl::fiber_tbox<int> results;
    acl::wait_group wg;
    
    // 提交计算任务
    for (int i = 0; i < 20; i++) {
        wg.add(1);
        pool.exec([i, &results, &wg]() {
            int result = i * i;
            printf("Computing %d^2 = %d\n", i, result);
            results.push(new int(result));
            wg.done();
        });
    }
    
    // 收集结果
    go[&wg, &results, &pool] {
        wg.wait();
        
        printf("\nResults:\n");
        for (int i = 0; i < 20; i++) {
            int* result = results.pop();
            printf("%d ", *result);
            delete result;
        }
        printf("\n");
        
        pool.stop();
    };
    
    acl::fiber::schedule();
    return 0;
}
```

### 处理大量文件

```cpp
#include <fiber/libfiber.hpp>
#include <vector>
#include <string>

void process_file(const std::string& filename, acl::wait_group& wg) {
    printf("Processing: %s\n", filename.c_str());
    
    // 模拟文件处理
    acl::fiber::delay(100);
    
    printf("Completed: %s\n", filename.c_str());
    wg.done();
}

int main() {
    // 创建协程池：最多 5 个并发处理
    acl::fiber_pool pool(1, 5);
    acl::wait_group wg;
    
    // 生成文件列表
    std::vector<std::string> files;
    for (int i = 0; i < 50; i++) {
        files.push_back("file_" + std::to_string(i) + ".txt");
    }
    
    wg.add(files.size());
    
    // 提交处理任务
    for (const auto& file : files) {
        pool.exec(process_file, file, std::ref(wg));
    }
    
    // 等待完成
    go[&wg, &pool] {
        wg.wait();
        printf("All files processed\n");
        pool.stop();
    };
    
    acl::fiber::schedule();
    return 0;
}
```

### 网络请求池

```cpp
#include <fiber/libfiber.hpp>
#include <vector>

struct Request {
    int id;
    std::string url;
    int timeout;
};

struct Response {
    int id;
    int status;
    std::string body;
};

void handle_request(const Request& req, acl::fiber_tbox<Response>& responses) {
    printf("Handling request %d: %s\n", req.id, req.url.c_str());
    
    // 模拟网络请求
    acl::fiber::delay(req.timeout);
    
    Response* resp = new Response();
    resp->id = req.id;
    resp->status = 200;
    resp->body = "Response from " + req.url;
    
    responses.push(resp);
}

int main() {
    // 创建协程池：最多 10 个并发请求
    acl::fiber_pool pool(2, 10);
    acl::fiber_tbox<Response> responses;
    acl::wait_group wg;
    
    // 生成请求
    std::vector<Request> requests;
    for (int i = 0; i < 30; i++) {
        requests.push_back({
            i, 
            "http://example.com/api/" + std::to_string(i),
            100 + (i % 5) * 50
        });
    }
    
    wg.add(requests.size());
    
    // 提交请求
    for (const auto& req : requests) {
        pool.exec([req, &responses, &wg]() {
            handle_request(req, responses);
            wg.done();
        });
    }
    
    // 处理响应
    go[&wg, &responses, &pool, &requests] {
        wg.wait();
        
        printf("\nProcessing responses:\n");
        for (size_t i = 0; i < requests.size(); i++) {
            Response* resp = responses.pop();
            printf("Response %d: status=%d, body=%s\n",
                   resp->id, resp->status, resp->body.c_str());
            delete resp;
        }
        
        pool.stop();
    };
    
    acl::fiber::schedule();
    return 0;
}
```

### 定时任务调度

```cpp
#include <fiber/libfiber.hpp>

class TaskScheduler {
public:
    TaskScheduler() : pool_(1, 10), running_(true) {
        start_scheduler();
    }
    
    void schedule(std::function<void()> task, int delay_ms) {
        pool_.exec([this, task, delay_ms]() {
            acl::fiber::delay(delay_ms);
            if (running_) {
                task();
            }
        });
    }
    
    void stop() {
        running_ = false;
        pool_.stop();
    }
    
private:
    acl::fiber_pool pool_;
    bool running_;
    
    void start_scheduler() {
        go[this] {
            acl::fiber::schedule();
        };
    }
};

int main() {
    TaskScheduler scheduler;
    
    // 调度任务
    scheduler.schedule([]() {
        printf("Task 1 executed\n");
    }, 1000);
    
    scheduler.schedule([]() {
        printf("Task 2 executed\n");
    }, 2000);
    
    scheduler.schedule([]() {
        printf("Task 3 executed\n");
    }, 3000);
    
    // 等待所有任务完成
    acl::fiber::delay(5000);
    scheduler.stop();
    
    return 0;
}
```

## 高级应用

### 动态调整并发度

```cpp
#include <fiber/libfiber.hpp>

class DynamicPool {
public:
    DynamicPool() : pool_(1, 50) {}
    
    void adjust_concurrency(size_t target) {
        size_t current = pool_.get_box_count();
        printf("Current: %zu, Target: %zu\n", current, target);
        
        // 协程池会自动调整到需要的数量
        // 这里只是显示当前状态
    }
    
    void submit_task(std::function<void()> task) {
        pool_.exec(task);
    }
    
    void stop() {
        pool_.stop();
    }
    
    void print_stats() {
        printf("Stats: min=%zu, max=%zu, count=%zu, idle=%zu\n",
               pool_.get_box_min(), pool_.get_box_max(),
               pool_.get_box_count(), pool_.get_box_idle());
    }
    
private:
    acl::fiber_pool pool_;
};

int main() {
    DynamicPool pool;
    
    // 提交少量任务
    for (int i = 0; i < 5; i++) {
        pool.submit_task([i]() {
            printf("Light task %d\n", i);
            acl::fiber::delay(100);
        });
    }
    
    acl::fiber::delay(500);
    pool.print_stats();
    
    // 提交大量任务
    for (int i = 0; i < 50; i++) {
        pool.submit_task([i]() {
            printf("Heavy task %d\n", i);
            acl::fiber::delay(500);
        });
    }
    
    acl::fiber::delay(1000);
    pool.print_stats();
    
    acl::fiber::delay(5000);
    pool.stop();
    
    return 0;
}
```

### 优先级任务队列

```cpp
#include <fiber/libfiber.hpp>
#include <queue>

enum Priority { LOW, NORMAL, HIGH };

struct Task {
    Priority priority;
    std::function<void()> func;
    
    bool operator<(const Task& other) const {
        return priority < other.priority;  // 优先级队列：大的优先
    }
};

class PriorityPool {
public:
    PriorityPool() : pool_(2, 20), running_(true) {
        start_worker();
    }
    
    void submit(Priority priority, std::function<void()> task) {
        mutex_.lock();
        tasks_.push({priority, task});
        mutex_.unlock();
        
        cond_.notify();
    }
    
    void stop() {
        running_ = false;
        cond_.notify();
        pool_.stop();
    }
    
private:
    acl::fiber_pool pool_;
    acl::fiber_mutex mutex_;
    acl::fiber_cond cond_;
    std::priority_queue<Task> tasks_;
    bool running_;
    
    void start_worker() {
        go[this] {
            while (running_) {
                mutex_.lock();
                
                while (tasks_.empty() && running_) {
                    cond_.wait(mutex_);
                }
                
                if (!running_) {
                    mutex_.unlock();
                    break;
                }
                
                Task task = tasks_.top();
                tasks_.pop();
                mutex_.unlock();
                
                // 提交到协程池执行
                pool_.exec(task.func);
            }
        };
    }
};

int main() {
    PriorityPool pool;
    acl::wait_group wg;
    
    // 提交不同优先级的任务
    for (int i = 0; i < 10; i++) {
        wg.add(1);
        Priority p = (i % 3 == 0) ? HIGH : (i % 2 == 0) ? NORMAL : LOW;
        pool.submit(p, [i, p, &wg]() {
            const char* prio_str = (p == HIGH) ? "HIGH" : 
                                   (p == NORMAL) ? "NORMAL" : "LOW";
            printf("Task %d [%s] executing\n", i, prio_str);
            acl::fiber::delay(100);
            wg.done();
        });
    }
    
    go[&wg, &pool] {
        wg.wait();
        printf("All tasks completed\n");
        pool.stop();
    };
    
    acl::fiber::schedule();
    return 0;
}
```

## 性能优化

### 选择合适的参数

```cpp
// 轻量级任务，频繁创建销毁
acl::fiber_pool pool1(5, 50, 30000, 1000, 64000, false);

// 重量级任务，长时间运行
acl::fiber_pool pool2(10, 20, -1, 100, 256000, false);

// 大量短任务，使用共享栈
acl::fiber_pool pool3(2, 100, 10000, 2000, 128000, true);
```

### 避免任务队列溢出

```cpp
void submit_with_backpressure(acl::fiber_pool& pool, 
                               std::function<void()> task) {
    // 如果任务队列过大，等待一下
    while (pool.get_box_buf() > 100) {
        acl::fiber::delay(10);
    }
    pool.exec(task);
}
```

## 注意事项

1. **参数配置**：
   - `min` 和 `max` 要根据实际负载合理配置
   - `idle_ms` 设置为 -1 表示协程永不超时
   - `box_buf` 影响任务排队能力

2. **生命周期**：
   - 确保协程池在所有任务提交前创建
   - 调用 `stop()` 后不能再提交任务
   - 协程池对象析构时会自动停止

3. **任务捕获**：
   - 使用 lambda 时注意变量捕获
   - 避免捕获临时对象的引用
   - 优先使用值捕获或智能指针

4. **错误处理**：
   - 任务中的异常不会传播到外部
   - 需要在任务内部捕获处理

5. **性能考虑**：
   - 协程池适合 I/O 密集型任务
   - CPU 密集型任务可能不适合
   - 避免在任务中长时间占用 CPU

## 最佳实践

1. **合理设置池大小**：
   ```cpp
   // 根据 CPU 核心数设置
   size_t ncpu = std::thread::hardware_concurrency();
   acl::fiber_pool pool(ncpu, ncpu * 2);
   ```

2. **使用 wait_group 同步**：
   ```cpp
   acl::wait_group wg;
   wg.add(N);
   for (int i = 0; i < N; i++) {
       pool.exec([&wg]() {
           // 任务代码
           wg.done();
       });
   }
   wg.wait();
   ```

3. **异常安全**：
   ```cpp
   pool.exec([&wg]() {
       try {
           // 任务代码
       } catch (...) {
           wg.done();
           throw;
       }
       wg.done();
   });
   ```

4. **避免死锁**：
   - 不要在任务中等待其他任务完成
   - 使用 wait_group 协调任务依赖

5. **资源管理**：
   - 使用智能指针管理动态分配的资源
   - 确保任务完成后资源被正确释放

## 相关文档

- [go_fiber](go_fiber.md) - 协程启动
- [wait_group](wait_group.md) - 任务同步
- [fiber 类](fiber_class.md) - 协程基类
- [fiber_tbox](fiber_tbox.md) - 消息队列

