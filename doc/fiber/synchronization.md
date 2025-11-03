# 协程同步原语

## 概述

ACL Fiber 提供了完整的同步原语，用于协程间、线程间以及协程与线程之间的同步。这些同步原语包括互斥锁、读写锁、条件变量、信号量和事件锁。

## 同步原语分类

### 仅限同线程协程间使用
- `fiber_lock` - 轻量级互斥锁
- `fiber_rwlock` - 读写锁
- `fiber_sem` - 信号量

### 支持跨线程使用
- `fiber_mutex` - 互斥锁（支持协程间、线程间、协程与线程之间）
- `fiber_cond` - 条件变量
- `fiber_event` - 事件锁

---

## fiber_mutex

### 概述

`fiber_mutex` 是功能最强大的互斥锁，可用于：
- 同一线程内的协程间互斥
- 不同线程间的协程互斥
- 协程与普通线程之间的互斥
- 普通线程之间的互斥

### 类定义

```cpp
class FIBER_CPP_API fiber_mutex {
public:
    explicit fiber_mutex(ACL_FIBER_MUTEX* mutex = NULL);
    ~fiber_mutex();
    
    bool lock();
    bool trylock();
    bool unlock();
    
    ACL_FIBER_MUTEX* get_mutex() const;
    
    // 死锁检测
    static bool deadlock(fiber_mutex_stats& out);
    static void deadlock_show();
};
```

### 构造函数

```cpp
explicit fiber_mutex(ACL_FIBER_MUTEX* mutex = NULL);
```

**参数：**
- `mutex`：
  - `NULL` - 内部自动创建 C 锁对象
  - 非空 - 使用提供的 C 锁对象创建 C++ 锁对象

### 核心方法

#### lock()

锁定互斥锁。

```cpp
bool lock();
```

**返回值：**
- `true` - 锁定成功
- `false` - 锁定失败

#### trylock()

尝试锁定互斥锁（非阻塞）。

```cpp
bool trylock();
```

**返回值：**
- `true` - 锁定成功
- `false` - 锁已被其他协程或线程持有

#### unlock()

解锁并唤醒等待者。

```cpp
bool unlock();
```

**返回值：**
- `true` - 解锁成功
- `false` - 解锁失败

### 死锁检测

#### deadlock()

检测死锁状态。

```cpp
static bool deadlock(fiber_mutex_stats& out);
```

**参数：**
- `out`：保存检测结果

**返回值：**
- `true` - 存在死锁
- `false` - 不存在死锁

#### deadlock_show()

检测死锁并将所有进入死锁状态的协程栈打印到标准输出。

```cpp
static void deadlock_show();
```

### fiber_mutex_guard

RAII 风格的互斥锁守卫，自动管理锁的生命周期。

```cpp
class FIBER_CPP_API fiber_mutex_guard {
public:
    explicit fiber_mutex_guard(fiber_mutex& mutex);
    ~fiber_mutex_guard();
};
```

### 使用示例

#### 基本使用

```cpp
#include <fiber/libfiber.hpp>

acl::fiber_mutex mutex;
int shared_counter = 0;

void worker(int id) {
    for (int i = 0; i < 100; i++) {
        mutex.lock();
        shared_counter++;
        printf("Worker %d: counter = %d\n", id, shared_counter);
        mutex.unlock();
        
        acl::fiber::delay(10);
    }
}

int main() {
    for (int i = 0; i < 5; i++) {
        go[i] { worker(i); };
    }
    
    acl::fiber::schedule();
    printf("Final counter: %d\n", shared_counter);
    return 0;
}
```

#### 使用 RAII 守卫

