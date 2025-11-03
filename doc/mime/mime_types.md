# MIME 类型和常量定义

## 概述

`mime_define.hpp` 定义了 MIME 模块使用的各种类型和编码常量，这些常量用于标识 MIME 内容类型、子类型和编码方式。

## 内容类型（Content Type）

### 主类型常量

```cpp
#define MIME_CTYPE_OTHER        0   // 其他/未知类型
#define MIME_CTYPE_TEXT         1   // 文本类型
#define MIME_CTYPE_MESSAGE      2   // 消息类型
#define MIME_CTYPE_MULTIPART    3   // 多部分类型
#define MIME_CTYPE_IMAGE        4   // 图片类型
#define MIME_CTYPE_APPLICATION  5   // 应用程序类型
```

### 主类型说明

#### MIME_CTYPE_TEXT (1)
文本内容类型。

**对应的 Content-Type**：
- `text/plain` - 纯文本
- `text/html` - HTML 文档
- `text/xml` - XML 文档
- `text/css` - CSS 样式表
- `text/javascript` - JavaScript 代码

**特点**：
- 通常是人类可读的文本
- 支持字符集参数（charset）
- 可以使用 Quoted-Printable 或 Base64 编码

**示例**：
```cpp
if (node->get_ctype() == MIME_CTYPE_TEXT) {
    std::cout << "这是文本内容" << std::endl;
}
```

#### MIME_CTYPE_MESSAGE (2)
封装的消息类型。

**对应的 Content-Type**：
- `message/rfc822` - 完整的邮件消息
- `message/partial` - 部分消息
- `message/external-body` - 外部正文引用

**特点**：
- 包含完整的邮件消息（含邮件头）
- 常用于转发邮件
- 可以递归包含其他 MIME 结构

**示例**：
```cpp
if (node->get_ctype() == MIME_CTYPE_MESSAGE) {
    std::cout << "这是一个封装的邮件消息" << std::endl;
}
```

#### MIME_CTYPE_MULTIPART (3)
多部分内容类型。

**对应的 Content-Type**：
- `multipart/mixed` - 混合内容（正文+附件）
- `multipart/alternative` - 可选内容（纯文本或HTML）
- `multipart/related` - 相关内容（HTML+图片）
- `multipart/digest` - 消息摘要
- `multipart/parallel` - 并行显示
- `multipart/report` - 报告

**特点**：
- 包含多个子部分（parts）
- 使用 boundary 分隔各部分
- 每个子部分可以有不同的类型

**结构示例**：
```
Content-Type: multipart/mixed; boundary="----=_Part_123"

------=_Part_123
Content-Type: text/plain

邮件正文
------=_Part_123
Content-Type: application/pdf

附件内容
------=_Part_123--
```

**代码示例**：
```cpp
if (node->get_ctype() == MIME_CTYPE_MULTIPART) {
    // 这是多部分邮件
    if (node->get_stype() == MIME_STYPE_MIXED) {
        std::cout << "包含正文和附件" << std::endl;
    }
}
```

#### MIME_CTYPE_IMAGE (4)
图片类型。

**对应的 Content-Type**：
- `image/jpeg` - JPEG 图片
- `image/gif` - GIF 图片
- `image/png` - PNG 图片
- `image/bmp` - BMP 图片
- `image/svg+xml` - SVG 矢量图

**特点**：
- 二进制数据
- 通常使用 Base64 编码
- 可以作为附件或内嵌图片

**示例**：
```cpp
if (node->get_ctype() == MIME_CTYPE_IMAGE) {
    const char* name = node->get_name();
    std::cout << "图片: " << (name ? name : "无名称") << std::endl;
}
```

#### MIME_CTYPE_APPLICATION (5)
应用程序数据类型。

**对应的 Content-Type**：
- `application/octet-stream` - 二进制数据
- `application/pdf` - PDF 文档
- `application/zip` - ZIP 压缩文件
- `application/msword` - Word 文档
- `application/json` - JSON 数据
- `application/xml` - XML 数据

**特点**：
- 通常是二进制数据
- 需要特定应用程序打开
- 常作为附件

**示例**：
```cpp
if (node->get_ctype() == MIME_CTYPE_APPLICATION) {
    std::cout << "应用程序数据/附件" << std::endl;
}
```

#### MIME_CTYPE_OTHER (0)
其他或未知类型。

**特点**：
- 不属于上述类别的类型
- 未识别的 Content-Type
- 需要特殊处理

## 内容子类型（Content Subtype）

