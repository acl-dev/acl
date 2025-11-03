# HTTP库API参考

## 核心类概览

### HTTP客户端
- `http_client` - 底层HTTP协议处理类
- `http_request` - 同步HTTP请求类
- `http_aclient` - 异步HTTP客户端类
- `http_request_pool` - HTTP连接池类
- `http_download` - 文件下载类

### HTTP服务端
- `HttpServlet` - Servlet基类
- `HttpServletRequest` - 请求封装类
- `HttpServletResponse` - 响应封装类
- `HttpSession` - Session管理类

### 公共组件
- `http_header` - HTTP头部类
- `HttpCookie` - Cookie类
- `http_mime` - MIME处理类
- `websocket` - WebSocket协议类
- `WebSocketServlet` - WebSocket Servlet类
- `http_utils` - 工具类

---

## 详细API

## http_client

底层HTTP协议处理类。

### 构造函数

```cpp
http_client();
http_client(socket_stream* client, bool is_request = false, 
           bool unzip = true, bool stream_fixed = true);
```

### 主要方法

#### 连接管理

```cpp
bool open(const char* addr, int conn_timeout = 60, 
         int rw_timeout = 60, bool unzip = true);
void reset();
```

#### 请求发送

```cpp
bool write_head(const http_header& header);
bool write_body(const void* data, size_t len);
ostream& get_ostream() const;
```

#### 响应接收

```cpp
bool read_head();
int read_body(char* buf, size_t size);
int read_body(string& out, bool clean = true, int* real_size = NULL);
bool body_gets(string& out, bool nonl = true, size_t* size = NULL);
```

#### 状态查询

```cpp
int response_status() const;
long long int body_length() const;
bool is_keep_alive() const;
bool body_finish() const;
bool disconnected() const;
const char* header_value(const char* name) const;
```

#### Range支持

```cpp
bool request_range(long long int& range_from, long long int& range_to);
bool response_range(long long int& range_from, long long int& range_to, 
                   long long int& total);
```

---

## http_request

同步HTTP请求类。

### 构造函数

```cpp
explicit http_request(socket_stream* client, int conn_timeout = 60,
                     bool unzip = true, bool stream_fixed = true);
explicit http_request(const char* addr, int conn_timeout = 60,
                     int rw_timeout = 60, bool unzip = true);
```

### 主要方法

#### 配置

```cpp
http_request& set_unzip(bool on);
http_request& set_ssl(sslbase_conf* conf);
http_request& set_ssl_sni(const char* sni);
http_request& set_local_charset(const char* local_charset);
```

#### 请求发送

```cpp
http_header& request_header();
bool get();
bool post(const char* data, size_t len);
bool request(const void* data, size_t len);
bool write_head();
bool write_body(const void* data, size_t len);
```

#### 响应接收

```cpp
int http_status() const;
long long int body_length() const;
bool keep_alive() const;
const char* header_value(const char* name) const;
bool body_finish() const;

bool get_body(xml& out, const char* to_charset = NULL);
bool get_body(json& out, const char* to_charset = NULL);
bool get_body(string& out, const char* to_charset = NULL);
int read_body(char* buf, size_t size);
int read_body(string& out, bool clean = false, int* real_size = NULL);
bool body_gets(string& out, bool nonl = true, size_t* size = NULL);
```

#### Range支持

```cpp
bool support_range() const;
long long int get_range_from() const;
long long int get_range_to() const;
long long int get_range_max() const;
```

#### Cookie

```cpp
const std::vector<HttpCookie*>* get_cookies() const;
const HttpCookie* get_cookie(const char* name, 
                             bool case_insensitive = true) const;
```

#### 其他

```cpp
http_client* get_client() const;
void reset();
```

---

## http_aclient

异步HTTP客户端类。

### 构造函数

```cpp
explicit http_aclient(aio_handle& handle, sslbase_conf* ssl_conf = NULL);
```

### 主要方法

#### 配置

```cpp
http_header& request_header();
http_aclient& unzip_body(bool on);
http_aclient& set_ssl_conf(sslbase_conf* ssl_conf);
http_aclient& set_ssl_sni(const char* sni);
http_aclient& enable_ssl(bool yes);
```

#### 连接