```cpp
#include <fiber/libfiber.hpp>

acl::fiber_mutex mutex;
std::vector<int> shared_data;

void add_data(int value) {
    acl::fiber_mutex_guard guard(mutex);  // 自动加锁
    
    shared_data.push_back(value);
    
    // 函数返回时自动解锁
}

int main() {
    for (int i = 0; i < 10; i++) {
        go[i] { add_data(i); };
    }
    
    acl::fiber::schedule();
    
    printf("Data size: %zu\n", shared_data.size());
    return 0;
}
```

#### trylock 示例

```cpp
#include <fiber/libfiber.hpp>

acl::fiber_mutex mutex;

go[] {
    if (mutex.trylock()) {
        printf("Fiber 1: Got lock\n");
        acl::fiber::delay(1000);
        mutex.unlock();
    } else {
        printf("Fiber 1: Failed to get lock\n");
    }
};

go[] {
    acl::fiber::delay(100);
    if (mutex.trylock()) {
        printf("Fiber 2: Got lock\n");
        mutex.unlock();
    } else {
        printf("Fiber 2: Failed to get lock\n");
    }
};

acl::fiber::schedule();
```

#### 跨线程使用

```cpp
#include <fiber/libfiber.hpp>
#include <thread>

acl::fiber_mutex mutex;
int counter = 0;

void fiber_worker() {
    for (int i = 0; i < 1000; i++) {
        mutex.lock();
        counter++;
        mutex.unlock();
    }
}

void thread_worker() {
    for (int i = 0; i < 1000; i++) {
        mutex.lock();
        counter++;
        mutex.unlock();
    }
}

int main() {
    // 启动协程
    go fiber_worker;
    
    // 启动线程
    std::thread t(thread_worker);
    
    acl::fiber::schedule();
    t.join();
    
    printf("Counter: %d\n", counter);  // 应该是 2000
    return 0;
}
```

#### 死锁检测

```cpp
#include <fiber/libfiber.hpp>

acl::fiber_mutex mutex1, mutex2;

go[] {
    mutex1.lock();
    acl::fiber::delay(100);
    mutex2.lock();  // 可能死锁
    
    mutex2.unlock();
    mutex1.unlock();
};

go[] {
    mutex2.lock();
    acl::fiber::delay(100);
    mutex1.lock();  // 可能死锁
    
    mutex1.unlock();
    mutex2.unlock();
};

go[] {
    acl::fiber::delay(200);
    
    // 检测并显示死锁
    acl::fiber_mutex::deadlock_show();
    
    acl::fiber::schedule_stop();
};

acl::fiber::schedule();
```

---

## fiber_lock

### 概述

`fiber_lock` 是轻量级互斥锁，只能用于同一线程内协程间的互斥。由于不需要考虑跨线程场景，性能比 `fiber_mutex` 更好。

### 类定义

```cpp
class FIBER_CPP_API fiber_lock {
public:
    fiber_lock();
    ~fiber_lock();
    
    bool lock();
    bool trylock();
    bool unlock();
};
```

### 使用示例

```cpp
#include <fiber/libfiber.hpp>

acl::fiber_lock lock;
int counter = 0;

void worker() {
    for (int i = 0; i < 100; i++) {
        lock.lock();
        counter++;
        lock.unlock();
    }
}

int main() {
    for (int i = 0; i < 5; i++) {
        go worker;
    }
    
    acl::fiber::schedule();
    printf("Counter: %d\n", counter);
    return 0;
}
```

---

## fiber_rwlock

### 概述

`fiber_rwlock` 是读写锁，允许多个读者同时持有锁，但写者独占锁。仅能用于同一线程内协程间的同步。

### 类定义

```cpp
class FIBER_CPP_API fiber_rwlock {
public:
    fiber_rwlock();
    ~fiber_rwlock();
    
    // 读锁
    void rlock();
    bool tryrlock();
    void runlock();
    
    // 写锁
    void wlock();
    bool trywlock();
    void wunlock();
};
```

### 方法说明

#### 读锁操作

```cpp
void rlock();          // 加读锁
bool tryrlock();       // 尝试加读锁
void runlock();        // 释放读锁
```