### 子类型常量

```cpp
#define MIME_STYPE_OTHER        6   // 其他/未知子类型
#define MIME_STYPE_PLAIN        7   // text/plain
#define MIME_STYPE_HTML         8   // text/html
#define MIME_STYPE_RFC822       9   // message/rfc822
#define MIME_STYPE_PARTIAL      10  // message/partial
#define MIME_STYPE_EXTERN_BODY  11  // message/external-body
#define MIME_STYPE_JPEG         12  // image/jpeg
#define MIME_STYPE_GIF          13  // image/gif
#define MIME_STYPE_BMP          14  // image/bmp
#define MIME_STYPE_PNG          15  // image/png
#define MIME_STYPE_OCTET_STREAM 16  // application/octet-stream
#define MIME_STYPE_MIXED        17  // multipart/mixed
#define MIME_STYPE_ALTERNATIVE  18  // multipart/alternative
#define MIME_STYPE_RELATED      19  // multipart/related
```

### 子类型说明

#### 文本子类型

**MIME_STYPE_PLAIN (7)** - `text/plain`
- 纯文本，无格式
- 最简单的文本格式
- 通常指定字符集

**MIME_STYPE_HTML (8)** - `text/html`
- HTML 格式的文本
- 支持富文本和超链接
- 可以引用内嵌图片

```cpp
if (node->get_stype() == MIME_STYPE_HTML) {
    std::cout << "HTML 正文" << std::endl;
}
```

#### 消息子类型

**MIME_STYPE_RFC822 (9)** - `message/rfc822`
- 完整的 RFC 822 邮件消息
- 用于转发邮件

**MIME_STYPE_PARTIAL (10)** - `message/partial`
- 分段的邮件消息
- 用于发送大邮件

**MIME_STYPE_EXTERN_BODY (11)** - `message/external-body`
- 外部正文引用
- 正文存储在外部位置

#### 图片子类型

**MIME_STYPE_JPEG (12)** - `image/jpeg`
- JPEG 格式图片
- 有损压缩
- 适合照片

**MIME_STYPE_GIF (13)** - `image/gif`
- GIF 格式图片
- 支持动画
- 256 色限制

**MIME_STYPE_BMP (14)** - `image/bmp`
- BMP 格式图片
- 无压缩
- 文件较大

**MIME_STYPE_PNG (15)** - `image/png`
- PNG 格式图片
- 无损压缩
- 支持透明

```cpp
switch (node->get_stype()) {
case MIME_STYPE_JPEG:
    std::cout << "JPEG 图片" << std::endl;
    break;
case MIME_STYPE_PNG:
    std::cout << "PNG 图片" << std::endl;
    break;
}
```

#### 应用程序子类型

**MIME_STYPE_OCTET_STREAM (16)** - `application/octet-stream`
- 通用二进制数据
- 未指定具体类型的文件
- 作为附件下载

#### 多部分子类型

**MIME_STYPE_MIXED (17)** - `multipart/mixed`
- 混合内容
- 包含正文和附件
- 最常见的多部分类型

**MIME_STYPE_ALTERNATIVE (18)** - `multipart/alternative`
- 可选内容
- 同一内容的不同表现形式
- 常用于纯文本+HTML

**MIME_STYPE_RELATED (19)** - `multipart/related`
- 相关内容
- HTML 和其引用的图片
- 使用 Content-ID 关联

```cpp
if (node->get_stype() == MIME_STYPE_ALTERNATIVE) {
    std::cout << "包含多种格式的正文" << std::endl;
}
```

## 编码方式（Transfer Encoding）

### 编码常量

```cpp
#define MIME_ENC_OTHER          20  // 其他/未知编码
#define MIME_ENC_QP             21  // Quoted-Printable
#define MIME_ENC_BASE64         22  // Base64
#define MIME_ENC_7BIT           23  // 7-bit ASCII
#define MIME_ENC_8BIT           24  // 8-bit
#define MIME_ENC_BINARY         25  // Binary
#define MIME_ENC_UUCODE         26  // UUEncode
#define MIME_ENC_XXCODE         27  // XXEncode
```

### 编码说明

#### MIME_ENC_7BIT (23)
- 7 位 ASCII 编码
- 所有字符 < 128
- 不需要转换编码
- 最简单的编码方式

**Content-Transfer-Encoding**: `7bit`

```cpp
if (node->get_encoding() == MIME_ENC_7BIT) {
    std::cout << "7-bit 编码，无需解码" << std::endl;
}
```

