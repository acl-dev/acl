# wait_group 协程等待组

## 概述

`wait_group` 是用于等待一组协程完成的同步原语，类似于 Go 语言的 `sync.WaitGroup`。它提供了一种简单有效的方式来等待多个并发任务完成。

## 类定义

```cpp
class FIBER_CPP_API wait_group {
public:
    wait_group();
    ~wait_group();
    
    void add(int n);
    void done();
    void wait();
};
```

## 核心方法

### add()

增加等待计数器的值。

```cpp
void add(int n);
```

**参数：**
- `n`：增加的数量（可以是正数或负数）

**说明：**
- 通常在启动协程前调用
- 可以多次调用累加计数

### done()

减少等待计数器的值（相当于 `add(-1)`）。

```cpp
void done();
```

**说明：**
- 在协程任务完成时调用
- 当计数器降为 0 时，会唤醒所有等待的协程

### wait()

阻塞等待，直到计数器变为 0。

```cpp
void wait();
```

**说明：**
- 会阻塞当前协程，直到所有任务完成
- 可以被多个协程同时调用

## 基本用法

### 简单示例

```cpp
#include <fiber/libfiber.hpp>

void worker(acl::wait_group& wg, int id) {
    printf("Worker %d started\n", id);
    acl::fiber::delay(500);
    printf("Worker %d finished\n", id);
    wg.done();
}

int main() {
    acl::wait_group wg;
    
    // 启动 5 个协程
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

### 批量添加

```cpp
#include <fiber/libfiber.hpp>

void task(acl::wait_group& wg, int id) {
    printf("Task %d running\n", id);
    acl::fiber::delay(100);
    wg.done();
}

int main() {
    acl::wait_group wg;
    const int N = 10;
    
    // 一次性添加多个计数
    wg.add(N);
    
    for (int i = 0; i < N; i++) {
        go[&wg, i] { task(wg, i); };
    }
    
    wg.wait();
    printf("All tasks completed\n");
    
    return 0;
}
```

## 实际应用示例

### 并发下载

```cpp
#include <fiber/libfiber.hpp>
#include <vector>
#include <string>

void download_file(const std::string& url, acl::wait_group& wg) {
    printf("Downloading: %s\n", url.c_str());
    
    // 模拟下载
    acl::fiber::delay(1000);
    
    printf("Downloaded: %s\n", url.c_str());
    wg.done();
}

int main() {
    std::vector<std::string> urls = {
        "http://example.com/file1.zip",
        "http://example.com/file2.zip",
        "http://example.com/file3.zip",
        "http://example.com/file4.zip",
        "http://example.com/file5.zip"
    };
    
    acl::wait_group wg;
    wg.add(urls.size());
    
    for (const auto& url : urls) {
        go[url, &wg] { download_file(url, wg); };
    }
    
    printf("Waiting for all downloads to complete...\n");
    wg.wait();
    printf("All downloads completed!\n");
    
    return 0;
}
```

### 数据处理管道

```cpp
#include <fiber/libfiber.hpp>
#include <vector>

// 第一阶段：生成数据
void generate(acl::channel<int>& ch, int count) {
    for (int i = 0; i < count; i++) {
        ch.put(i);
    }
}

// 第二阶段：处理数据
void process(acl::channel<int>& in, acl::channel<int>& out, 
             acl::wait_group& wg, int id) {
    for (int i = 0; i < 10; i++) {
        int value;
        in.pop(value);
        int result = value * 2;
        printf("Processor %d: %d -> %d\n", id, value, result);
        out.put(result);
    }
    wg.done();
}

// 第三阶段：输出结果
void output(acl::channel<int>& ch, int count) {
    for (int i = 0; i < count; i++) {
        int value;
        ch.pop(value);
        printf("Result: %d\n", value);
    }
}

int main() {
    acl::channel<int> stage1_out, stage2_out;
    acl::wait_group wg;
    const int NUM_PROCESSORS = 3;
    const int TOTAL_DATA = 30;
    
    // 阶段 1：生成
    go[&stage1_out] { generate(stage1_out, TOTAL_DATA); };
    
    // 阶段 2：处理（多个处理器）
    wg.add(NUM_PROCESSORS);
    for (int i = 0; i < NUM_PROCESSORS; i++) {
        go[&stage1_out, &stage2_out, &wg, i] {
            process(stage1_out, stage2_out, wg, i);
        };
    }
    
    // 阶段 3：输出
    go[&stage2_out] { output(stage2_out, TOTAL_DATA); };
    
    // 等待处理完成
    wg.wait();
    printf("Pipeline processing completed\n");
    
    return 0;
}
```

### 分组任务执行

```cpp
#include <fiber/libfiber.hpp>
#include <vector>

