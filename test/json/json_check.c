#include <stdio.h>
#include <string.h>

#include "lib_acl.h"

#define STR acl_vstring_str

#define UTF8_NI_HAO "\xE4\xBD\xA0\xE5\xA5\xBD"
#define UTF8_EMOJI "\xF0\x9F\x98\x80"
#define UTF8_KEY_NI "key\xE4\xBD\xA0"
#define UTF8_V_HAO "v\xE5\xA5\xBD"
#define UTF8_REPLACEMENT "\xEF\xBF\xBD"

static int check_tag_value(ACL_JSON *json, const char *tag, const char *expected)
{
	ACL_JSON_NODE *node = acl_json_getFirstElementByTagName(json, tag);

	if (node == NULL) {
		printf("missing tag: %s\n", tag);
		return 0;
	}

	if (node->text == NULL || strcmp(STR(node->text), expected) != 0) {
		printf("tag %s mismatch: got=[%s] expected=[%s]\n", tag,
			node->text ? STR(node->text) : "", expected);
		return 0;
	}

	return 1;
}

static int test_full_input(void)
{
	ACL_JSON *json = acl_json_alloc();
	const char *data =
		"{\"text\":\"\\u4F60\\u597D\","
		"\"emoji\":\"\\uD83D\\uDE00\","
		"\"key\\u4F60\":\"v\\u597D\"}";
	int ok;

	acl_json_update(json, data);
	ok = acl_json_finish(json)
		&& check_tag_value(json, "text", UTF8_NI_HAO)
		&& check_tag_value(json, "emoji", UTF8_EMOJI)
		&& check_tag_value(json, UTF8_KEY_NI, UTF8_V_HAO);

	acl_json_free(json);
	return ok;
}

static int test_stream_input(void)
{
	ACL_JSON *json = acl_json_alloc();
	int ok;

	acl_json_update(json, "{\"split\":\"\\u4F");
	acl_json_update(json, "60\\u597");
	acl_json_update(json, "D\",\"emoji\":\"\\uD83D");
	acl_json_update(json, "\\uDE00\"}");

	ok = acl_json_finish(json)
		&& check_tag_value(json, "split", UTF8_NI_HAO)
		&& check_tag_value(json, "emoji", UTF8_EMOJI);

	acl_json_free(json);
	return ok;
}

static int test_invalid_hex_input(void)
{
	ACL_JSON *json = acl_json_alloc();
	const char *data = "{\"bad\":\"A\\u12G4B\"}";
	const char *expect = "A" UTF8_REPLACEMENT "G4B";
	int ok;

	acl_json_update(json, data);
	ok = acl_json_finish(json)
		&& check_tag_value(json, "bad", expect);

	acl_json_free(json);
	return ok;
}

static int test_invalid_surrogate_input(void)
{
	ACL_JSON *json = acl_json_alloc();
	const char *data =
		"{\"high_only\":\"\\uD83D!\","
		"\"high_bad_low\":\"\\uD83D\\u0041\","
		"\"high_bad_u\":\"\\uD83D\\x\"}";
	const char *expect_high_only = UTF8_REPLACEMENT "!";
	const char *expect_high_bad_low = UTF8_REPLACEMENT "A";
	const char *expect_high_bad_u = UTF8_REPLACEMENT "x";
	int ok;

	acl_json_update(json, data);
	ok = acl_json_finish(json)
		&& check_tag_value(json, "high_only", expect_high_only)
		&& check_tag_value(json, "high_bad_low", expect_high_bad_low)
		&& check_tag_value(json, "high_bad_u", expect_high_bad_u);

	acl_json_free(json);
	return ok;
}

int main(void)
{
	if (!test_full_input()) {
		printf("test_full_input failed\n");
		return 1;
	}

	if (!test_stream_input()) {
		printf("test_stream_input failed\n");
		return 1;
	}

	if (!test_invalid_hex_input()) {
		printf("test_invalid_hex_input failed\n");
		return 1;
	}

	if (!test_invalid_surrogate_input()) {
		printf("test_invalid_surrogate_input failed\n");
		return 1;
	}

	printf("json unicode checks passed\n");
	return 0;
}