```cpp
bool open(const char* addr, int conn_timeout, int rw_timeout,
         const char *local = NULL);
void close();
aio_socket_stream* get_conn() const;
```

#### 请求发送

```cpp
void send_request(const void* body, size_t len);
```

#### WebSocket

```cpp
void ws_handshake(const void* key, size_t len);
void ws_handshake(const char* key = "123456789xxx");
void ws_read_wait(int timeout = 0);
bool ws_send_text(char* data, size_t len);
bool ws_send_binary(void* data, size_t len);
bool ws_send_ping(void* data, size_t len);
bool ws_send_pong(void* data, size_t len);
```

### 回调方法（需要重写）

```cpp
virtual void destroy() = 0;
virtual bool on_connect() = 0;
virtual void on_ns_failed();
virtual void on_connect_timeout();
virtual void on_connect_failed();
virtual bool on_read_timeout();
virtual void on_disconnect();

// HTTP回调
virtual bool on_http_res_hdr(const http_header& header);
virtual bool on_http_res_body(char* data, size_t dlen);
virtual bool on_http_res_finish(bool success);

// WebSocket回调
virtual bool on_ws_handshake();
virtual void on_ws_handshake_failed(int status);
virtual bool on_ws_frame_text();
virtual bool on_ws_frame_binary();
virtual void on_ws_frame_closed();
virtual bool on_ws_frame_data(char* data, size_t dlen);
virtual bool on_ws_frame_finish();
virtual void on_ws_frame_ping(string& data);
virtual void on_ws_frame_pong(string& data);
```

---

## http_header

HTTP头部类。

### 构造函数

```cpp
http_header(dbuf_guard* dbuf = NULL);
http_header(const char* url, dbuf_guard* dbuf = NULL, 
           bool encoding = true);
http_header(int status, dbuf_guard* dbuf = NULL);
```

### 主要方法

#### 通用方法

```cpp
void reset();
http_header& set_proto_version(const char* version);
http_header& set_request_mode(bool onoff);
http_header& add_entry(const char* name, const char* value, 
                       bool replace = true);
void disable_header(const char* name, bool yes);
const char* get_entry(const char* name) const;
```

#### Content相关

```cpp
http_header& set_content_length(long long int n);
long long int get_content_length() const;
http_header& set_content_type(const char* value);
```

#### Range支持

```cpp
http_header& set_range(long long from, long long to);
http_header& set_range_total(long long total);
void get_range(long long int* from, long long int* to);
```

#### 连接管理

```cpp
http_header& set_keep_alive(bool on);
bool get_keep_alive() const;
```

#### Cookie

```cpp
http_header& add_cookie(const char* name, const char* value,
                       const char* domain = NULL, const char* path = NULL,
                       time_t expires = 0);
http_header& add_cookie(const HttpCookie* cookie);
const HttpCookie* get_cookie(const char* name) const;
```

#### 请求方法

```cpp
bool build_request(string& buf) const;
http_header& set_url(const char* url, bool encoding = true);
http_header& set_host(const char* value);
const char* get_host() const;
http_header& set_method(http_method_t method);
http_header& set_method(const char* method);
http_method_t get_method(string* buf = NULL) const;
http_header& accept_gzip(bool on);
http_header& set_param_override(bool yes);
```

#### 请求参数

```cpp
http_header& add_param(const char* name, const char* value);
http_header& add_int(const char* name, int value);
http_header& add_format(const char* name, const char* fmt, ...);
```

#### WebSocket

```cpp
http_header& set_upgrade(const char* value = "websocket");
const char* get_upgrade() const;
http_header& set_ws_origin(const char* url);
http_header& set_ws_key(const void* key, size_t len);
http_header& set_ws_protocol(const char* proto);
http_header& set_ws_version(int ver);
http_header& set_ws_accept(const char* key);
```

#### 响应方法

```cpp
bool build_response(string& buf) const;
http_header& set_status(int status);
int get_status() const;
http_header& set_chunked(bool on);
bool chunked_transfer() const;
http_header& set_cgi_mode(bool on);
bool is_cgi_mode() const;
http_header& set_transfer_gzip(bool on);
bool is_transfer_gzip() const;
```

#### 重定向