struct Task {
    int id;
    std::string name;
    int duration;
};

void execute_task(const Task& task, acl::wait_group& wg) {
    printf("Executing task %d: %s\n", task.id, task.name.c_str());
    acl::fiber::delay(task.duration);
    printf("Task %d completed\n", task.id);
    wg.done();
}

void execute_group(const std::vector<Task>& tasks, const std::string& group_name) {
    acl::wait_group wg;
    wg.add(tasks.size());
    
    printf("Starting group: %s\n", group_name.c_str());
    
    for (const auto& task : tasks) {
        go[task, &wg] { execute_task(task, wg); };
    }
    
    wg.wait();
    printf("Group %s completed\n", group_name.c_str());
}

int main() {
    std::vector<Task> group1 = {
        {1, "Task1", 500},
        {2, "Task2", 300},
        {3, "Task3", 400}
    };
    
    std::vector<Task> group2 = {
        {4, "Task4", 600},
        {5, "Task5", 200}
    };
    
    acl::wait_group main_wg;
    main_wg.add(2);
    
    go[group1, &main_wg] {
        execute_group(group1, "Group1");
        main_wg.done();
    };
    
    go[group2, &main_wg] {
        execute_group(group2, "Group2");
        main_wg.done();
    };
    
    main_wg.wait();
    printf("All groups completed\n");
    
    return 0;
}
```

### 超时控制

```cpp
#include <fiber/libfiber.hpp>
#include <atomic>

void worker(acl::wait_group& wg, std::atomic<int>& counter, int id, int delay) {
    printf("Worker %d started\n", id);
    acl::fiber::delay(delay);
    counter++;
    printf("Worker %d finished\n", id);
    wg.done();
}

int main() {
    acl::wait_group wg;
    std::atomic<int> completed(0);
    const int NUM_WORKERS = 5;
    
    wg.add(NUM_WORKERS);
    
    // 启动工作协程（有的会超时）
    for (int i = 0; i < NUM_WORKERS; i++) {
        int delay = (i + 1) * 500;  // 500, 1000, 1500, 2000, 2500 ms
        go[&wg, &completed, i, delay] { worker(wg, completed, i, delay); };
    }
    
    // 超时监控协程
    bool timeout = false;
    go[&wg, &timeout] {
        acl::fiber::delay(1800);  // 1.8 秒超时
        timeout = true;
        printf("Timeout! Stopping wait.\n");
    };
    
    // 等待协程
    go[&wg, &timeout, &completed] {
        wg.wait();
        if (timeout) {
            printf("Some tasks timed out. Completed: %d\n", completed.load());
        } else {
            printf("All tasks completed: %d\n", completed.load());
        }
        acl::fiber::schedule_stop();
    };
    
    acl::fiber::schedule();
    return 0;
}
```

### 错误处理

```cpp
#include <fiber/libfiber.hpp>
#include <vector>
#include <atomic>

struct Result {
    int id;
    bool success;
    std::string error_msg;
};

void risky_task(int id, acl::wait_group& wg, 
                std::vector<Result>& results, acl::fiber_mutex& mutex) {
    Result result;
    result.id = id;
    
    try {
        printf("Task %d: processing...\n", id);
        acl::fiber::delay(100);
        
        // 模拟随机失败
        if (id % 3 == 0) {
            throw std::runtime_error("Simulated error");
        }
        
        result.success = true;
        printf("Task %d: succeeded\n", id);
    } catch (const std::exception& e) {
        result.success = false;
        result.error_msg = e.what();
        printf("Task %d: failed - %s\n", id, e.what());
    }
    
    // 保存结果
    mutex.lock();
    results.push_back(result);
    mutex.unlock();
    
    wg.done();
}

int main() {
    acl::wait_group wg;
    acl::fiber_mutex mutex;
    std::vector<Result> results;
    const int NUM_TASKS = 10;
    
    wg.add(NUM_TASKS);
    
    for (int i = 0; i < NUM_TASKS; i++) {
        go[i, &wg, &results, &mutex] {
            risky_task(i, wg, results, mutex);
        };
    }
    
    wg.wait();
    
    // 统计结果
    int success_count = 0;
    int failure_count = 0;
    for (const auto& result : results) {
        if (result.success) {
            success_count++;
        } else {
            failure_count++;
        }
    }
    
    printf("\nSummary:\n");
    printf("  Total: %d\n", NUM_TASKS);
    printf("  Success: %d\n", success_count);
    printf("  Failure: %d\n", failure_count);
    
    return 0;
}
```

### 配合协程池使用

```cpp
#include <fiber/libfiber.hpp>

