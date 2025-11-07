# ACL 数据库模块使用文档

## 1. 快速入门

### 1.1 基本使用流程

```cpp
#include "acl_cpp/lib_acl.hpp"

int main() {
    // 1. 创建数据库连接
    acl::db_mysql db("127.0.0.1:3306", "testdb", "root", "password");
    
    // 2. 打开数据库
    if (!db.open()) {
        printf("打开数据库失败: %s\n", db.get_error());
        return 1;
    }
    
    // 3. 执行查询
    const char* sql = "SELECT * FROM users WHERE age >= 18";
    if (!db.sql_select(sql)) {
        printf("查询失败: %s\n", db.get_error());
        return 1;
    }
    
    // 4. 遍历结果
    for (size_t i = 0; i < db.length(); i++) {
        const acl::db_row* row = db[i];
        printf("id=%s, name=%s, age=%d\n",
            row->field_value("id"),
            row->field_value("name"),
            row->field_int("age"));
    }
    
    // 5. 执行更新
    const char* update_sql = "UPDATE users SET status='active' WHERE id=1";
    if (db.sql_update(update_sql)) {
        printf("更新了 %d 行\n", db.affect_count());
    }
    
    return 0;
}
```

## 2. MySQL 数据库使用

### 2.1 创建连接

#### 方式一：直接传参
```cpp
acl::db_mysql db(
    "127.0.0.1:3306",           // 数据库地址
    "testdb",                   // 数据库名
    "root",                     // 用户名
    "password",                 // 密码
    0,                          // 标志位
    true,                       // 自动提交
    60,                         // 连接超时(秒)
    60,                         // 读写超时(秒)
    "utf8"                      // 字符集
);
```

#### 方式二：使用配置对象
```cpp
acl::mysql_conf conf("127.0.0.1:3306", "testdb");
conf.set_dbuser("root")
    .set_dbpass("password")
    .set_charset("utf8")
    .set_conn_timeout(60)
    .set_rw_timeout(60)
    .set_auto_commit(true);

acl::db_mysql db(conf);
```

#### 方式三：UNIX 域套接字
```cpp
acl::db_mysql db("/tmp/mysql.sock", "testdb", "root", "password");
```

### 2.2 基本操作

#### 打开连接
```cpp
if (!db.open()) {
    printf("连接失败: %s (errno: %d)\n", db.get_error(), db.get_errno());
    return false;
}

if (db.is_opened()) {
    printf("数据库已打开\n");
}
```

#### 查询操作
```cpp
const char* sql = "SELECT id, name, email FROM users WHERE status='active'";
acl::db_rows result;

if (db.sql_select(sql, &result)) {
    // 遍历结果
    for (size_t i = 0; i < result.length(); i++) {
        const acl::db_row* row = result[i];
        printf("ID: %s, Name: %s, Email: %s\n",
            row->field_value("id"),
            row->field_value("name"),
            row->field_value("email"));
    }
}

// 或者使用内部结果集
if (db.sql_select(sql)) {
    const acl::db_row* first = db.get_first_row();
    if (first) {
        printf("第一行: %s\n", first->field_value("name"));
    }
    
    // 使用完后释放
    db.free_result();
}
```

#### 更新操作
```cpp
const char* sql = "UPDATE users SET last_login=NOW() WHERE id=1";
if (db.sql_update(sql)) {
    printf("成功更新 %d 行\n", db.affect_count());
} else {
    printf("更新失败: %s\n", db.get_error());
}
```

#### 插入操作
```cpp
const char* sql = "INSERT INTO users (name, email) VALUES ('张三', 'zhangsan@example.com')";
if (db.sql_update(sql)) {
    printf("成功插入，影响行数: %d\n", db.affect_count());
}
```

#### 删除操作
```cpp
const char* sql = "DELETE FROM users WHERE id=1";
if (db.sql_update(sql)) {
    printf("成功删除 %d 行\n", db.affect_count());
}
```

### 2.3 事务处理

```cpp
// 开始事务
if (!db.begin_transaction()) {
    printf("开始事务失败\n");
    return false;
}

try {
    // 执行多个操作
    db.sql_update("UPDATE accounts SET balance=balance-100 WHERE id=1");
    db.sql_update("UPDATE accounts SET balance=balance+100 WHERE id=2");
    
    // 提交事务
    if (!db.commit()) {
        printf("提交失败，回滚\n");
        db.rollback();
        return false;
    }
    
    printf("事务提交成功\n");
} catch (...) {
    // 回滚事务
    db.rollback();
    printf("发生异常，已回滚\n");
}
```

**注意**：使用事务时，构造 `db_mysql` 时需要设置 `auto_commit` 为 `false`：
```cpp
acl::db_mysql db("127.0.0.1:3306", "testdb", "root", "password",
                 0, false);  // auto_commit = false
```

### 2.4 检查表是否存在

