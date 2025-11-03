# ACL Fiber C++ 文档索引

## 文档概览

本目录包含了 ACL Fiber C++ 库的完整文档，涵盖了从入门到高级的所有内容。

## 📚 文档列表

### 入门文档

1. **[README.md](README.md)** - 总览和快速入门
   - 库概述和主要特性
   - 快速开始示例
   - 编译和链接说明
   - 性能优化建议

2. **[quick_reference.md](quick_reference.md)** - 快速参考手册
   - 常用 API 速查
   - 代码片段示例
   - 常见模式
   - 调试检查清单

3. **[examples.md](examples.md)** - 完整示例代码
   - 10+ 个可运行的完整示例
   - 从 Hello World 到复杂应用
   - 包含编译和运行说明

### 核心概念

4. **[fiber_class.md](fiber_class.md)** - 协程基类详解
   - fiber 类的完整 API
   - 协程生命周期管理
   - 调度器使用
   - 错误处理和调试

5. **[go_fiber.md](go_fiber.md)** - Go 风格协程启动
   - `go` 宏的使用
   - Lambda 表达式和闭包
   - 与传统方式的对比
   - 最佳实践

### 同步机制

6. **[synchronization.md](synchronization.md)** - 同步原语详解
   - `fiber_mutex` - 互斥锁
   - `fiber_lock` - 轻量级锁
   - `fiber_rwlock` - 读写锁
   - `fiber_cond` - 条件变量
   - `fiber_sem` - 信号量
   - `fiber_event` - 事件锁
   - 性能对比和选择建议

### 通信机制

7. **[channel.md](channel.md)** - 协程间通道通信
   - channel 基本用法
   - 生产者-消费者模式
   - Pipeline 模式
   - Fan-out/Fan-in 模式
   - 注意事项和最佳实践

8. **[fiber_tbox.md](fiber_tbox.md)** - 消息队列
   - `fiber_tbox` - 指针消息队列
   - `fiber_tbox2` - 值消息队列
   - `fiber_sbox` - 基于信号量的消息队列
   - `fiber_sbox2` - 基于信号量的值消息队列
   - 各类型对比和选择

### 协程管理

9. **[wait_group.md](wait_group.md)** - 协程等待组
   - wait_group 用法
   - 并发控制
   - 错误处理
   - 嵌套使用
   - 配合协程池使用

10. **[fiber_pool.md](fiber_pool.md)** - 协程池
    - 协程池配置和使用
    - 任务提交和管理
    - 动态调整
    - 高级应用场景
    - 性能优化

### 架构和参考

11. **[class_hierarchy.md](class_hierarchy.md)** - 类层次结构
    - 完整的类继承关系
    - 组件架构图
    - 文件组织结构
    - 依赖关系
    - 设计模式

## 📖 文档导航

### 按学习路径

#### 初学者路径
1. [README.md](README.md) - 了解基础概念
2. [examples.md](examples.md) - 运行 Hello World
3. [go_fiber.md](go_fiber.md) - 学习协程启动
4. [quick_reference.md](quick_reference.md) - 查阅常用 API

#### 进阶路径
1. [fiber_class.md](fiber_class.md) - 深入理解协程
2. [synchronization.md](synchronization.md) - 掌握同步机制
3. [channel.md](channel.md) - 学习协程通信
4. [wait_group.md](wait_group.md) - 协程同步

#### 高级路径
1. [fiber_pool.md](fiber_pool.md) - 协程池应用
2. [fiber_tbox.md](fiber_tbox.md) - 高级通信机制
3. [class_hierarchy.md](class_hierarchy.md) - 理解架构设计

### 按功能分类

#### 协程基础
- [fiber_class.md](fiber_class.md)
- [go_fiber.md](go_fiber.md)

#### 同步与通信
- [synchronization.md](synchronization.md)
- [channel.md](channel.md)
- [fiber_tbox.md](fiber_tbox.md)

#### 管理与优化
- [wait_group.md](wait_group.md)
- [fiber_pool.md](fiber_pool.md)

#### 参考与示例
- [quick_reference.md](quick_reference.md)
- [examples.md](examples.md)
- [class_hierarchy.md](class_hierarchy.md)

## 🔍 快速查找

### 我想学习如何...

#### 创建和启动协程
- → [go_fiber.md](go_fiber.md)
- → [fiber_class.md](fiber_class.md)

