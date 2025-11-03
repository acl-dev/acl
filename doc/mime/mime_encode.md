# MIME 编码/解码详解

## 概述

MIME 编码模块提供了多种编码和解码功能，用于处理邮件内容的传输编码。支持的编码方式包括：

- **Base64**：最常用的二进制编码方式
- **Quoted-Printable (QP)**：适合文本内容的编码方式
- **UUEncode**：Unix 传统的编码方式
- **XXEncode**：UUEncode 的变种
- **RFC2047**：专门用于邮件头的编码

## 编码类继承关系

```
mime_code (抽象基类)
    ├── mime_base64
    ├── mime_quoted_printable
    ├── mime_uucode
    └── mime_xxcode

rfc2047 (独立类，用于邮件头编码)
```

## Base64 编码

Base64 是最常用的 MIME 编码方式，将二进制数据编码为 ASCII 字符串。

### 静态方法（推荐用于简单场景）

```cpp
#include "acl_cpp/mime/mime_base64.hpp"
#include "acl_cpp/stdlib/string.hpp"

// 编码
const char* input = "Hello, World!";
acl::string encoded;
acl::mime_base64::encode(input, strlen(input), &encoded);
std::cout << "编码结果: " << encoded << std::endl;
// 输出: SGVsbG8sIFdvcmxkIQ==

// 解码
acl::string decoded;
acl::mime_base64::decode(encoded.c_str(), encoded.length(), &decoded);
std::cout << "解码结果: " << decoded << std::endl;
// 输出: Hello, World!
```

### 流式编码（适合大数据）

```cpp
acl::mime_base64 encoder(
    true,   // addCrlf: 每行末尾添加 \r\n
    false   // addInvalid: 解码时是否保留非法字符
);

acl::string output;

// 分块编码
encoder.encode_update("Hello", 5, &output);
encoder.encode_update(", ", 2, &output);
encoder.encode_update("World!", 6, &output);

// 完成编码（获取剩余数据）
encoder.encode_finish(&output);

std::cout << "编码结果: " << output << std::endl;
```

### 流式解码

```cpp
acl::mime_base64 decoder(false, false);

acl::string decoded;

// 分块解码
decoder.decode_update("SGVs", 4, &decoded);
decoder.decode_update("bG8s", 4, &decoded);
decoder.decode_update("IFdvcmxkIQ==", 12, &decoded);

// 完成解码
decoder.decode_finish(&decoded);

std::cout << "解码结果: " << decoded << std::endl;
```

### Base64 特点

- **编码效率**：编码后大小约为原始数据的 133%（4/3）
- **字符集**：使用 A-Z, a-z, 0-9, +, / 共 64 个字符
- **填充字符**：使用 = 作为填充字符
- **换行**：通常每 76 个字符换行（可选）
- **用途**：二进制附件、图片等

## Quoted-Printable 编码

Quoted-Printable 编码适合文本内容，特别是大部分为 ASCII 字符、少量非 ASCII 字符的文本。

### 静态方法

```cpp
#include "acl_cpp/mime/mime_quoted_printable.hpp"

// 编码
const char* text = "Hello=World\n中文测试";
acl::string encoded;
acl::mime_quoted_printable::encode(text, strlen(text), &encoded);
std::cout << "编码结果: " << encoded << std::endl;
// 非ASCII字符会被编码为 =XX 格式

// 解码
acl::string decoded;
acl::mime_quoted_printable::decode(encoded.c_str(), encoded.length(), &decoded);
std::cout << "解码结果: " << decoded << std::endl;
```

### 流式编码

```cpp
acl::mime_quoted_printable encoder(
    true,   // addCrlf: 添加 \r\n
    false   // addInvalid: 是否保留非法字符
);

acl::string output;

// 分块编码
encoder.encode_update("Line 1\n", 7, &output);
encoder.encode_update("Line 2\n", 7, &output);
encoder.encode_update("特殊字符", strlen("特殊字符"), &output);

// 完成编码
encoder.encode_finish(&output);

std::cout << output << std::endl;
```

### 流式解码

```cpp
acl::mime_quoted_printable decoder(false, true);

acl::string decoded;

// 分块解码
decoder.decode_update("Hello=3DWorld", 13, &decoded);
decoder.decode_finish(&decoded);

std::cout << decoded << std::endl;  // Hello=World
```