```cpp
if (db.tbl_exists("users")) {
    printf("表 users 存在\n");
} else {
    printf("表 users 不存在\n");
}
```

### 2.5 动态加载 MySQL 库

```cpp
// 设置库路径（可选）
acl::db_handle::set_loadpath("/usr/local/mysql/lib");

// 加载库
if (acl::db_mysql::load()) {
    printf("MySQL 库加载成功\n");
} else {
    printf("MySQL 库加载失败\n");
    return 1;
}

// 后续正常使用
acl::db_mysql db("127.0.0.1:3306", "testdb", "root", "password");
```

## 3. PostgreSQL 数据库使用

### 3.1 创建连接

```cpp
// 使用配置对象
acl::pgsql_conf conf("127.0.0.1:5432", "testdb");
conf.set_dbuser("postgres")
    .set_dbpass("password")
    .set_charset("utf8")
    .set_conn_timeout(60)
    .set_rw_timeout(60);

acl::db_pgsql db(conf);

if (!db.open()) {
    printf("连接失败: %s\n", db.get_error());
    return 1;
}
```

### 3.2 UNIX 域套接字连接

PostgreSQL 的 UNIX 域套接字配置与 MySQL 不同：
- PostgreSQL socket 文件：`/tmp/.s.PGSQL.5432`
- 只需指定目录：`/tmp`（不包含文件名）

```cpp
acl::pgsql_conf conf("/tmp", "testdb");  // 只指定目录
conf.set_dbuser("postgres").set_dbpass("password");

acl::db_pgsql db(conf);
```

### 3.3 基本操作

PostgreSQL 的基本操作与 MySQL 类似：

```cpp
// 查询
const char* sql = "SELECT * FROM users WHERE age >= 18";
if (db.sql_select(sql)) {
    for (size_t i = 0; i < db.length(); i++) {
        const acl::db_row* row = db[i];
        printf("name=%s\n", row->field_value("name"));
    }
    db.free_result();
}

// 更新
if (db.sql_update("UPDATE users SET status='active' WHERE id=1")) {
    printf("更新了 %d 行\n", db.affect_count());
}

// 事务
db.begin_transaction();
db.sql_update("INSERT INTO log (message) VALUES ('test')");
db.commit();
```

### 3.4 动态加载

```cpp
if (acl::db_pgsql::load()) {
    printf("PostgreSQL 库加载成功\n");
}
```

## 4. SQLite 数据库使用

### 4.1 创建连接

```cpp
// 指定数据库文件路径
acl::db_sqlite db("./test.db", "utf-8");

if (!db.open()) {
    printf("打开数据库失败: %s\n", db.get_error());
    return 1;
}
```

### 4.2 配置 SQLite

SQLite 支持通过 PRAGMA 语句配置：

```cpp
// 设置同步模式
db.set_conf("PRAGMA synchronous = NORMAL");

// 设置日志模式
db.set_conf("PRAGMA journal_mode = WAL");

// 设置缓存大小
db.set_conf("PRAGMA cache_size = 10000");

// 获取配置
acl::string value;
db.get_conf("PRAGMA synchronous", value);
printf("同步模式: %s\n", value.c_str());

// 显示所有配置
db.show_conf();
```

### 4.3 基本操作

```cpp
// 创建表
const char* create_sql = 
    "CREATE TABLE IF NOT EXISTS users ("
    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "name TEXT NOT NULL,"
    "age INTEGER,"
    "email TEXT)";
    
if (!db.sql_update(create_sql)) {
    printf("创建表失败: %s\n", db.get_error());
}

// 插入数据
const char* insert_sql = "INSERT INTO users (name, age, email) VALUES ('张三', 25, 'zhangsan@example.com')";
if (db.sql_update(insert_sql)) {
    printf("插入成功，影响行数: %d\n", db.affect_count());
}

// 查询数据
if (db.sql_select("SELECT * FROM users")) {
    for (size_t i = 0; i < db.length(); i++) {
        const acl::db_row* row = db[i];
        printf("ID: %d, Name: %s, Age: %d\n",
            row->field_int("id"),
            row->field_string("name"),
            row->field_int("age"));
    }
    db.free_result();
}
```

### 4.4 使用游标（预编译语句）

游标支持更高效的查询，适合重复执行的 SQL：

```cpp
// 创建查询对象
acl::query q;
q.create("SELECT * FROM users WHERE age >= :age AND name LIKE :name")
 .set_parameter("age", 18)
 .set_parameter("name", "%张%");

// 创建游标
acl::sqlite_cursor cursor(q);

// 准备语句
if (!db.prepare(cursor)) {
    printf("准备失败: %s\n", db.get_error());
    return 1;
}

// 执行查询并遍历结果
bool done = false;
while (db.next(cursor, &done) && !done) {
    acl::db_row* row = cursor.get_row();
    if (row) {
        printf("Name: %s, Age: %d\n",
            row->field_string("name"),
            row->field_int("age"));
    }
}
```