```cpp
bool redirect(const char* url);
http_header& set_redirect(unsigned int n = 5);
unsigned int get_redirect() const;
```

---

## HttpServlet

HTTP Servlet基类。

### 构造函数

```cpp
HttpServlet(socket_stream* stream, session* session);
explicit HttpServlet(socket_stream* stream);
HttpServlet();
```

### 主要方法

#### 配置

```cpp
HttpServlet& setLocalCharset(const char* charset);
HttpServlet& setRwTimeout(int rw_timeout);
HttpServlet& setParseBody(bool yes);
HttpServlet& setParseBodyLimit(int length);
```

#### 运行

```cpp
bool start();
virtual bool doRun();
virtual bool doRun(session& session, socket_stream* stream = NULL);
```

#### 访问器

```cpp
session& getSession() const;
socket_stream* getStream() const;
```

### 处理方法（需要重写）

```cpp
virtual bool doGet(HttpServletRequest&, HttpServletResponse&);
virtual bool doPost(HttpServletRequest&, HttpServletResponse&);
virtual bool doPut(HttpServletRequest&, HttpServletResponse&);
virtual bool doDelete(HttpServletRequest&, HttpServletResponse&);
virtual bool doHead(HttpServletRequest&, HttpServletResponse&);
virtual bool doOptions(HttpServletRequest&, HttpServletResponse&);
virtual bool doConnect(HttpServletRequest&, HttpServletResponse&);
virtual bool doPatch(HttpServletRequest&, HttpServletResponse&);
virtual bool doPurge(HttpServletRequest&, HttpServletResponse&);
virtual bool doPropfind(HttpServletRequest&, HttpServletResponse&);
virtual bool doWebSocket(HttpServletRequest&, HttpServletResponse&);
virtual bool doOther(HttpServletRequest&, HttpServletResponse&, 
                    const char* method);
virtual bool doUnknown(HttpServletRequest&, HttpServletResponse&);
virtual bool doError(HttpServletRequest&, HttpServletResponse&);
```

---

## HttpServletRequest

HTTP请求封装类。

### 主要方法

#### 请求信息

```cpp
http_method_t getMethod(string* method_s = NULL) const;
const char* getQueryString() const;
const char* getPathInfo() const;
const char* getRequestUri() const;
long long int getContentLength() const;
bool getRange(long long int& range_from, long long int& range_to);
const char* getContentType(bool part = true, http_ctype* ctype = NULL) const;
const char* getCharacterEncoding() const;
http_request_t getRequestType() const;
```

#### 地址信息

```cpp
const char* getLocalAddr() const;
unsigned short getLocalPort() const;
const char* getRemoteAddr() const;
unsigned short getRemotePort() const;
const char* getRemoteHost() const;
const char* getUserAgent() const;
const char* getRequestReferer() const;
```

#### 参数获取

```cpp
const char* getParameter(const char* name, 
                         bool case_sensitive = false) const;
```

#### 请求头

```cpp
const char* getHeader(const char* name) const;
```

#### Cookie

```cpp
const std::vector<HttpCookie*>& getCookies() const;
const char* getCookieValue(const char* name) const;
void setCookie(const char* name, const char* value);
```

#### Session

```cpp
HttpSession& getSession(bool create = true, const char* sid = NULL);
```

#### 请求体

```cpp
http_mime* getHttpMime();
json* getJson(size_t body_limit = 1024000);
bool getJson(json& out, size_t body_limit = 1024000);
xml* getXml(size_t body_limit = 1024000);
bool getXml(xml& out, size_t body_limit = 1024000);
string* getBody(size_t body_limit = 1024000);
bool getBody(string& out, size_t body_limit = 1024000);
```

#### 其他

```cpp
istream& getInputStream() const;
socket_stream& getSocketStream() const;
http_client* getClient() const;
bool isKeepAlive() const;
int getKeepAlive() const;
bool getVersion(unsigned& major, unsigned& minor) const;
void getAcceptEncoding(std::vector<string>& out) const;
```

---

## HttpServletResponse

HTTP响应封装类。

### 主要方法

#### 响应头设置

