# HTTP服务端使用指南

## 概述

ACL提供了Servlet风格的HTTP服务器框架，简化了HTTP服务的开发。支持多种HTTP方法、Session管理、文件上传、Cookie处理等特性。

## 核心类

### 1. HttpServlet - Servlet基类

所有HTTP服务处理类都应继承`HttpServlet`基类。

```cpp
#include "acl_cpp/lib_acl.hpp"

class MyServlet : public acl::HttpServlet {
public:
    MyServlet(acl::socket_stream* stream, acl::session* session)
        : HttpServlet(stream, session) {}
    
    ~MyServlet() {}

protected:
    // 处理GET请求
    bool doGet(acl::HttpServletRequest& req, 
               acl::HttpServletResponse& res) override {
        return handle(req, res);
    }
    
    // 处理POST请求
    bool doPost(acl::HttpServletRequest& req, 
                acl::HttpServletResponse& res) override {
        return handle(req, res);
    }
    
    // 处理PUT请求
    bool doPut(acl::HttpServletRequest& req, 
               acl::HttpServletResponse& res) override {
        return handle(req, res);
    }
    
    // 处理DELETE请求
    bool doDelete(acl::HttpServletRequest& req, 
                  acl::HttpServletResponse& res) override {
        return handle(req, res);
    }

private:
    bool handle(acl::HttpServletRequest& req, 
                acl::HttpServletResponse& res) {
        // 实现具体逻辑
        res.setStatus(200);
        res.setContentType("text/plain");
        res.write("Hello, World!", 13);
        return true;
    }
};
```

### 2. HttpServletRequest - 请求封装

`HttpServletRequest`封装了HTTP请求的所有信息。

#### 2.1 获取请求信息

```cpp
bool MyServlet::doGet(acl::HttpServletRequest& req, 
                      acl::HttpServletResponse& res) {
    // 获取请求方法
    acl::http_method_t method = req.getMethod();
    
    // 获取请求路径
    const char* path = req.getPathInfo();        // /api/test
    const char* uri = req.getRequestUri();       // /api/test?id=1
    const char* query = req.getQueryString();    // id=1
    
    // 获取客户端信息
    const char* remote_addr = req.getRemoteAddr();
    unsigned short remote_port = req.getRemotePort();
    const char* user_agent = req.getUserAgent();
    
    // 获取本地信息
    const char* local_addr = req.getLocalAddr();
    unsigned short local_port = req.getLocalPort();
    
    return true;
}
```

#### 2.2 获取请求参数

```cpp
bool MyServlet::doGet(acl::HttpServletRequest& req, 
                      acl::HttpServletResponse& res) {
    // GET参数: /api/user?id=123&name=john
    const char* id = req.getParameter("id");
    const char* name = req.getParameter("name");
    
    printf("ID: %s, Name: %s\n", id, name);
    
    return true;
}

bool MyServlet::doPost(acl::HttpServletRequest& req, 
                       acl::HttpServletResponse& res) {
    // POST参数（application/x-www-form-urlencoded）
    const char* username = req.getParameter("username");
    const char* password = req.getParameter("password");
    
    // POST参数会自动解析到getParameter中
    
    return true;
}
```

#### 2.3 获取请求头

```cpp
bool MyServlet::doGet(acl::HttpServletRequest& req, 
                      acl::HttpServletResponse& res) {
    // 获取特定请求头
    const char* host = req.getHeader("Host");
    const char* user_agent = req.getUserAgent();
    const char* content_type = req.getContentType();
    const char* referer = req.getRequestReferer();
    
    // 获取Content-Length
    long long content_length = req.getContentLength();
    
    return true;
}
```

#### 2.4 获取Cookie

```cpp
bool MyServlet::doGet(acl::HttpServletRequest& req, 
                      acl::HttpServletResponse& res) {
    // 获取所有Cookie
    const std::vector<acl::HttpCookie*>& cookies = req.getCookies();
    for (auto cookie : cookies) {
        printf("Cookie: %s=%s\n", 
               cookie->getName(), 
               cookie->getValue());
    }
    
    // 获取特定Cookie
    const char* session_id = req.getCookieValue("sessionid");
    if (session_id) {
        printf("Session ID: %s\n", session_id);
    }
    
    return true;
}
```

#### 2.5 获取请求体

```cpp
bool MyServlet::doPost(acl::HttpServletRequest& req, 
                       acl::HttpServletResponse& res) {
    // 获取原始请求体
    acl::string body;
    if (req.getBody(body)) {
        printf("Body: %s\n", body.c_str());
    }
    
    return true;
}
```

