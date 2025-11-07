# ACL 数据库模块架构文档

## 1. 概述

ACL 数据库模块（`lib_acl_cpp/include/acl_cpp/db`）提供了一套统一的 C++ 数据库访问接口，支持 MySQL、PostgreSQL 和 SQLite 三种主流数据库。该模块采用分层设计，提供了从底层数据库操作到高层异步服务的完整解决方案。

## 2. 架构分层

```
┌─────────────────────────────────────────────────────────────┐
│                     应用层                                   │
├─────────────────────────────────────────────────────────────┤
│              异步服务层 (Async Service Layer)                │
│  - db_service (基类)                                         │
│  - db_service_mysql, db_service_sqlite                      │
│  - db_query (异步回调接口)                                   │
├─────────────────────────────────────────────────────────────┤
│              连接池管理层 (Connection Pool Layer)            │
│  - mysql_manager, pgsql_manager, sqlite_manager             │
│  - mysql_pool, pgsql_pool, sqlite_pool                      │
│  - db_pool (基类)                                            │
│  - db_guard (连接池守护)                                     │
├─────────────────────────────────────────────────────────────┤
│              配置层 (Configuration Layer)                     │
│  - mysql_conf, pgsql_conf                                   │
├─────────────────────────────────────────────────────────────┤
│              数据库抽象层 (Database Abstraction Layer)        │
│  - db_handle (抽象基类)                                      │
│  - db_mysql, db_pgsql, db_sqlite (具体实现)                 │
│  - query (SQL 安全构建器)                                    │
├─────────────────────────────────────────────────────────────┤
│              数据结构层 (Data Structure Layer)                │
│  - db_rows (结果集容器)                                      │
│  - db_row (行数据)                                           │
│  - db_cursor (游标基类)                                      │
│  - sqlite_cursor (SQLite 游标)                              │
├─────────────────────────────────────────────────────────────┤
│              底层驱动层 (Driver Layer)                        │
│  - MySQL Client Library (libmysqlclient)                    │
│  - PostgreSQL Client Library (libpq)                        │
│  - SQLite Library (libsqlite3)                              │
└─────────────────────────────────────────────────────────────┘
```

## 3. 核心组件详解

### 3.1 数据结构层

#### 3.1.1 db_row
- **功能**：封装查询结果中的单行记录
- **主要方法**：
  - `field_value()`: 按字段名或索引获取字段值
  - `field_int()`, `field_int64()`: 获取整型字段值
  - `field_double()`: 获取浮点型字段值
  - `field_string()`: 获取字符串字段值
  - `field_length()`: 获取字段值长度
- **设计特点**：
  - 支持按字段名和索引两种方式访问
  - 提供类型安全的数据获取方法
  - 支持空值处理（可指定默认值）

#### 3.1.2 db_rows
- **功能**：封装查询结果集合
- **主要方法**：
  - `get_rows()`: 获取所有行数据
  - `get_rows(name, value)`: 按条件过滤行数据
  - `operator[]`: 按索引访问行
  - `empty()`, `length()`: 判断结果集状态
- **设计特点**：
  - 内部使用 `std::vector` 存储行数据
  - 支持条件过滤功能
  - 自动管理内存生命周期

#### 3.1.3 db_cursor
- **功能**：游标基类，用于流式查询
- **实现**：
  - `sqlite_cursor`: SQLite 专用游标，支持预编译语句

### 3.2 数据库抽象层

#### 3.2.1 db_handle（核心抽象基类）
- **功能**：定义统一的数据库操作接口
- **继承自**：`connect_client`（连接池客户端）
- **核心虚函数**：
  ```cpp
  virtual bool dbopen(const char* charset = NULL) = 0;
  virtual bool is_opened() const = 0;
  virtual bool close() = 0;
  virtual bool tbl_exists(const char* tbl_name) = 0;
  virtual bool sql_select(const char* sql, db_rows* result = NULL) = 0;
  virtual bool sql_update(const char* sql) = 0;
  virtual int affect_count() const = 0;
  ```
- **事务支持**：
  ```cpp
  virtual bool begin_transaction();
  virtual bool commit();
  virtual bool rollback();
  ```
- **安全特性**：
  - `escape_string()`: SQL 注入防护
  - `exec_select()`, `exec_update()`: 使用 query 对象的安全执行方法
- **结果管理**：
  - `get_result()`: 获取查询结果
  - `get_first_row()`: 获取第一行（常用于单行查询）
  - `free_result()`: 释放结果集内存

