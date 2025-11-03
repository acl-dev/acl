# ACL MIME 模块文档

## 概述

ACL MIME 模块是一个功能完整的 MIME（多用途互联网邮件扩展）处理库，提供了邮件解析、构建、编码/解码等全套功能。该模块符合 RFC 822、RFC 2047 等相关标准，可用于电子邮件客户端、邮件服务器、邮件网关等应用场景。

## 目录结构

```
lib_acl_cpp/include/acl_cpp/mime/
├── mime.hpp                    # MIME 主类，提供邮件解析和构建功能
├── mime_define.hpp             # MIME 类型和编码常量定义
├── mime_node.hpp              # MIME 节点基类
├── mime_head.hpp              # MIME 邮件头处理
├── mime_body.hpp              # MIME 邮件正文处理
├── mime_attach.hpp            # MIME 附件处理
├── mime_image.hpp             # MIME 图片处理
├── mime_code.hpp              # MIME 编码基类
├── mime_base64.hpp            # Base64 编码/解码
├── mime_quoted_printable.hpp  # Quoted-Printable 编码/解码
├── mime_uucode.hpp            # UUEncode 编码/解码
├── mime_xxcode.hpp            # XXEncode 编码/解码
├── rfc2047.hpp                # RFC2047 邮件头编码处理
└── rfc822.hpp                 # RFC822 邮件地址和日期处理
```

## 主要功能

### 1. 邮件解析（Email Parsing）
- 解析完整的 MIME 邮件文件
- 流式解析支持（适合大邮件）
- 自动识别邮件结构（multipart/alternative、multipart/mixed 等）
- 提取邮件头信息（发件人、收件人、主题等）
- 提取邮件正文（HTML/纯文本）
- 提取附件和图片

### 2. 邮件构建（Email Building）
- 创建符合 MIME 标准的邮件
- 设置邮件头信息
- 添加邮件正文
- 添加附件
- 添加内嵌图片

### 3. 编码/解码（Encoding/Decoding）
- Base64 编码/解码
- Quoted-Printable 编码/解码
- UUEncode 编码/解码
- XXEncode 编码/解码
- RFC2047 邮件头编码/解码
- 字符集转换支持

### 4. RFC 标准支持
- RFC 822：邮件地址格式和日期格式
- RFC 2047：MIME 邮件头编码
- 其他 MIME 相关标准

## 核心类说明

### mime 类
主要的 MIME 处理类，提供邮件解析和构建的高层接口。

**主要功能：**
- 解析邮件文件或数据流
- 获取邮件头信息
- 获取邮件正文（HTML/纯文本）
- 获取附件列表
- 获取图片列表
- 构建邮件并保存

### mime_node 类
MIME 节点基类，表示 MIME 邮件中的一个部分（part）。

**派生类：**
- `mime_body`：邮件正文节点
- `mime_attach`：附件节点
- `mime_image`：图片节点

### mime_head 类
邮件头处理类，负责邮件头字段的解析和构建。

### 编码类
- `mime_code`：编码基类
- `mime_base64`：Base64 编码
- `mime_quoted_printable`：Quoted-Printable 编码
- `mime_uucode`：UUEncode 编码
- `mime_xxcode`：XXEncode 编码

### RFC 标准类
- `rfc2047`：RFC2047 邮件头编码处理
- `rfc822`：RFC822 邮件地址和日期解析

## 使用场景

1. **邮件客户端开发**
   - 解析接收的邮件
   - 显示邮件内容
   - 下载附件

2. **邮件服务器开发**
   - 邮件存储和检索
   - 邮件转发
   - 邮件过滤

3. **邮件网关开发**
   - 邮件格式转换
   - 邮件内容分析
   - 病毒扫描

4. **Web 邮件系统**
   - 邮件列表显示
   - 邮件详情查看
   - 附件上传下载

## 快速开始

### 解析邮件示例

```cpp
#include "acl_cpp/mime/mime.hpp"

// 解析邮件文件
acl::mime mail;
if (mail.parse("/path/to/email.eml")) {
    // 获取邮件主题
    const acl::string& subject = mail.subject();
    
    // 获取发件人
    const acl::string& from = mail.from();
    
    // 获取邮件正文
    acl::mime_body* body = mail.get_body_node(true);
    if (body) {
        body->save_body(std::cout);
    }
    
    // 获取附件列表
    const std::list<acl::mime_attach*>& attachments = mail.get_attachments();
    for (auto* attach : attachments) {
        const char* filename = attach->get_filename();
        attach->save("/path/to/save/" + std::string(filename));
    }
}
```

### 构建邮件示例

```cpp
#include "acl_cpp/mime/mime.hpp"

// 创建邮件
acl::mime mail;
mail.set_from("sender@example.com")
    .set_subject("测试邮件")
    .add_to("recipient@example.com")
    .set_type("text", "plain");

// 保存邮件
mail.save_as("/path/to/output.eml");
```

### Base64 编码示例

```cpp
#include "acl_cpp/mime/mime_base64.hpp"
#include "acl_cpp/stdlib/string.hpp"

acl::string input = "Hello World!";
acl::string output;

// 编码
acl::mime_base64::encode(input.c_str(), input.length(), &output);

// 解码
acl::string decoded;
acl::mime_base64::decode(output.c_str(), output.length(), &decoded);
```

## 详细文档

- [MIME 解析详解](./mime_parse.md) - 邮件解析功能详细说明
- [MIME 构建详解](./mime_build.md) - 邮件构建功能详细说明
- [MIME 编码详解](./mime_encode.md) - 编码/解码功能详细说明
- [RFC 标准实现](./mime_rfc.md) - RFC 标准相关功能说明
- [类型定义说明](./mime_types.md) - MIME 类型和常量定义
- [API 参考](./mime_api.md) - 完整的 API 参考文档

## 注意事项

1. **字符集处理**：模块支持多种字符集转换，默认目标字符集为 GB2312，可根据需要修改。

2. **内存管理**：某些返回的对象需要调用者负责释放内存，请注意文档中的说明。

3. **线程安全**：大部分类都继承自 `noncopyable`，不支持拷贝，在多线程环境中请为每个线程创建独立实例。

4. **编译选项**：如果定义了 `ACL_MIME_DISABLE` 宏，MIME 模块将被禁用。

## 相关资源

- [ACL 项目主页](https://github.com/acl-dev/acl)
- [RFC 822 标准](https://www.rfc-editor.org/rfc/rfc822)
- [RFC 2047 标准](https://www.rfc-editor.org/rfc/rfc2047)
- [MIME 标准系列](https://www.iana.org/assignments/media-types/media-types.xhtml)

## 版本历史

请参考 ACL 项目的版本发布说明。

## 联系方式

如有问题或建议，请访问 [ACL 项目主页](https://github.com/acl-dev/acl) 提交 Issue。

