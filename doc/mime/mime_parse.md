# MIME 邮件解析详解

## 概述

MIME 解析模块提供了完整的邮件解析功能，支持各种标准的 MIME 格式邮件，包括单部分邮件、多部分邮件（multipart）、带附件邮件等。

## 核心类：mime

`mime` 类是邮件解析的核心类，提供了解析邮件文件和邮件数据流的功能。

### 基本用法

#### 1. 解析邮件文件

```cpp
#include "acl_cpp/mime/mime.hpp"

acl::mime mail;

// 解析邮件文件
if (!mail.parse("/path/to/email.eml")) {
    std::cerr << "解析失败" << std::endl;
    return;
}

// 解析成功，可以访问邮件内容
```

#### 2. 流式解析（适合大邮件）

对于大型邮件，可以使用流式解析方式，边读取边解析：

```cpp
acl::mime mail;
mail.update_begin(NULL);  // 开始流式解析

// 循环读取邮件数据
while (read_data(buffer, size)) {
    bool finished = mail.update(buffer, size);
    if (finished) {
        break;  // multipart 邮件解析完成
    }
}

mail.update_end();  // 结束流式解析
```

#### 3. 重置解析器

如果需要解析多个邮件，可以重置解析器状态：

```cpp
acl::mime mail;

// 解析第一封邮件
mail.parse("/path/to/email1.eml");

// 重置后解析第二封邮件
mail.reset();
mail.parse("/path/to/email2.eml");
```

## 获取邮件头信息

### 发件人信息

```cpp
// 获取发件人（From 字段）
const acl::string& from = mail.from();
std::cout << "发件人: " << from << std::endl;

// 获取发送者（Sender 字段）
const acl::string& sender = mail.sender();

// 获取回复地址（Reply-To 字段）
const acl::string& replyto = mail.replyto();

// 获取返回路径（Return-Path 字段）
const acl::string& returnpath = mail.returnpath();
```

### 收件人信息

```cpp
// 获取收件人列表（To 字段）
const std::list<char*>& to_list = mail.to_list();
for (auto* addr : to_list) {
    std::cout << "收件人: " << addr << std::endl;
}

// 获取抄送列表（CC 字段）
const std::list<char*>& cc_list = mail.cc_list();

// 获取密送列表（BCC 字段）
const std::list<char*>& bcc_list = mail.bcc_list();

// 获取所有收件人（To + CC + BCC）
const std::list<char*>& rcpt_list = mail.rcpt_list();
```

### 主题

```cpp
// 获取邮件主题
const acl::string& subject = mail.subject();
std::cout << "主题: " << subject << std::endl;
```

### 自定义头字段

```cpp
// 获取单个头字段值
const char* message_id = mail.header_value("Message-ID");
if (message_id) {
    std::cout << "消息ID: " << message_id << std::endl;
}

// 获取多值头字段（如 Received）
std::list<const char*> received_list;
int count = mail.header_values("Received", &received_list);
std::cout << "经过 " << count << " 个服务器" << std::endl;

// 获取所有头字段
const std::list<HEADER*>& headers = mail.header_list();
for (auto* header : headers) {
    std::cout << header->name << ": " << header->value << std::endl;
}
```

### Content-Type 信息

```cpp
// 获取主类型（如 text, multipart, image）
const char* ctype = mail.get_ctype();

// 获取子类型（如 plain, html, mixed）
const char* stype = mail.get_stype();

std::cout << "Content-Type: " << ctype << "/" << stype << std::endl;
```

## 获取邮件正文

### 1. 自动选择正文类型

```cpp
// 优先获取 HTML 正文，没有则获取纯文本
acl::mime_body* body = mail.get_body_node(
    true,        // htmlFirst: 优先HTML
    true,        // enableDecode: 自动解码
    "utf-8"      // toCharset: 目标字符集
);

if (body) {
    // 保存到文件
    body->save_body("body.html");
    
    // 或输出到流
    body->save_body(std::cout);
    
    // 或保存到字符串
    acl::string content;
    body->save_body(content, NULL, 0);
}
```

### 2. 获取纯文本正文

```cpp
acl::mime_body* plain_body = mail.get_plain_body(
    true,        // enableDecode: 自动解码
    "utf-8"      // toCharset: 目标字符集
);

if (plain_body) {
    std::cout << "纯文本正文：" << std::endl;
    plain_body->save_body(std::cout);
}
```

### 3. 获取 HTML 正文

