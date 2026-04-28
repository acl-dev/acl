include ./Makefile.in

# 定义要编译的测试程序
PROGRAMS = json_check json_escape_check

# 使用最新编译的 JSON 模块对象文件（包含 Unicode 和控制字符转义支持）
JSON_OBJS = /tmp/acl_json.o /tmp/acl_json_parse.o /tmp/acl_json_util.o

# 扩展链接标志，包含修改过的 JSON 对象文件
LDFLAGS := $(JSON_OBJS) $(LDFLAGS)