#### 3.2.2 query（SQL 安全构建器）
- **功能**：构建参数化的安全 SQL 查询
- **设计理念**：类似于 Java Hibernate 的参数化查询
- **核心方法**：
  - `create()`, `create_sql()`: 创建 SQL 模板
  - `set_parameter()`: 设置参数（支持多种类型）
  - `set_date()`: 设置日期时间参数
  - `set_format()`: 格式化参数
  - `to_string()`: 生成最终 SQL 语句
  - `escape()`: 静态转义方法
- **使用示例**：
  ```cpp
  query q;
  q.create("SELECT * FROM users WHERE name = :name AND age >= :age")
   .set_parameter("name", "张三")
   .set_parameter("age", 18);
  db_handle->exec_select(q, &result);
  ```
- **安全特性**：
  - 自动转义特殊字符
  - 防止 SQL 注入攻击
  - 类型安全的参数绑定

#### 3.2.3 数据库具体实现

##### db_mysql
- **支持特性**：
  - TCP 和 UNIX 域套接字连接
  - 字符集设置
  - 连接超时和读写超时
  - 事务支持
  - 自动提交控制
- **动态加载**：支持运行时动态加载 libmysqlclient.so
- **配置参数**：
  - `dbaddr`: 数据库地址（ip:port 或 unix socket 路径）
  - `dbname`: 数据库名
  - `dbuser`, `dbpass`: 认证信息
  - `dbflags`: MySQL 标志位
  - `auto_commit`: 自动提交设置
  - `conn_timeout`, `rw_timeout`: 超时设置
  - `charset`: 字符集

##### db_pgsql
- **支持特性**：
  - PostgreSQL 协议支持
  - 事务支持
  - 参数化查询
- **动态加载**：支持运行时动态加载 libpq.so
- **配置**：通过 `pgsql_conf` 配置

##### db_sqlite
- **支持特性**：
  - 文件数据库
  - 字符集转换（支持 UTF-8、GBK 等）
  - PRAGMA 配置支持
  - 预编译语句（通过 sqlite_cursor）
  - 事务支持
- **特色功能**：
  - `set_conf()`: 设置 PRAGMA 配置
  - `get_conf()`: 获取配置值
  - `show_conf()`: 显示配置
  - `prepare()`, `next()`: 游标操作
  - `sqlite3_*()`: 封装的 SQLite3 原生接口
- **动态加载**：支持运行时动态加载 libsqlite3.so

### 3.3 配置层

#### 3.3.1 mysql_conf
- **功能**：MySQL 数据库配置封装
- **配置项**：
  - 基本连接：dbaddr, dbname, dbuser, dbpass
  - 连接池：dblimit（连接池大小）
  - 性能：conn_timeout, rw_timeout
  - 行为：auto_commit, dbflags
  - 字符集：charset
- **设计特点**：
  - 链式调用风格
  - 拷贝构造支持
  - 生成 dbkey（dbname@dbaddr）用于唯一标识

#### 3.3.2 pgsql_conf
- **功能**：PostgreSQL 数据库配置封装
- **特殊说明**：
  - UNIX 域套接字配置：只需提供目录路径，不需要完整文件路径
  - 例如：PostgreSQL 的 socket 文件为 `/tmp/.s.PGSQL.5432`，只需设置 dbaddr 为 `/tmp`

### 3.4 连接池管理层

#### 3.4.1 db_pool
- **功能**：数据库连接池基类
- **继承自**：`connect_pool`
- **核心方法**：
  - `peek_open()`: 从连接池获取一个已打开的数据库连接
  - `get_dblimit()`: 获取连接池最大连接数
  - `get_dbcount()`: 获取当前连接数
  - `set_idle()`: 设置空闲连接超时时间
- **生命周期管理**：
  - 连接从池中获取后，使用完毕应调用 `pool->put(db_handle*)` 归还
  - 或者使用 `db_guard` 自动管理

#### 3.4.2 具体连接池实现

##### mysql_pool
- **构造方式**：
  1. 直接传递参数
  2. 使用 `mysql_conf` 配置对象
- **实现**：重写 `create_connect()` 创建 `db_mysql` 实例

##### pgsql_pool
- **构造方式**：使用 `pgsql_conf` 配置对象
- **实现**：重写 `create_connect()` 创建 `db_pgsql` 实例

##### sqlite_pool
- **构造方式**：传递数据库文件路径
- **实现**：重写 `create_connect()` 创建 `db_sqlite` 实例
- **特点**：SQLite 为文件数据库，多连接共享同一文件