#### 写锁操作

```cpp
void wlock();          // 加写锁
bool trywlock();       // 尝试加写锁
void wunlock();        // 释放写锁
```

### 使用示例

```cpp
#include <fiber/libfiber.hpp>
#include <map>

acl::fiber_rwlock rwlock;
std::map<int, std::string> shared_map;

// 读者
void reader(int id) {
    for (int i = 0; i < 10; i++) {
        rwlock.rlock();
        
        printf("Reader %d: map size = %zu\n", id, shared_map.size());
        for (const auto& pair : shared_map) {
            printf("  %d -> %s\n", pair.first, pair.second.c_str());
        }
        
        rwlock.runlock();
        acl::fiber::delay(100);
    }
}

// 写者
void writer(int id) {
    for (int i = 0; i < 5; i++) {
        rwlock.wlock();
        
        shared_map[id * 100 + i] = "value_" + std::to_string(id);
        printf("Writer %d: added key %d\n", id, id * 100 + i);
        
        rwlock.wunlock();
        acl::fiber::delay(200);
    }
}

int main() {
    // 启动多个读者
    for (int i = 0; i < 3; i++) {
        go[i] { reader(i); };
    }
    
    // 启动多个写者
    for (int i = 0; i < 2; i++) {
        go[i] { writer(i); };
    }
    
    acl::fiber::schedule();
    return 0;
}
```

---

## fiber_cond

### 概述

`fiber_cond` 是条件变量，用于协程间、线程间以及协程与线程之间的条件同步。必须与 `fiber_mutex` 配合使用。

### 类定义

```cpp
class FIBER_CPP_API fiber_cond {
public:
    fiber_cond();
    ~fiber_cond();
    
    bool wait(fiber_mutex& mutex, int timeout = -1);
    bool notify();
    
    ACL_FIBER_COND* get_cond() const;
};
```

### 方法说明

#### wait()

等待条件变量可用。

```cpp
bool wait(fiber_mutex& mutex, int timeout = -1);
```

**参数：**
- `mutex`：关联的互斥锁
- `timeout`：等待超时时间（毫秒），-1 表示永久等待

**返回值：**
- `true` - 被通知唤醒
- `false` - 超时

#### notify()

唤醒等待在条件变量上的协程或线程。

```cpp
bool notify();
```

**返回值：**
- `true` - 通知成功
- `false` - 通知失败

### 使用示例

#### 生产者-消费者模式

```cpp
#include <fiber/libfiber.hpp>
#include <queue>

acl::fiber_mutex mutex;
acl::fiber_cond cond;
std::queue<int> queue;
const int MAX_QUEUE_SIZE = 10;

// 生产者
void producer(int id) {
    for (int i = 0; i < 20; i++) {
        mutex.lock();
        
        while (queue.size() >= MAX_QUEUE_SIZE) {
            printf("Producer %d: queue full, waiting...\n", id);
            cond.wait(mutex);
        }
        
        int item = id * 100 + i;
        queue.push(item);
        printf("Producer %d: produced %d, queue size = %zu\n", 
               id, item, queue.size());
        
        cond.notify();  // 通知消费者
        mutex.unlock();
        
        acl::fiber::delay(50);
    }
}

// 消费者
void consumer(int id) {
    for (int i = 0; i < 20; i++) {
        mutex.lock();
        
        while (queue.empty()) {
            printf("Consumer %d: queue empty, waiting...\n", id);
            cond.wait(mutex);
        }
        
        int item = queue.front();
        queue.pop();
        printf("Consumer %d: consumed %d, queue size = %zu\n",
               id, item, queue.size());
        
        cond.notify();  // 通知生产者
        mutex.unlock();
        
        acl::fiber::delay(100);
    }
}

int main() {
    go[] { producer(1); };
    go[] { producer(2); };
    go[] { consumer(1); };
    go[] { consumer(2); };
    
    acl::fiber::schedule();
    return 0;
}
```