### 4.5 事务处理

```cpp
if (db.begin_transaction()) {
    db.sql_update("INSERT INTO users (name, age) VALUES ('李四', 30)");
    db.sql_update("INSERT INTO users (name, age) VALUES ('王五', 35)");
    
    if (db.commit()) {
        printf("事务提交成功\n");
    } else {
        db.rollback();
        printf("提交失败，已回滚\n");
    }
}
```

### 4.6 设置忙碌超时

```cpp
// 设置忙碌超时为 5000 毫秒
db.set_busy_timeout(5000);
```

### 4.7 直接使用 SQLite3 接口

`db_sqlite` 封装了常用的 SQLite3 接口：

```cpp
// 获取 SQLite3 连接句柄
sqlite3* conn = (sqlite3*)db.get_conn();

// 使用封装的接口
sqlite3_stmt* stmt = NULL;
int ret = db.sqlite3_prepare_v2("SELECT * FROM users", -1, &stmt, NULL);
if (ret == SQLITE_OK) {
    while (db.sqlite3_step(stmt) == SQLITE_ROW) {
        const char* name = (const char*)db.sqlite3_column_text(stmt, 0);
        printf("Name: %s\n", name);
    }
    db.sqlite3_finalize(stmt);
}
```

### 4.8 字符集转换

SQLite 支持自动字符集转换：

```cpp
// 创建时指定字符集
acl::db_sqlite db("./test.db", "gbk");

// SQLite 内部使用 UTF-8
// 插入和查询时会自动在 GBK 和 UTF-8 之间转换
```

## 5. 安全查询（防止 SQL 注入）

### 5.1 使用 query 类

`query` 类提供参数化查询，自动转义特殊字符：

```cpp
acl::query q;
q.create("SELECT * FROM users WHERE name = :name AND age >= :age")
 .set_parameter("name", user_input_name)  // 自动转义
 .set_parameter("age", 18);

acl::db_rows result;
if (db.exec_select(q, &result)) {
    // 处理结果
}
```

### 5.2 手动转义

```cpp
acl::string escaped;
db.escape_string(user_input, strlen(user_input), escaped);

acl::string sql;
sql.format("SELECT * FROM users WHERE name='%s'", escaped.c_str());
db.sql_select(sql.c_str());
```

### 5.3 query 支持的参数类型

```cpp
acl::query q;
q.create("INSERT INTO users (name, age, salary, created_at, active, comment) "
         "VALUES (:name, :age, :salary, :created_at, :active, :comment)");

// 字符串
q.set_parameter("name", "张三");

// 整数
q.set_parameter("age", 25);

// 浮点数
q.set_parameter("salary", 12345.67, 2);  // 精度为 2

// 日期时间
q.set_date("created_at", time(NULL), "%Y-%m-%d %H:%M:%S");

// 字符
q.set_parameter("active", 'Y');

// 格式化字符串
q.set_format("comment", "这是 %s 的评论，编号 %d", "张三", 1);

// 生成 SQL
const acl::string& sql = q.to_string();
printf("SQL: %s\n", sql.c_str());
```

### 5.4 使用 exec_select 和 exec_update

这两个方法使用 `query` 对象，更安全：

```cpp
// 查询
acl::query q1;
q1.create("SELECT * FROM users WHERE status = :status")
  .set_parameter("status", "active");

acl::db_rows result;
if (db.exec_select(q1, &result)) {
    printf("查询到 %d 条记录\n", (int)result.length());
}

// 更新
acl::query q2;
q2.create("UPDATE users SET status = :status WHERE id = :id")
  .set_parameter("status", "inactive")
  .set_parameter("id", 123);

if (db.exec_update(q2)) {
    printf("更新了 %d 行\n", db.affect_count());
}
```

### 5.5 重用 query 对象

```cpp
acl::query q;
q.create("SELECT * FROM users WHERE name = :name");

// 第一次查询
q.set_parameter("name", "张三");
db.exec_select(q, &result1);

// 重置后再次使用
q.reset();
q.set_parameter("name", "李四");
db.exec_select(q, &result2);
```

## 6. 连接池使用

### 6.1 MySQL 连接池

#### 创建连接池

```cpp
// 方式一：直接传参
acl::mysql_pool pool(
    "127.0.0.1:3306",    // 地址
    "testdb",            // 数据库名
    "root",              // 用户
    "password",          // 密码
    64,                  // 最大连接数
    0,                   // 标志位
    true,                // 自动提交
    60,                  // 连接超时
    60,                  // 读写超时
    "utf8"               // 字符集
);

// 方式二：使用配置对象
acl::mysql_conf conf("127.0.0.1:3306", "testdb");
conf.set_dbuser("root")
    .set_dbpass("password")
    .set_dblimit(64)
    .set_charset("utf8");

acl::mysql_pool pool2(conf);
```

