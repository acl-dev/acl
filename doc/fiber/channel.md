# Channel 通道通信

## 概述

`channel` 是 ACL Fiber 提供的协程间通道通信机制，类似于 Go 语言的 channel。它提供了一种类型安全、线程安全的协程间通信方式。

## 类定义

```cpp
template <typename T>
class channel {
public:
    channel();
    ~channel();
    
    channel& operator << (T& t);
    channel& put(T& t);
    void pop(T& t);
};
```

## 特性

- **类型安全**：使用模板实现，编译时类型检查
- **阻塞式通信**：发送和接收操作都是阻塞的
- **缓冲区**：内置缓冲区，默认大小为 100
- **简洁的语法**：支持 `<<` 运算符

## 构造函数

### channel()

创建一个 channel 对象，默认缓冲区大小为 100。

```cpp
acl::channel<int> ch;
```

## 核心方法

### put() / operator<<

向通道发送数据。

```cpp
channel& put(T& t);
channel& operator << (T& t);
```

**参数：**
- `t`：要发送的数据引用

**返回值：**
- 返回 channel 引用，支持链式调用

**特性：**
- 如果通道缓冲区已满，发送操作会阻塞
- 阻塞的发送协程会在有空间时被唤醒

### pop()

从通道接收数据。

```cpp
void pop(T& t);
```

**参数：**
- `t`：接收数据的引用

**特性：**
- 如果通道为空，接收操作会阻塞
- 阻塞的接收协程会在有数据时被唤醒

## 使用示例

### 基本示例

```cpp
#include <fiber/libfiber.hpp>

int main() {
    acl::channel<int> ch;
    
    // 发送者协程
    go[&ch] {
        for (int i = 0; i < 10; i++) {
            ch.put(i);
            printf("Sent: %d\n", i);
        }
    };
    
    // 接收者协程
    go[&ch] {
        for (int i = 0; i < 10; i++) {
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

### 使用 << 运算符

```cpp
#include <fiber/libfiber.hpp>

int main() {
    acl::channel<std::string> ch;
    
    // 发送者
    go[&ch] {
        std::string msg1 = "Hello";
        std::string msg2 = "World";
        
        ch << msg1 << msg2;  // 链式调用
        printf("Sent: %s, %s\n", msg1.c_str(), msg2.c_str());
    };
    
    // 接收者
    go[&ch] {
        std::string msg1, msg2;
        ch.pop(msg1);
        ch.pop(msg2);
        printf("Received: %s, %s\n", msg1.c_str(), msg2.c_str());
        
        acl::fiber::schedule_stop();
    };
    
    acl::fiber::schedule();
    return 0;
}
```

### 多生产者-单消费者

```cpp
#include <fiber/libfiber.hpp>

void producer(acl::channel<int>& ch, int id) {
    for (int i = 0; i < 5; i++) {
        int value = id * 100 + i;
        ch.put(value);
        printf("Producer %d: sent %d\n", id, value);
        acl::fiber::delay(100);
    }
}

void consumer(acl::channel<int>& ch, int count) {
    for (int i = 0; i < count; i++) {
        int value;
        ch.pop(value);
        printf("Consumer: received %d\n", value);
    }
    
    acl::fiber::schedule_stop();
}

int main() {
    acl::channel<int> ch;
    
    // 启动 3 个生产者
    for (int i = 0; i < 3; i++) {
        go[&ch, i] { producer(ch, i); };
    }
    
    // 启动 1 个消费者
    go[&ch] { consumer(ch, 15); };  // 3 个生产者 * 5 条消息
    
    acl::fiber::schedule();
    return 0;
}
```

### 单生产者-多消费者

```cpp
#include <fiber/libfiber.hpp>

void producer(acl::channel<int>& ch, int count) {
    for (int i = 0; i < count; i++) {
        ch.put(i);
        printf("Producer: sent %d\n", i);
        acl::fiber::delay(50);
    }
}

void consumer(acl::channel<int>& ch, int id, int count) {
    for (int i = 0; i < count; i++) {
        int value;
        ch.pop(value);
        printf("Consumer %d: received %d\n", id, value);
    }
}

