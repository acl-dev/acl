#include <stdio.h>
#include <string.h>

#include "lib_acl.h"

#define STR acl_vstring_str

/* 检查生成的 JSON 字符串是否包含期望子串 */
static int check_contains(const char *src, const char *needle)
{
	if (strstr(src, needle) != NULL)
		return 1;
	printf("FAIL: [%s] does not contain [%s]\n", src, needle);
	return 0;
}

/* 检查生成的 JSON 字符串不包含裸控制字符 */
static int check_no_raw_ctrl(const char *src)
{
	const unsigned char *p = (const unsigned char *) src;
	while (*p) {
		if (*p < 0x20) {
			printf("FAIL: raw control char 0x%02X found in: %s\n",
				*p, src);
			return 0;
		}
		p++;
	}
	return 1;
}

/*
 * 构建 {"ctrl":"<CR><LF><TAB><BS><FF><NULL><DEL_like>"} 并校验输出
 */
static int test_ctrl_chars_in_value(void)
{
	ACL_JSON *json = acl_json_alloc();
	ACL_VSTRING *buf = acl_vstring_alloc(256);
	const char *value = "\r\n\t\b\f\x01\x1F";
	const char *out;
	int ok;

	acl_json_node_add_child(json->root,
		acl_json_create_text(json, "ctrl", value));
	acl_json_build(json, buf);
	out = STR(buf);

	ok = check_no_raw_ctrl(out)
		&& check_contains(out, "\\r")
		&& check_contains(out, "\\n")
		&& check_contains(out, "\\t")
		&& check_contains(out, "\\b")
		&& check_contains(out, "\\f")
		&& check_contains(out, "\\u0001")
		&& check_contains(out, "\\u001F");

	acl_json_free(json);
	acl_vstring_free(buf);
	return ok;
}

/*
 * 控制字符在 key 里同样应被转义
 */
static int test_ctrl_chars_in_key(void)
{
	ACL_JSON *json = acl_json_alloc();
	ACL_VSTRING *buf = acl_vstring_alloc(256);
	/* 用字符串拼接阻止 \x02e 被 C 编译器解析为单个十六进制转义 (0x2E = '.') */
	const char *key = "k\x02" "ey";
	const char *out;
	int ok;

	acl_json_node_add_child(json->root,
		acl_json_create_text(json, key, "v"));
	acl_json_build(json, buf);
	out = STR(buf);

	ok = check_no_raw_ctrl(out)
		&& check_contains(out, "\\u0002")
		&& check_contains(out, "ey");

	acl_json_free(json);
	acl_vstring_free(buf);
	return ok;
}

/*
 * U+2028 / U+2029 应输出 \u2028 / \u2029
 */
static int test_line_terminators(void)
{
	ACL_JSON *json = acl_json_alloc();
	ACL_VSTRING *buf = acl_vstring_alloc(256);
	/* U+2028 = E2 80 A8,  U+2029 = E2 80 A9 */
	const char *value = "a\xE2\x80\xA8"  "b\xE2\x80\xA9" "c";
	const char *out;
	int ok;

	acl_json_node_add_child(json->root,
		acl_json_create_text(json, "ls", value));
	acl_json_build(json, buf);
	out = STR(buf);

	ok = check_contains(out, "\\u2028")
		&& check_contains(out, "\\u2029");

	acl_json_free(json);
	acl_vstring_free(buf);
	return ok;
}

int main(void)
{
	if (!test_ctrl_chars_in_value()) {
		printf("test_ctrl_chars_in_value failed\n");
		return 1;
	}

	if (!test_ctrl_chars_in_key()) {
		printf("test_ctrl_chars_in_key failed\n");
		return 1;
	}

	if (!test_line_terminators()) {
		printf("test_line_terminators failed\n");
		return 1;
	}

	printf("json escape checks passed\n");
	return 0;
}