#### 使用连接（手动管理）

```cpp
// 从连接池获取连接
acl::db_handle* db = pool.peek_open();
if (db == NULL) {
    printf("获取连接失败\n");
    return 1;
}

// 使用连接
db->sql_select("SELECT * FROM users");

// 归还连接到池中
pool.put(db);
```

#### 使用连接（RAII 方式，推荐）

```cpp
// 使用 db_guard 自动管理
{
    acl::db_guard guard(pool);
    acl::db_handle* db = (acl::db_handle*)guard.peek();
    
    if (db) {
        db->sql_select("SELECT * FROM users");
        // 处理结果...
    }
    
    // 离开作用域时自动归还连接
}
```

#### 设置空闲超时

```cpp
// 设置空闲连接 120 秒后自动关闭
pool.set_idle(120);
```

#### 查询连接池状态

```cpp
printf("最大连接数: %d\n", (int)pool.get_dblimit());
printf("当前连接数: %d\n", (int)pool.get_dbcount());
```

### 6.2 PostgreSQL 连接池

```cpp
acl::pgsql_conf conf("127.0.0.1:5432", "testdb");
conf.set_dbuser("postgres")
    .set_dbpass("password")
    .set_dblimit(64);

acl::pgsql_pool pool(conf);

// 使用方式与 MySQL 相同
acl::db_guard guard(pool);
acl::db_handle* db = (acl::db_handle*)guard.peek();
if (db) {
    db->sql_select("SELECT * FROM users");
}
```

### 6.3 SQLite 连接池

```cpp
acl::sqlite_pool pool(
    "./test.db",    // 数据库文件
    64,             // 最大连接数
    "utf-8"         // 字符集
);

acl::db_guard guard(pool);
acl::db_handle* db = (acl::db_handle*)guard.peek();
if (db) {
    db->sql_select("SELECT * FROM users");
}
```

### 6.4 连接池管理器

管理器用于管理多个数据库的连接池：

```cpp
// 创建管理器
acl::mysql_manager manager(120);  // 空闲超时 120 秒

// 添加多个数据库
manager.add("192.168.1.100:3306", "db1", "root", "pass1", 64);
manager.add("192.168.1.101:3306", "db2", "root", "pass2", 64);

// 使用配置对象添加
acl::mysql_conf conf("192.168.1.102:3306", "db3");
conf.set_dbuser("root").set_dbpass("pass3").set_dblimit(32);
manager.add(conf);

// 从管理器获取连接池
acl::connect_pool* pool = manager.get("db1@192.168.1.100:3306");
if (pool) {
    acl::db_guard guard(*pool);
    acl::db_handle* db = (acl::db_handle*)guard.peek();
    // 使用连接...
}
```

## 7. 异步数据库服务

异步服务在后台线程池中执行数据库操作，不阻塞主线程。

### 7.1 MySQL 异步服务

#### 创建服务

```cpp
// 创建异步服务
acl::db_service_mysql service(
    "127.0.0.1:3306",    // 地址
    "testdb",            // 数据库名
    "root",              // 用户
    "password",          // 密码
    0,                   // 标志位
    true,                // 自动提交
    60,                  // 连接超时
    60,                  // 读写超时
    100,                 // 连接池大小
    2,                   // 线程数
    false                // 非 Win32 GUI
);
```

#### 定义回调类

```cpp
class MyQuery : public acl::db_query {
public:
    MyQuery() {}
    ~MyQuery() {}
    
    // 错误回调
    virtual void on_error(acl::db_status status) {
        if (status == acl::DB_ERR_OPEN) {
            printf("打开数据库失败\n");
        } else if (status == acl::DB_ERR_EXEC_SQL) {
            printf("执行 SQL 失败\n");
        }
    }
    
    // 成功回调
    virtual void on_ok(const acl::db_rows* rows, int affected) {
        if (rows) {
            // 查询结果
            printf("查询到 %d 行\n", (int)rows->length());
            for (size_t i = 0; i < rows->length(); i++) {
                const acl::db_row* row = (*rows)[i];
                printf("Name: %s\n", row->field_value("name"));
            }
        } else {
            // 更新结果
            printf("影响了 %d 行\n", affected);
        }
    }
    
    // 清理回调
    virtual void destroy() {
        delete this;  // 如果是动态分配的
    }
};
```

#### 执行异步查询

```cpp
// 异步查询
const char* select_sql = "SELECT * FROM users WHERE age >= 18";
MyQuery* query1 = new MyQuery();
service.sql_select(select_sql, query1);

// 异步更新
const char* update_sql = "UPDATE users SET status='active' WHERE id=1";
MyQuery* query2 = new MyQuery();
service.sql_update(update_sql, query2);

// 主线程继续执行其他任务...
```

### 7.2 SQLite 异步服务