```cpp
HttpServletResponse& setContentLength(long long int n);
HttpServletResponse& setChunkedTransferEncoding(bool on);
HttpServletResponse& setKeepAlive(bool on);
HttpServletResponse& setContentType(const char* value);
HttpServletResponse& setContentEncoding(bool gzip);
HttpServletResponse& setCharacterEncoding(const char* charset);
HttpServletResponse& setDateHeader(const char* name, time_t value);
HttpServletResponse& setHeader(const char* name, const char* value);
HttpServletResponse& setHeader(const char* name, int value);
HttpServletResponse& setRange(long long from, long long to, long long total);
HttpServletResponse& setStatus(int status);
HttpServletResponse& setCgiMode(bool on);
HttpServletResponse& setRedirect(const char* location, int status = 302);
```

#### Cookie

```cpp
HttpServletResponse& addCookie(HttpCookie* cookie);
HttpServletResponse& addCookie(const char* name, const char* value,
                              const char* domain = NULL, 
                              const char* path = NULL,
                              time_t expires = 0);
```

#### 响应体发送

```cpp
bool write(const void* data, size_t len);
bool write(const string& buf);
bool write(const xml& body, const char* charset = "utf-8");
bool write(const json& body, const char* charset = "utf-8");
int format(const char* fmt, ...);
int vformat(const char* fmt, va_list ap);
```

#### 其他

```cpp
http_header& getHttpHeader() const;
bool sendHeader();
ostream& getOutputStream() const;
socket_stream& getSocketStream() const;
http_client* getClient() const;
```

---

## websocket

WebSocket协议类。

### 构造函数

```cpp
explicit websocket(socket_stream& client);
```

### 主要方法

#### 配置

```cpp
websocket& reset();
socket_stream& get_stream() const;
websocket& set_frame_fin(bool yes);
websocket& set_frame_rsv1(bool yes);
websocket& set_frame_rsv2(bool yes);
websocket& set_frame_rsv3(bool yes);
websocket& set_frame_opcode(unsigned char type);
websocket& set_frame_payload_len(unsigned long long len);
websocket& set_frame_masking_key(unsigned int mask);
```

#### 发送

```cpp
bool send_frame_data(const void* data, size_t len);
bool send_frame_pong(const void* data, size_t len);
bool send_frame_ping(const void* data, size_t len);
```

#### 接收

```cpp
bool read_frame_head();
int read_frame_data(void* buf, size_t size);
bool peek_frame_head();
int peek_frame_data(char* buf, size_t size);
```

#### 状态查询

```cpp
bool is_head_finish() const;
bool eof();
const frame_header& get_frame_header() const;
bool frame_is_fin() const;
bool frame_is_rsv1() const;
bool frame_is_rsv2() const;
bool frame_is_rsv3() const;
unsigned char get_frame_opcode() const;
bool frame_has_mask() const;
unsigned long long get_frame_payload_len() const;
unsigned int get_frame_masking_key() const;
unsigned long long get_frame_payload_nread() const;
```

---

## WebSocketServlet

WebSocket Servlet基类。

### 构造函数

```cpp
WebSocketServlet();
WebSocketServlet(socket_stream* stream, session* session);
```

### 主要方法

#### 发送

```cpp
bool sendBinary(const char *buf, int len);
bool sendText(const char *text);
bool sendPong(const char *buffer = NULL);
bool sendPing(const char *buffer = NULL);
```

#### 读取

```cpp
int readPayload(void* buf, size_t size);
```

#### 访问器

```cpp
websocket* get_websocket() const;
```

### 回调方法（需要重写）

```cpp
virtual void onClose();
virtual bool onPing(unsigned long long payload_len, bool finish) = 0;
virtual bool onPong(unsigned long long payload_len, bool finish) = 0;
virtual bool onMessage(unsigned long long payload_len, 
                      bool text, bool finish) = 0;
```

---

## HttpCookie

Cookie类。

### 构造函数

```cpp
HttpCookie(const char* name, const char* value, dbuf_guard* dbuf = NULL);
explicit HttpCookie(dbuf_guard* dbuf = NULL);
explicit HttpCookie(const HttpCookie* cookie, dbuf_guard* dbuf = NULL);
```

### 主要方法