#### 在协程间通信
- → [channel.md](channel.md)
- → [fiber_tbox.md](fiber_tbox.md)

#### 同步多个协程
- → [synchronization.md](synchronization.md)
- → [wait_group.md](wait_group.md)

#### 管理协程池
- → [fiber_pool.md](fiber_pool.md)

#### 查看完整示例
- → [examples.md](examples.md)

#### 快速查找 API
- → [quick_reference.md](quick_reference.md)

#### 理解整体架构
- → [class_hierarchy.md](class_hierarchy.md)
- → [README.md](README.md)

## 📊 文档统计

| 文档 | 类型 | 页数估计 | 难度 |
|------|------|----------|------|
| README.md | 概述 | 8-10 | ⭐ |
| quick_reference.md | 参考 | 6-8 | ⭐⭐ |
| examples.md | 示例 | 15-20 | ⭐ |
| fiber_class.md | 详解 | 12-15 | ⭐⭐⭐ |
| go_fiber.md | 详解 | 10-12 | ⭐⭐ |
| synchronization.md | 详解 | 20-25 | ⭐⭐⭐ |
| channel.md | 详解 | 8-10 | ⭐⭐ |
| fiber_tbox.md | 详解 | 12-15 | ⭐⭐⭐ |
| wait_group.md | 详解 | 10-12 | ⭐⭐ |
| fiber_pool.md | 详解 | 15-18 | ⭐⭐⭐ |
| class_hierarchy.md | 参考 | 8-10 | ⭐⭐ |

**总计**: 约 120-150 页

## 🎯 学习建议

### 第一天：基础入门
- 阅读 [README.md](README.md)
- 运行 [examples.md](examples.md) 中的前 3 个示例
- 浏览 [quick_reference.md](quick_reference.md)

**学习目标**: 能够创建和运行简单的协程程序

### 第二天：协程通信
- 学习 [go_fiber.md](go_fiber.md)
- 学习 [channel.md](channel.md)
- 运行通信相关示例

**学习目标**: 掌握协程间通信的基本方法

### 第三天：同步机制
- 学习 [synchronization.md](synchronization.md)
- 学习 [wait_group.md](wait_group.md)
- 实践生产者-消费者模式

**学习目标**: 掌握协程同步的各种方法

### 第四天：高级特性
- 学习 [fiber_pool.md](fiber_pool.md)
- 学习 [fiber_tbox.md](fiber_tbox.md)
- 运行复杂示例

**学习目标**: 能够构建生产级的协程应用

### 第五天：深入理解
- 学习 [fiber_class.md](fiber_class.md)
- 学习 [class_hierarchy.md](class_hierarchy.md)
- 性能优化和调试

**学习目标**: 深入理解 ACL Fiber 的设计和实现

## 💡 使用技巧

### 查找信息
1. **快速查找 API**: 使用 [quick_reference.md](quick_reference.md)
2. **查看示例**: 在 [examples.md](examples.md) 中搜索关键词
3. **深入学习**: 阅读对应的详解文档

### 学习方法
1. **边学边练**: 每学一个概念就运行相关示例
2. **修改示例**: 在示例基础上进行修改实验
3. **构建项目**: 用学到的知识构建实际项目

### 遇到问题时
1. 查看 [quick_reference.md](quick_reference.md) 的调试检查清单
2. 阅读相关详解文档的"注意事项"部分
3. 参考 [examples.md](examples.md) 中的类似场景

## 🔗 相关资源

### ACL 项目
- [ACL 项目主页](https://github.com/acl-dev/acl)
- [在线文档](https://acl-dev.github.io/acl/)

### 问题反馈
- [GitHub Issues](https://github.com/acl-dev/acl/issues)

### 其他文档
- [ACL C++ 库文档](../stream/)
- [MIME 库文档](../mime/)

## 📝 文档更新记录

| 日期 | 版本 | 说明 |
|------|------|------|
| 2025-11-03 | 1.0 | 初始版本，包含完整文档集 |

## 🤝 贡献

如果您发现文档中的错误或有改进建议，欢迎：
1. 提交 Issue
2. 提交 Pull Request
3. 联系项目维护者

## 📄 许可证

本文档采用与 ACL 项目相同的 Apache License 2.0 许可证。

---

**祝您学习愉快！**

如有任何问题，请随时查阅相关文档或联系社区。

