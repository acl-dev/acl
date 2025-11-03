# MIME 邮件构建详解

## 概述

MIME 构建模块提供了创建符合标准的 MIME 邮件的功能，支持设置邮件头、添加正文、附件等。

## 基本邮件构建

### 1. 创建简单的纯文本邮件

```cpp
#include "acl_cpp/mime/mime.hpp"

acl::mime mail;

// 设置邮件头
mail.set_from("sender@example.com")
    .set_subject("测试邮件")
    .add_to("recipient@example.com")
    .set_type("text", "plain");

// 保存邮件
mail.save_as("/path/to/email.eml");
```

### 2. 创建 HTML 邮件

```cpp
acl::mime mail;

mail.set_from("sender@example.com")
    .set_subject("HTML 测试邮件")
    .add_to("recipient@example.com")
    .set_type("text", "html");

// 保存邮件
mail.save_as("/path/to/html_email.eml");
```

## 设置邮件头

### 发件人信息

```cpp
acl::mime mail;

// 设置发件人（From）
mail.set_from("sender@example.com");

// 或使用带名称的格式
mail.set_from("张三 <zhangsan@example.com>");

// 设置发送者（Sender）
mail.set_sender("sender@example.com");

// 设置回复地址（Reply-To）
mail.set_replyto("reply@example.com");

// 设置返回路径（Return-Path）
mail.set_returnpath("return@example.com");
```

### 收件人信息

```cpp
// 添加收件人（To）
mail.add_to("recipient1@example.com");
mail.add_to("recipient2@example.com");
mail.add_to("李四 <lisi@example.com>");

// 添加抄送（CC）
mail.add_cc("cc1@example.com");
mail.add_cc("cc2@example.com");

// 添加密送（BCC）
mail.add_bcc("bcc1@example.com");
mail.add_bcc("bcc2@example.com");

// 添加到收件人列表（包含 To、CC、BCC）
mail.add_rcpt("rcpt@example.com");
```

### 主题

```cpp
// 设置邮件主题
mail.set_subject("这是邮件主题");

// 如果主题包含非ASCII字符，会自动进行RFC2047编码
mail.set_subject("测试邮件：中文主题");
```

### 自定义头字段

```cpp
// 添加自定义头字段
mail.add_header("X-Mailer", "ACL MIME Library");
mail.add_header("X-Priority", "1");
mail.add_header("Message-ID", "<123456@example.com>");

// 添加日期头
time_t now = time(NULL);
char date_buf[128];
acl::rfc822 rfc;
rfc.mkdate_cst(now, date_buf, sizeof(date_buf));
mail.add_header("Date", date_buf);
```

### Content-Type

```cpp
// 设置 Content-Type
mail.set_type("text", "plain");        // text/plain
mail.set_type("text", "html");         // text/html
mail.set_type("multipart", "mixed");   // multipart/mixed
mail.set_type("multipart", "alternative"); // multipart/alternative

// 设置边界字符串（用于 multipart 类型）
mail.set_boundary("----=_Part_123456_789012345.1234567890123");
```

## 链式调用

所有设置邮件头的方法都返回 `mime&` 引用，支持链式调用：

```cpp
acl::mime mail;

mail.set_from("sender@example.com")
    .set_subject("测试邮件")
    .add_to("recipient1@example.com")
    .add_to("recipient2@example.com")
    .add_cc("cc@example.com")
    .set_type("text", "plain")
    .add_header("X-Mailer", "ACL");

// 保存
mail.save_as("email.eml");
```

## 完整的邮件头示例

```cpp
#include "acl_cpp/mime/mime.hpp"
#include "acl_cpp/mime/rfc822.hpp"

acl::mime mail;

// 发件人信息
mail.set_from("张三 <zhangsan@example.com>")
    .set_sender("zhangsan@example.com")
    .set_replyto("reply@example.com")
    .set_returnpath("<zhangsan@example.com>");

// 收件人信息
mail.add_to("李四 <lisi@example.com>")
    .add_to("王五 <wangwu@example.com>")
    .add_cc("抄送用户 <cc@example.com>")
    .add_bcc("密送用户 <bcc@example.com>");

// 主题
mail.set_subject("这是一封测试邮件");

// 日期
acl::rfc822 rfc;
char date_buf[128];
time_t now = time(NULL);
rfc.mkdate_cst(now, date_buf, sizeof(date_buf));
mail.add_header("Date", date_buf);

// 其他头字段
mail.add_header("Message-ID", "<123456789@example.com>")
    .add_header("X-Mailer", "ACL MIME Library 1.0")
    .add_header("X-Priority", "3")
    .add_header("MIME-Version", "1.0");

// Content-Type
mail.set_type("text", "plain");

// 保存
mail.save_as("complete_email.eml");
```

