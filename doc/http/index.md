# ACL HTTP库文档索引

本目录包含ACL HTTP库的完整设计与使用文档。

## 文档列表

### 1. [README.md](README.md) - HTTP库总体介绍
- 概述和主要特性
- 架构设计
- 快速开始示例
- 编译与链接
- 性能特点和兼容性

### 2. [http_client.md](http_client.md) - HTTP客户端使用指南
- http_client - 底层协议处理
- http_request - 同步HTTP请求
  - 基本使用
  - GET/POST请求
  - 请求参数和请求头
  - Cookie管理
  - 断点续传
  - 流式处理
- http_aclient - 异步HTTP客户端
- http_request_pool - 连接池
- http_download - 文件下载
- SSL/TLS支持
- 字符集转换
- GZIP压缩
- 最佳实践

### 3. [http_server.md](http_server.md) - HTTP服务端使用指南
- HttpServlet - Servlet基类
- HttpServletRequest - 请求封装
  - 获取请求信息
  - 获取请求参数
  - 获取请求头和Cookie
  - 解析JSON/XML请求
  - 处理文件上传
- HttpServletResponse - 响应封装
  - 设置响应状态和响应头
  - 发送响应体
  - 发送JSON/XML响应
  - Chunked传输
  - 文件下载
  - Range请求处理
  - 设置Cookie
  - 重定向
- HttpSession - Session管理
- 服务器集成
- 多线程服务器
- 高级特性

### 4. [websocket.md](websocket.md) - WebSocket使用指南
- websocket - WebSocket协议处理
- WebSocket帧类型
- 服务端WebSocket
  - WebSocketServlet基类
  - 发送消息
  - 处理分片消息
  - 主动关闭连接
- 客户端WebSocket
  - 同步客户端
  - 异步客户端
- WebSocket高级特性
  - 心跳检测
  - 广播消息
  - 消息压缩
  - 子协议
- SSL/TLS支持
- 最佳实践

### 5. [advanced.md](advanced.md) - HTTP高级特性
- 内存池管理
- 字符集转换
  - 自动字符集转换
  - 手动字符集转换
- HTTP压缩
  - 客户端请求压缩
  - 服务端压缩响应
  - 手动压缩/解压
- SSL/TLS加密
  - 客户端SSL
  - 服务端SSL
  - 证书验证
- 连接池
  - HTTP连接池配置
  - 连接池统计
- 异步HTTP
  - 异步客户端
  - 异步POST请求
- 流式处理
  - 流式发送
  - 流式接收
- 多线程/多进程
- 性能优化技巧
  - Keep-Alive
  - 缓冲区优化
  - 批量操作
- 调试技巧
- 常见陷阱
- 最佳实践总结

### 6. [api_reference.md](api_reference.md) - API参考
- 核心类概览
- 详细API文档
  - http_client
  - http_request
  - http_aclient
  - http_header
  - HttpServlet
  - HttpServletRequest
  - HttpServletResponse
  - websocket
  - WebSocketServlet
  - HttpCookie
  - HttpSession
  - http_mime
  - http_download
  - http_request_pool
- 枚举类型
- 常量定义
- 完整示例

## 快速导航

### 按使用场景

#### HTTP客户端开发
1. 阅读 [README.md](README.md) 了解整体架构
2. 参考 [http_client.md](http_client.md) 学习客户端API
3. 查看 [advanced.md](advanced.md) 了解高级特性
4. 使用 [api_reference.md](api_reference.md) 作为API速查手册

#### HTTP服务端开发
1. 阅读 [README.md](README.md) 了解整体架构
2. 参考 [http_server.md](http_server.md) 学习服务端API
3. 查看 [advanced.md](advanced.md) 了解高级特性
4. 使用 [api_reference.md](api_reference.md) 作为API速查手册

#### WebSocket开发
1. 阅读 [README.md](README.md) 了解整体架构
2. 参考 [websocket.md](websocket.md) 学习WebSocket API
3. 查看 [advanced.md](advanced.md) 了解SSL等高级特性
4. 使用 [api_reference.md](api_reference.md) 作为API速查手册

### 按功能特性

| 功能 | 相关文档 |
|------|---------|
| HTTP GET/POST请求 | [http_client.md](http_client.md) |
| 文件上传/下载 | [http_client.md](http_client.md), [http_server.md](http_server.md) |
| Cookie管理 | [http_client.md](http_client.md), [http_server.md](http_server.md) |
| Session管理 | [http_server.md](http_server.md) |
| WebSocket通信 | [websocket.md](websocket.md) |
| SSL/TLS加密 | [advanced.md](advanced.md) |
| GZIP压缩 | [advanced.md](advanced.md) |
| 连接池 | [http_client.md](http_client.md), [advanced.md](advanced.md) |
| 异步HTTP | [http_client.md](http_client.md), [advanced.md](advanced.md) |
| 字符集转换 | [advanced.md](advanced.md) |

## 示例代码

### HTTP客户端示例

```cpp
#include "acl_cpp/lib_acl.hpp"

int main() {
    acl::http_request req("www.example.com:80");
    req.request_header().set_url("/api/test");
    req.request_header().set_host("www.example.com");
    
    if (req.request(NULL, 0)) {
        acl::string body;
        req.get_body(body);
        printf("响应: %s\n", body.c_str());
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
        res.setStatus(200);
        res.setContentType("text/plain");
        res.write("Hello, World!", 13);
        return true;
    }
};
```

### WebSocket服务端示例

```cpp
#include "acl_cpp/lib_acl.hpp"

class MyWebSocket : public acl::WebSocketServlet {
protected:
    bool onMessage(unsigned long long len, bool text, bool finish) override {
        char buf[8192];
        int ret = readPayload(buf, sizeof(buf) - 1);
        if (ret > 0) {
            buf[ret] = '\0';
            sendText(buf);  // 回显消息
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

## 相关资源

- **项目主页**: https://github.com/acl-dev/acl
- **示例代码**: acl/app/wizard_demo/
- **API文档**: 本目录下的文档
- **RFC文档**: 
  - [rfc3492.txt](rfc3492.txt) - Punycode编码
  - [websocket.pdf](websocket.pdf) - WebSocket协议

## 技术支持

- **作者邮箱**: shuxin.zheng@qq.com
- **QQ群**: 242722074
- **GitHub Issues**: https://github.com/acl-dev/acl/issues

## 文档版本

- **版本**: 1.0
- **更新日期**: 2024年11月3日
- **适用ACL版本**: 3.5.0及以上

## 许可证

本文档遵循与ACL项目相同的Apache License 2.0开源协议。

---

**欢迎贡献**: 如果您发现文档中的错误或有改进建议，欢迎提交Issue或Pull Request。