int main() {
    acl::channel<int> ch;
    acl::wait_group wg;
    
    // 启动生产者
    go[&ch] { producer(ch, 20); };
    
    // 启动 4 个消费者，每个消费 5 条消息
    for (int i = 0; i < 4; i++) {
        wg.add(1);
        go[&ch, i, &wg] {
            consumer(ch, i, 5);
            wg.done();
        };
    }
    
    go[&wg] {
        wg.wait();
        acl::fiber::schedule_stop();
    };
    
    acl::fiber::schedule();
    return 0;
}
```

### 传递复杂对象

```cpp
#include <fiber/libfiber.hpp>
#include <string>

struct Task {
    int id;
    std::string name;
    int priority;
    
    Task() : id(0), priority(0) {}
    Task(int i, const std::string& n, int p) 
        : id(i), name(n), priority(p) {}
};

int main() {
    acl::channel<Task> ch;
    
    // 任务生产者
    go[&ch] {
        for (int i = 0; i < 5; i++) {
            Task task(i, "Task_" + std::to_string(i), i * 10);
            ch.put(task);
            printf("Sent task: %d, %s, priority=%d\n", 
                   task.id, task.name.c_str(), task.priority);
        }
    };
    
    // 任务消费者
    go[&ch] {
        for (int i = 0; i < 5; i++) {
            Task task;
            ch.pop(task);
            printf("Received task: %d, %s, priority=%d\n",
                   task.id, task.name.c_str(), task.priority);
        }
        
        acl::fiber::schedule_stop();
    };
    
    acl::fiber::schedule();
    return 0;
}
```

### 使用智能指针

```cpp
#include <fiber/libfiber.hpp>
#include <memory>

struct Data {
    int value;
    std::string info;
    
    Data(int v, const std::string& i) : value(v), info(i) {}
    ~Data() { printf("Data destroyed: %d\n", value); }
};

int main() {
    acl::channel<std::shared_ptr<Data>> ch;
    
    // 发送者
    go[&ch] {
        for (int i = 0; i < 5; i++) {
            auto data = std::make_shared<Data>(i, "info_" + std::to_string(i));
            ch.put(data);
            printf("Sent: %d\n", data->value);
        }
    };
    
    // 接收者
    go[&ch] {
        for (int i = 0; i < 5; i++) {
            std::shared_ptr<Data> data;
            ch.pop(data);
            printf("Received: %d, %s\n", data->value, data->info.c_str());
        }
        
        acl::fiber::schedule_stop();
    };
    
    acl::fiber::schedule();
    return 0;
}
```

### Pipeline 模式

```cpp
#include <fiber/libfiber.hpp>

// 阶段 1：生成数字
void generate(acl::channel<int>& out) {
    for (int i = 1; i <= 10; i++) {
        out.put(i);
    }
}

// 阶段 2：平方
void square(acl::channel<int>& in, acl::channel<int>& out) {
    for (int i = 0; i < 10; i++) {
        int value;
        in.pop(value);
        int result = value * value;
        out.put(result);
        printf("Square: %d -> %d\n", value, result);
    }
}

// 阶段 3：输出
void output(acl::channel<int>& in) {
    for (int i = 0; i < 10; i++) {
        int value;
        in.pop(value);
        printf("Output: %d\n", value);
    }
    
    acl::fiber::schedule_stop();
}

int main() {
    acl::channel<int> ch1, ch2;
    
    go[&ch1] { generate(ch1); };
    go[&ch1, &ch2] { square(ch1, ch2); };
    go[&ch2] { output(ch2); };
    
    acl::fiber::schedule();
    return 0;
}
```

### Fan-out/Fan-in 模式

```cpp
#include <fiber/libfiber.hpp>
#include <vector>

// 工作协程
void worker(int id, acl::channel<int>& in, acl::channel<int>& out) {
    while (true) {
        int value;
        in.pop(value);
        
        if (value < 0) break;  // 结束信号
        
        // 模拟处理
        int result = value * 2;
        printf("Worker %d: %d -> %d\n", id, value, result);
        acl::fiber::delay(100);
        
        out.put(result);
    }
}