### Quoted-Printable 特点

- **编码规则**：
  - ASCII 可打印字符（33-126，除了=）：不编码
  - 空格和制表符：行尾需编码
  - 其他字符：编码为 =XX（XX 为十六进制）
  - 行长度：通常不超过 76 字符
  - 软换行：使用 = 作为行尾表示继续

- **优点**：
  - 主要为 ASCII 文本时，编码膨胀小
  - 可读性好（ASCII 部分不编码）

- **用途**：
  - 文本邮件正文
  - 包含少量非 ASCII 字符的文本

## UUEncode

UUEncode 是 Unix 传统的编码方式，现在较少使用。

### 静态方法

```cpp
#include "acl_cpp/mime/mime_uucode.hpp"

// 编码
const char* input = "Hello, World!";
acl::string encoded;
acl::mime_uucode::encode(input, strlen(input), &encoded);

// 解码
acl::string decoded;
acl::mime_uucode::decode(encoded.c_str(), encoded.length(), &decoded);
```

### 流式编码/解码

```cpp
acl::mime_uucode encoder(true, false);

acl::string output;
encoder.encode_update("Hello", 5, &output);
encoder.encode_finish(&output);
```

## XXEncode

XXEncode 是 UUEncode 的变种，使用不同的字符集。

### 静态方法

```cpp
#include "acl_cpp/mime/mime_xxcode.hpp"

// 编码
const char* input = "Hello, World!";
acl::string encoded;
acl::mime_xxcode::encode(input, strlen(input), &encoded);

// 解码
acl::string decoded;
acl::mime_xxcode::decode(encoded.c_str(), encoded.length(), &decoded);
```

## RFC2047 编码（邮件头编码）

RFC2047 专门用于编码邮件头中的非 ASCII 字符，格式为：`=?charset?encoding?encoded-text?=`

### 静态方法

```cpp
#include "acl_cpp/mime/rfc2047.hpp"

// 编码邮件主题
const char* subject = "测试邮件：包含中文";
acl::string encoded_subject;

acl::rfc2047::encode(
    subject,
    strlen(subject),
    &encoded_subject,
    "utf-8",    // 字符集
    'B',        // 编码方式：'B'=Base64, 'Q'=Quoted-Printable
    true        // 是否添加 CRLF
);

std::cout << "编码后: " << encoded_subject << std::endl;
// 输出类似: =?utf-8?B?5rWL6K+V6YKu5Lu2?=

// 解码
acl::string decoded_subject;
acl::rfc2047::decode(
    encoded_subject.c_str(),
    encoded_subject.length(),
    &decoded_subject,
    "utf-8",    // 目标字符集
    true,       // 是否去除空格
    true        // 是否保留非法字符
);

std::cout << "解码后: " << decoded_subject << std::endl;
```

### 流式编码

```cpp
acl::rfc2047 encoder(
    true,   // strip_sp: 去除空格
    true    // addCrlf: 添加 CRLF
);

acl::string output;

// 编码
encoder.encode_update(
    "中文主题",
    strlen("中文主题"),
    &output,
    "utf-8",    // 字符集
    'B'         // 编码方式
);

encoder.encode_finish(&output);
```

### 流式解码

```cpp
acl::rfc2047 decoder(true, true);

// 解码
decoder.decode_update("=?utf-8?B?5rWL6K+V?=", 24);
decoder.decode_update("=?utf-8?B?6YKu5Lu2?=", 24);

acl::string output;
decoder.decode_finish("utf-8", &output, true);

std::cout << output << std::endl;
```

### RFC2047 格式说明

**编码格式**：`=?charset?encoding?encoded-text?=`

- `charset`：字符集（如 utf-8, gb2312, iso-8859-1）
- `encoding`：编码方式
  - `B`：Base64
  - `Q`：Quoted-Printable（变种）
- `encoded-text`：编码后的文本

**示例**：
```
=?utf-8?B?5rWL6K+V6YKu5Lu2?=
=?gb2312?Q?=B2=E2=CA=D4=D3=CA=BC=FE?=
=?iso-8859-1?Q?Hello_World?=
```