#### 带超时的等待

```cpp
#include <fiber/libfiber.hpp>

acl::fiber_mutex mutex;
acl::fiber_cond cond;
bool ready = false;

go[] {
    mutex.lock();
    
    // 等待 2 秒
    if (cond.wait(mutex, 2000)) {
        printf("Fiber 1: Condition signaled\n");
    } else {
        printf("Fiber 1: Timeout\n");
    }
    
    mutex.unlock();
};

go[] {
    acl::fiber::delay(1000);
    
    mutex.lock();
    ready = true;
    cond.notify();
    mutex.unlock();
    
    printf("Fiber 2: Notified\n");
};

acl::fiber::schedule();
```

---

## fiber_sem

### 概述

`fiber_sem` 是信号量，用于控制对共享资源的访问数量。**仅能用于同一线程内的协程间同步**，不支持跨线程使用。支持同步和异步两种模式。

### 类定义

```cpp
enum fiber_sem_attr_t {
    fiber_sem_t_sync  = 0,        // 同步模式
    fiber_sem_t_async = (1 << 0)  // 异步模式
};

class FIBER_CPP_API fiber_sem {
public:
    explicit fiber_sem(size_t max, fiber_sem_attr_t attr = fiber_sem_t_async);
    explicit fiber_sem(size_t max, size_t buf);
    ~fiber_sem();
    
    int wait(int ms = -1);
    int trywait();
    int post();
    
    size_t num() const;
};
```

### 构造函数

```cpp
explicit fiber_sem(size_t max, fiber_sem_attr_t attr = fiber_sem_t_async);
explicit fiber_sem(size_t max, size_t buf);
```

**参数：**
- `max`：信号量的最大值
- `attr`：信号量属性（同步/异步）
- `buf`：缓冲区大小

### 方法说明

#### wait()

等待信号量。

```cpp
int wait(int ms = -1);
```

**参数：**
- `ms`：超时时间（毫秒），-1 表示永久等待

**返回值：**
- 成功时返回非负值
- 失败时返回负值

#### trywait()

尝试等待信号量（非阻塞）。

```cpp
int trywait();
```

#### post()

释放信号量。

```cpp
int post();
```

#### num()

获取当前信号量的值。

```cpp
size_t num() const;
```

### fiber_sem_guard

RAII 风格的信号量守卫。

```cpp
class FIBER_CPP_API fiber_sem_guard {
public:
    explicit fiber_sem_guard(fiber_sem& sem);
    ~fiber_sem_guard();
};
```

### 使用示例

#### 限制并发数

```cpp
#include <fiber/libfiber.hpp>

// 最多允许 3 个协程同时访问资源
acl::fiber_sem sem(3);

void worker(int id) {
    printf("Worker %d: waiting for semaphore...\n", id);
    
    sem.wait();
    
    printf("Worker %d: got semaphore, working...\n", id);
    acl::fiber::delay(1000);  // 模拟工作
    printf("Worker %d: done\n", id);
    
    sem.post();
}

int main() {
    // 启动 10 个协程，但最多 3 个同时运行
    for (int i = 0; i < 10; i++) {
        go[i] { worker(i); };
    }
    
    acl::fiber::schedule();
    return 0;
}
```

#### 使用 RAII 守卫

```cpp
#include <fiber/libfiber.hpp>

acl::fiber_sem sem(5);

void task(int id) {
    acl::fiber_sem_guard guard(sem);  // 自动 wait
    
    printf("Task %d: executing...\n", id);
    acl::fiber::delay(500);
    
    // 函数返回时自动 post
}

int main() {
    for (int i = 0; i < 20; i++) {
        go[i] { task(i); };
    }
    
    acl::fiber::schedule();
    return 0;
}
```

---

## fiber_event

### 概述

`fiber_event` 是事件锁，可用于协程间、线程间以及协程与线程之间通过事件等待/通知进行同步。