## 构建带正文的邮件

注意：当前 `mime` 类主要用于解析邮件，构建复杂的多部分邮件（包含正文、附件等）需要手动构建 MIME 结构或使用其他工具。

### 手动构建邮件内容

```cpp
#include <fstream>

// 创建邮件头
acl::mime mail;
mail.set_from("sender@example.com")
    .set_to("recipient@example.com")
    .set_subject("Test")
    .set_type("text", "plain");

// 获取邮件头
acl::string header_buf;
const acl::mime_head& header = mail.primary_header();
header.build_head(header_buf, false);

// 写入文件
std::ofstream ofs("email.eml");
ofs << header_buf.c_str();
ofs << "\r\n";  // 头和正文之间的空行
ofs << "这是邮件正文内容。\r\n";
ofs << "第二行内容。\r\n";
ofs.close();
```

## 使用 mime_head 类

`mime_head` 类专门用于处理邮件头：

```cpp
#include "acl_cpp/mime/mime_head.hpp"

acl::mime_head header;

// 设置头字段
header.set_from("sender@example.com")
      .set_subject("测试")
      .add_to("recipient@example.com")
      .set_type("text", "plain")
      .add_header("X-Custom", "value");

// 构建头部字符串
acl::string header_buf;
header.build_head(header_buf, false);

std::cout << header_buf.c_str() << std::endl;

// 重置（重新使用）
header.reset();
```

## 读取已设置的头字段

```cpp
acl::mime mail;

// 设置一些头字段
mail.set_from("sender@example.com")
    .set_subject("Test Subject")
    .add_to("recipient@example.com");

// 读取头字段
std::cout << "From: " << mail.from() << std::endl;
std::cout << "Subject: " << mail.subject() << std::endl;

// 读取收件人列表
const std::list<char*>& to_list = mail.to_list();
for (auto* addr : to_list) {
    std::cout << "To: " << addr << std::endl;
}

// 读取 Content-Type
std::cout << "Content-Type: " << mail.get_ctype() 
          << "/" << mail.get_stype() << std::endl;

// 读取自定义头字段
const char* x_mailer = mail.header_value("X-Mailer");
if (x_mailer) {
    std::cout << "X-Mailer: " << x_mailer << std::endl;
}
```

## 获取邮件头对象

```cpp
acl::mime mail;

// 设置头字段
mail.set_from("sender@example.com");

// 获取邮件头对象
const acl::mime_head& header = mail.primary_header();

// 使用邮件头对象的方法
const acl::string& from = header.from();
const acl::string& boundary = header.get_boundary();
```

## 更新模式

当需要在流式处理中构建邮件时，可以使用更新模式：

```cpp
acl::mime mail;

// 开始更新模式
mail.update_begin(NULL);

// 标记主邮件头已完成
mail.primary_head_finish();

// 检查主邮件头是否完成
if (mail.primary_head_ok()) {
    std::cout << "主邮件头已设置完成" << std::endl;
}
```

## 保存邮件

### 1. 保存到文件

```cpp
// 保存到文件
if (mail.save_as("email.eml")) {
    std::cout << "保存成功" << std::endl;
} else {
    std::cerr << "保存失败" << std::endl;
}
```

### 2. 保存到流

```cpp
#include <fstream>

std::ofstream ofs("email.eml");
if (mail.save_as(ofs)) {
    std::cout << "保存成功" << std::endl;
}
ofs.close();

// 或输出到标准输出
mail.save_as(std::cout);
```

## 字符集和编码

### RFC2047 编码（邮件头）

对于包含非ASCII字符的邮件头字段（如主题、发件人名称），需要使用 RFC2047 编码：

```cpp
#include "acl_cpp/mime/rfc2047.hpp"

// 编码邮件主题
acl::string encoded_subject;
acl::rfc2047::encode(
    "测试邮件主题",
    strlen("测试邮件主题"),
    &encoded_subject,
    "utf-8",      // 字符集
    'B',          // 编码方式：B=Base64, Q=Quoted-Printable
    true          // 添加 CRLF
);

mail.set_subject(encoded_subject.c_str());

// 编码发件人名称
acl::string encoded_name;
acl::rfc2047::encode(
    "张三",
    strlen("张三"),
    &encoded_name,
    "utf-8",
    'B',
    true
);

// 构建完整的 From 字段
acl::string from_field;
from_field.format("%s <zhangsan@example.com>", encoded_name.c_str());
mail.set_from(from_field.c_str());
```

### 正文编码

邮件正文通常需要进行 Content-Transfer-Encoding：