#### 2.6 解析JSON请求

```cpp
bool MyServlet::doPost(acl::HttpServletRequest& req, 
                       acl::HttpServletResponse& res) {
    // 解析JSON请求体
    acl::json* json = req.getJson();
    if (json == NULL) {
        res.setStatus(400);
        res.write("Invalid JSON", 12);
        return true;
    }
    
    // 获取JSON字段
    const char* name = json->get_text("name");
    int age = json->get_int32("age");
    
    printf("Name: %s, Age: %d\n", name, age);
    
    return true;
}
```

#### 2.7 解析XML请求

```cpp
bool MyServlet::doPost(acl::HttpServletRequest& req, 
                       acl::HttpServletResponse& res) {
    // 解析XML请求体
    acl::xml* xml = req.getXml();
    if (xml == NULL) {
        res.setStatus(400);
        res.write("Invalid XML", 11);
        return true;
    }
    
    // 解析XML
    const char* value = xml->get_text("/root/node");
    
    return true;
}
```

#### 2.8 处理文件上传

```cpp
bool MyServlet::doPost(acl::HttpServletRequest& req, 
                       acl::HttpServletResponse& res) {
    // 获取MIME对象（multipart/form-data）
    acl::http_mime* mime = req.getHttpMime();
    if (mime == NULL) {
        res.setStatus(400);
        res.write("No file uploaded", 16);
        return true;
    }
    
    // 遍历所有上传的文件和参数
    const std::list<acl::http_mime_node*>& nodes = mime->get_nodes();
    for (auto node : nodes) {
        const char* name = node->get_name();
        
        if (node->get_mime_type() == acl::HTTP_MIME_FILE) {
            // 这是一个文件
            const char* filename = node->get_filename();
            const char* filepath = node->get_save_path();
            
            printf("文件: %s, 保存路径: %s\n", filename, filepath);
        } else if (node->get_mime_type() == acl::HTTP_MIME_PARAM) {
            // 这是一个普通参数
            const char* value = node->get_value();
            printf("参数: %s=%s\n", name, value);
        }
    }
    
    return true;
}
```

### 3. HttpServletResponse - 响应封装

`HttpServletResponse`用于构建HTTP响应。

#### 3.1 设置响应状态

```cpp
bool MyServlet::doGet(acl::HttpServletRequest& req, 
                      acl::HttpServletResponse& res) {
    // 设置状态码
    res.setStatus(200);  // OK
    res.setStatus(404);  // Not Found
    res.setStatus(500);  // Internal Server Error
    
    return true;
}
```

#### 3.2 设置响应头

```cpp
bool MyServlet::doGet(acl::HttpServletRequest& req, 
                      acl::HttpServletResponse& res) {
    // 设置Content-Type
    res.setContentType("text/html; charset=utf-8");
    res.setContentType("application/json");
    res.setContentType("application/xml");
    
    // 设置Content-Length
    res.setContentLength(1024);
    
    // 设置自定义头部
    res.setHeader("X-Custom-Header", "value");
    res.setHeader("Cache-Control", "no-cache");
    
    // 设置日期头
    res.setDateHeader("Last-Modified", time(NULL));
    
    // 设置Keep-Alive
    res.setKeepAlive(true);
    
    return true;
}
```

#### 3.3 发送响应体

```cpp
bool MyServlet::doGet(acl::HttpServletRequest& req, 
                      acl::HttpServletResponse& res) {
    res.setStatus(200);
    res.setContentType("text/html; charset=utf-8");
    
    // 发送字符串
    const char* html = "<html><body>Hello</body></html>";
    res.write(html, strlen(html));
    
    // 或使用acl::string
    acl::string body = "<html><body>World</body></html>";
    res.write(body);
    
    return true;
}
```

#### 3.4 发送JSON响应

```cpp
bool MyServlet::doGet(acl::HttpServletRequest& req, 
                      acl::HttpServletResponse& res) {
    res.setStatus(200);
    
    // 构建JSON
    acl::json json;
    json.add_text("message", "success");
    json.add_number("code", 0);
    json.add_text("data", "some data");
    
    // 发送JSON响应
    res.write(json, "utf-8");
    
    return true;
}
```

#### 3.5 发送XML响应

```cpp
bool MyServlet::doGet(acl::HttpServletRequest& req, 
                      acl::HttpServletResponse& res) {
    res.setStatus(200);
    
    // 构建XML
    acl::xml xml;
    xml.add_child("root", true);
    xml.add_child("message", "success");
    xml.add_child("code", 0);
    
    // 发送XML响应
    res.write(xml, "utf-8");
    
    return true;
}
```