### RFC2047 使用场景

RFC2047 主要用于编码以下邮件头字段：

- Subject（主题）
- From, To, Cc, Bcc（地址中的显示名称）
- Comments
- 其他包含非 ASCII 字符的头字段

**注意**：
- 邮件地址部分（xxx@xxx.xxx）不能使用 RFC2047 编码
- 只有显示名称部分可以编码

```cpp
// 正确：编码显示名称
"=?utf-8?B?5byg5LiJ?= <zhangsan@example.com>"

// 错误：不能编码邮件地址
"zhangsan@=?utf-8?B?ZXhhbXBsZQ==?=.com"
```

## 通用编码器（mime_code）

`mime_code` 是所有编码类的基类，提供了统一的接口。

### 创建编码器

```cpp
#include "acl_cpp/mime/mime_code.hpp"
#include "acl_cpp/mime/mime_define.hpp"

// 根据编码类型创建编码器
acl::mime_code* encoder = acl::mime_code::create(
    MIME_ENC_BASE64,    // 编码类型
    true                // 未找到时是否警告
);

if (encoder) {
    acl::string output;
    
    // 编码
    encoder->encode_update("Hello", 5, &output);
    encoder->encode_finish(&output);
    
    // 释放
    delete encoder;
}
```

### 支持的编码类型

```cpp
MIME_ENC_BASE64         // Base64
MIME_ENC_QP             // Quoted-Printable
MIME_ENC_UUCODE         // UUEncode
MIME_ENC_XXCODE         // XXEncode
MIME_ENC_7BIT           // 7-bit（不编码）
MIME_ENC_8BIT           // 8-bit（不编码）
MIME_ENC_BINARY         // Binary（不编码）
MIME_ENC_OTHER          // 其他（不编码）
```

### 使用 pipe_stream 接口

`mime_code` 继承自 `pipe_stream`，可以用作管道处理：

```cpp
acl::mime_base64 encoder(false, false);

// 设置为编码模式
encoder.set_status(true);

acl::string output;
size_t max_output = 1024;

// 推送数据
encoder.push_pop("Hello", 5, &output, max_output);
encoder.push_pop(" World", 6, &output, max_output);

// 结束
encoder.pop_end(&output, max_output);
```

## 编码选择指南

### Base64

**适用场景**：
- 二进制数据（图片、音频、视频、可执行文件等）
- 任何需要保证数据完整性的场景
- 邮件附件

**优点**：
- 简单可靠
- 所有数据统一处理
- 广泛支持

**缺点**：
- 数据膨胀 33%
- 文本不可读

### Quoted-Printable

**适用场景**：
- 主要为 ASCII 的文本
- 包含少量非 ASCII 字符的文本
- 需要保持一定可读性的内容

**优点**：
- ASCII 部分不编码，可读性好
- 大部分为 ASCII 时，膨胀率低

**缺点**：
- 二进制数据膨胀严重（最坏 3倍）
- 实现复杂

### RFC2047

**适用场景**：
- 邮件头字段中的非 ASCII 字符
- Subject, From, To 等字段

**优点**：
- 专门为邮件头设计
- 支持多种字符集
- 兼容性好

**缺点**：
- 只能用于邮件头
- 有长度限制

## 编码对比

| 编码方式 | 膨胀率 | 适用数据 | 可读性 | 常用程度 |
|---------|--------|---------|--------|---------|
| Base64 | 133% | 二进制/任意 | 差 | 高 |
| Quoted-Printable | 100%-300% | 文本 | 好 | 中 |
| UUEncode | 135% | 二进制 | 差 | 低 |
| XXEncode | 135% | 二进制 | 差 | 低 |
| RFC2047 | 133%-300% | 邮件头 | 差 | 高 |

## 实际应用示例

### 示例 1：编码邮件附件

