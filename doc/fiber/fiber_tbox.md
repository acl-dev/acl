# 消息队列 (fiber_tbox/fiber_tbox2/fiber_sbox/fiber_sbox2)

## 概述

ACL Fiber 提供了多种消息队列实现，用于协程间、线程间以及协程与线程之间的消息通信。这些消息队列支持超时、缓冲等特性，比简单的 channel 功能更丰富。

## 消息队列类型

| 类型 | 传递方式 | 同步机制 | 特性 |
|------|---------|---------|------|
| `fiber_tbox<T*>` | 指针 | 条件变量+互斥锁 | 支持超时，可传 NULL |
| `fiber_tbox2<T>` | 值 | 条件变量+互斥锁 | 支持超时，值传递 |
| `fiber_sbox<T*>` | 指针 | 信号量 | 基于信号量，可传 NULL |
| `fiber_sbox2<T>` | 值 | 信号量 | 基于信号量，值传递 |

---

## fiber_tbox

### 概述

`fiber_tbox` 用于传递指针类型的消息，基于条件变量和互斥锁实现，支持超时等待。

### 类定义

```cpp
template<typename T>
class fiber_tbox : public box<T> {
public:
    explicit fiber_tbox(bool free_obj = true);
    ~fiber_tbox();
    
    bool push(T* t, bool notify_first = true);
    T* pop(int ms = -1, bool* found = NULL);
    size_t pop(std::vector<T*>& out, size_t max, int ms);
    
    void clear(bool free_obj = false);
    bool has_null() const;
    size_t size() const;
    
    void lock();
    void unlock();
};
```

### 构造函数

```cpp
explicit fiber_tbox(bool free_obj = true);
```

**参数：**
- `free_obj`：析构时是否自动删除未消费的消息对象

### 核心方法

#### push()

发送消息对象。

```cpp
bool push(T* t, bool notify_first = true);
```

**参数：**
- `t`：要发送的对象指针（可以为 NULL）
- `notify_first`：
  - `true` - 先通知后解锁（适合短生命周期 tbox）
  - `false` - 先解锁后通知（适合长生命周期 tbox，性能更好）

**返回值：**
- `true` - 发送成功

#### pop()

接收消息对象。

```cpp
T* pop(int ms = -1, bool* found = NULL);
```

**参数：**
- `ms`：超时时间（毫秒）
  - `-1` - 永久等待
  - `>= 0` - 等待指定毫秒数
- `found`：用于存储是否成功获取消息

**返回值：**
- 非 NULL - 获取到消息对象
- NULL - 超时或接收到 NULL 消息（通过 `found` 参数区分）

#### pop() 批量版本

批量接收消息。

```cpp
size_t pop(std::vector<T*>& out, size_t max, int ms);
```

**参数：**
- `out`：存储接收到的消息
- `max`：最多接收的消息数（0 表示不限制）
- `ms`：超时时间

**返回值：**
- 实际接收到的消息数量

### 使用示例

#### 基本用法

```cpp
#include <fiber/libfiber.hpp>

struct Message {
    int id;
    std::string content;
    
    Message(int i, const std::string& c) : id(i), content(c) {}
};

int main() {
    acl::fiber_tbox<Message> mbox;
    
    // 发送者
    go[&mbox] {
        for (int i = 0; i < 5; i++) {
            Message* msg = new Message(i, "Message " + std::to_string(i));
            mbox.push(msg);
            printf("Sent: %d\n", msg->id);
        }
    };
    
    // 接收者
    go[&mbox] {
        for (int i = 0; i < 5; i++) {
            Message* msg = mbox.pop();
            printf("Received: %d - %s\n", msg->id, msg->content.c_str());
            delete msg;
        }
        acl::fiber::schedule_stop();
    };
    
    acl::fiber::schedule();
    return 0;
}
```

#### 超时等待

```cpp
#include <fiber/libfiber.hpp>

struct Task {
    int id;
    std::string name;
};

int main() {
    acl::fiber_tbox<Task> tbox;
    
    // 接收者（带超时）
    go[&tbox] {
        while (true) {
            bool found;
            Task* task = tbox.pop(1000, &found);  // 1秒超时
            
            if (found) {
                if (task) {
                    printf("Got task: %d - %s\n", task->id, task->name.c_str());
                    delete task;
                } else {
                    printf("Got NULL (stop signal)\n");
                    break;
                }
            } else {
                printf("Timeout, no task available\n");
            }
        }
        acl::fiber::schedule_stop();
    };
    
    // 发送者
    go[&tbox] {
        for (int i = 0; i < 3; i++) {
            acl::fiber::delay(500);
            Task* task = new Task{i, "Task_" + std::to_string(i)};
            tbox.push(task);
        }
        
        // 发送结束信号
        acl::fiber::delay(1000);
        tbox.push(NULL);
    };
    
    acl::fiber::schedule();
    return 0;
}
```

#### 批量接收