#### MIME_ENC_8BIT (24)
- 8 位编码
- 字符可以 >= 128
- 不进行转换编码
- 依赖传输通道支持 8-bit

**Content-Transfer-Encoding**: `8bit`

#### MIME_ENC_BINARY (25)
- 二进制编码
- 不进行任何转换
- 可能包含任意字节
- 需要传输通道支持二进制

**Content-Transfer-Encoding**: `binary`

#### MIME_ENC_BASE64 (22)
- Base64 编码
- 最常用的二进制编码
- 将 3 字节编码为 4 个 ASCII 字符
- 编码效率 75%

**Content-Transfer-Encoding**: `base64`

**适用场景**：
- 图片附件
- 其他二进制附件
- 需要确保数据完整性

```cpp
if (node->get_encoding() == MIME_ENC_BASE64) {
    std::cout << "需要 Base64 解码" << std::endl;
}
```

#### MIME_ENC_QP (21)
- Quoted-Printable 编码
- 适合文本内容
- ASCII 字符不编码
- 非 ASCII 字符编码为 =XX

**Content-Transfer-Encoding**: `quoted-printable`

**适用场景**：
- 文本邮件正文
- 包含少量非 ASCII 字符的文本

```cpp
if (node->get_encoding() == MIME_ENC_QP) {
    std::cout << "需要 Quoted-Printable 解码" << std::endl;
}
```

#### MIME_ENC_UUCODE (26)
- UUEncode 编码
- Unix 传统编码方式
- 现在较少使用

**Content-Transfer-Encoding**: `x-uuencode` 或 `uuencode`

#### MIME_ENC_XXCODE (27)
- XXEncode 编码
- UUEncode 的变种
- 现在较少使用

**Content-Transfer-Encoding**: `x-xxencode` 或 `xxencode`

#### MIME_ENC_OTHER (20)
- 其他编码方式
- 未知或不支持的编码

## 常量范围

```cpp
// 主类型范围
#define MIME_CTYPE_MIN          MIME_CTYPE_OTHER        // 0
#define MIME_CTYPE_MAX          MIME_CTYPE_APPLICATION  // 5

// 子类型范围
#define MIME_STYPE_MIN          MIME_STYPE_OTHER        // 6
#define MIME_STYPE_MAX          MIME_STYPE_RELATED      // 19

// 编码方式范围
#define MIME_ENC_MIN            MIME_ENC_OTHER          // 20
#define MIME_ENC_MAX            MIME_ENC_XXCODE         // 27

// 总范围
#define MIME_MIN                MIME_CTYPE_OTHER        // 0
#define MIME_MAX                MIME_ENC_MAX            // 27
```

## 实际应用示例

### 示例 1：判断内容类型

```cpp
void analyze_mime_node(acl::mime_node* node) {
    int ctype = node->get_ctype();
    int stype = node->get_stype();
    int encoding = node->get_encoding();
    
    // 判断主类型
    switch (ctype) {
    case MIME_CTYPE_TEXT:
        if (stype == MIME_STYPE_HTML) {
            std::cout << "HTML 正文" << std::endl;
        } else if (stype == MIME_STYPE_PLAIN) {
            std::cout << "纯文本正文" << std::endl;
        }
        break;
        
    case MIME_CTYPE_IMAGE:
        std::cout << "图片: ";
        switch (stype) {
        case MIME_STYPE_JPEG:
            std::cout << "JPEG";
            break;
        case MIME_STYPE_PNG:
            std::cout << "PNG";
            break;
        case MIME_STYPE_GIF:
            std::cout << "GIF";
            break;
        }
        std::cout << std::endl;
        break;
        
    case MIME_CTYPE_APPLICATION:
        std::cout << "应用程序数据/附件" << std::endl;
        break;
        
    case MIME_CTYPE_MULTIPART:
        std::cout << "多部分内容" << std::endl;
        break;
    }
    
    // 判断编码方式
    if (encoding == MIME_ENC_BASE64) {
        std::cout << "使用 Base64 编码" << std::endl;
    } else if (encoding == MIME_ENC_QP) {
        std::cout << "使用 Quoted-Printable 编码" << std::endl;
    }
}
```

### 示例 2：创建编码器

