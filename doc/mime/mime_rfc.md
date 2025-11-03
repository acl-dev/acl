# RFC 标准实现详解

## 概述

ACL MIME 模块实现了多个与电子邮件相关的 RFC 标准，主要包括：

- **RFC 822**：邮件地址格式和日期格式
- **RFC 2047**：MIME 邮件头编码
- 其他 MIME 相关标准

## RFC 822 实现

RFC 822 定义了互联网文本消息的格式，是电子邮件的基础标准。

### rfc822 类

`rfc822` 类提供了 RFC 822 标准相关的功能：

```cpp
#include "acl_cpp/mime/rfc822.hpp"

acl::rfc822 rfc;
```

## 日期和时间处理

### 1. 解析 RFC 822 日期

```cpp
#include "acl_cpp/mime/rfc822.hpp"

acl::rfc822 rfc;

// 解析日期字符串
const char* date_str = "Wed, 11 May 2011 09:44:37 +0800 (CST)";
time_t timestamp = rfc.parse_date(date_str);

if (timestamp != (time_t)-1) {
    std::cout << "时间戳: " << timestamp << std::endl;
}

// 支持的格式
const char* date1 = "Wed, 11 May 2011 09:44:37 +0800 (CST)";
const char* date2 = "Wed, 11 May 2011 16:17:39 GMT";
const char* date3 = "11 May 2011 09:44:37 +0800";
```

### 2. 生成 RFC 822 日期

#### 使用当前时间

```cpp
acl::rfc822 rfc;
time_t now = time(NULL);
char date_buf[128];

// 生成 CST 时间（中国标准时间，UTC+8）
rfc.mkdate_cst(now, date_buf, sizeof(date_buf));
std::cout << "CST: " << date_buf << std::endl;
// 输出类似: Wed, 11 May 2011 09:44:37 +0800 (CST)

// 生成 GMT 时间（格林威治标准时间）
rfc.mkdate_gmt(now, date_buf, sizeof(date_buf));
std::cout << "GMT: " << date_buf << std::endl;
// 输出类似: Wed, 11 May 2011 01:44:37 GMT
```

#### 指定时区

```cpp
acl::rfc822 rfc;
time_t timestamp = time(NULL);
char date_buf[128];

// 使用枚举指定时区
rfc.mkdate(timestamp, date_buf, sizeof(date_buf), tzone_cst);  // CST
rfc.mkdate(timestamp, date_buf, sizeof(date_buf), tzone_gmt);  // GMT
```

### 日期格式说明

**RFC 822 日期格式**：
```
day-of-week, DD Mon YYYY HH:MM:SS timezone (zone-name)
```

**示例**：
- `Wed, 11 May 2011 09:44:37 +0800 (CST)`
- `Wed, 11 May 2011 16:17:39 GMT`
- `Mon, 03 Nov 2025 10:30:00 +0800 (CST)`

**组成部分**：
- `day-of-week`：星期几（Mon, Tue, Wed, Thu, Fri, Sat, Sun）
- `DD`：日期（01-31）
- `Mon`：月份（Jan, Feb, Mar, Apr, May, Jun, Jul, Aug, Sep, Oct, Nov, Dec）
- `YYYY`：年份（4位数字）
- `HH:MM:SS`：时间（24小时制）
- `timezone`：时区偏移（如 +0800, -0500, GMT）
- `zone-name`：时区名称（如 CST, GMT, EST）

## 邮件地址处理

### 1. 解析单个邮件地址

```cpp
#include "acl_cpp/mime/rfc822.hpp"

acl::rfc822 rfc;

// 简单地址
const rfc822_addr* addr = rfc.parse_addr("user@example.com");
if (addr) {
    std::cout << "地址: " << addr->addr << std::endl;
    std::cout << "注释: " << (addr->comment ? addr->comment : "无") << std::endl;
}

// 带显示名称的地址
addr = rfc.parse_addr("张三 <zhangsan@example.com>");
if (addr) {
    std::cout << "地址: " << addr->addr << std::endl;
    std::cout << "显示名称: " << (addr->comment ? addr->comment : "无") << std::endl;
}

// 带 RFC2047 编码的地址
addr = rfc.parse_addr(
    "=?utf-8?B?5byg5LiJ?= <zhangsan@example.com>",
    "utf-8"  // 目标字符集
);
if (addr) {
    std::cout << "地址: " << addr->addr << std::endl;
    std::cout << "显示名称: " << addr->comment << std::endl;  // 已解码
}
```

### 2. 解析邮件地址列表