```cpp
// 创建异步服务
acl::db_service_sqlite service(
    "testdb",            // 数据库名（标识）
    "./test.db",         // 数据库文件
    100,                 // 连接池大小
    2,                   // 线程数
    false                // 非 Win32 GUI
);

// 使用方式与 MySQL 相同
MyQuery* query = new MyQuery();
service.sql_select("SELECT * FROM users", query);
```

### 7.3 Windows GUI 集成

在 Windows GUI 应用中，可以使用消息循环：

```cpp
#ifdef _WIN32
// 创建服务，启用 Win32 GUI 模式
acl::db_service_mysql service(
    "127.0.0.1:3306", "testdb", "root", "password",
    0, true, 60, 60, 100, 2, true  // win32_gui = true
);

// 在窗口过程中处理消息
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    // 异步服务会通过窗口消息通知结果
    // ...
}
#endif
```

## 8. 结果集处理

### 8.1 访问字段值

```cpp
const acl::db_row* row = db[0];  // 获取第一行

// 按字段名访问
const char* name = row->field_value("name");
const char* email = row["email"];  // 操作符方式

// 按索引访问
const char* col0 = row->field_value(0);
const char* col1 = row[1];

// 获取字段名
const char* field_name = row->field_name(0);
```

### 8.2 类型转换

```cpp
const acl::db_row* row = db[0];

// 整型
int age = row->field_int("age");
int age_with_default = row->field_int("age", -1);  // 指定默认值

// 64 位整型
long long big_num = row->field_int64("big_number");

// 浮点型
double salary = row->field_double("salary");
double salary_with_default = row->field_double("salary", 0.0);

// 字符串（同 field_value）
const char* name = row->field_string("name");

// 获取字段长度
size_t len = row->field_length("name");
```

### 8.3 检查空值

```cpp
const acl::db_row* row = db[0];

const char* email = row->field_value("email");
if (email == NULL || *email == '\0') {
    printf("Email 为空\n");
}

// 使用默认值
int age = row->field_int("age", -1);
if (age == -1) {
    printf("Age 字段为空或不存在\n");
}
```

### 8.4 遍历结果集

#### 方式一：使用索引
```cpp
for (size_t i = 0; i < db.length(); i++) {
    const acl::db_row* row = db[i];
    printf("Name: %s\n", row->field_value("name"));
}
```

#### 方式二：使用迭代器
```cpp
const std::vector<acl::db_row*>* rows = db.get_rows();
if (rows) {
    for (std::vector<acl::db_row*>::const_iterator it = rows->begin();
         it != rows->end(); ++it) {
        printf("Name: %s\n", (*it)->field_value("name"));
    }
}
```

#### 方式三：只获取第一行
```cpp
const acl::db_row* first = db.get_first_row();
if (first) {
    printf("第一条记录: %s\n", first->field_value("name"));
}
```

### 8.5 过滤结果

```cpp
// 查询所有用户
db.sql_select("SELECT * FROM users");

// 从结果中筛选特定条件的记录
const std::vector<const acl::db_row*>* filtered = 
    db.get_rows("status", "active");

if (filtered) {
    for (size_t i = 0; i < filtered->size(); i++) {
        const acl::db_row* row = (*filtered)[i];
        printf("Active user: %s\n", row->field_value("name"));
    }
}
```

### 8.6 打印结果集

```cpp
// 打印所有结果
db.print_out();

// 打印前 10 行
db.print_out(10);
```

### 8.7 使用外部结果集对象

```cpp
acl::db_rows result1;
db.sql_select("SELECT * FROM users", &result1);

acl::db_rows result2;
db.sql_select("SELECT * FROM orders", &result2);

// result1 和 result2 可以独立管理
printf("Users: %d, Orders: %d\n", 
    (int)result1.length(), 
    (int)result2.length());
```

## 9. 最佳实践

### 9.1 连接管理

#### 推荐：使用连接池
```cpp
// ✅ 推荐
acl::mysql_pool pool("127.0.0.1:3306", "testdb", "root", "password", 64);

void process_request() {
    acl::db_guard guard(pool);
    acl::db_handle* db = (acl::db_handle*)guard.peek();
    if (db) {
        db->sql_select("SELECT * FROM users");
    }
    // 自动归还连接
}
```

#### 不推荐：频繁创建连接
```cpp
// ❌ 不推荐
void process_request() {
    acl::db_mysql db("127.0.0.1:3306", "testdb", "root", "password");
    if (db.open()) {
        db.sql_select("SELECT * FROM users");
    }
    // 每次都创建和销毁连接，性能差
}
```

### 9.2 SQL 安全

#### 推荐：使用参数化查询
```cpp
// ✅ 推荐
acl::query q;
q.create("SELECT * FROM users WHERE name = :name")
 .set_parameter("name", user_input);
db.exec_select(q, &result);
```