```cpp
#include <fiber/libfiber.hpp>
#include <vector>

int main() {
    acl::fiber_tbox<int> tbox;
    
    // 发送者
    go[&tbox] {
        for (int i = 0; i < 20; i++) {
            tbox.push(new int(i));
            acl::fiber::delay(50);
        }
    };
    
    // 批量接收者
    go[&tbox] {
        while (true) {
            std::vector<int*> batch;
            size_t n = tbox.pop(batch, 5, 1000);  // 最多5个，超时1秒
            
            if (n == 0) {
                printf("No more data\n");
                break;
            }
            
            printf("Received %zu items: ", n);
            for (int* val : batch) {
                printf("%d ", *val);
                delete val;
            }
            printf("\n");
        }
        acl::fiber::schedule_stop();
    };
    
    acl::fiber::schedule();
    return 0;
}
```

#### 跨线程通信

```cpp
#include <fiber/libfiber.hpp>
#include <thread>

struct Data {
    int value;
    std::string info;
};

acl::fiber_tbox<Data> global_tbox;

void thread_producer() {
    for (int i = 0; i < 10; i++) {
        Data* data = new Data{i, "From thread"};
        global_tbox.push(data);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    global_tbox.push(NULL);  // 结束信号
}

void fiber_consumer() {
    while (true) {
        Data* data = global_tbox.pop();
        if (!data) break;
        
        printf("Fiber received: %d - %s\n", data->value, data->info.c_str());
        delete data;
    }
    acl::fiber::schedule_stop();
}

int main() {
    std::thread t(thread_producer);
    go fiber_consumer;
    
    acl::fiber::schedule();
    t.join();
    
    return 0;
}
```

---

## fiber_tbox2

### 概述

`fiber_tbox2` 用于传递值类型的消息，内部会进行对象拷贝或移动，适合传递由 `std::shared_ptr` 管理的对象。

### 类定义

```cpp
template<typename T>
class fiber_tbox2 : public box2<T> {
public:
    fiber_tbox2();
    ~fiber_tbox2();
    
    bool push(T t, bool notify_first = true);
    bool pop(T& t, int ms = -1);
    size_t pop(std::vector<T>& out, size_t max, int ms);
    
    void clear();
    size_t size() const;
    bool has_null() const;
    
    void lock();
    void unlock();
};
```

### 使用示例

#### 传递值类型

```cpp
#include <fiber/libfiber.hpp>

struct Point {
    int x, y;
    
    Point() : x(0), y(0) {}
    Point(int x_, int y_) : x(x_), y(y_) {}
};

int main() {
    acl::fiber_tbox2<Point> tbox;
    
    // 发送者
    go[&tbox] {
        for (int i = 0; i < 5; i++) {
            Point p(i, i * 2);
            tbox.push(p);
            printf("Sent: (%d, %d)\n", p.x, p.y);
        }
    };
    
    // 接收者
    go[&tbox] {
        for (int i = 0; i < 5; i++) {
            Point p;
            if (tbox.pop(p)) {
                printf("Received: (%d, %d)\n", p.x, p.y);
            }
        }
        acl::fiber::schedule_stop();
    };
    
    acl::fiber::schedule();
    return 0;
}
```

#### 使用 shared_ptr

```cpp
#include <fiber/libfiber.hpp>
#include <memory>

struct Data {
    int id;
    std::vector<int> values;
    
    Data(int i) : id(i) {
        for (int j = 0; j < 10; j++) {
            values.push_back(j);
        }
    }
    
    ~Data() {
        printf("Data %d destroyed\n", id);
    }
};

int main() {
    acl::fiber_tbox2<std::shared_ptr<Data>> tbox;
    
    // 发送者
    go[&tbox] {
        for (int i = 0; i < 5; i++) {
            auto data = std::make_shared<Data>(i);
            tbox.push(data);
            printf("Sent: %d\n", data->id);
        }
    };
    
    // 接收者
    go[&tbox] {
        for (int i = 0; i < 5; i++) {
            std::shared_ptr<Data> data;
            if (tbox.pop(data)) {
                printf("Received: %d, size=%zu\n", data->id, data->values.size());
            }
        }
        acl::fiber::schedule_stop();
    };
    
    acl::fiber::schedule();
    return 0;
}
```

---

## fiber_sbox

### 概述

`fiber_sbox` 基于信号量实现，用于传递指针类型的消息。

### 类定义

```cpp
template<typename T>
class fiber_sbox : public box<T> {
public:
    explicit fiber_sbox(bool free_obj = true, bool async = true);
    explicit fiber_sbox(size_t buf, bool free_obj = true);
    ~fiber_sbox();
    
    bool push(T* t, bool dummy = false);
    T* pop(int ms, bool* found = NULL);
    size_t pop(std::vector<T*>& out, size_t max, int ms);
    
    bool has_null() const;
    size_t size() const;
    void clear(bool free_obj = false);
};
```

### 构造函数

```cpp
explicit fiber_sbox(bool free_obj = true, bool async = true);
explicit fiber_sbox(size_t buf, bool free_obj = true);
```