```cpp
acl::rfc822 rfc;

// 地址列表（多种格式混合）
const char* addr_list = 
    "\"张三\" <zhangsan@example.com>;"
    "lisi@example.com;"
    "=?utf-8?B?546L5LqU?= <wangwu@example.com>;"
    "<zhaoliu@example.com>;";

// 解析地址列表
const std::list<rfc822_addr*>& addrs = rfc.parse_addrs(addr_list, "utf-8");

// 遍历地址
for (auto* addr : addrs) {
    std::cout << "地址: " << addr->addr;
    if (addr->comment) {
        std::cout << ", 显示名称: " << addr->comment;
    }
    std::cout << std::endl;
}
```

### 3. 支持的地址格式

```cpp
// 1. 简单地址
"user@example.com"

// 2. 尖括号地址
"<user@example.com>"

// 3. 带显示名称
"User Name <user@example.com>"

// 4. 带引号的显示名称
"\"User Name\" <user@example.com>"

// 5. 带 RFC2047 编码
"=?utf-8?B?5byg5LiJ?= <user@example.com>"

// 6. 多个地址（分号或逗号分隔）
"user1@example.com; user2@example.com"
"user1@example.com, user2@example.com"

// 7. 混合格式
"User1 <user1@example.com>; user2@example.com; \"User3\" <user3@example.com>"

// 8. 多行格式
"User1 <user1@example.com>;\r\n"
"\tUser2 <user2@example.com>;\r\n"
"\tUser3 <user3@example.com>"
```

### 4. 验证邮件地址

```cpp
acl::rfc822 rfc;

// 检查地址是否有效
if (rfc.check_addr("user@example.com")) {
    std::cout << "地址有效" << std::endl;
} else {
    std::cout << "地址无效" << std::endl;
}

// 各种格式的验证
rfc.check_addr("user@example.com");              // 有效
rfc.check_addr("user.name@example.com");         // 有效
rfc.check_addr("user+tag@example.co.uk");        // 有效
rfc.check_addr("<user@example.com>");            // 有效
rfc.check_addr("User <user@example.com>");       // 有效
rfc.check_addr("invalid@");                      // 无效
rfc.check_addr("@example.com");                  // 无效
```

### rfc822_addr 结构

```cpp
struct rfc822_addr {
    char* addr;       // 邮件地址（如 user@example.com）
    char* comment;    // 显示名称或注释
};
```

**注意**：
- 解析后的地址对象由 `rfc822` 对象管理
- 不要手动释放 `rfc822_addr` 对象
- `rfc822` 对象销毁时会自动清理

## RFC 2047 实现

RFC 2047 定义了在邮件头中使用非 ASCII 字符的编码方法。

### rfc2047 类