int main() {
    acl::channel<int> jobs, results;
    const int NUM_WORKERS = 3;
    const int NUM_JOBS = 10;
    
    // 启动多个工作协程（Fan-out）
    for (int i = 0; i < NUM_WORKERS; i++) {
        go[i, &jobs, &results] { worker(i, jobs, results); };
    }
    
    // 分发任务
    go[&jobs] {
        for (int i = 0; i < NUM_JOBS; i++) {
            jobs.put(i);
        }
        
        // 发送结束信号
        for (int i = 0; i < NUM_WORKERS; i++) {
            int end_signal = -1;
            jobs.put(end_signal);
        }
    };
    
    // 收集结果（Fan-in）
    go[&results] {
        for (int i = 0; i < NUM_JOBS; i++) {
            int result;
            results.pop(result);
            printf("Result: %d\n", result);
        }
        
        acl::fiber::schedule_stop();
    };
    
    acl::fiber::schedule();
    return 0;
}
```

## 底层实现

`channel` 内部使用以下 C API 实现：

```cpp
ACL_CHANNEL *channel_create(int elemsize, int bufsize);
void channel_free(ACL_CHANNEL *c);
int channel_send(ACL_CHANNEL *c, void *v);
int channel_recv(ACL_CHANNEL *c, void *v);
```

## 性能特性

1. **缓冲区**：默认缓冲区大小为 100，可以减少协程切换
2. **零拷贝**：内部通过指针传递，避免数据拷贝
3. **高效调度**：使用协程调度器，无系统调用开销

## 注意事项

1. **类型要求**：
   - 传递的类型 `T` 必须支持拷贝构造
   - 对于大对象，建议传递指针或智能指针

2. **生命周期**：
   - channel 对象必须在所有使用它的协程之前创建
   - channel 对象必须在所有使用它的协程结束后销毁

3. **阻塞行为**：
   - `put()` 在缓冲区满时会阻塞
   - `pop()` 在通道为空时会阻塞
   - 确保有对应的接收者和发送者，避免死锁

4. **关闭通道**：
   - ACL Fiber 的 channel 不支持显式关闭
   - 可以使用特殊值作为结束信号

5. **线程安全**：
   - channel 内部实现是协程安全的
   - 可以在不同协程中使用同一个 channel

## 与其他通信方式对比

| 通信方式 | 适用场景 | 类型安全 | 缓冲 | 复杂度 |
|---------|---------|---------|------|--------|
| `channel` | 简单的点对点通信 | ✅ | ✅ | 低 |
| `fiber_tbox` | 指针传递，支持超时 | ✅ | ✅ | 中 |
| `fiber_tbox2` | 值传递，支持超时 | ✅ | ✅ | 中 |
| `fiber_sbox` | 基于信号量的消息队列 | ✅ | ✅ | 中 |
| `fiber_mutex + queue` | 自定义需求 | ❌ | 自定义 | 高 |

## 最佳实践

1. **选择合适的缓冲区大小**：
   - 默认 100 适合大多数场景
   - 生产消费速度不匹配时，需要调整缓冲区大小（需修改源码）

2. **避免死锁**：
   - 确保发送和接收数量匹配
   - 使用超时机制（可考虑使用 fiber_tbox）

3. **传递大对象**：
   - 使用指针或智能指针
   - 避免不必要的拷贝

4. **结束信号**：
   - 使用特殊值或标志位表示结束
   - 确保所有消费者都能收到结束信号

5. **错误处理**：
   - channel 操作不返回错误码
   - 使用 `fiber::self_killed()` 检查协程是否被杀死

## 相关文档

- [fiber_tbox](fiber_tbox.md) - 功能更丰富的消息队列
- [go_fiber](go_fiber.md) - 协程启动
- [wait_group](wait_group.md) - 协程同步
- [同步原语](synchronization.md) - 其他同步机制