```cpp
acl::mime_body* html_body = mail.get_html_body(
    true,        // enableDecode: 自动解码
    "utf-8"      // toCharset: 目标字符集
);

if (html_body) {
    std::cout << "HTML 正文：" << std::endl;
    html_body->save_body(std::cout);
}
```

## 获取附件

### 1. 获取所有附件

```cpp
// 获取附件列表
const std::list<acl::mime_attach*>& attachments = mail.get_attachments(
    true,        // enableDecode: 自动解码
    "utf-8",     // toCharset: 目标字符集
    0,           // off: 偏移量
    true         // all: 包含所有类型（message/application/image）
);

// 遍历附件
for (auto* attach : attachments) {
    // 获取附件文件名
    const char* filename = attach->get_filename();
    if (filename) {
        std::cout << "附件: " << filename << std::endl;
        
        // 保存附件
        std::string save_path = "/path/to/save/" + std::string(filename);
        attach->save(save_path.c_str());
    }
}
```

### 2. 附件详细信息

```cpp
for (auto* attach : attachments) {
    // 文件名
    const char* filename = attach->get_filename();
    
    // Content-Type
    int ctype = attach->get_ctype();   // MIME_CTYPE_XXX
    int stype = attach->get_stype();   // MIME_STYPE_XXX
    const char* ctype_s = attach->get_ctype_s();
    const char* stype_s = attach->get_stype_s();
    
    // 编码方式
    int encoding = attach->get_encoding();  // MIME_ENC_XXX
    
    // 字符集
    const char* charset = attach->get_charset();
    
    // 数据位置（在原始邮件文件中的位置）
    off_t begin = attach->get_bodyBegin();
    off_t end = attach->get_bodyEnd();
    
    std::cout << "文件名: " << (filename ? filename : "无")
              << ", 类型: " << ctype_s << "/" << stype_s
              << ", 编码: " << encoding
              << ", 大小: " << (end - begin) << " 字节"
              << std::endl;
}
```

## 获取图片

### 1. 获取所有图片

```cpp
// 获取图片列表
const std::list<acl::mime_image*>& images = mail.get_images(
    true,        // enableDecode: 自动解码
    "utf-8"      // toCharset: 目标字符集
);

// 遍历图片
for (auto* image : images) {
    // 获取图片名称
    const char* name = image->get_name();
    
    // 获取图片位置标识（Content-ID）
    const char* location = image->get_location();
    
    if (name) {
        std::string save_path = "/path/to/images/" + std::string(name);
        image->save(save_path.c_str());
    }
}
```

### 2. 根据 Content-ID 获取图片

```cpp
// HTML 中引用的图片通常使用 cid:xxx 格式
// <img src="cid:image001.jpg@01D2E3F4.5A6B7C8D">
const char* cid = "image001.jpg@01D2E3F4.5A6B7C8D";
acl::mime_image* image = mail.get_image(cid);

if (image) {
    // 保存图片
    image->save("/path/to/image.jpg");
}
```

## 获取所有 MIME 节点

如果需要遍历邮件的所有部分（part），可以获取所有 MIME 节点：

```cpp
const std::list<acl::mime_node*>& nodes = mail.get_mime_nodes(
    true,        // enableDecode: 自动解码
    "utf-8"      // toCharset: 目标字符集
);

for (auto* node : nodes) {
    // 节点类型
    int ctype = node->get_ctype();
    int stype = node->get_stype();
    
    std::cout << "节点类型: " 
              << node->get_ctype_s() << "/" 
              << node->get_stype_s() 
              << std::endl;
    
    // 节点内容
    if (ctype == MIME_CTYPE_TEXT) {
        // 文本内容
        node->save(std::cout);
    } else {
        // 其他内容
        const char* name = node->get_name();
        if (name) {
            node->save(name);
        }
    }
}
```

## MIME 节点层次结构

MIME 邮件通常是树形结构，节点之间有父子关系：

```cpp
acl::mime_node* node = ...; // 某个节点

// 检查是否有父节点
if (node->has_parent()) {
    // 获取父节点
    acl::mime_node* parent = node->get_parent();
    
    if (parent) {
        // 父节点类型
        int parent_ctype = node->parent_ctype();
        int parent_stype = node->parent_stype();
        
        // 父节点字符集
        char* parent_charset = node->parent_charset();
        
        // 父节点头字段
        const char* parent_ctype_header = node->parent_header_value("Content-Type");
        
        // 使用完父节点后需要释放
        delete parent;
    }
}
```