详细用法请参考 [MIME 编码详解](./mime_encode.md#rfc2047-编码)。

### RFC 2047 编码格式

**格式**：`=?charset?encoding?encoded-text?=`

**组成部分**：
- `charset`：字符集（如 utf-8, gb2312）
- `encoding`：编码方式
  - `B`：Base64 编码
  - `Q`：Quoted-Printable 编码（修改版）
- `encoded-text`：编码后的文本

**示例**：
```
=?utf-8?B?5rWL6K+V6YKu5Lu2?=
=?gb2312?B?suLK1M7E0rvA7Q==?=
=?iso-8859-1?Q?Fran=E7ois?=
```

### 编码邮件头示例

```cpp
#include "acl_cpp/mime/rfc2047.hpp"

// 编码主题
acl::string encoded_subject;
acl::rfc2047::encode(
    "测试邮件：重要通知",
    strlen("测试邮件：重要通知"),
    &encoded_subject,
    "utf-8",
    'B',
    true
);

std::cout << "Subject: " << encoded_subject << std::endl;
// 输出: Subject: =?utf-8?B?5rWL6K+V6YKu5Lu2...?=
```

### 解码邮件头示例

```cpp
acl::string decoded;
acl::rfc2047::decode(
    "=?utf-8?B?5rWL6K+V6YKu5Lu2?=",
    strlen("=?utf-8?B?5rWL6K+V6YKu5Lu2?="),
    &decoded,
    "utf-8",     // 目标字符集
    true,        // 去除空格
    true         // 保留非法字符
);

std::cout << "解码后: " << decoded << std::endl;
```

## 实际应用示例

### 示例 1：创建标准的邮件头

```cpp
#include "acl_cpp/mime/rfc822.hpp"
#include "acl_cpp/mime/rfc2047.hpp"

std::string create_email_headers(
    const char* from_addr,
    const char* from_name,
    const char* to_addr,
    const char* to_name,
    const char* subject
) {
    acl::rfc822 rfc;
    std::ostringstream oss;
    
    // Date 字段
    char date_buf[128];
    rfc.mkdate_cst(time(NULL), date_buf, sizeof(date_buf));
    oss << "Date: " << date_buf << "\r\n";
    
    // From 字段
    if (from_name && from_name[0]) {
        acl::string encoded_name;
        acl::rfc2047::encode(from_name, strlen(from_name), 
                            &encoded_name, "utf-8", 'B', false);
        oss << "From: " << encoded_name << " <" << from_addr << ">\r\n";
    } else {
        oss << "From: " << from_addr << "\r\n";
    }
    
    // To 字段
    if (to_name && to_name[0]) {
        acl::string encoded_name;
        acl::rfc2047::encode(to_name, strlen(to_name), 
                            &encoded_name, "utf-8", 'B', false);
        oss << "To: " << encoded_name << " <" << to_addr << ">\r\n";
    } else {
        oss << "To: " << to_addr << "\r\n";
    }
    
    // Subject 字段
    acl::string encoded_subject;
    acl::rfc2047::encode(subject, strlen(subject), 
                        &encoded_subject, "utf-8", 'B', false);
    oss << "Subject: " << encoded_subject << "\r\n";
    
    return oss.str();
}

// 使用
std::string headers = create_email_headers(
    "sender@example.com",
    "张三",
    "recipient@example.com",
    "李四",
    "测试邮件：重要通知"
);

std::cout << headers << std::endl;
```

### 示例 2：解析邮件头中的地址

```cpp
#include "acl_cpp/mime/rfc822.hpp"

void parse_email_addresses(const char* header_value) {
    acl::rfc822 rfc;
    
    // 解析地址列表
    const std::list<rfc822_addr*>& addrs = 
        rfc.parse_addrs(header_value, "utf-8");
    
    std::cout << "找到 " << addrs.size() << " 个地址:" << std::endl;
    
    for (auto* addr : addrs) {
        std::cout << "  地址: " << addr->addr;
        if (addr->comment) {
            std::cout << " (" << addr->comment << ")";
        }
        std::cout << std::endl;
    }
}

// 使用
parse_email_addresses(
    "张三 <zhangsan@example.com>; "
    "lisi@example.com; "
    "=?utf-8?B?546L5LqU?= <wangwu@example.com>"
);
```

### 示例 3：生成邮件列表的日期索引

```cpp
#include "acl_cpp/mime/rfc822.hpp"
#include <map>

void index_emails_by_date(const std::vector<std::string>& email_dates) {
    acl::rfc822 rfc;
    std::map<time_t, std::vector<int>> date_index;
    
    for (size_t i = 0; i < email_dates.size(); i++) {
        time_t timestamp = rfc.parse_date(email_dates[i].c_str());
        if (timestamp != (time_t)-1) {
            date_index[timestamp].push_back(i);
        }
    }
    
    // 按日期排序输出
    for (auto& pair : date_index) {
        char date_buf[128];
        rfc.mkdate_cst(pair.first, date_buf, sizeof(date_buf));
        std::cout << date_buf << ": ";
        for (int idx : pair.second) {
            std::cout << idx << " ";
        }
        std::cout << std::endl;
    }
}
```

### 示例 4：邮件地址验证和规范化

```cpp
#include "acl_cpp/mime/rfc822.hpp"

std::string normalize_email_address(const char* input) {
    acl::rfc822 rfc;
    
    // 验证地址
    if (!rfc.check_addr(input)) {
        return "";  // 无效地址
    }
    
    // 解析地址
    const rfc822_addr* addr = rfc.parse_addr(input, "utf-8");
    if (!addr) {
        return "";
    }
    
    // 构建规范格式
    std::ostringstream oss;
    if (addr->comment) {
        oss << addr->comment << " ";
    }
    oss << "<" << addr->addr << ">";
    
    return oss.str();
}

// 使用
std::string normalized = normalize_email_address("张三 <zhangsan@example.com>");
std::cout << normalized << std::endl;
// 输出: 张三 <zhangsan@example.com>
```

## 时区处理

### 支持的时区

```cpp
enum tzone_t {
    tzone_gmt,    // 格林威治标准时间（UTC+0）
    tzone_cst     // 中国标准时间（UTC+8）
};
```

### 时区转换

```cpp
acl::rfc822 rfc;
time_t timestamp = time(NULL);
char date_buf[128];

// CST 时间（UTC+8）
rfc.mkdate_cst(timestamp, date_buf, sizeof(date_buf));
std::cout << "CST: " << date_buf << std::endl;

// GMT 时间（UTC+0）
rfc.mkdate_gmt(timestamp, date_buf, sizeof(date_buf));
std::cout << "GMT: " << date_buf << std::endl;

// 两者相差 8 小时
```

### 解析不同时区的日期

```cpp
acl::rfc822 rfc;

// 解析各种时区的日期
time_t t1 = rfc.parse_date("Wed, 11 May 2011 09:44:37 +0800");  // UTC+8
time_t t2 = rfc.parse_date("Wed, 11 May 2011 01:44:37 GMT");    // UTC+0
time_t t3 = rfc.parse_date("Tue, 10 May 2011 20:44:37 -0500");  // UTC-5

// t1, t2, t3 应该表示相同的时刻（可能有小的差异）
std::cout << "t1 = " << t1 << std::endl;
std::cout << "t2 = " << t2 << std::endl;
std::cout << "t3 = " << t3 << std::endl;
```

## RFC 标准兼容性

### RFC 822（已废弃，被 RFC 5322 取代）

ACL 实现的是 RFC 822 的核心功能，同时兼容 RFC 5322 的大部分特性。

**主要差异**：
- RFC 822：1982 年发布的原始标准
- RFC 5322：2008 年发布的更新标准
- 主要改进：更明确的语法定义、更好的国际化支持

### RFC 2047

完全实现 RFC 2047 标准，支持：
- Base64 编码（B encoding）
- Quoted-Printable 编码（Q encoding）
- 多种字符集
- 长文本自动分段

## 注意事项

### 日期处理

1. **时区**：确保正确处理时区信息
2. **本地时间**：`time_t` 通常是 UTC 时间
3. **夏令时**：注意夏令时的影响
4. **年份**：使用 4 位数年份

### 地址处理

1. **显示名称**：非 ASCII 字符需要 RFC2047 编码
2. **地址验证**：使用 `check_addr` 验证地址格式
3. **分隔符**：支持分号和逗号作为分隔符
4. **空白字符**：自动处理空格、制表符、换行符
5. **内存管理**：解析结果由 `rfc822` 对象管理，不要手动释放

### RFC 2047 编码

1. **适用范围**：只用于邮件头字段，不用于邮件地址部分
2. **长度限制**：编码文本通常不超过 75 字符
3. **字符集**：选择合适的字符集（推荐 utf-8）
4. **编码方式**：
   - Base64（B）：通用，适合所有文本
   - Quoted-Printable（Q）：适合主要为 ASCII 的文本

## 完整示例

### 创建符合标准的邮件

```cpp
#include "acl_cpp/mime/rfc822.hpp"
#include "acl_cpp/mime/rfc2047.hpp"
#include <fstream>

bool create_standard_email(const char* output_file) {
    acl::rfc822 rfc;
    std::ofstream ofs(output_file);
    
    if (!ofs) {
        return false;
    }
    
    // 生成 Date 字段
    char date_buf[128];
    rfc.mkdate_cst(time(NULL), date_buf, sizeof(date_buf));
    ofs << "Date: " << date_buf << "\r\n";
    
    // From 字段（带编码）
    acl::string encoded_from_name;
    acl::rfc2047::encode("张三", strlen("张三"), 
                        &encoded_from_name, "utf-8", 'B', false);
    ofs << "From: " << encoded_from_name 
        << " <zhangsan@example.com>\r\n";
    
    // To 字段（带编码）
    acl::string encoded_to_name;
    acl::rfc2047::encode("李四", strlen("李四"), 
                        &encoded_to_name, "utf-8", 'B', false);
    ofs << "To: " << encoded_to_name 
        << " <lisi@example.com>\r\n";
    
    // Subject 字段（带编码）
    acl::string encoded_subject;
    acl::rfc2047::encode("测试邮件：重要通知", 
                        strlen("测试邮件：重要通知"),
                        &encoded_subject, "utf-8", 'B', false);
    ofs << "Subject: " << encoded_subject << "\r\n";
    
    // 其他标准字段
    ofs << "MIME-Version: 1.0\r\n";
    ofs << "Content-Type: text/plain; charset=utf-8\r\n";
    ofs << "Content-Transfer-Encoding: 8bit\r\n";
    
    // 头和正文之间的空行
    ofs << "\r\n";
    
    // 邮件正文
    ofs << "这是邮件正文内容。\r\n";
    ofs << "第二行内容。\r\n";
    
    ofs.close();
    return true;
}
```

## 相关标准文档

- [RFC 822 - Standard for ARPA Internet Text Messages](https://www.rfc-editor.org/rfc/rfc822)
- [RFC 5322 - Internet Message Format](https://www.rfc-editor.org/rfc/rfc5322)
- [RFC 2047 - MIME Part Three: Message Header Extensions](https://www.rfc-editor.org/rfc/rfc2047)
- [RFC 5321 - Simple Mail Transfer Protocol](https://www.rfc-editor.org/rfc/rfc5321)

## 扩展阅读

- [MIME 编码详解](./mime_encode.md)
- [MIME 解析详解](./mime_parse.md)
- [MIME 构建详解](./mime_build.md)

