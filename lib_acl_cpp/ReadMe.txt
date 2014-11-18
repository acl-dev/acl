1) 编译
1.1) WIN32 平台：可以用 vc2003 或 vc2010 打开工程文件进行编译，编译后的 lib_acl.lib 库被拷贝至
lib/ 目录下，如果需要编译 samples/ 下的例子，还需要将 acl_project 的编译后的库
lib_acl_d.dll/lib_acl_d.lib, lib_acl.dll/lib_acl.lib, lib_acl_vc2003d.lib, lib_acl_vc2003.lib,
lib_acl_vc2010d.lib, lib_acl_vc2010.lib 以及 lib_protocol_d.dll/lib_protocol_d.lib,
lib_protocol.dll/lib_protocol.lib, lib_protocol_vc2003d.lib, lib_protocol_vc2003.lib,
lib_protocol_vc2010d.lib, lib_protocol_vc2010.lib 拷贝至 lib/ 目录下，另外，如果需要编译
与压缩、数据库操作的示例，则还需要将 mysql, sqlite3 以及 zlib 的库拷贝至 lib/ 目录下，如果
需要编译 ssl 相关的示例，需要将 polarssl 的库拷贝至 lib/ 目录下.

1.2) Linux 平台：直接运行 make 便可在 lib/ 目录下生成 lib_acl.a 库，如果需要生成动态库，则需要运行
make rpath=xxx, 其中 xxx 代表 mysql, sqlite, polarssl 库所在的路径

2) 依赖关系
2.1) acl: 必须是 acl.2.1.2.8.src.2012.8.1.tgz 以后的版本, https://sourceforge.net/projects/acl/
2.2) zlib: http://zlib.net/
2.3) polarssl: http://polarssl.org/
2.4) libmysqlcient