```cpp
bool setCookie(const char* value);
void destroy();

HttpCookie& setDomain(const char* domain);
HttpCookie& setPath(const char* path);
HttpCookie& setExpires(time_t timeout);
HttpCookie& setExpires(const char* expires);
HttpCookie& setMaxAge(int max_age);
HttpCookie& add(const char* name, const char* value);

const char* getName() const;
const char* getValue() const;
const char* getExpires() const;
const char* getDomain() const;
const char* getPath() const;
int getMaxAge() const;
const char* getParam(const char* name, 
                    bool case_insensitive = true) const;
const std::list<HTTP_PARAM*>& getParams() const;
```

---

## HttpSession

Session管理类。

### 构造函数

```cpp
explicit HttpSession(session& session);
```

### 主要方法

```cpp
virtual const char* getAttribute(const char* name) const;
virtual const void* getAttribute(const char* name, size_t* size) const;
virtual bool getAttributes(std::map<string, session_string>& attrs) const;

virtual bool setAttribute(const char* name, const char* value);
virtual bool setAttribute(const char* name, const void* value, size_t len);
virtual bool setAttributes(const std::map<string, session_string>& attrs);

virtual bool removeAttribute(const char* name);
virtual bool setMaxAge(time_t ttl);
virtual bool invalidate();

const char* getSid() const;
```

---

## http_mime

MIME处理类。

### 构造函数

```cpp
http_mime(const char* boundary, const char* local_charset = "gb2312");
```

### 主要方法

```cpp
void set_saved_path(const char* path);
bool update(const char* data, size_t len);
const std::list<http_mime_node*>& get_nodes() const;
const http_mime_node* get_node(const char* name) const;
```

---

## http_mime_node

MIME节点类。

### 主要方法

```cpp
http_mime_t get_mime_type() const;
const char* get_value() const;
const char* get_name() const;
const char* get_filename() const;
```

---

## http_download

文件下载类。

### 构造函数

```cpp
http_download(const char* url, const char* addr = NULL);
```

### 主要方法

```cpp
http_header* request_header() const;
http_request* request() const;
bool get(long long int range_from = -1, long long int range_to = -1,
        const char* req_body = NULL, size_t len = 0);
bool reset(const char* url = NULL, const char* addr = NULL);
const char* get_url() const;
const char* get_addr() const;
```

### 回调方法（需要重写）

```cpp
virtual bool on_response(http_client* conn);
virtual bool on_length(long long int n);
virtual bool on_save(const void* data, size_t len) = 0;
```

---

## http_request_pool

HTTP连接池类。

### 构造函数

```cpp
http_request_pool(const char* addr, size_t count, size_t idx = 0);
```

### 主要方法

```cpp
void set_ssl(sslbase_conf* ssl_conf);
```

继承自`connect_pool`的方法：

```cpp
connect_client* peek();
void put(connect_client* conn, bool keep = true);
void set_timeout(int conn_timeout, int rw_timeout);
void set_idle(int ttl);
int get_count() const;
int get_idle_count() const;
```

---

## 枚举类型

### http_method_t

HTTP请求方法：

```cpp
HTTP_METHOD_UNKNOWN = 0,
HTTP_METHOD_GET,
HTTP_METHOD_POST,
HTTP_METHOD_PUT,
HTTP_METHOD_DELETE,
HTTP_METHOD_HEAD,
HTTP_METHOD_OPTIONS,
HTTP_METHOD_CONNECT,
HTTP_METHOD_PURGE,
HTTP_METHOD_PATCH,
HTTP_METHOD_PROPFIND
```

### http_request_t

HTTP请求类型：

```cpp
HTTP_REQUEST_NORMAL = 0,
HTTP_REQUEST_MULTIPART_FORM,
HTTP_REQUEST_OCTET_STREAM,
HTTP_REQUEST_OTHER
```

### http_mime_t

MIME类型：

```cpp
HTTP_MIME_PARAM = 0,
HTTP_MIME_FILE,
HTTP_MIME_OTHER
```

---

## 常量

### WebSocket帧类型

```cpp
FRAME_CONTINUATION = 0x00,
FRAME_TEXT         = 0x01,
FRAME_BINARY       = 0x02,
FRAME_CLOSE        = 0x08,
FRAME_PING         = 0x09,
FRAME_PONG         = 0x0A
```

---

## 完整示例

请参考各个使用指南文档中的示例代码。