```cpp
#include "acl_cpp/mime/mime_base64.hpp"
#include <fstream>

bool encode_attachment(const char* input_file, const char* output_file) {
    // 读取文件
    std::ifstream ifs(input_file, std::ios::binary);
    if (!ifs) {
        return false;
    }
    
    // 获取文件大小
    ifs.seekg(0, std::ios::end);
    size_t size = ifs.tellg();
    ifs.seekg(0, std::ios::beg);
    
    // 读取数据
    char* buffer = new char[size];
    ifs.read(buffer, size);
    ifs.close();
    
    // Base64 编码
    acl::string encoded;
    acl::mime_base64::encode(buffer, size, &encoded);
    delete[] buffer;
    
    // 写入输出文件
    std::ofstream ofs(output_file);
    if (!ofs) {
        return false;
    }
    
    ofs << encoded;
    ofs.close();
    
    return true;
}
```

### 示例 2：编码邮件主题

```cpp
#include "acl_cpp/mime/rfc2047.hpp"

std::string encode_subject(const char* subject, const char* charset = "utf-8") {
    acl::string encoded;
    
    if (acl::rfc2047::encode(subject, strlen(subject), &encoded, 
                             charset, 'B', true)) {
        return std::string(encoded.c_str());
    }
    
    return std::string(subject);  // 编码失败，返回原文
}

// 使用
std::string subject = encode_subject("测试邮件：重要通知");
```

### 示例 3：解码邮件内容

```cpp
#include "acl_cpp/mime/mime_base64.hpp"
#include "acl_cpp/mime/mime_quoted_printable.hpp"

bool decode_content(const char* encoded_data, int len, 
                    int encoding_type, acl::string& decoded) {
    switch (encoding_type) {
    case MIME_ENC_BASE64:
        acl::mime_base64::decode(encoded_data, len, &decoded);
        return true;
        
    case MIME_ENC_QP:
        acl::mime_quoted_printable::decode(encoded_data, len, &decoded);
        return true;
        
    case MIME_ENC_7BIT:
    case MIME_ENC_8BIT:
    case MIME_ENC_BINARY:
        decoded.append(encoded_data, len);
        return true;
        
    default:
        return false;
    }
}
```

### 示例 4：流式处理大文件

```cpp
#include "acl_cpp/mime/mime_base64.hpp"
#include <fstream>

bool encode_large_file(const char* input_file, const char* output_file) {
    std::ifstream ifs(input_file, std::ios::binary);
    std::ofstream ofs(output_file);
    
    if (!ifs || !ofs) {
        return false;
    }
    
    acl::mime_base64 encoder(true, false);  // 添加换行
    
    char buffer[4096];
    acl::string encoded;
    
    while (ifs.read(buffer, sizeof(buffer)) || ifs.gcount() > 0) {
        size_t bytes_read = ifs.gcount();
        
        encoded.clear();
        encoder.encode_update(buffer, bytes_read, &encoded);
        
        if (!encoded.empty()) {
            ofs << encoded;
        }
    }
    
    // 完成编码
    encoded.clear();
    encoder.encode_finish(&encoded);
    if (!encoded.empty()) {
        ofs << encoded;
    }
    
    ifs.close();
    ofs.close();
    
    return true;
}
```

## 性能优化建议

1. **选择合适的编码方式**：
   - 二进制数据 → Base64
   - ASCII 文本 → Quoted-Printable 或 7-bit
   - 邮件头 → RFC2047

2. **使用流式接口**：
   - 大数据避免一次性编码
   - 使用 encode_update/decode_update

3. **缓冲区大小**：
   - Base64：建议 3 的倍数（减少填充）
   - Quoted-Printable：建议按行处理

4. **重用对象**：
   - 使用 reset() 重置状态
   - 避免频繁创建销毁对象

## 常见问题

### 1. Base64 编码后出现换行符

```cpp
// 不添加换行
acl::mime_base64 encoder(false, false);  // addCrlf = false

// 或使用静态方法（默认不添加换行）
acl::mime_base64::encode(data, len, &output);
```

### 2. Quoted-Printable 解码出现乱码

检查字符集是否正确，可能需要字符集转换。

### 3. RFC2047 编码过长

RFC2047 编码的文本有长度限制（通常 75 字符），长文本会自动分段。

### 4. 解码失败

- 检查编码类型是否正确
- 检查输入数据是否完整
- 设置 `addInvalid` 为 true 以保留非法字符

## 扩展阅读

- [MIME 解析详解](./mime_parse.md)
- [MIME 构建详解](./mime_build.md)
- [RFC 标准实现](./mime_rfc.md)
- [类型定义说明](./mime_types.md)