**参数：**
- `free_obj`：析构时是否自动删除未消费的消息
- `async`：是否使用异步模式
- `buf`：信号量缓冲区大小

### 使用示例

```cpp
#include <fiber/libfiber.hpp>

struct Job {
    int id;
    std::string description;
    
    Job(int i, const std::string& desc) : id(i), description(desc) {}
};

int main() {
    acl::fiber_sbox<Job> sbox;
    
    // 生产者
    go[&sbox] {
        for (int i = 0; i < 10; i++) {
            Job* job = new Job(i, "Job_" + std::to_string(i));
            sbox.push(job);
            printf("Produced: %d\n", job->id);
        }
        sbox.push(NULL);  // 结束信号
    };
    
    // 消费者
    go[&sbox] {
        while (true) {
            Job* job = sbox.pop(-1);
            if (!job) break;
            
            printf("Processing: %d - %s\n", job->id, job->description.c_str());
            acl::fiber::delay(100);
            delete job;
        }
        acl::fiber::schedule_stop();
    };
    
    acl::fiber::schedule();
    return 0;
}
```

---

## fiber_sbox2

### 概述

`fiber_sbox2` 基于信号量实现，用于传递值类型的消息。

### 类定义

```cpp
template<typename T>
class fiber_sbox2 : public box2<T> {
public:
    explicit fiber_sbox2(bool async = true);
    explicit fiber_sbox2(size_t buf);
    ~fiber_sbox2();
    
    bool push(T t, bool dummy = false);
    bool pop(T& t, int ms = -1);
    size_t pop(std::vector<T>& out, size_t max, int ms);
    
    size_t size() const;
    bool has_null() const;
};
```

### 使用示例

```cpp
#include <fiber/libfiber.hpp>

int main() {
    acl::fiber_sbox2<int> sbox;
    
    // 生产者
    go[&sbox] {
        for (int i = 0; i < 20; i++) {
            sbox.push(i);
        }
    };
    
    // 消费者
    go[&sbox] {
        for (int i = 0; i < 20; i++) {
            int value;
            if (sbox.pop(value)) {
                printf("Got: %d\n", value);
            }
        }
        acl::fiber::schedule_stop();
    };
    
    acl::fiber::schedule();
    return 0;
}
```

---

## 消息队列对比

| 特性 | fiber_tbox | fiber_tbox2 | fiber_sbox | fiber_sbox2 |
|------|-----------|-------------|-----------|-------------|
| 传递类型 | 指针 | 值 | 指针 | 值 |
| 同步机制 | 条件变量 | 条件变量 | 信号量 | 信号量 |
| 超时支持 | ✅ | ✅ | ✅ | ✅ |
| 批量接收 | ✅ | ✅ | ✅ | ✅ |
| 跨线程 | ✅ | ✅ | ✅ | ✅ |
| 传 NULL | ✅ | ❌ | ✅ | ❌ |
| 拷贝开销 | 小 | 中 | 小 | 中 |

## 选择建议

1. **传递指针时**：
   - 优先使用 `fiber_tbox<T*>`（功能最全）
   - 需要信号量特性时使用 `fiber_sbox<T*>`

2. **传递值时**：
   - 优先使用 `fiber_tbox2<T>`
   - 配合 `std::shared_ptr` 使用效果最佳

3. **性能要求高时**：
   - 使用 sbox 系列（基于信号量，性能稍好）

4. **需要 NULL 信号时**：
   - 使用指针版本（tbox/sbox）

## 注意事项

1. **生命周期管理**：
   - tbox/sbox 指针版本：需手动管理内存
   - 使用 `shared_ptr` 可自动管理

2. **超时处理**：
   - 使用 `found` 参数区分超时和接收到 NULL

3. **线程安全**：
   - 所有消息队列都支持跨线程使用
   - 内部有锁保护

4. **notify_first 参数**：
   - 长生命周期 tbox 使用 `false`
   - 短生命周期 tbox 使用 `true`

5. **批量操作**：
   - 批量接收可以提高吞吐量
   - 适合高并发场景

## 最佳实践

1. **使用智能指针**：
   ```cpp
   acl::fiber_tbox2<std::shared_ptr<Data>> tbox;
   ```

2. **设置合理超时**：
   ```cpp
   Data* data = tbox.pop(1000, &found);
   if (!found) {
       // 超时处理
   }
   ```

3. **使用 NULL 作为结束信号**：
   ```cpp
   // 生产者
   tbox.push(NULL);
   
   // 消费者
   while (true) {
       Data* data = tbox.pop();
       if (!data) break;
       // 处理数据
   }
   ```

4. **批量处理提高性能**：
   ```cpp
   std::vector<T*> batch;
   size_t n = tbox.pop(batch, 100, 0);
   for (auto* item : batch) {
       process(item);
   }
   ```

## 相关文档

- [channel](channel.md) - 简单的通道通信
- [同步原语](synchronization.md) - 互斥锁、条件变量、信号量
- [go_fiber](go_fiber.md) - 协程启动
- [wait_group](wait_group.md) - 协程同步