#### 3.6 格式化输出

```cpp
bool MyServlet::doGet(acl::HttpServletRequest& req, 
                      acl::HttpServletResponse& res) {
    res.setStatus(200);
    res.setContentType("text/plain");
    
    // 格式化输出
    const char* name = req.getParameter("name");
    res.format("Hello, %s!\n", name ? name : "World");
    res.format("Time: %ld\n", time(NULL));
    
    return true;
}
```

#### 3.7 Chunked传输

```cpp
bool MyServlet::doGet(acl::HttpServletRequest& req, 
                      acl::HttpServletResponse& res) {
    res.setStatus(200);
    res.setContentType("text/plain");
    
    // 启用chunked传输
    res.setChunkedTransferEncoding(true);
    
    // 分块发送数据
    for (int i = 0; i < 10; i++) {
        acl::string chunk;
        chunk.format("Chunk %d\n", i);
        res.write(chunk);
        sleep(1);
    }
    
    // 发送结束标志
    res.write(NULL, 0);
    
    return true;
}
```

#### 3.8 文件下载

```cpp
bool MyServlet::doGet(acl::HttpServletRequest& req, 
                      acl::HttpServletResponse& res) {
    const char* filepath = "/path/to/file.pdf";
    
    // 打开文件
    FILE* fp = fopen(filepath, "rb");
    if (fp == NULL) {
        res.setStatus(404);
        res.write("File not found", 14);
        return true;
    }
    
    // 获取文件大小
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    // 设置响应头
    res.setStatus(200);
    res.setContentType("application/pdf");
    res.setContentLength(file_size);
    res.setHeader("Content-Disposition", 
                  "attachment; filename=file.pdf");
    
    // 发送文件内容
    char buf[8192];
    while (true) {
        size_t n = fread(buf, 1, sizeof(buf), fp);
        if (n == 0) break;
        res.write(buf, n);
    }
    
    fclose(fp);
    return true;
}
```

#### 3.9 Range请求处理

```cpp
bool MyServlet::doGet(acl::HttpServletRequest& req, 
                      acl::HttpServletResponse& res) {
    long long range_from, range_to;
    
    // 检查是否是Range请求
    if (req.getRange(range_from, range_to)) {
        // 处理Range请求
        res.setStatus(206);  // Partial Content
        res.setRange(range_from, range_to, file_total_size);
        
        // 发送部分内容
        // ...
    } else {
        // 完整文件
        res.setStatus(200);
        res.setContentLength(file_total_size);
        
        // 发送完整内容
        // ...
    }
    
    return true;
}
```

#### 3.10 设置Cookie

```cpp
bool MyServlet::doGet(acl::HttpServletRequest& req, 
                      acl::HttpServletResponse& res) {
    // 添加Cookie
    res.addCookie("sessionid", "abc123", 
                  ".example.com",  // domain
                  "/",             // path
                  3600);           // 过期时间（秒）
    
    // 或使用HttpCookie对象
    acl::HttpCookie* cookie = new acl::HttpCookie("userid", "12345");
    cookie->setDomain(".example.com");
    cookie->setPath("/");
    cookie->setMaxAge(86400);  // 1天
    res.addCookie(cookie);
    
    return true;
}
```

#### 3.11 重定向

```cpp
bool MyServlet::doGet(acl::HttpServletRequest& req, 
                      acl::HttpServletResponse& res) {
    // 临时重定向 (302)
    res.setRedirect("/new/location");
    
    // 永久重定向 (301)
    res.setRedirect("/new/location", 301);
    
    return true;
}
```

### 4. HttpSession - Session管理

Session用于在多个请求之间保持用户状态。

```cpp
bool MyServlet::doGet(acl::HttpServletRequest& req, 
                      acl::HttpServletResponse& res) {
    // 获取Session（如果不存在则创建）
    acl::HttpSession& session = req.getSession(true);
    
    // 存储Session数据
    session.setAttribute("username", "john");
    session.setAttribute("login_time", "2024-01-01");
    
    // 获取Session数据
    const char* username = session.getAttribute("username");
    if (username) {
        printf("用户: %s\n", username);
    }
    
    // 删除Session属性
    session.removeAttribute("login_time");
    
    // 设置Session过期时间（秒）
    session.setMaxAge(1800);  // 30分钟
    
    // 使Session失效
    // session.invalidate();
    
    return true;
}
```

## 服务器集成