#### 不推荐：字符串拼接
```cpp
// ❌ 不推荐 - 存在 SQL 注入风险
acl::string sql;
sql.format("SELECT * FROM users WHERE name = '%s'", user_input);
db.sql_select(sql.c_str());
```

### 9.3 结果集管理

#### 推荐：及时释放结果集
```cpp
// ✅ 推荐
db.sql_select("SELECT * FROM users");
// 处理结果...
db.free_result();  // 及时释放

db.sql_select("SELECT * FROM orders");
// 处理新结果...
db.free_result();
```

#### 推荐：使用外部结果集对象
```cpp
// ✅ 推荐 - 需要保留多个结果集时
acl::db_rows result1;
db.sql_select("SELECT * FROM users", &result1);

acl::db_rows result2;
db.sql_select("SELECT * FROM orders", &result2);

// 两个结果集可以同时使用
```

### 9.4 错误处理

#### 推荐：检查返回值
```cpp
// ✅ 推荐
if (!db.open()) {
    logger_error("打开数据库失败: %s (errno: %d)", 
        db.get_error(), db.get_errno());
    return false;
}

if (!db.sql_select(sql)) {
    logger_error("查询失败: %s", db.get_error());
    return false;
}
```

#### 不推荐：忽略错误
```cpp
// ❌ 不推荐
db.open();  // 没有检查返回值
db.sql_select(sql);  // 没有检查返回值
```

### 9.5 事务处理

#### 推荐：使用 RAII 风格
```cpp
// ✅ 推荐
class Transaction {
    acl::db_handle& db_;
    bool committed_;
public:
    Transaction(acl::db_handle& db) : db_(db), committed_(false) {
        db_.begin_transaction();
    }
    
    ~Transaction() {
        if (!committed_) {
            db_.rollback();
        }
    }
    
    bool commit() {
        if (db_.commit()) {
            committed_ = true;
            return true;
        }
        return false;
    }
};

// 使用
{
    Transaction trans(db);
    db.sql_update("UPDATE accounts SET balance=balance-100 WHERE id=1");
    db.sql_update("UPDATE accounts SET balance=balance+100 WHERE id=2");
    trans.commit();
    // 如果没有 commit，析构时自动回滚
}
```

### 9.6 字符集处理

#### 推荐：统一使用 UTF-8
```cpp
// ✅ 推荐
acl::mysql_pool pool("127.0.0.1:3306", "testdb", "root", "password",
    64, 0, true, 60, 60, "utf8mb4");  // 使用 utf8mb4

// 应用程序内部也使用 UTF-8
// 避免字符集转换问题
```

### 9.7 性能优化

#### 使用批量操作
```cpp
// ✅ 推荐
acl::string sql = "INSERT INTO users (name, age) VALUES ";
for (int i = 0; i < 1000; i++) {
    sql.format_append("('user%d', %d)", i, 20 + i % 50);
    if (i < 999) sql.append(",");
}
db.sql_update(sql.c_str());
```

#### 使用事务批量插入
```cpp
// ✅ 推荐
db.begin_transaction();
for (int i = 0; i < 1000; i++) {
    acl::string sql;
    sql.format("INSERT INTO users (name, age) VALUES ('user%d', %d)", i, 20 + i % 50);
    db.sql_update(sql.c_str());
}
db.commit();
```

#### 使用预编译语句（SQLite）
```cpp
// ✅ 推荐 - 重复执行时性能更好
acl::query q;
q.create("INSERT INTO users (name, age) VALUES (:name, :age)");

for (int i = 0; i < 1000; i++) {
    q.reset();
    q.set_parameter("name", acl::string().format("user%d", i).c_str());
    q.set_parameter("age", 20 + i % 50);
    db.exec_update(q);
}
```

### 9.8 多线程使用

#### 推荐：每线程一连接
```cpp
// ✅ 推荐
void thread_func(acl::mysql_pool& pool) {
    acl::db_guard guard(pool);
    acl::db_handle* db = (acl::db_handle*)guard.peek();
    if (db) {
        db->sql_select("SELECT * FROM users");
        // 处理结果...
    }
}

// 创建多个线程
acl::mysql_pool pool("127.0.0.1:3306", "testdb", "root", "password", 100);
std::thread t1(thread_func, std::ref(pool));
std::thread t2(thread_func, std::ref(pool));
```

#### 不推荐：跨线程共享连接
```cpp
// ❌ 不推荐 - 数据库连接不是线程安全的
acl::db_mysql* global_db = new acl::db_mysql(...);

void thread_func() {
    global_db->sql_select("SELECT * FROM users");  // 危险！
}
```

## 10. 完整示例

### 10.1 Web 应用用户管理

