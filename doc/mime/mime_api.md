# MIME API 完整参考

## 目录

1. [mime 类](#mime-类)
2. [mime_node 类](#mime_node-类)
3. [mime_head 类](#mime_head-类)
4. [mime_body 类](#mime_body-类)
5. [mime_attach 类](#mime_attach-类)
6. [mime_image 类](#mime_image-类)
7. [mime_code 类](#mime_code-类)
8. [mime_base64 类](#mime_base64-类)
9. [mime_quoted_printable 类](#mime_quoted_printable-类)
10. [mime_uucode 类](#mime_uucode-类)
11. [mime_xxcode 类](#mime_xxcode-类)
12. [rfc2047 类](#rfc2047-类)
13. [rfc822 类](#rfc822-类)

---

## mime 类

主要的 MIME 处理类，提供邮件解析和构建功能。

**头文件**：`acl_cpp/mime/mime.hpp`

**命名空间**：`acl`

### 构造函数和析构函数

```cpp
mime();
~mime();
```

### 解析相关方法

#### reset

```cpp
mime& reset();
```

重置 MIME 对象状态，用于解析新邮件。

**返回值**：自身引用（支持链式调用）

#### parse

```cpp
bool parse(const char* file_path);
```

解析邮件文件。

**参数**：
- `file_path`：邮件文件路径

**返回值**：成功返回 `true`，失败返回 `false`

#### update_begin

```cpp
void update_begin(const char* path);
```

开始流式解析模式。

**参数**：
- `path`：邮件文件路径（可选，可为 `NULL`）

#### update

```cpp
bool update(const char* data, size_t len);
```

流式解析邮件数据。

**参数**：
- `data`：邮件数据
- `len`：数据长度

**返回值**：
- `true`：multipart 邮件解析完成
- `false`：需要继续解析或非 multipart 邮件

#### update_end

```cpp
void update_end();
```

结束流式解析。

#### primary_head_finish

```cpp
void primary_head_finish();
```

标记主邮件头已完成。

#### primary_head_ok

```cpp
bool primary_head_ok() const;
```

检查主邮件头是否已完成。

**返回值**：已完成返回 `true`

### 获取邮件内容方法

#### get_body_node

```cpp
mime_body* get_body_node(bool htmlFirst, bool enableDecode = true,
                        const char* toCharset = "gb2312", off_t off = 0);
```

获取邮件正文节点。

**参数**：
- `htmlFirst`：优先获取 HTML 正文
- `enableDecode`：是否自动解码
- `toCharset`：目标字符集
- `off`：偏移量

**返回值**：正文节点指针，未找到返回 `NULL`

#### get_plain_body

```cpp
mime_body* get_plain_body(bool enableDecode = true,
                         const char* toCharset = "gb2312", off_t off = 0);
```

获取纯文本正文。

#### get_html_body

```cpp
mime_body* get_html_body(bool enableDecode = true,
                        const char* toCharset = "gb2312", off_t off = 0);
```

获取 HTML 正文。

#### get_mime_nodes

```cpp
const std::list<mime_node*>& get_mime_nodes(bool enableDecode = true,
                                           const char* toCharset = "gb2312", 
                                           off_t off = 0);
```

获取所有 MIME 节点列表。

#### get_attachments

```cpp
const std::list<mime_attach*>& get_attachments(bool enableDecode = true,
                                              const char* toCharset = "gb2312",
                                              off_t off = 0, bool all = true);
```

获取附件列表。

**参数**：
- `all`：是否包含所有类型（message/application/image）

#### get_images

```cpp
const std::list<mime_image*>& get_images(bool enableDecode = true,
                                        const char* toCharset = "gb2312", 
                                        off_t off = 0);
```

获取图片列表。

#### get_image

```cpp
mime_image* get_image(const char* cid, bool enableDecode = true,
                     const char* toCharset = "gb2312", off_t off = 0);
```

根据 Content-ID 获取图片。

**参数**：
- `cid`：Content-ID

### 邮件头设置方法

#### set_sender / set_from / set_replyto / set_returnpath

```cpp
mime& set_sender(const char* addr);
mime& set_from(const char* addr);
mime& set_replyto(const char* addr);
mime& set_returnpath(const char* addr);
```

设置发件人相关字段。

**返回值**：自身引用（支持链式调用）

#### set_subject

```cpp
mime& set_subject(const char* s);
```

设置邮件主题。

#### add_to / add_cc / add_bcc / add_rcpt

```cpp
mime& add_to(const char* addr);
mime& add_cc(const char* addr);
mime& add_bcc(const char* addr);
mime& add_rcpt(const char* addr);
```

添加收件人。

#### add_header

```cpp
mime& add_header(const char* name, const char* value);
```

添加自定义头字段。

#### set_type

```cpp
mime& set_type(const char* ctype, const char* stype);
```

设置 Content-Type。

**参数**：
- `ctype`：主类型（如 "text", "multipart"）
- `stype`：子类型（如 "plain", "html"）

#### set_boundary

```cpp
mime& set_boundary(const char* s);
```

设置边界字符串（用于 multipart）。

### 邮件头获取方法

#### sender / from / replyto / returnpath

```cpp
const string& sender() const;
const string& from() const;
const string& replyto() const;
const string& returnpath() const;
```

获取发件人相关字段。

#### subject

```cpp
const string& subject() const;
```

获取邮件主题。

#### to_list / cc_list / bcc_list / rcpt_list

```cpp
const std::list<char*>& to_list() const;
const std::list<char*>& cc_list() const;
const std::list<char*>& bcc_list() const;
const std::list<char*>& rcpt_list() const;
```

获取收件人列表。

#### header_list

```cpp
const std::list<HEADER*>& header_list() const;
```

获取所有头字段列表。

#### header_value

```cpp
const char* header_value(const char* name) const;
```

获取指定头字段的值。

**返回值**：字段值，不存在返回 `NULL`

#### header_values

```cpp
int header_values(const char* name, std::list<const char*>* values) const;
```

获取指定头字段的所有值（多值字段）。

**返回值**：值的数量

#### get_ctype / get_stype

```cpp
const char* get_ctype() const;
const char* get_stype() const;
```

获取 Content-Type 的主类型和子类型。

#### primary_header

```cpp
const mime_head& primary_header() const;
```

获取邮件头对象。

### 保存方法

#### save_as

```cpp
bool save_as(ostream& out);
bool save_as(const char* file_path);
```

保存邮件。

#### save_mail

```cpp
bool save_mail(const char* path, const char* filename,
              bool enableDecode = true, const char* toCharset = "gb2312",
              off_t off = 0);
```

保存为 HTML 格式（适合浏览器显示）。

### 调试方法

#### mime_debug

```cpp
void mime_debug(const char* save_path, bool decode = true);
```

输出调试信息。

---

## mime_node 类

MIME 节点基类，表示邮件中的一个部分。

**头文件**：`acl_cpp/mime/mime_node.hpp`

**继承关系**：`mime_body`、`mime_attach`、`mime_image` 继承自此类

### 构造函数

```cpp
mime_node(const char* emailFile, const MIME_NODE* node,
         bool enableDecode = true, const char* toCharset = "gb2312",
         off_t off = 0);
```

### 属性获取方法

#### get_name

```cpp
const char* get_name() const;
```

获取节点名称（Content-Type 中的 name 字段）。

#### get_ctype / get_stype

```cpp
int get_ctype() const;
int get_stype() const;
```

获取类型常量（MIME_CTYPE_XXX / MIME_STYPE_XXX）。

#### get_ctype_s / get_stype_s

```cpp
const char* get_ctype_s() const;
const char* get_stype_s() const;
```

获取类型字符串。

#### get_encoding

```cpp
int get_encoding() const;
```

获取编码方式（MIME_ENC_XXX）。

#### get_charset / get_toCharset

```cpp
const char* get_charset() const;
const char* get_toCharset() const;
```

获取源字符集和目标字符集。

#### get_bodyBegin / get_bodyEnd

```cpp
off_t get_bodyBegin() const;
off_t get_bodyEnd() const;
```

获取正文在邮件文件中的起始和结束位置。

#### header_value

```cpp
const char* header_value(const char* name) const;
```

获取节点头字段值。

#### get_headers

```cpp
const std::map<string, string>& get_headers() const;
```

获取所有头字段。

### 保存方法

#### save

```cpp
bool save(pipe_manager& out) const;
bool save(pipe_manager& out, const char* src, int len) const;
bool save(ostream& out, const char* src = NULL, int len = 0) const;
bool save(const char* outFile, const char* src = NULL, int len = 0) const;
bool save(string& out, const char* src, int len) const;
```

保存节点内容到不同目标。

**参数**：
- `src`：邮件数据源地址（可选）
- `len`：数据长度（可选）

### 父节点方法

#### get_parent

```cpp
mime_node* get_parent() const;
```

获取父节点（需要调用者 delete）。

#### has_parent

```cpp
bool has_parent() const;
```

判断是否有父节点。

#### parent_xxx

```cpp
int parent_ctype() const;
const char* parent_ctype_s() const;
int parent_stype() const;
const char* parent_stype_s() const;
int parent_encoding() const;
char* parent_charset() const;
off_t parent_bodyBegin() const;
off_t parent_bodyEnd() const;
const char* parent_header_value(const char* name) const;
```

获取父节点的各种属性。

---

## mime_head 类

邮件头处理类。

**头文件**：`acl_cpp/mime/mime_head.hpp`

### 构造函数

```cpp
mime_head();
~mime_head();
```

### 设置方法

```cpp
mime_head& set_sender(const char*);
mime_head& set_from(const char*);
mime_head& set_replyto(const char*);
mime_head& set_returnpath(const char*);
mime_head& set_subject(const char*);
mime_head& add_to(const char*);
mime_head& add_cc(const char*);
mime_head& add_bcc(const char*);
mime_head& add_rcpt(const char*);
mime_head& add_header(const char*, const char*);
mime_head& set_type(const char*, const char*);
mime_head& set_boundary(const char*);
```

### 获取方法

```cpp
const string& get_boundary() const;
const char* get_ctype() const;
const char* get_stype() const;
const string& sender() const;
const string& from() const;
const string& replyto() const;
const string& returnpath() const;
const string& subject() const;
const std::list<char*>& to_list() const;
const std::list<char*>& cc_list() const;
const std::list<char*>& bcc_list() const;
const std::list<char*>& rcpt_list() const;
const std::list<HEADER*>& header_list() const;
const char* header_value(const char* name) const;
int header_values(const char* name, std::list<const char*>* values) const;
```

### 构建方法

#### build_head

```cpp
void build_head(string& buf, bool clean);
```

构建邮件头字符串。

**参数**：
- `buf`：输出缓冲区
- `clean`：是否清空缓冲区

#### reset

```cpp
mime_head& reset();
```

重置邮件头。

---

## mime_body 类

邮件正文类，继承自 `mime_node`。

**头文件**：`acl_cpp/mime/mime_body.hpp`

### 构造函数

```cpp
mime_body(const char* emailFile, const MIME_NODE* node,
         bool htmlFirst = true, bool enableDecode = true,
         const char* toCharset = "gb2312", off_t off = 0);
```

### 方法

#### set_status

```cpp
void set_status(bool htmlFirst);
```

设置是否优先获取 HTML 正文。

#### save_body

```cpp
bool save_body(pipe_manager& out, const char* src = NULL, int len = 0);
bool save_body(ostream& out, const char* src = NULL, int len = 0);
bool save_body(const char* file_path, const char* src = NULL, int len = 0);
bool save_body(pipe_string& out, const char* src = NULL, int len = 0);
bool save_body(string& out, const char* src = NULL, int len = 0);
```

保存正文内容。

#### html_stype

```cpp
bool html_stype() const;
```

判断是否为 HTML 类型。

---

## mime_attach 类

附件类，继承自 `mime_node`。

**头文件**：`acl_cpp/mime/mime_attach.hpp`

### 构造函数

```cpp
mime_attach(const char* emailFile, const MIME_NODE* node,
           bool enableDecode = true, const char* toCharset = "gb2312",
           off_t off = 0);
```

### 方法

#### get_filename

```cpp
const char* get_filename() const;
```

获取附件文件名。

**返回值**：文件名，不存在返回 `NULL`

---

## mime_image 类

图片类，继承自 `mime_node`。

**头文件**：`acl_cpp/mime/mime_image.hpp`

### 构造函数

```cpp
mime_image(const char* emailFile, const MIME_NODE* node,
          bool enableDecode = true, const char* toCharset = "gb2312",
          off_t off = 0);
```

### 方法

#### get_location

```cpp
const char* get_location() const;
```

获取图片位置标识（Content-ID）。

---

## mime_code 类

编码基类，抽象类。

**头文件**：`acl_cpp/mime/mime_code.hpp`

### 构造函数

```cpp
mime_code(bool addCrlf, bool addInvalid, const char* encoding_type);
virtual ~mime_code() = 0;
```

### 编码方法

#### encode_update

```cpp
virtual void encode_update(const char *src, int n, string* out);
```

流式编码。

#### encode_finish

```cpp
virtual void encode_finish(string* out);
```

完成编码。

### 解码方法

#### decode_update

```cpp
virtual void decode_update(const char *src, int n, string* out);
```

流式解码。

#### decode_finish

```cpp
virtual void decode_finish(string* out);
```

完成解码。

### 控制方法

#### reset

```cpp
virtual void reset();
```

重置编码器状态。

#### add_crlf

```cpp
virtual void add_crlf(bool on);
```

设置是否添加 CRLF。

#### add_invalid

```cpp
virtual void add_invalid(bool on);
```

设置是否保留非法字符。

#### set_status

```cpp
void set_status(bool encoding = true);
```

设置编码/解码模式。

### 静态方法

#### create

```cpp
static mime_code* create(int encoding, bool warn_unsupport = true);
```

创建编码器。

**参数**：
- `encoding`：编码类型（MIME_ENC_XXX）
- `warn_unsupport`：未找到时是否警告

**返回值**：编码器指针，失败返回 `NULL`

---

## mime_base64 类

Base64 编码类，继承自 `mime_code`。

**头文件**：`acl_cpp/mime/mime_base64.hpp`

### 构造函数

```cpp
mime_base64(bool addCrlf = false, bool addInvalid = false);
~mime_base64();
```

### 静态方法

#### encode

```cpp
static void encode(const char* in, int n, string* out);
```

Base64 编码（静态方法）。

#### decode

```cpp
static void decode(const char* in, int n, string* out);
```

Base64 解码（静态方法）。

---

## mime_quoted_printable 类

Quoted-Printable 编码类，继承自 `mime_code`。

**头文件**：`acl_cpp/mime/mime_quoted_printable.hpp`

### 构造函数

```cpp
mime_quoted_printable(bool addCrlf = false, bool addInvalid = false);
~mime_quoted_printable();
```

### 实例方法

```cpp
void encode_update(const char *src, int n, string* out);
void encode_finish(string* out);
void decode_update(const char *src, int n, string* out);
void decode_finish(string* out);
void reset();
void add_crlf(bool on);
void add_invalid(bool on);
```

### 静态方法

```cpp
static void encode(const char* in, int n, string* out);
static void decode(const char* in, int n, string* out);
```

---

## mime_uucode 类

UUEncode 编码类，继承自 `mime_code`。

**头文件**：`acl_cpp/mime/mime_uucode.hpp`

### 构造函数

```cpp
mime_uucode(bool addCrlf = false, bool addInvalid  = false);
~mime_uucode();
```

### 静态方法

```cpp
static void encode(const char* in, int n, string* out);
static void decode(const char* in, int n, string* out);
```

---

## mime_xxcode 类

XXEncode 编码类，继承自 `mime_code`。

**头文件**：`acl_cpp/mime/mime_xxcode.hpp`

### 构造函数

```cpp
mime_xxcode(bool addCrlf = false, bool addInvalid = false);
~mime_xxcode();
```

### 静态方法

```cpp
static void encode(const char* in, int n, string* out);
static void decode(const char* in, int n, string* out);
```

---

## rfc2047 类

RFC2047 邮件头编码类。

**头文件**：`acl_cpp/mime/rfc2047.hpp`

### 构造函数

```cpp
rfc2047(bool strip_sp = true, bool addCrlf = true);
~rfc2047();
```

**参数**：
- `strip_sp`：解码时是否去除空格和制表符
- `addCrlf`：编码时是否添加 CRLF

### 解码方法

#### decode_update

```cpp
void decode_update(const char* in, int n);
```

流式解码。

#### decode_finish

```cpp
bool decode_finish(const char* to_charset, string* out,
                  bool addInvalid = true);
```

完成解码并转换字符集。

### 编码方法

#### encode_update

```cpp
bool encode_update(const char* in, int n, string* out,
                  const char* charset = "gb2312", char coding = 'B');
```

流式编码。

**参数**：
- `charset`：字符集
- `coding`：编码方式（'B' = Base64, 'Q' = Quoted-Printable）

#### encode_finish

```cpp
bool encode_finish(string* out);
```

完成编码。

### 静态方法

#### encode

```cpp
static bool encode(const char* in, int n, string* out,
                  const char* charset = "gb2312", char coding = 'B',
                  bool addCrlf = true);
```

静态编码方法。

#### decode

```cpp
static bool decode(const char* in, int n, string* out,
                  const char* to_charset = "gb2312", bool strip_sp = false,
                  bool addInvalid = true);
```

静态解码方法。

### 其他方法

#### get_list

```cpp
const std::list<rfc2047_entry*>& get_list() const;
```

获取解码后的条目列表。

#### reset

```cpp
void reset(bool strip_sp = true);
```

重置状态。

#### debug_rfc2047

```cpp
void debug_rfc2047() const;
```

输出调试信息。

---

## rfc822 类

RFC822 邮件地址和日期处理类。

**头文件**：`acl_cpp/mime/rfc822.hpp`

### 构造函数

```cpp
rfc822();
~rfc822();
```

### 日期方法

#### parse_date

```cpp
time_t parse_date(const char *in);
```

解析 RFC822 日期字符串。

**返回值**：时间戳，失败返回 (time_t)-1

#### mkdate

```cpp
void mkdate(time_t t, char* out, size_t size, tzone_t zone = tzone_cst);
```

生成 RFC822 日期字符串。

**参数**：
- `t`：时间戳
- `out`：输出缓冲区
- `size`：缓冲区大小
- `zone`：时区（`tzone_cst` 或 `tzone_gmt`）

#### mkdate_cst

```cpp
void mkdate_cst(time_t t, char* out, size_t size);
```

生成 CST 时区日期。

#### mkdate_gmt

```cpp
void mkdate_gmt(time_t t, char* out, size_t size);
```

生成 GMT 时区日期。

### 地址方法

#### parse_addrs

```cpp
const std::list<rfc822_addr*>& parse_addrs(const char* in,
                                          const char* to_charset = "utf-8");
```

解析邮件地址列表。

**返回值**：地址列表

#### parse_addr

```cpp
const rfc822_addr* parse_addr(const char* in,
                             const char* to_charset = "utf-8");
```

解析单个邮件地址。

**返回值**：地址结构，失败返回 `NULL`

#### check_addr

```cpp
bool check_addr(const char* in);
```

验证邮件地址是否有效。

---

## 数据结构

### HEADER

```cpp
typedef struct HEADER {
    char *name;
    char *value;
} HEADER;
```

邮件头字段结构。

### rfc822_addr

```cpp
struct rfc822_addr {
    char* addr;       // 邮件地址
    char* comment;    // 显示名称/注释
};
```

邮件地址结构。

### rfc2047_entry

```cpp
struct rfc2047_entry {
    string* pData;       // 编码数据
    string* pCharset;    // 字符集
    char  coding;        // 编码方式
};
```

RFC2047 条目结构。

### tzone_t

```cpp
typedef enum {
    tzone_gmt,    // GMT 时区
    tzone_cst     // CST 时区
} tzone_t;
```

时区枚举。

---

## 使用示例

### 完整的邮件解析示例

```cpp
#include "acl_cpp/mime/mime.hpp"
#include <iostream>

void parse_email(const char* email_file) {
    acl::mime mail;
    
    // 解析邮件
    if (!mail.parse(email_file)) {
        std::cerr << "解析失败" << std::endl;
        return;
    }
    
    // 获取邮件头
    std::cout << "From: " << mail.from() << std::endl;
    std::cout << "To: ";
    for (auto* addr : mail.to_list()) {
        std::cout << addr << " ";
    }
    std::cout << std::endl;
    std::cout << "Subject: " << mail.subject() << std::endl;
    
    // 获取正文
    acl::mime_body* body = mail.get_body_node(true);
    if (body) {
        std::cout << "\n正文:\n";
        body->save_body(std::cout);
    }
    
    // 获取附件
    const auto& attachments = mail.get_attachments();
    std::cout << "\n附件数量: " << attachments.size() << std::endl;
    for (auto* attach : attachments) {
        const char* filename = attach->get_filename();
        if (filename) {
            std::cout << "  - " << filename << std::endl;
        }
    }
}
```

### 完整的邮件构建示例

```cpp
#include "acl_cpp/mime/mime.hpp"
#include "acl_cpp/mime/rfc822.hpp"
#include "acl_cpp/mime/rfc2047.hpp"

void create_email(const char* output_file) {
    acl::mime mail;
    acl::rfc822 rfc;
    
    // 设置日期
    char date_buf[128];
    rfc.mkdate_cst(time(NULL), date_buf, sizeof(date_buf));
    mail.add_header("Date", date_buf);
    
    // 设置发件人
    mail.set_from("sender@example.com");
    
    // 设置收件人
    mail.add_to("recipient@example.com");
    
    // 设置主题（自动 RFC2047 编码）
    mail.set_subject("测试邮件");
    
    // 设置类型
    mail.set_type("text", "plain");
    
    // 保存
    mail.save_as(output_file);
}
```

---

## 注意事项

1. **内存管理**：
   - 从 `mime` 对象获取的节点指针由 `mime` 对象管理，不要手动 delete
   - `get_parent()` 返回的父节点需要调用者 delete
   - `mime_code::create()` 返回的编码器需要调用者 delete

2. **线程安全**：
   - 大部分类都继承自 `noncopyable`，不支持拷贝
   - 每个线程应使用独立的对象实例

3. **字符集**：
   - 默认目标字符集为 "gb2312"
   - 推荐使用 "utf-8"
   - 支持常见字符集（gbk, gb18030, iso-8859-1 等）

4. **编码**：
   - 二进制数据使用 Base64
   - 文本数据可使用 Quoted-Printable
   - 邮件头使用 RFC2047

5. **错误处理**：
   - 解析失败返回 `false` 或 `NULL`
   - 检查返回值以确保操作成功

---

## 相关文档

- [MIME 解析详解](./mime_parse.md)
- [MIME 构建详解](./mime_build.md)
- [MIME 编码详解](./mime_encode.md)
- [RFC 标准实现](./mime_rfc.md)
- [类型定义说明](./mime_types.md)