void task(acl::wait_group& wg, int id) {
    printf("Task %d executing\n", id);
    acl::fiber::delay(100);
    wg.done();
}

int main() {
    // 创建协程池：最小1个，最大10个协程
    acl::fiber_pool pool(1, 10, 60, 500, 64000, false);
    acl::wait_group wg;
    
    const int NUM_TASKS = 100;
    wg.add(NUM_TASKS);
    
    // 提交任务到协程池
    for (int i = 0; i < NUM_TASKS; i++) {
        pool.exec([&wg, i]() {
            task(wg, i);
        });
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

## 嵌套使用

```cpp
#include <fiber/libfiber.hpp>

void sub_task(acl::wait_group& wg, int parent_id, int sub_id) {
    printf("  Sub-task %d-%d running\n", parent_id, sub_id);
    acl::fiber::delay(100);
    wg.done();
}

void main_task(acl::wait_group& main_wg, int id) {
    printf("Main task %d started\n", id);
    
    // 创建子任务的 wait_group
    acl::wait_group sub_wg;
    const int NUM_SUB = 3;
    sub_wg.add(NUM_SUB);
    
    for (int i = 0; i < NUM_SUB; i++) {
        go[&sub_wg, id, i] { sub_task(sub_wg, id, i); };
    }
    
    // 等待所有子任务完成
    sub_wg.wait();
    printf("Main task %d completed\n", id);
    
    main_wg.done();
}

int main() {
    acl::wait_group main_wg;
    const int NUM_MAIN = 4;
    main_wg.add(NUM_MAIN);
    
    for (int i = 0; i < NUM_MAIN; i++) {
        go[&main_wg, i] { main_task(main_wg, i); };
    }
    
    main_wg.wait();
    printf("All main tasks completed\n");
    
    return 0;
}
```

## 内部实现

`wait_group` 内部使用原子计数器和 `fiber_tbox` 实现：

```cpp
class wait_group {
private:
    atomic_long state_;
    fiber_tbox<unsigned long>* box_;
};
```

## 注意事项

1. **调用顺序**：
   - 必须在启动协程**之前**调用 `add()`
   - 在任务完成时调用 `done()`
   - 最后调用 `wait()` 等待

2. **计数平衡**：
   - `add()` 的总和必须等于 `done()` 的调用次数
   - 否则 `wait()` 会永久阻塞或提前返回

3. **异常安全**：
   - 确保即使发生异常也要调用 `done()`
   - 可以使用 RAII 包装器

4. **避免竞态**：
   ```cpp
   // 错误：先启动协程再 add
   go[&wg] {
       // ...
       wg.done();
   };
   wg.add(1);  // 可能导致竞态
   
   // 正确：先 add 再启动协程
   wg.add(1);
   go[&wg] {
       // ...
       wg.done();
   };
   ```

5. **多次等待**：
   - 可以多个协程同时调用 `wait()`
   - 所有等待者会在计数归零时同时被唤醒

6. **重用**：
   - `wait_group` 在计数归零后可以重新使用
   - 再次调用 `add()` 开始新的等待周期

## RAII 包装器（可选）

```cpp
class WaitGroupGuard {
public:
    explicit WaitGroupGuard(acl::wait_group& wg) : wg_(wg) {
        wg_.add(1);
    }
    
    ~WaitGroupGuard() {
        wg_.done();
    }
    
private:
    acl::wait_group& wg_;
};

// 使用
void task(acl::wait_group& wg) {
    WaitGroupGuard guard(wg);  // 自动 add
    
    // 执行任务
    acl::fiber::delay(100);
    
    // 析构时自动 done
}
```

## 性能特性

- **轻量级**：基于原子操作和协程通信
- **无锁设计**：使用原子计数器
- **高效唤醒**：计数归零时批量唤醒所有等待者
- **内存占用小**：只包含一个原子变量和一个 tbox 指针

## 最佳实践

1. **使用 RAII**：创建守卫类自动管理 add/done

2. **提前 add**：在启动协程前调用 add()

3. **异常处理**：
   ```cpp
   try {
       // 任务代码
   } catch (...) {
       wg.done();  // 确保调用
       throw;
   }
   wg.done();
   ```

4. **批量添加**：
   ```cpp
   wg.add(10);  // 一次性添加
   for (int i = 0; i < 10; i++) {
       go[&wg] { /* ... */ wg.done(); };
   }
   ```

5. **超时处理**：配合定时器实现超时

6. **错误收集**：使用共享数据结构收集任务结果

## 相关文档

- [go_fiber](go_fiber.md) - Go 风格协程启动
- [fiber_pool](fiber_pool.md) - 协程池
- [channel](channel.md) - 协程通信
- [fiber_tbox](fiber_tbox.md) - 消息队列