```cpp
#include "acl_cpp/lib_acl.hpp"

class UserManager {
private:
    acl::mysql_pool& pool_;
    
public:
    UserManager(acl::mysql_pool& pool) : pool_(pool) {}
    
    // 创建用户
    bool create_user(const char* name, const char* email, int age) {
        acl::db_guard guard(pool_);
        acl::db_handle* db = (acl::db_handle*)guard.peek();
        if (!db) return false;
        
        acl::query q;
        q.create("INSERT INTO users (name, email, age, created_at) "
                 "VALUES (:name, :email, :age, NOW())")
         .set_parameter("name", name)
         .set_parameter("email", email)
         .set_parameter("age", age);
        
        return db->exec_update(q);
    }
    
    // 查询用户
    bool get_user(int id, acl::db_rows& result) {
        acl::db_guard guard(pool_);
        acl::db_handle* db = (acl::db_handle*)guard.peek();
        if (!db) return false;
        
        acl::query q;
        q.create("SELECT * FROM users WHERE id = :id")
         .set_parameter("id", id);
        
        return db->exec_select(q, &result);
    }
    
    // 更新用户
    bool update_user(int id, const char* name, const char* email) {
        acl::db_guard guard(pool_);
        acl::db_handle* db = (acl::db_handle*)guard.peek();
        if (!db) return false;
        
        acl::query q;
        q.create("UPDATE users SET name = :name, email = :email "
                 "WHERE id = :id")
         .set_parameter("id", id)
         .set_parameter("name", name)
         .set_parameter("email", email);
        
        return db->exec_update(q);
    }
    
    // 删除用户
    bool delete_user(int id) {
        acl::db_guard guard(pool_);
        acl::db_handle* db = (acl::db_handle*)guard.peek();
        if (!db) return false;
        
        acl::query q;
        q.create("DELETE FROM users WHERE id = :id")
         .set_parameter("id", id);
        
        return db->exec_update(q);
    }
    
    // 列出所有用户
    bool list_users(acl::db_rows& result) {
        acl::db_guard guard(pool_);
        acl::db_handle* db = (acl::db_handle*)guard.peek();
        if (!db) return false;
        
        return db->sql_select("SELECT * FROM users ORDER BY id", &result);
    }
};

int main() {
    // 创建连接池
    acl::mysql_conf conf("127.0.0.1:3306", "testdb");
    conf.set_dbuser("root")
        .set_dbpass("password")
        .set_dblimit(64)
        .set_charset("utf8mb4");
    
    acl::mysql_pool pool(conf);
    
    // 创建用户管理器
    UserManager manager(pool);
    
    // 创建用户
    if (manager.create_user("张三", "zhangsan@example.com", 25)) {
        printf("用户创建成功\n");
    }
    
    // 查询用户
    acl::db_rows result;
    if (manager.get_user(1, result) && !result.empty()) {
        const acl::db_row* row = result[0];
        printf("用户信息: ID=%s, Name=%s, Email=%s, Age=%d\n",
            row->field_value("id"),
            row->field_value("name"),
            row->field_value("email"),
            row->field_int("age"));
    }
    
    // 更新用户
    manager.update_user(1, "张三", "zhangsan_new@example.com");
    
    // 列出所有用户
    if (manager.list_users(result)) {
        printf("共有 %d 个用户:\n", (int)result.length());
        for (size_t i = 0; i < result.length(); i++) {
            const acl::db_row* row = result[i];
            printf("  %s: %s (%s)\n",
                row->field_value("id"),
                row->field_value("name"),
                row->field_value("email"));
        }
    }
    
    // 删除用户
    manager.delete_user(1);
    
    return 0;
}
```

### 10.2 日志系统（SQLite）

```cpp
#include "acl_cpp/lib_acl.hpp"

class Logger {
private:
    acl::sqlite_pool& pool_;
    
public:
    Logger(acl::sqlite_pool& pool) : pool_(pool) {
        init_table();
    }
    
    void init_table() {
        acl::db_guard guard(pool_);
        acl::db_handle* db = (acl::db_handle*)guard.peek();
        if (!db) return;
        
        const char* sql = 
            "CREATE TABLE IF NOT EXISTS logs ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "level TEXT NOT NULL,"
            "message TEXT NOT NULL,"
            "created_at DATETIME DEFAULT CURRENT_TIMESTAMP)";
        
        db->sql_update(sql);
    }
    
    void log(const char* level, const char* message) {
        acl::db_guard guard(pool_);
        acl::db_handle* db = (acl::db_handle*)guard.peek();
        if (!db) return;
        
        acl::query q;
        q.create("INSERT INTO logs (level, message) VALUES (:level, :message)")
         .set_parameter("level", level)
         .set_parameter("message", message);
        
        db->exec_update(q);
    }
    
    void info(const char* message) { log("INFO", message); }
    void warn(const char* message) { log("WARN", message); }
    void error(const char* message) { log("ERROR", message); }
    
    void query_recent(int limit, acl::db_rows& result) {
        acl::db_guard guard(pool_);
        acl::db_handle* db = (acl::db_handle*)guard.peek();
        if (!db) return;
        
        acl::query q;
        q.create_sql("SELECT * FROM logs ORDER BY id DESC LIMIT %d", limit);
        
        db->exec_select(q, &result);
    }
};

int main() {
    // 创建连接池
    acl::sqlite_pool pool("./app.db", 10, "utf-8");
    
    // 创建日志器
    Logger logger(pool);
    
    // 记录日志
    logger.info("应用程序启动");
    logger.warn("这是一个警告");
    logger.error("发生了一个错误");
    
    // 查询最近的日志
    acl::db_rows result;
    logger.query_recent(10, result);
    
    printf("最近 %d 条日志:\n", (int)result.length());
    for (size_t i = 0; i < result.length(); i++) {
        const acl::db_row* row = result[i];
        printf("[%s] %s - %s\n",
            row->field_value("created_at"),
            row->field_value("level"),
            row->field_value("message"));
    }
    
    return 0;
}
```