### 类定义

```cpp
class FIBER_CPP_API fiber_event {
public:
    fiber_event(bool use_mutex = true, bool fatal_on_error = true);
    ~fiber_event();
    
    bool wait();
    bool trywait();
    bool notify();
    
    ACL_FIBER_EVENT* get_event() const;
};
```

### 构造函数

```cpp
fiber_event(bool use_mutex = true, bool fatal_on_error = true);
```

**参数：**
- `use_mutex`：
  - `true` - 使用内部互斥锁保护（适合大量线程场景）
  - `false` - 使用原子数保护（适合少量线程场景）
- `fatal_on_error`：内部错误时是否直接崩溃以便调试

### 方法说明

#### wait()

等待事件锁。

```cpp
bool wait();
```

**返回值：**
- `true` - 获取锁成功
- `false` - 内部错误

#### trywait()

尝试等待事件锁（非阻塞）。

```cpp
bool trywait();
```

**返回值：**
- `true` - 获取锁成功
- `false` - 锁当前正在被使用

#### notify()

释放事件锁并通知等待者。

```cpp
bool notify();
```

**返回值：**
- `true` - 通知成功
- `false` - 内部错误

### 使用示例

#### 事件通知

```cpp
#include <fiber/libfiber.hpp>

acl::fiber_event event;
bool data_ready = false;

// 等待者
go[] {
    printf("Waiter: waiting for event...\n");
    event.wait();
    printf("Waiter: event received, data_ready = %d\n", data_ready);
};

// 通知者
go[] {
    acl::fiber::delay(1000);
    
    data_ready = true;
    printf("Notifier: sending event\n");
    event.notify();
};

acl::fiber::schedule();
```

#### 跨线程事件同步

```cpp
#include <fiber/libfiber.hpp>
#include <thread>

acl::fiber_event event;

void fiber_worker() {
    printf("Fiber: waiting for event...\n");
    event.wait();
    printf("Fiber: event received\n");
}

void thread_worker() {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    printf("Thread: sending event\n");
    event.notify();
}

int main() {
    go fiber_worker;
    
    std::thread t(thread_worker);
    
    acl::fiber::schedule();
    t.join();
    
    return 0;
}
```

---

## 性能对比

| 同步原语 | 适用场景 | 性能 | 跨线程 |
|---------|---------|------|--------|
| `fiber_lock` | 同线程协程间 | 最快 | ❌ |
| `fiber_rwlock` | 同线程协程间读多写少 | 快 | ❌ |
| `fiber_sem` | 同线程协程间资源计数 | 快 | ❌ |
| `fiber_mutex` | 协程间/线程间/混合 | 中等 | ✅ |
| `fiber_cond` | 条件等待 | 中等 | ✅ |
| `fiber_event` | 事件通知 | 中等 | ✅ |

## 最佳实践

1. **选择合适的同步原语**：
   - 仅同线程协程使用 `fiber_lock` 性能最好
   - 需要跨线程使用 `fiber_mutex`
   - 读多写少使用 `fiber_rwlock`

2. **使用 RAII 守卫**：
   - 使用 `fiber_mutex_guard` 和 `fiber_sem_guard` 自动管理资源
   - 避免忘记解锁导致的死锁

3. **避免死锁**：
   - 始终按相同顺序获取多个锁
   - 使用 `trylock()` 或设置超时
   - 使用 `fiber_mutex::deadlock_show()` 检测死锁

4. **条件变量使用**：
   - 总是在循环中使用 `cond.wait()`
   - 在改变条件后立即通知

5. **信号量使用**：
   - 用于控制资源访问数量
   - 适合生产者-消费者模式

## 相关文档

- [fiber 类](fiber_class.md)
- [Channel 通信](channel.md)
- [fiber_tbox](fiber_tbox.md)
- [wait_group](wait_group.md)

