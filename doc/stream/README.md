# ACL Stream 库文档

## 概述

ACL Stream 是 ACL C++ 库中的流处理模块，提供了统一的、面向对象的 I/O 接口，支持文件流、网络流、异步流等多种流类型。该模块位于 `lib_acl_cpp/include/acl_cpp/stream` 目录下。

## 主要特性

- **统一的流接口**: 所有流类型继承自公共基类，提供一致的 API
- **同步和异步支持**: 同时支持阻塞式同步 I/O 和非阻塞式异步 I/O
- **多种流类型**: 文件流、网络流、标准输入输出流等
- **SSL/TLS 支持**: 内置对 OpenSSL、mbed TLS 和 PolarSSL 的支持
- **Hook 机制**: 允许用户自定义 I/O 行为
- **缓冲区管理**: 内置高效的缓冲区管理机制
- **跨平台**: 支持 Windows、Linux、Unix 等多平台

## 文档结构

- [架构设计](./architecture.md) - Stream 模块的整体架构设计
- [使用指南](./usage_guide.md) - 各种流的使用方法和最佳实践
- [示例代码](./examples.md) - 常见场景的示例代码
- [API 参考](./api_reference.md) - 详细的 API 文档

## 快速开始

### 文件读写示例

```cpp
#include "acl_cpp/lib_acl.hpp"

// 读取文件
acl::fstream file;
if (file.open_read("test.txt")) {
    acl::string line;
    while (file.gets(line)) {
        std::cout << line.c_str() << std::endl;
    }
    file.close();
}

// 写入文件
acl::fstream file;
if (file.create("output.txt")) {
    file.write("Hello, ACL!\n", 12);
    file.close();
}
```

### 网络通信示例

```cpp
#include "acl_cpp/lib_acl.hpp"

// 客户端连接
acl::socket_stream conn;
if (conn.open("www.example.com:80", 10, 10)) {
    conn.write("GET / HTTP/1.0\r\n\r\n", 18);
    
    acl::string buf;
    while (conn.gets(buf)) {
        std::cout << buf.c_str();
    }
    conn.close();
}

// 服务端监听
acl::server_socket server;
if (server.open("0.0.0.0:8080")) {
    while (true) {
        acl::socket_stream* client = server.accept();
        if (client) {
            // 处理客户端请求
            client->write("Hello, Client!\n", 15);
            delete client;
        }
    }
}
```

## 类层次结构

```
stream (基类)
├── istream (输入流)
│   ├── fstream (文件流)
│   ├── socket_stream (网络流)
│   ├── stdin_stream (标准输入)
│   └── ifstream (只读文件流)
│
├── ostream (输出流)
│   ├── fstream (文件流)
│   ├── socket_stream (网络流)
│   ├── stdout_stream (标准输出)
│   └── ofstream (只写文件流)
│
└── aio_stream (异步流基类)
    ├── aio_istream (异步输入流)
    ├── aio_ostream (异步输出流)
    ├── aio_fstream (异步文件流)
    └── aio_socket_stream (异步网络流)
```

## 版本要求

- C++11 或更高版本（部分特性需要 C++11 支持）
- ACL 基础库

## 相关资源

- [ACL 项目主页](https://github.com/acl-dev/acl)
- [ACL 文档](https://github.com/acl-dev/acl/tree/master/doc)

## 许可证

本项目采用 Apache 2.0 许可证

