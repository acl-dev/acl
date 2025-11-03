# ACL 数据库模块文档

欢迎使用 ACL 数据库模块！本目录包含了完整的架构文档和使用指南。

## 📚 文档列表

### [架构文档 (architecture.md)](./architecture.md)
深入了解 ACL 数据库模块的设计架构，包括：
- 模块分层设计
- 核心组件详解
- 设计模式与原则
- 技术特性说明
- 扩展性指南

**适合人群**：想要深入理解模块设计、需要扩展功能或贡献代码的开发者

### [使用文档 (usage.md)](./usage.md)
详细的使用指南和示例代码，包括：
- 快速入门教程
- MySQL/PostgreSQL/SQLite 使用方法
- 安全查询（防SQL注入）
- 连接池使用
- 异步服务使用
- 最佳实践
- 完整示例代码
- 常见问题解答

**适合人群**：使用 ACL 数据库模块的应用开发者

## 🚀 快速开始

### 1. 最简单的例子

```cpp
#include "acl_cpp/lib_acl.hpp"

int main() {
    // 创建数据库连接
    acl::db_mysql db("127.0.0.1:3306", "testdb", "root", "password");
    
    // 打开连接
    if (!db.open()) {
        printf("连接失败: %s\n", db.get_error());
        return 1;
    }
    
    // 执行查询
    if (db.sql_select("SELECT * FROM users")) {
        printf("查询到 %d 条记录\n", (int)db.length());
        
        // 遍历结果
        for (size_t i = 0; i < db.length(); i++) {
            const acl::db_row* row = db[i];
            printf("Name: %s\n", row->field_value("name"));
        }
        
        db.free_result();
    }
    
    return 0;
}
```

### 2. 使用连接池（推荐）

```cpp
#include "acl_cpp/lib_acl.hpp"

int main() {
    // 创建连接池（最大64个连接）
    acl::mysql_pool pool("127.0.0.1:3306", "testdb", "root", "password", 64);
    
    // 使用 RAII 方式获取连接
    {
        acl::db_guard guard(pool);
        acl::db_handle* db = (acl::db_handle*)guard.peek();
        
        if (db && db->sql_select("SELECT * FROM users")) {
            printf("查询到 %d 条记录\n", (int)db->length());
        }
        // 离开作用域自动归还连接
    }
    
    return 0;
}
```

### 3. 安全查询（防止SQL注入）

```cpp
#include "acl_cpp/lib_acl.hpp"

int main() {
    acl::db_mysql db("127.0.0.1:3306", "testdb", "root", "password");
    db.open();
    
    // 使用参数化查询
    acl::query q;
    q.create("SELECT * FROM users WHERE name = :name AND age >= :age")
     .set_parameter("name", user_input_name)  // 自动转义
     .set_parameter("age", 18);
    
    acl::db_rows result;
    if (db.exec_select(q, &result)) {
        printf("查询到 %d 条记录\n", (int)result.length());
    }
    
    return 0;
}
```

## 🎯 核心特性

### 多数据库支持
- ✅ **MySQL** - 最流行的开源关系型数据库
- ✅ **PostgreSQL** - 强大的企业级数据库
- ✅ **SQLite** - 轻量级嵌入式数据库

### 统一接口
所有数据库使用相同的 API，轻松切换不同数据库：
```cpp
acl::db_handle* db;

// MySQL
db = new acl::db_mysql("127.0.0.1:3306", "testdb", "root", "password");

// PostgreSQL
acl::pgsql_conf conf("127.0.0.1:5432", "testdb");
conf.set_dbuser("postgres").set_dbpass("password");
db = new acl::db_pgsql(conf);

// SQLite
db = new acl::db_sqlite("./test.db", "utf-8");

// 统一的使用方式
db->open();
db->sql_select("SELECT * FROM users");
```

### 连接池管理
- 高性能连接复用
- 自动生命周期管理
- 支持空闲连接回收
- 线程安全

### SQL 注入防护
- `query` 类参数化查询
- 自动转义特殊字符
- 类型安全的参数绑定

### 异步服务支持
- 后台线程池执行
- 不阻塞主线程
- 回调通知结果
- 支持 Windows GUI 集成

### 事务支持
```cpp
db.begin_transaction();
db.sql_update("UPDATE accounts SET balance=balance-100 WHERE id=1");
db.sql_update("UPDATE accounts SET balance=balance+100 WHERE id=2");
db.commit();  // 或 db.rollback()
```