```cpp
#include "acl_cpp/mime/mime_code.hpp"

acl::mime_code* create_encoder(int encoding_type) {
    acl::mime_code* encoder = acl::mime_code::create(encoding_type, true);
    
    if (!encoder) {
        std::cerr << "不支持的编码类型: " << encoding_type << std::endl;
        return NULL;
    }
    
    return encoder;
}

// 使用
acl::mime_code* base64_encoder = create_encoder(MIME_ENC_BASE64);
acl::mime_code* qp_encoder = create_encoder(MIME_ENC_QP);
```

### 示例 3：统计邮件内容类型

```cpp
void count_mime_types(const std::list<acl::mime_node*>& nodes) {
    std::map<int, int> type_counts;
    
    for (auto* node : nodes) {
        int ctype = node->get_ctype();
        type_counts[ctype]++;
    }
    
    std::cout << "内容类型统计:" << std::endl;
    for (auto& pair : type_counts) {
        std::cout << "  类型 " << pair.first << ": " 
                  << pair.second << " 个" << std::endl;
    }
}
```

### 示例 4：根据类型处理节点

```cpp
void process_by_type(acl::mime_node* node) {
    int ctype = node->get_ctype();
    int stype = node->get_stype();
    
    if (ctype == MIME_CTYPE_TEXT && stype == MIME_STYPE_HTML) {
        // 处理 HTML 正文
        save_html(node);
    }
    else if (ctype == MIME_CTYPE_IMAGE) {
        // 处理图片
        save_image(node);
    }
    else if (ctype == MIME_CTYPE_APPLICATION) {
        // 处理附件
        save_attachment(node);
    }
    else if (ctype == MIME_CTYPE_MULTIPART) {
        // 递归处理子部分
        // （实际需要通过 mime 类获取子节点）
        std::cout << "多部分内容，需要递归处理" << std::endl;
    }
}
```

## Content-Type 映射表

| 常量 | Content-Type | 说明 |
|------|-------------|------|
| MIME_CTYPE_TEXT + MIME_STYPE_PLAIN | text/plain | 纯文本 |
| MIME_CTYPE_TEXT + MIME_STYPE_HTML | text/html | HTML 文档 |
| MIME_CTYPE_MESSAGE + MIME_STYPE_RFC822 | message/rfc822 | RFC 822 消息 |
| MIME_CTYPE_MULTIPART + MIME_STYPE_MIXED | multipart/mixed | 混合内容 |
| MIME_CTYPE_MULTIPART + MIME_STYPE_ALTERNATIVE | multipart/alternative | 可选内容 |
| MIME_CTYPE_MULTIPART + MIME_STYPE_RELATED | multipart/related | 相关内容 |
| MIME_CTYPE_IMAGE + MIME_STYPE_JPEG | image/jpeg | JPEG 图片 |
| MIME_CTYPE_IMAGE + MIME_STYPE_PNG | image/png | PNG 图片 |
| MIME_CTYPE_IMAGE + MIME_STYPE_GIF | image/gif | GIF 图片 |
| MIME_CTYPE_APPLICATION + MIME_STYPE_OCTET_STREAM | application/octet-stream | 二进制数据 |

## 编码方式映射表

| 常量 | Content-Transfer-Encoding | 说明 |
|------|-------------------------|------|
| MIME_ENC_7BIT | 7bit | 7-bit ASCII |
| MIME_ENC_8BIT | 8bit | 8-bit |
| MIME_ENC_BINARY | binary | 二进制 |
| MIME_ENC_BASE64 | base64 | Base64 编码 |
| MIME_ENC_QP | quoted-printable | Quoted-Printable 编码 |
| MIME_ENC_UUCODE | x-uuencode | UUEncode 编码 |
| MIME_ENC_XXCODE | x-xxencode | XXEncode 编码 |

## 注意事项

1. **常量值**：这些常量的具体数值是内部实现细节，应该使用宏名称而不是直接使用数值
2. **扩展性**：未来可能添加新的类型，应该处理 `MIME_XXX_OTHER` 情况
3. **组合使用**：主类型和子类型需要配合使用才能完整描述内容类型
4. **字符串转换**：可以使用 `get_ctype_s()` 和 `get_stype_s()` 获取字符串形式的类型名称

## 相关函数

```cpp
// 在 mime_node 类中
int get_ctype() const;              // 获取主类型
int get_stype() const;              // 获取子类型
int get_encoding() const;           // 获取编码方式
const char* get_ctype_s() const;    // 获取主类型字符串
const char* get_stype_s() const;    // 获取子类型字符串
```

## 扩展阅读

- [MIME 解析详解](./mime_parse.md)
- [MIME 编码详解](./mime_encode.md)
- [API 参考](./mime_api.md)