```cpp
#include "acl_cpp/mime/mime_base64.hpp"
#include "acl_cpp/mime/mime_quoted_printable.hpp"

// Base64 编码
acl::string body_content = "邮件正文内容...";
acl::string encoded_body;
acl::mime_base64::encode(
    body_content.c_str(),
    body_content.length(),
    &encoded_body
);

// Quoted-Printable 编码
acl::string qp_encoded;
acl::mime_quoted_printable::encode(
    body_content.c_str(),
    body_content.length(),
    &qp_encoded
);
```

## 实际应用示例

### 示例 1：发送纯文本邮件

```cpp
#include "acl_cpp/mime/mime.hpp"
#include "acl_cpp/mime/rfc822.hpp"
#include <fstream>

bool create_plain_text_email(const char* output_file) {
    acl::mime mail;
    
    // 设置邮件头
    mail.set_from("sender@example.com")
        .set_subject("Plain Text Email")
        .add_to("recipient@example.com")
        .set_type("text", "plain");
    
    // 添加日期
    acl::rfc822 rfc;
    char date_buf[128];
    rfc.mkdate_cst(time(NULL), date_buf, sizeof(date_buf));
    mail.add_header("Date", date_buf);
    
    // 添加其他头字段
    mail.add_header("MIME-Version", "1.0")
        .add_header("Content-Transfer-Encoding", "8bit");
    
    // 构建邮件头
    acl::string header_buf;
    mail.primary_header().build_head(header_buf, false);
    
    // 写入文件
    std::ofstream ofs(output_file);
    if (!ofs) {
        return false;
    }
    
    ofs << header_buf.c_str();
    ofs << "\r\n";
    ofs << "This is the email body content.\r\n";
    ofs << "Line 2 of the email.\r\n";
    ofs << "Line 3 of the email.\r\n";
    ofs.close();
    
    return true;
}
```

### 示例 2：发送带编码主题的邮件

```cpp
#include "acl_cpp/mime/mime.hpp"
#include "acl_cpp/mime/rfc2047.hpp"

bool create_email_with_encoded_subject(const char* output_file) {
    // 编码主题
    const char* subject = "测试邮件：包含中文的主题";
    acl::string encoded_subject;
    acl::rfc2047::encode(
        subject, strlen(subject),
        &encoded_subject,
        "utf-8", 'B', true
    );
    
    // 创建邮件
    acl::mime mail;
    mail.set_from("sender@example.com")
        .set_subject(encoded_subject.c_str())
        .add_to("recipient@example.com")
        .set_type("text", "plain");
    
    // 保存
    return mail.save_as(output_file);
}
```

### 示例 3：批量发送邮件

```cpp
#include "acl_cpp/mime/mime.hpp"
#include <vector>

bool send_batch_emails(const std::vector<std::string>& recipients) {
    acl::mime mail;
    
    // 设置公共的邮件头字段
    mail.set_from("newsletter@example.com")
        .set_subject("Newsletter")
        .set_type("text", "html");
    
    // 为每个收件人创建邮件
    for (const auto& recipient : recipients) {
        // 重置收件人列表
        mail.reset();
        mail.set_from("newsletter@example.com")
            .set_subject("Newsletter")
            .add_to(recipient.c_str())
            .set_type("text", "html");
        
        // 保存邮件
        std::string filename = recipient + ".eml";
        if (!mail.save_as(filename.c_str())) {
            std::cerr << "Failed to create email for " 
                      << recipient << std::endl;
            return false;
        }
    }
    
    return true;
}
```

## 最佳实践

1. **使用链式调用**：提高代码可读性和简洁性
2. **设置必要的头字段**：From、To、Subject、Date、MIME-Version
3. **字符集处理**：非ASCII字符使用适当的编码（RFC2047、Base64、QP）
4. **MIME 版本**：添加 `MIME-Version: 1.0` 头字段
5. **Content-Type**：明确指定内容类型和字符集
6. **Date 字段**：使用 RFC822 标准的日期格式
7. **Message-ID**：为每封邮件生成唯一的 Message-ID

## 注意事项

1. 当前 `mime` 类主要侧重于邮件解析，构建复杂的多部分邮件（带附件、内嵌图片等）需要手动构建 MIME 结构
2. 邮件头字段名不区分大小写，但值可能区分
3. 邮件行应以 `\r\n` 结尾（CRLF）
4. 头和正文之间需要一个空行（`\r\n\r\n`）
5. 长行应该进行折叠（RFC 5322）
6. 非ASCII字符必须进行适当编码

## 扩展阅读

- [MIME 解析详解](./mime_parse.md)
- [MIME 编码详解](./mime_encode.md)
- [RFC 标准实现](./mime_rfc.md)

