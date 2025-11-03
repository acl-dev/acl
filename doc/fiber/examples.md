# ACL Fiber 完整示例

本文档提供了 ACL Fiber 的完整可运行示例代码。

## 目录

1. [Hello World](#hello-world)
2. [协程间通信](#协程间通信)
3. [生产者消费者](#生产者消费者)
4. [并发下载器](#并发下载器)
5. [HTTP 客户端并发](#http-客户端并发)
6. [简单 HTTP 服务器](#简单-http-服务器)
7. [Echo 服务器](#echo-服务器)
8. [协程池应用](#协程池应用)
9. [定时任务调度](#定时任务调度)
10. [并行计算](#并行计算)

---

## Hello World

最简单的 ACL Fiber 程序。

```cpp
#include <stdio.h>
#include <fiber/libfiber.hpp>

int main() {
    // 启动一个协程
    go[] {
        printf("Hello from fiber %u\n", acl::fiber::self());
    };
    
    // 启动协程调度器
    acl::fiber::schedule();
    
    return 0;
}
```

**编译：**
```bash
g++ -std=c++11 -o hello hello.cpp -lacl_fiber -lacl_cpp -lacl -lpthread -ldl -lz
```

**运行：**
```bash
./hello
```

---

## 协程间通信

使用 channel 进行协程间通信。

```cpp
#include <stdio.h>
#include <fiber/libfiber.hpp>

void producer(acl::channel<int>& ch, int start, int count) {
    for (int i = 0; i < count; i++) {
        int value = start + i;
        ch.put(value);
        printf("Produced: %d\n", value);
        acl::fiber::delay(100);
    }
}

void consumer(acl::channel<int>& ch, int count) {
    for (int i = 0; i < count; i++) {
        int value;
        ch.pop(value);
        printf("Consumed: %d\n", value);
    }
    acl::fiber::schedule_stop();
}

int main() {
    acl::channel<int> ch;
    const int COUNT = 10;
    
    // 启动生产者
    go[&ch] { producer(ch, 0, COUNT); };
    
    // 启动消费者
    go[&ch] { consumer(ch, COUNT); };
    
    acl::fiber::schedule();
    
    return 0;
}
```

---

## 生产者消费者

多生产者多消费者模式，使用 fiber_tbox 和 wait_group。

```cpp
#include <stdio.h>
#include <stdlib.h>
#include <fiber/libfiber.hpp>

struct Task {
    int id;
    int producer_id;
    
    Task(int i, int p) : id(i), producer_id(p) {}
};

void producer(acl::fiber_tbox<Task>& tasks, acl::wait_group& wg, 
              int producer_id, int task_count) {
    for (int i = 0; i < task_count; i++) {
        Task* task = new Task(i, producer_id);
        tasks.push(task);
        printf("Producer %d: created task %d\n", producer_id, task->id);
        acl::fiber::delay(rand() % 100);
    }
    wg.done();
}

void consumer(acl::fiber_tbox<Task>& tasks, int consumer_id, 
              int total_tasks) {
    for (int i = 0; i < total_tasks; i++) {
        Task* task = tasks.pop();
        printf("Consumer %d: processing task %d from producer %d\n",
               consumer_id, task->id, task->producer_id);
        acl::fiber::delay(rand() % 50);
        delete task;
    }
}

int main() {
    const int NUM_PRODUCERS = 3;
    const int NUM_CONSUMERS = 2;
    const int TASKS_PER_PRODUCER = 5;
    const int TOTAL_TASKS = NUM_PRODUCERS * TASKS_PER_PRODUCER;
    
    acl::fiber_tbox<Task> tasks;
    acl::wait_group producer_wg;
    
    // 启动生产者
    producer_wg.add(NUM_PRODUCERS);
    for (int i = 0; i < NUM_PRODUCERS; i++) {
        go[&tasks, &producer_wg, i] {
            producer(tasks, producer_wg, i, TASKS_PER_PRODUCER);
        };
    }
    
    // 启动消费者
    for (int i = 0; i < NUM_CONSUMERS; i++) {
        go[&tasks, i] {
            consumer(tasks, i, TOTAL_TASKS / NUM_CONSUMERS);
        };
    }
    
    // 等待所有生产者完成
    go[&producer_wg] {
        producer_wg.wait();
        printf("All producers finished\n");
    };
    
    acl::fiber::schedule();
    
    return 0;
}
```

---

## 并发下载器

模拟并发下载多个文件。

```cpp
#include <stdio.h>
#include <string>
#include <vector>
#include <fiber/libfiber.hpp>

struct DownloadResult {
    std::string url;
    bool success;
    size_t size;
    int duration_ms;
};

void download_file(const std::string& url, acl::fiber_tbox<DownloadResult>& results) {
    printf("Downloading: %s\n", url.c_str());
    
    // 模拟下载（随机耗时）
    int duration = 500 + (rand() % 1500);
    acl::fiber::delay(duration);
    
    DownloadResult* result = new DownloadResult();
    result->url = url;
    result->success = true;
    result->size = 1024 * (rand() % 1000);
    result->duration_ms = duration;
    
    results.push(result);
}

int main() {
    std::vector<std::string> urls = {
        "http://example.com/file1.zip",
        "http://example.com/file2.tar.gz",
        "http://example.com/file3.pdf",
        "http://example.com/file4.mp4",
        "http://example.com/file5.iso",
        "http://example.com/file6.img",
        "http://example.com/file7.dat",
        "http://example.com/file8.bin"
    };
    
    acl::fiber_tbox<DownloadResult> results;
    acl::wait_group wg;
    
    // 启动下载协程
    wg.add(urls.size());
    for (const auto& url : urls) {
        go[url, &results, &wg] {
            download_file(url, results);
            wg.done();
        };
    }
    
    // 收集结果
    go[&wg, &results, &urls] {
        wg.wait();
        
        printf("\n=== Download Summary ===\n");
        size_t total_size = 0;
        int total_time = 0;
        
        for (size_t i = 0; i < urls.size(); i++) {
            DownloadResult* result = results.pop();
            printf("%s: %s, %zu KB, %d ms\n",
                   result->url.c_str(),
                   result->success ? "SUCCESS" : "FAILED",
                   result->size / 1024,
                   result->duration_ms);
            total_size += result->size;
            if (result->duration_ms > total_time) {
                total_time = result->duration_ms;
            }
            delete result;
        }
        
        printf("\nTotal: %zu KB in %d ms\n", total_size / 1024, total_time);
        acl::fiber::schedule_stop();
    };
    
    acl::fiber::schedule();
    
    return 0;
}
```

---

## HTTP 客户端并发

并发发送 HTTP 请求（模拟）。

```cpp
#include <stdio.h>
#include <string>
#include <vector>
#include <fiber/libfiber.hpp>

struct Request {
    std::string url;
    std::string method;
};

struct Response {
    int status_code;
    std::string body;
    int latency_ms;
};

Response http_request(const Request& req) {
    printf("Requesting: %s %s\n", req.method.c_str(), req.url.c_str());
    
    // 模拟网络延迟
    int latency = 100 + (rand() % 400);
    acl::fiber::delay(latency);
    
    Response resp;
    resp.status_code = 200;
    resp.body = "Response from " + req.url;
    resp.latency_ms = latency;
    
    return resp;
}

int main() {
    std::vector<Request> requests = {
        {"http://api.example.com/users", "GET"},
        {"http://api.example.com/posts", "GET"},
        {"http://api.example.com/comments", "GET"},
        {"http://api.example.com/photos", "GET"},
        {"http://api.example.com/albums", "GET"}
    };
    
    acl::fiber_tbox2<Response> responses;
    acl::wait_group wg;
    
    // 并发发送请求
    wg.add(requests.size());
    for (const auto& req : requests) {
        go[req, &responses, &wg] {
            Response resp = http_request(req);
            responses.push(resp);
            wg.done();
        };
    }
    
    // 处理响应
    go[&wg, &responses, &requests] {
        wg.wait();
        
        printf("\n=== Responses ===\n");
        int total_latency = 0;
        
        for (size_t i = 0; i < requests.size(); i++) {
            Response resp;
            if (responses.pop(resp)) {
                printf("Status: %d, Latency: %d ms\n",
                       resp.status_code, resp.latency_ms);
                if (resp.latency_ms > total_latency) {
                    total_latency = resp.latency_ms;
                }
            }
        }
        
        printf("\nMax latency: %d ms\n", total_latency);
        acl::fiber::schedule_stop();
    };
    
    acl::fiber::schedule();
    
    return 0;
}
```

---

## 简单 HTTP 服务器

基于 ACL Fiber 的 HTTP 服务器。

```cpp
#include <stdio.h>
#include <fiber/libfiber.hpp>

int main() {
    acl::http_server server("0.0.0.0:8080");
    
    // GET /
    server.Get("/", [](acl::HttpRequest&, acl::HttpResponse& res) {
        res.setStatus(200)
           .setContentType("text/html")
           .setBody("<h1>Welcome to ACL Fiber HTTP Server</h1>");
        return true;
    });
    
    // GET /hello
    server.Get("/hello", [](acl::HttpRequest& req, acl::HttpResponse& res) {
        const char* name = req.getParameter("name");
        std::string response = "Hello, ";
        response += name ? name : "World";
        response += "!";
        
        res.setStatus(200)
           .setContentType("text/plain")
           .setBody(response);
        return true;
    });
    
    // POST /echo
    server.Post("/echo", [](acl::HttpRequest& req, acl::HttpResponse& res) {
        std::string body;
        req.getBody(body);
        
        res.setStatus(200)
           .setContentType("text/plain")
           .setBody(body);
        return true;
    });
    
    // GET /json
    server.Get("/json", [](acl::HttpRequest&, acl::HttpResponse& res) {
        res.setStatus(200)
           .setContentType("application/json")
           .setBody("{\"message\":\"Hello JSON\",\"status\":\"ok\"}");
        return true;
    });
    
    printf("Server starting on http://0.0.0.0:8080\n");
    printf("Try:\n");
    printf("  curl http://localhost:8080/\n");
    printf("  curl http://localhost:8080/hello?name=Alice\n");
    printf("  curl -X POST -d 'test data' http://localhost:8080/echo\n");
    printf("  curl http://localhost:8080/json\n");
    
    server.run_alone("0.0.0.0:8080");
    
    return 0;
}
```

---

## Echo 服务器

基于 master_fiber 的 TCP Echo 服务器。

```cpp
#include <stdio.h>
#include <fiber/libfiber.hpp>

class EchoServer : public acl::master_fiber {
protected:
    void on_accept(acl::socket_stream& stream) override {
        printf("Client connected: %s\n", stream.get_peer());
        
        char buf[4096];
        while (true) {
            int n = stream.read(buf, sizeof(buf) - 1, false);
            if (n <= 0) {
                printf("Client disconnected: %s\n", stream.get_peer());
                break;
            }
            
            buf[n] = '\0';
            printf("Received from %s: %s", stream.get_peer(), buf);
            
            // Echo back
            if (stream.write(buf, n) != n) {
                printf("Write error\n");
                break;
            }
        }
    }
    
    void thread_on_init() override {
        printf("Thread %lu initialized\n", pthread_self());
    }
};

int main(int argc, char* argv[]) {
    EchoServer server;
    
    printf("Echo server starting on 0.0.0.0:8888\n");
    printf("Test with: telnet localhost 8888\n");
    
    server.run_alone("0.0.0.0:8888");
    
    return 0;
}
```

---

## 协程池应用

使用协程池处理大量任务。

```cpp
#include <stdio.h>
#include <vector>
#include <fiber/libfiber.hpp>

// 模拟CPU密集型任务
long long fibonacci(int n) {
    if (n <= 1) return n;
    return fibonacci(n - 1) + fibonacci(n - 2);
}

void compute_task(int task_id, int n, acl::wait_group& wg,
                  std::vector<long long>& results, acl::fiber_mutex& mutex) {
    printf("Task %d: computing fibonacci(%d)\n", task_id, n);
    
    long long result = fibonacci(n);
    
    mutex.lock();
    results[task_id] = result;
    mutex.unlock();
    
    printf("Task %d: result = %lld\n", task_id, result);
    wg.done();
}

int main() {
    const int NUM_TASKS = 20;
    
    // 创建协程池：最多5个并发
    acl::fiber_pool pool(1, 5);
    
    acl::wait_group wg;
    acl::fiber_mutex mutex;
    std::vector<long long> results(NUM_TASKS);
    
    wg.add(NUM_TASKS);
    
    // 提交任务
    for (int i = 0; i < NUM_TASKS; i++) {
        int n = 30 + (i % 10);  // fibonacci(30) ~ fibonacci(39)
        pool.exec(compute_task, i, n, std::ref(wg), 
                  std::ref(results), std::ref(mutex));
    }
    
    // 等待完成
    go[&wg, &pool, &results] {
        wg.wait();
        
        printf("\n=== Results ===\n");
        for (size_t i = 0; i < results.size(); i++) {
            printf("Task %zu: %lld\n", i, results[i]);
        }
        
        pool.stop();
    };
    
    acl::fiber::schedule();
    
    return 0;
}
```

---

## 定时任务调度

实现一个简单的定时任务调度器。

```cpp
#include <stdio.h>
#include <string>
#include <vector>
#include <fiber/libfiber.hpp>

struct ScheduledTask {
    std::string name;
    int interval_ms;
    int repeat_count;
    std::function<void()> func;
};

void scheduler(const std::vector<ScheduledTask>& tasks) {
    acl::wait_group wg;
    wg.add(tasks.size());
    
    for (const auto& task : tasks) {
        go[task, &wg] {
            for (int i = 0; i < task.repeat_count; i++) {
                acl::fiber::delay(task.interval_ms);
                printf("[%s] Execution #%d\n", task.name.c_str(), i + 1);
                task.func();
            }
            printf("[%s] Completed\n", task.name.c_str());
            wg.done();
        };
    }
    
    wg.wait();
    printf("All tasks completed\n");
    acl::fiber::schedule_stop();
}

int main() {
    std::vector<ScheduledTask> tasks = {
        {
            "Task1",
            1000,  // 每秒执行
            5,     // 执行5次
            []() { printf("  Task1 working...\n"); }
        },
        {
            "Task2",
            2000,  // 每2秒执行
            3,     // 执行3次
            []() { printf("  Task2 working...\n"); }
        },
        {
            "Task3",
            1500,  // 每1.5秒执行
            4,     // 执行4次
            []() { printf("  Task3 working...\n"); }
        }
    };
    
    go[tasks] { scheduler(tasks); };
    
    acl::fiber::schedule();
    
    return 0;
}
```

---

## 并行计算

并行计算示例：矩阵乘法。

```cpp
#include <stdio.h>
#include <vector>
#include <fiber/libfiber.hpp>

const int N = 1000;  // 矩阵大小

void compute_row(int row, const std::vector<std::vector<double>>& A,
                 const std::vector<std::vector<double>>& B,
                 std::vector<std::vector<double>>& C,
                 acl::wait_group& wg) {
    for (int j = 0; j < N; j++) {
        double sum = 0.0;
        for (int k = 0; k < N; k++) {
            sum += A[row][k] * B[k][j];
        }
        C[row][j] = sum;
    }
    wg.done();
}

int main() {
    printf("Initializing matrices...\n");
    
    // 初始化矩阵
    std::vector<std::vector<double>> A(N, std::vector<double>(N, 1.0));
    std::vector<std::vector<double>> B(N, std::vector<double>(N, 1.0));
    std::vector<std::vector<double>> C(N, std::vector<double>(N, 0.0));
    
    printf("Computing matrix multiplication (%dx%d)...\n", N, N);
    
    acl::wait_group wg;
    acl::fiber_pool pool(4, 8);  // 使用协程池
    
    wg.add(N);
    
    // 每行一个任务
    for (int i = 0; i < N; i++) {
        pool.exec(compute_row, i, std::ref(A), std::ref(B), 
                  std::ref(C), std::ref(wg));
    }
    
    // 等待完成
    go[&wg, &pool, &C] {
        wg.wait();
        
        // 验证结果（所有元素应该都是N）
        bool correct = true;
        for (int i = 0; i < N && correct; i++) {
            for (int j = 0; j < N && correct; j++) {
                if (C[i][j] != N) {
                    correct = false;
                    printf("Error at [%d][%d]: %.0f (expected %d)\n",
                           i, j, C[i][j], N);
                }
            }
        }
        
        if (correct) {
            printf("Matrix multiplication completed successfully!\n");
        }
        
        pool.stop();
    };
    
    acl::fiber::schedule();
    
    return 0;
}
```

---

## 编译所有示例

```bash
#!/bin/bash

# 设置编译参数
CXX=g++
CXXFLAGS="-std=c++11 -O2 -Wall"
INCLUDES="-I/usr/local/include"
LIBS="-L/usr/local/lib -lacl_fiber -lacl_cpp -lacl -lpthread -ldl -lz"

# 编译所有示例
echo "Compiling examples..."

$CXX $CXXFLAGS $INCLUDES -o hello_world 01_hello_world.cpp $LIBS
$CXX $CXXFLAGS $INCLUDES -o fiber_communication 02_fiber_communication.cpp $LIBS
$CXX $CXXFLAGS $INCLUDES -o producer_consumer 03_producer_consumer.cpp $LIBS
$CXX $CXXFLAGS $INCLUDES -o concurrent_downloader 04_concurrent_downloader.cpp $LIBS
$CXX $CXXFLAGS $INCLUDES -o http_client 05_http_client.cpp $LIBS
$CXX $CXXFLAGS $INCLUDES -o http_server 06_http_server.cpp $LIBS
$CXX $CXXFLAGS $INCLUDES -o echo_server 07_echo_server.cpp $LIBS
$CXX $CXXFLAGS $INCLUDES -o fiber_pool_app 08_fiber_pool_app.cpp $LIBS
$CXX $CXXFLAGS $INCLUDES -o task_scheduler 09_task_scheduler.cpp $LIBS
$CXX $CXXFLAGS $INCLUDES -o parallel_compute 10_parallel_compute.cpp $LIBS

echo "Done! Run examples with ./<example_name>"
```

## Makefile

```makefile
CXX = g++
CXXFLAGS = -std=c++11 -O2 -Wall
INCLUDES = -I/usr/local/include
LIBS = -L/usr/local/lib -lacl_fiber -lacl_cpp -lacl -lpthread -ldl -lz

EXAMPLES = hello_world fiber_communication producer_consumer \
           concurrent_downloader http_client http_server \
           echo_server fiber_pool_app task_scheduler parallel_compute

all: $(EXAMPLES)

hello_world: 01_hello_world.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $< $(LIBS)

fiber_communication: 02_fiber_communication.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $< $(LIBS)

producer_consumer: 03_producer_consumer.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $< $(LIBS)

concurrent_downloader: 04_concurrent_downloader.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $< $(LIBS)

http_client: 05_http_client.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $< $(LIBS)

http_server: 06_http_server.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $< $(LIBS)

echo_server: 07_echo_server.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $< $(LIBS)

fiber_pool_app: 08_fiber_pool_app.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $< $(LIBS)

task_scheduler: 09_task_scheduler.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $< $(LIBS)

parallel_compute: 10_parallel_compute.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $< $(LIBS)

clean:
	rm -f $(EXAMPLES)

.PHONY: all clean
```

---

## 相关文档

- [README](README.md) - 库概述和快速入门
- [快速参考](quick_reference.md) - API 快速查找
- [fiber 类](fiber_class.md) - 核心协程类
- [同步原语](synchronization.md) - 锁和同步
- [协程池](fiber_pool.md) - 协程池详解
- [通信机制](channel.md) - 协程通信

