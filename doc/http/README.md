# ACL HTTP库设计与使用文档

## 概述

ACL HTTP库是一个功能完善、高性能的C++ HTTP通信库，提供了完整的HTTP客户端和服务端实现，同时支持WebSocket协议、SSL/TLS加密、HTTP压缩等特性。

## 主要特性

### 1. HTTP客户端
- 同步HTTP客户端（http_request, http_client）
- 异步HTTP客户端（http_aclient）
- 支持HTTP/1.0和HTTP/1.1协议
- 支持连接池（http_request_pool）
- 自动重定向
- 断点续传（Range请求）
- GZIP压缩/解压
- Cookie管理
- SSL/TLS支持

### 2. HTTP服务端
- Servlet风格的HTTP服务器框架
- 支持GET、POST、PUT、DELETE等HTTP方法
- 自动解析请求参数
- Cookie和Session管理
- MIME文件上传
- Chunked传输编码
- 字符集自动转换

### 3. WebSocket
- 完整的WebSocket协议实现（RFC 6455）
- 支持TEXT和BINARY帧
- PING/PONG心跳机制
- 同步和异步模式

### 4. 其他特性
- 灵活的HTTP头部管理
- 完善的错误处理
- 内存池优化
- 流式数据处理

## 架构设计

### 核心类层次结构

```
HTTP客户端体系:
├── http_client         # 底层HTTP协议处理
├── http_request        # HTTP请求封装（同步）
├── http_aclient        # HTTP请求封装（异步）
├── http_request_pool   # 连接池管理
└── http_download       # 文件下载

HTTP服务端体系:
├── HttpServlet         # Servlet基类
├── HttpServletRequest  # 请求封装
├── HttpServletResponse # 响应封装
├── HttpSession         # Session管理
└── WebSocketServlet    # WebSocket Servlet

公共组件:
├── http_header         # HTTP头部
├── http_mime           # MIME处理
├── HttpCookie          # Cookie管理
├── websocket           # WebSocket协议
└── http_utils          # 工具类
```

## 快速开始

### HTTP客户端示例

```cpp
#include "acl_cpp/lib_acl.hpp"

int main() {
    acl::http_request req("www.example.com");
    
    // 设置请求URL
    req.request_header().set_url("/api/test");
    req.request_header().set_host("www.example.com");
    
    // 发送GET请求
    if (!req.request(NULL, 0)) {
        std::cout << "请求失败" << std::endl;
        return 1;
    }
    
    // 获取响应
    acl::string body;
    if (req.get_body(body)) {
        std::cout << "响应: " << body.c_str() << std::endl;
    }
    
    return 0;
}
```

### HTTP服务端示例

```cpp
#include "acl_cpp/lib_acl.hpp"

class MyServlet : public acl::HttpServlet {
protected:
    bool doGet(acl::HttpServletRequest& req, 
               acl::HttpServletResponse& res) override {
        // 获取请求参数
        const char* name = req.getParameter("name");
        
        // 设置响应
        res.setContentType("text/html; charset=utf-8");
        res.setStatus(200);
        
        // 输出响应
        acl::string body;
        body.format("<html><body>Hello, %s!</body></html>", 
                   name ? name : "World");
        res.write(body);
        
        return true;
    }
};
```

### WebSocket示例

```cpp
#include "acl_cpp/lib_acl.hpp"

class MyWebSocket : public acl::WebSocketServlet {
protected:
    bool onMessage(unsigned long long len, bool text, bool finish) override {
        acl::string buf;
        
        // 读取消息
        int ret = readPayload(buf.buf(), buf.capacity());
        if (ret > 0) {
            buf.set_offset(ret);
            // 回显消息
            sendText(buf.c_str());
        }
        
        return ret >= 0;
    }
    
    bool onPing(unsigned long long len, bool finish) override {
        sendPong(NULL);
        return true;
    }
    
    bool onPong(unsigned long long len, bool finish) override {
        return true;
    }
};
```

## 文档导航

- [HTTP客户端使用指南](http_client.md)
- [HTTP服务端使用指南](http_server.md)
- [WebSocket使用指南](websocket.md)
- [HTTP高级特性](advanced.md)
- [API参考](api_reference.md)

## 编译与链接

### 编译选项

```bash
g++ -o myapp myapp.cpp -I/path/to/acl/lib_acl_cpp/include \
    -L/path/to/acl/lib_acl_cpp/lib -l_acl_cpp \
    -L/path/to/acl/lib_acl/lib -l_acl \
    -L/path/to/acl/lib_protocol/lib -l_protocol \
    -lpthread -lz -ldl
```

### CMake配置

```cmake
find_package(ACL REQUIRED)

add_executable(myapp myapp.cpp)
target_link_libraries(myapp 
    ACL::acl_cpp 
    ACL::acl 
    ACL::protocol
)
```

## 性能特点

- **高性能**: 基于ACL高性能网络框架
- **低延迟**: 支持连接池，减少连接建立开销
- **内存优化**: 使用内存池技术，减少内存分配
- **异步支持**: 提供完整的异步HTTP客户端
- **可扩展**: 清晰的架构设计，易于扩展

## 兼容性

- **平台**: Linux, Windows, macOS, FreeBSD, Solaris
- **编译器**: GCC 4.8+, Clang 3.5+, MSVC 2015+
- **协议**: HTTP/1.0, HTTP/1.1, WebSocket (RFC 6455)
- **加密**: OpenSSL, PolarSSL, MbedTLS

## 许可证

ACL使用Apache License 2.0开源协议。

## 技术支持

- 项目主页: https://github.com/acl-dev/acl
- 作者邮箱: shuxin.zheng@qq.com
- QQ群: 242722074

## 版本历史

详见项目CHANGES文件。