### 1. 与acl_master集成

```cpp
#include "acl_cpp/lib_acl.hpp"

// 服务器回调函数
static void http_service(acl::socket_stream* conn, void* ctx) {
    (void) ctx;
    
    // 创建Session管理器（使用memcached）
    acl::memcache_session session("127.0.0.1:11211");
    
    // 创建Servlet
    MyServlet servlet(conn, &session);
    
    // 设置字符集
    servlet.setLocalCharset("utf-8");
    
    // 设置读写超时
    servlet.setRwTimeout(60);
    
    // 启动处理
    servlet.doRun();
}

int main(int argc, char* argv[]) {
    // 初始化ACL
    acl::acl_cpp_init();
    
    // 创建服务器
    acl::server_socket server;
    if (!server.open("0.0.0.0:8080")) {
        printf("打开端口失败\n");
        return 1;
    }
    
    printf("服务器运行在 8080 端口\n");
    
    // 接受连接
    while (true) {
        acl::socket_stream* conn = server.accept();
        if (conn == NULL) {
            continue;
        }
        
        // 处理请求
        http_service(conn, NULL);
        
        delete conn;
    }
    
    return 0;
}
```

### 2. 多线程服务器

```cpp
#include "acl_cpp/lib_acl.hpp"
#include <thread>

void handle_client(acl::socket_stream* conn) {
    acl::memcache_session session("127.0.0.1:11211");
    MyServlet servlet(conn, &session);
    servlet.setLocalCharset("utf-8");
    servlet.doRun();
    delete conn;
}

int main() {
    acl::acl_cpp_init();
    
    acl::server_socket server;
    server.open("0.0.0.0:8080");
    
    while (true) {
        acl::socket_stream* conn = server.accept();
        if (conn) {
            // 创建新线程处理
            std::thread t(handle_client, conn);
            t.detach();
        }
    }
    
    return 0;
}
```

## 高级特性

### 1. GZIP压缩

```cpp
bool MyServlet::doGet(acl::HttpServletRequest& req, 
                      acl::HttpServletResponse& res) {
    // 检查客户端是否支持GZIP
    std::vector<acl::string> encodings;
    req.getAcceptEncoding(encodings);
    
    bool support_gzip = false;
    for (auto& enc : encodings) {
        if (enc == "gzip") {
            support_gzip = true;
            break;
        }
    }
    
    if (support_gzip) {
        // 启用GZIP压缩
        res.setContentEncoding(true);
    }
    
    // 发送响应
    acl::string large_data = "...";  // 大量数据
    res.write(large_data);
    
    return true;
}
```

### 2. CGI模式

```cpp
bool MyServlet::doGet(acl::HttpServletRequest& req, 
                      acl::HttpServletResponse& res) {
    // 设置为CGI模式（不输出完整HTTP头）
    res.setCgiMode(true);
    
    // CGI模式下只输出响应体
    res.write("Content-type: text/html\r\n\r\n");
    res.write("<html><body>CGI Mode</body></html>");
    
    return true;
}
```

### 3. 字符集转换

```cpp
MyServlet::MyServlet(acl::socket_stream* stream, 
                     acl::session* session)
    : HttpServlet(stream, session) {
    // 设置本地字符集
    setLocalCharset("gbk");
}

bool MyServlet::doPost(acl::HttpServletRequest& req, 
                       acl::HttpServletResponse& res) {
    // 请求体会自动从UTF-8转换为GBK
    acl::string body;
    req.getBody(body);
    
    // 处理GBK编码的数据
    // ...
    
    return true;
}
```

## 最佳实践

1. **资源清理**: 使用RAII管理资源，避免内存泄漏
2. **异常处理**: 完善的错误检查和异常处理
3. **安全性**: 验证所有输入，防止注入攻击
4. **性能优化**: 
   - 使用内存池
   - 启用Keep-Alive
   - 合理使用GZIP压缩
5. **日志记录**: 记录关键操作和错误
6. **Session管理**: 合理设置Session过期时间
7. **文件上传**: 限制上传文件大小和类型

## 常见问题

### Q: 如何处理大文件上传?
A: 设置`setParseBodyLimit()`限制解析大小，使用流式处理。

### Q: Session存储在哪里?
A: 支持内存和memcached两种方式。

### Q: 如何实现RESTful API?
A: 重写doGet、doPost、doPut、doDelete方法。

### Q: 如何处理跨域请求?
A: 设置CORS响应头：
```cpp
res.setHeader("Access-Control-Allow-Origin", "*");
res.setHeader("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE");
```