#### 3.4.3 db_guard
- **功能**：连接池守护类，RAII 风格
- **继承自**：`connect_guard`
- **用途**：自动管理数据库连接的获取和归还
- **使用示例**：
  ```cpp
  db_guard guard(db_pool);
  db_handle* db = (db_handle*) guard.peek();
  // 使用 db...
  // 析构时自动归还连接
  ```

#### 3.4.4 连接池管理器

##### mysql_manager
- **功能**：管理多个 MySQL 数据库连接池
- **继承自**：`connect_manager`
- **核心方法**：
  - `add()`: 添加数据库实例（支持两种方式）
    1. 直接传递参数
    2. 使用 `mysql_conf` 对象
- **实现**：
  - 重写 `create_pool()` 创建 `mysql_pool`
  - 内部使用 `std::map` 管理多个配置
- **空闲管理**：支持设置连接空闲超时时间

##### pgsql_manager、sqlite_manager
- **功能**：类似 `mysql_manager`，分别管理 PostgreSQL 和 SQLite 连接池

### 3.5 异步服务层

#### 3.5.1 db_service
- **功能**：异步数据库服务基类
- **继承自**：`ipc_service`（进程间通信服务）
- **设计模式**：
  - 后台线程池执行数据库操作
  - 异步回调通知结果
  - 支持 Windows GUI 消息循环和通用 Socket 通信
- **核心方法**：
  - `sql_select()`: 异步执行查询
  - `sql_update()`: 异步执行更新
  - `push_back()`: 添加数据库连接到连接池
- **虚函数**：
  - `db_create()`: 子类实现，创建具体数据库连接
  - `on_accept()`: 接受客户端连接
  - `win32_proc()`: Windows 消息处理（仅 Win32）
- **内部结构**：
  - `dbpool_`: 数据库连接池
  - `dblimit_`: 连接池大小限制
  - `dbsize_`: 当前连接数

#### 3.5.2 db_query
- **功能**：异步查询回调接口
- **核心方法**：
  - `on_error()`: 错误回调
  - `on_ok()`: 成功回调
  - `destroy()`: 清理回调（可用于 delete this）
- **状态枚举**：
  - `DB_OK`: 成功
  - `DB_ERR_OPEN`: 打开数据库失败
  - `DB_ERR_EXEC_SQL`: 执行 SQL 失败

#### 3.5.3 具体异步服务实现

##### db_service_mysql
- **功能**：MySQL 异步数据库服务
- **实现**：
  - 重写 `db_create()` 创建 `db_mysql` 实例
  - 封装 MySQL 连接参数

##### db_service_sqlite
- **功能**：SQLite 异步数据库服务
- **实现**：
  - 重写 `db_create()` 创建 `db_sqlite` 实例
  - 封装 SQLite 文件路径

## 4. 设计模式与原则

### 4.1 使用的设计模式

1. **抽象工厂模式**
   - `db_handle` 作为抽象产品
   - `db_mysql`, `db_pgsql`, `db_sqlite` 作为具体产品
   - 各连接池的 `create_connect()` 方法作为工厂方法

2. **模板方法模式**
   - `db_handle` 定义骨架算法
   - 子类实现具体步骤

3. **策略模式**
   - 不同数据库实现不同的访问策略
   - 通过多态实现策略切换

4. **对象池模式**
   - `db_pool` 及其子类实现连接池
   - 减少连接创建开销，提高性能

5. **RAII 模式**
   - `db_guard` 自动管理连接生命周期
   - `db_rows` 自动管理结果集内存

6. **观察者模式**
   - `db_service` + `db_query` 实现异步回调
   - 分离数据库操作和结果处理

### 4.2 设计原则

1. **单一职责原则**
   - 每个类专注于单一功能
   - 配置、连接、查询、结果分别封装

2. **开闭原则**
   - 对扩展开放：易于添加新数据库支持
   - 对修改封闭：基类接口稳定

3. **里氏替换原则**
   - 所有 `db_handle` 子类可互相替换
   - 多态调用保证一致性

4. **接口隔离原则**
   - 分离同步接口和异步接口
   - 分离基础操作和高级特性

5. **依赖倒置原则**
   - 上层依赖抽象 `db_handle`
   - 不依赖具体实现

## 5. 关键技术特性

### 5.1 动态库加载
- 支持运行时动态加载数据库客户端库
- `db_mysql::load()`, `db_pgsql::load()`, `db_sqlite::load()`
- 可通过 `db_handle::set_loadpath()` 设置库路径
- 优点：
  - 减少编译依赖
  - 灵活部署
  - 按需加载