### 动态库加载
运行时动态加载数据库客户端库，无需编译时依赖：
```cpp
acl::db_mysql::load();   // 加载 libmysqlclient.so
acl::db_pgsql::load();   // 加载 libpq.so
acl::db_sqlite::load();  // 加载 libsqlite3.so
```

## 📖 学习路径

### 初学者
1. 阅读 [使用文档](./usage.md) 的"快速入门"部分
2. 运行简单示例
3. 学习连接池使用
4. 掌握安全查询方法

### 进阶用户
1. 学习异步服务使用
2. 了解事务处理
3. 掌握性能优化技巧
4. 阅读最佳实践

### 高级开发者
1. 阅读 [架构文档](./architecture.md)
2. 了解模块设计原理
3. 学习如何扩展新数据库支持
4. 参与贡献代码

## 🔗 相关资源

### ACL 项目
- **GitHub**: https://github.com/acl-dev/acl
- **Gitee**: https://gitee.com/acl-dev/acl

### 其他 ACL 文档
- [Fiber 协程库](../fiber/)
- [HTTP 模块](../http/)
- [Redis 客户端](../redis/)
- [连接池](../connpool/)
- [Master 框架](../master/)

### 数据库官方文档
- [MySQL Documentation](https://dev.mysql.com/doc/)
- [PostgreSQL Documentation](https://www.postgresql.org/docs/)
- [SQLite Documentation](https://www.sqlite.org/docs.html)

## 🤝 贡献

欢迎提交问题报告和改进建议！

- 问题反馈：[GitHub Issues](https://github.com/acl-dev/acl/issues)
- 功能建议：[GitHub Discussions](https://github.com/acl-dev/acl/discussions)
- 代码贡献：[Pull Requests](https://github.com/acl-dev/acl/pulls)

## 📝 版本历史

### 当前版本
- 支持 MySQL、PostgreSQL、SQLite
- 完整的连接池管理
- 异步服务支持
- SQL 注入防护
- 事务支持
- 动态库加载

## 📄 许可证

ACL 项目使用 Apache License 2.0 许可证。详见项目根目录的 LICENSE 文件。

## 💡 常见使用场景

### Web 应用
使用连接池处理大量并发请求：
```cpp
acl::mysql_pool pool("127.0.0.1:3306", "webapp_db", "root", "password", 100);

void handle_request(HttpRequest& req) {
    acl::db_guard guard(pool);
    acl::db_handle* db = (acl::db_handle*)guard.peek();
    // 处理数据库操作...
}
```

### 桌面应用
使用 SQLite 作为本地数据存储：
```cpp
acl::db_sqlite db("./app_data.db");
db.open();
db.sql_update("CREATE TABLE IF NOT EXISTS settings (key TEXT, value TEXT)");
```

### 高性能服务
使用异步服务实现非阻塞数据库操作：
```cpp
acl::db_service_mysql service("127.0.0.1:3306", "testdb", "root", "password",
                               0, true, 60, 60, 100, 4);  // 4个后台线程

class MyQuery : public acl::db_query {
    virtual void on_ok(const acl::db_rows* rows, int affected) {
        // 处理结果...
    }
};

service.sql_select("SELECT * FROM users", new MyQuery());
```

### 数据分析
批量处理大量数据：
```cpp
acl::db_mysql db("127.0.0.1:3306", "analytics_db", "root", "password");
db.open();

db.begin_transaction();
for (int i = 0; i < 10000; i++) {
    acl::query q;
    q.create("INSERT INTO events (type, data) VALUES (:type, :data)")
     .set_parameter("type", "page_view")
     .set_parameter("data", event_data[i]);
    db.exec_update(q);
}
db.commit();
```

## 📞 获取帮助

- 📧 邮件列表：acl-dev@googlegroups.com
- 💬 QQ群：242722074
- 📖 查看 [使用文档](./usage.md) 的"常见问题"部分
- 🔍 搜索 [GitHub Issues](https://github.com/acl-dev/acl/issues)

---

**开始使用**: 建议先阅读 [使用文档](./usage.md)，快速上手 ACL 数据库模块！

**深入学习**: 阅读 [架构文档](./architecture.md)，了解模块设计和高级特性！