## 邮件保存

### 1. 保存完整邮件

```cpp
// 保存到文件
mail.save_as("/path/to/output.eml");

// 保存到流
std::ofstream ofs("output.eml");
mail.save_as(ofs);
```

### 2. 保存为 HTML 格式（用于浏览器显示）

```cpp
mail.save_mail(
    "/path/to/save",     // 保存路径
    "email.html",        // HTML 文件名
    true,                // enableDecode: 是否解码
    "utf-8",             // toCharset: 目标字符集
    0                    // off: 偏移量
);
```

这个函数会：
- 生成一个 HTML 文件显示邮件内容
- 自动提取并保存附件
- 自动提取并保存图片
- 在 HTML 中正确引用图片和附件

## 调试功能

```cpp
// 输出调试信息，将 MIME 结构保存到文件
mail.mime_debug("/path/to/debug", true);
```

## 字符集转换

模块支持自动字符集转换，在解析时可以指定目标字符集：

```cpp
// 转换为 UTF-8
acl::mime_body* body = mail.get_body_node(true, true, "utf-8");

// 转换为 GBK
acl::mime_body* body = mail.get_body_node(true, true, "gbk");

// 转换为 GB18030
acl::mime_body* body = mail.get_body_node(true, true, "gb18030");
```

## 常见邮件类型处理

### 1. 纯文本邮件

```cpp
// Content-Type: text/plain
acl::mime mail;
if (mail.parse("plain.eml")) {
    acl::mime_body* body = mail.get_plain_body();
    if (body) {
        body->save_body(std::cout);
    }
}
```

### 2. HTML 邮件

```cpp
// Content-Type: text/html
acl::mime mail;
if (mail.parse("html.eml")) {
    acl::mime_body* body = mail.get_html_body();
    if (body) {
        body->save_body("email.html");
    }
}
```

### 3. 多部分邮件（纯文本 + HTML）

```cpp
// Content-Type: multipart/alternative
acl::mime mail;
if (mail.parse("multipart.eml")) {
    // 优先获取 HTML 版本
    acl::mime_body* body = mail.get_body_node(true);
    if (body) {
        if (body->html_stype()) {
            std::cout << "HTML 版本" << std::endl;
        } else {
            std::cout << "纯文本版本" << std::endl;
        }
        body->save_body(std::cout);
    }
}
```

### 4. 带附件邮件

```cpp
// Content-Type: multipart/mixed
acl::mime mail;
if (mail.parse("with_attachments.eml")) {
    // 获取正文
    acl::mime_body* body = mail.get_body_node(true);
    
    // 获取附件
    const std::list<acl::mime_attach*>& attachments = mail.get_attachments();
    std::cout << "附件数量: " << attachments.size() << std::endl;
}
```

### 5. 带内嵌图片的 HTML 邮件

```cpp
// Content-Type: multipart/related
acl::mime mail;
if (mail.parse("html_with_images.eml")) {
    // 获取 HTML 正文
    acl::mime_body* body = mail.get_html_body();
    
    // 获取内嵌图片
    const std::list<acl::mime_image*>& images = mail.get_images();
    
    // 保存为完整的 HTML 页面
    mail.save_mail("/path/to/save", "email.html");
}
```

## 性能优化建议

1. **大邮件处理**：使用流式解析（`update_begin`/`update`/`update_end`）
2. **不需要解码**：设置 `enableDecode` 为 `false` 可以提高性能
3. **字符集转换**：如果不需要字符集转换，目标字符集设为 `NULL`
4. **重用对象**：处理多个邮件时，使用 `reset()` 重置对象而不是创建新对象
5. **按需获取**：只调用需要的获取函数，避免不必要的解析

## 错误处理

```cpp
acl::mime mail;

// 解析文件
if (!mail.parse("/path/to/email.eml")) {
    std::cerr << "无法打开或解析邮件文件" << std::endl;
    return;
}

// 获取正文（可能返回 NULL）
acl::mime_body* body = mail.get_body_node(true);
if (!body) {
    std::cerr << "邮件没有正文内容" << std::endl;
}

// 获取附件（返回空列表）
const std::list<acl::mime_attach*>& attachments = mail.get_attachments();
if (attachments.empty()) {
    std::cout << "邮件没有附件" << std::endl;
}
```

## 完整示例

参考 [完整解析示例](./examples/mime_parse_example.cpp)