## 11. 常见问题

### 11.1 连接失败

**问题**：无法连接到数据库

**解决方法**：
1. 检查地址和端口是否正确
2. 检查用户名和密码
3. 检查数据库服务是否启动
4. 检查防火墙设置
5. 增加连接超时时间

```cpp
acl::mysql_conf conf("127.0.0.1:3306", "testdb");
conf.set_dbuser("root")
    .set_dbpass("password")
    .set_conn_timeout(120);  // 增加超时时间

acl::db_mysql db(conf);
if (!db.open()) {
    printf("连接失败: %s (errno: %d)\n", db.get_error(), db.get_errno());
}
```

### 11.2 字符集问题

**问题**：中文显示乱码

**解决方法**：
1. 确保数据库、连接、应用程序使用相同字符集
2. 推荐使用 UTF-8 (MySQL 使用 utf8mb4)

```cpp
// 设置连接字符集
conf.set_charset("utf8mb4");

// 或在打开数据库时指定
db.dbopen("utf8mb4");
```

### 11.3 连接池耗尽

**问题**：`peek_open()` 返回 NULL

**解决方法**：
1. 增加连接池大小
2. 确保及时归还连接（使用 `db_guard`）
3. 检查是否有连接泄漏

```cpp
// 增大连接池
acl::mysql_pool pool("127.0.0.1:3306", "testdb", "root", "password", 200);

// 使用 db_guard 自动归还
{
    acl::db_guard guard(pool);
    // 使用连接...
}  // 自动归还
```

### 11.4 内存泄漏

**问题**：长时间运行后内存增长

**解决方法**：
1. 及时调用 `free_result()`
2. 使用外部 `db_rows` 对象
3. 使用 `db_guard` 管理连接

```cpp
// 方式一：及时释放
db.sql_select("SELECT * FROM users");
// 处理结果...
db.free_result();

// 方式二：使用外部对象
acl::db_rows result;
db.sql_select("SELECT * FROM users", &result);
// result 析构时自动释放
```

### 11.5 事务不生效

**问题**：rollback 后数据仍然被修改

**解决方法**：
创建连接时禁用自动提交

```cpp
acl::db_mysql db("127.0.0.1:3306", "testdb", "root", "password",
                 0, false);  // auto_commit = false

db.begin_transaction();
// 执行操作...
db.rollback();  // 现在会生效
```

### 11.6 SQLite 数据库锁定

**问题**：数据库被锁定，无法写入

**解决方法**：
1. 设置忙碌超时
2. 使用 WAL 模式
3. 减少连接池大小

```cpp
acl::db_sqlite db("./test.db");
db.open();

// 设置忙碌超时
db.set_busy_timeout(5000);

// 启用 WAL 模式
db.set_conf("PRAGMA journal_mode = WAL");
```

## 12. 参考资源

- ACL 项目主页：https://github.com/acl-dev/acl
- MySQL 文档：https://dev.mysql.com/doc/
- PostgreSQL 文档：https://www.postgresql.org/docs/
- SQLite 文档：https://www.sqlite.org/docs.html

## 13. 总结

ACL 数据库模块提供了：

1. **统一接口**：一套代码支持多种数据库
2. **连接池**：高性能连接管理
3. **安全查询**：防止 SQL 注入
4. **异步支持**：不阻塞主线程
5. **易于使用**：RAII 风格，链式调用

选择合适的使用方式：
- **简单应用**：直接使用 `db_mysql`/`db_pgsql`/`db_sqlite`
- **高并发**：使用连接池 `mysql_pool`/`pgsql_pool`/`sqlite_pool`
- **多数据库**：使用管理器 `mysql_manager`/`pgsql_manager`
- **异步处理**：使用 `db_service_mysql`/`db_service_sqlite`

记住最佳实践：
- 使用连接池
- 使用参数化查询（`query` 类）
- 使用 `db_guard` 自动管理连接
- 及时释放结果集
- 检查返回值并处理错误

祝您使用愉快！