### 5.2 连接池管理
- 预创建连接，减少连接开销
- 自动管理连接生命周期
- 支持空闲连接超时
- 支持连接健康检查
- 线程安全

### 5.3 SQL 注入防护
- `query` 类参数化查询
- `db_handle::escape_string()` 转义特殊字符
- 强制使用安全接口

### 5.4 事务支持
- 统一的事务接口
- `begin_transaction()`, `commit()`, `rollback()`
- 支持自动提交控制

### 5.5 字符集支持
- 灵活的字符集设置
- 自动字符集转换（SQLite）
- 支持 UTF-8、GBK 等常用字符集

### 5.6 错误处理
- `get_errno()`: 获取错误码
- `get_error()`: 获取错误描述
- 统一的错误处理接口

### 5.7 跨平台支持
- 支持 Linux、Windows、macOS
- 统一的接口，屏蔽平台差异
- Windows 特殊支持：GUI 消息循环集成

## 6. 内存管理

### 6.1 结果集内存管理
- `db_rows` 自动管理行数据内存
- 析构时自动释放
- `free_result()` 手动释放
- 重复查询时自动清理旧结果

### 6.2 连接内存管理
- 连接池统一管理连接对象
- 使用 `db_guard` 或手动 `put()` 归还
- 析构时自动关闭连接

### 6.3 配置对象管理
- 配置对象支持拷贝构造
- 内部深拷贝字符串数据
- 析构时自动释放

## 7. 线程安全

### 7.1 连接池线程安全
- 内部使用锁保护共享资源
- 支持多线程并发获取连接
- 连接本身非线程安全，需独占使用

### 7.2 异步服务线程模型
- 后台线程池执行数据库操作
- 回调在主线程或指定线程执行
- 避免阻塞主线程

### 7.3 使用建议
- 每个线程使用独立的数据库连接
- 或使用连接池，每次获取独立连接
- 不要跨线程共享 `db_handle` 对象

## 8. 性能优化

### 8.1 连接池优化
- 预创建连接，减少连接开销
- 连接复用，避免频繁创建/销毁
- 空闲连接自动回收

### 8.2 查询优化
- 支持预编译语句（SQLite）
- 结果集按需加载
- 减少内存拷贝

### 8.3 异步处理
- 数据库操作不阻塞主线程
- 后台线程池并发处理
- 提高整体吞吐量

## 9. 扩展性

### 9.1 添加新数据库支持
1. 继承 `db_handle`
2. 实现所有纯虚函数
3. 创建对应的配置类
4. 创建对应的连接池类
5. 可选：创建异步服务类

### 9.2 自定义连接池策略
- 继承 `db_pool`
- 重写 `create_connect()`
- 实现自定义连接管理逻辑

### 9.3 自定义异步服务
- 继承 `db_service`
- 实现 `db_create()`
- 可扩展 `on_accept()` 实现自定义通信协议

## 10. 编译选项

### 10.1 ACL_DB_DISABLE
- 定义此宏可完全禁用数据库模块
- 减小编译后的库大小
- 适用于不需要数据库功能的场景

### 10.2 ACL_CLIENT_ONLY
- 定义此宏禁用服务端功能（包括 MySQL/PostgreSQL）
- 只保留 SQLite 支持
- 适用于客户端或嵌入式场景

## 11. 依赖关系

```
db_service_mysql ──> db_service ──> ipc_service
                 └─> db_mysql   ──> db_handle ──> connect_client
                                               └─> db_rows ──> db_row

mysql_pool ──────> db_pool ──────> connect_pool
            └───> mysql_conf

mysql_manager ───> connect_manager
              └─> mysql_pool

query ───────────> (independent utility)

sqlite_cursor ───> db_cursor
              └─> query
```

## 12. 总结

ACL 数据库模块具有以下显著特点：

1. **统一抽象**：提供统一的接口访问不同数据库
2. **分层清晰**：从底层驱动到高层服务，职责明确
3. **高性能**：连接池、异步处理、预编译语句
4. **安全性**：SQL 注入防护、事务支持、错误处理
5. **易用性**：RAII 风格、链式调用、丰富的工具类
6. **灵活性**：动态加载、配置灵活、易于扩展
7. **跨平台**：支持主流操作系统和数据库

该模块适用于从简单的同步查询到复杂的异步服务等各种场景，是构建高性能数据库应用的理想选择。

