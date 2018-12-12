/**
 * writen by yan.zhang
 */

#include "stdafx.h"
#include "charset.h"
#include "chinese_utf8.h"

static inline int utf8_len(char *buf)
{
	unsigned char *ptr;
	int ret;
	ptr = (unsigned char *)buf;
	if (((*ptr) <= 0x7F))
		ret = 1;
	else if (((*ptr) & 0xF0) == 0xF0)
		ret = 4;
	else if (((*ptr) & 0xE0) == 0xE0)
		ret = 3;
	else if (((*ptr) & 0xC0) == 0xC0)
		ret = 2;
	else
		ret = 5;

	return ret;
}

static inline int chinese_word_find(unsigned char *word)
{
	int start = 0, middle, end = chinese_utf8_list_count - 1;
	unsigned int mint, wint;
	unsigned char *wp;

	wint = (word[0] << 16) | (word[1] << 8) | (word[2]);

	while (1)
	{
		if (start > end)
			return 0;

		middle = (start + end) / 2;
		wp = chinese_utf8_list + middle * 3;
		mint = (wp[0] << 16) | (wp[1] << 8) | (wp[2]);

		if (wint < mint)
			end = middle - 1;
		else if (mint < wint)
			start = middle + 1;
		else
			return 1;
	}

	return 0;
}

static int chinese_word_count(char *str, int len)
{
	int i = 0, ulen, count = 0;

	while (i + 2 < len)
	{
		ulen = utf8_len(str + i);
		if (ulen != 3)
		{
			i += ulen;
			continue;
		}

		count += chinese_word_find((unsigned char *) str + i);
		i += ulen;
	}

	return count;
}

#define  detact_debug(fmt, args...) { if (debug_mode_) { \
	printf(fmt, ##args); \
} }

charset_radar::charset_radar(void)
: debug_mode_(false)
{
}

charset_radar::~charset_radar(void)
{
}

bool charset_radar::detact(const char *data, int len, acl::string &result_charset)
{
	typedef struct {
		int group_id; /* 字符集分组id */
		const char *charset;
		int result_len;
	} eas_charset;

	eas_charset eas_cs[] = {
		{1, "UTF-8", 0},
		{2, "GB18030", 0},
		{2, "BIG5", 0},
		{0, 0, 0}
	};
	eas_charset *csp, *csp_find = 0;
	acl::charset_conv conv;
	acl::string data2;
	acl::string toCharset;
	int max_len = 0;
	int count = 0;
	int w_count;
	int w_count_max = 0;
	const char *result_cc = 0;

	if (len < 2)
		return false;

	for (csp = eas_cs; csp->group_id; csp++)
	{
		conv.reset();
		conv.set_add_invalid(false);
		data2 = "";

		toCharset = csp->charset;
		toCharset += "//IGNORE";
		if (!conv.convert(csp->charset, toCharset.c_str(), data, len, &data2))
			continue;

		csp->result_len = data2.length();
		detact_debug("%-10s:\t%d\n", csp->charset, csp->result_len);

		if(csp->result_len > max_len)
		{
			max_len = csp->result_len;
			csp_find = csp;
		}
		else if (csp->result_len == max_len && csp_find
			&& csp->group_id != csp_find->group_id)
		{
			result_charset = "UTF-8";
			return true;
		}
	}

	if (!csp_find)
	{
		result_charset = "UTF-8";
		return true;
	}

	for (csp = eas_cs; csp->group_id; csp++)
	{
		if (csp_find->group_id != csp->group_id)
			continue;
		if (csp_find->result_len != csp->result_len)
			continue;
		count++;
	}

	detact_debug("count: %d\n", count);

	if (count==1)
	{
		result_charset = csp_find->charset;
		return true;
	}

#if 1
	if (csp_find->group_id !=2 )
	{

		result_charset = csp_find->charset;
		return true;
	}
#endif
	for (csp = eas_cs; csp->group_id; csp++)
	{
		if (csp_find->group_id != csp->group_id)
			continue;

		if (csp_find->result_len != csp->result_len)
			continue;

		conv.reset();
		conv.set_add_invalid(false);
		data2 = "";
		if (!conv.convert(csp->charset, "UTF-8//IGNORE", data, len, &data2))
			continue;

		w_count = chinese_word_count(data2.c_str(), data2.length());
		detact_debug("%s, %zd, %d\n", csp->charset, data2.length(), w_count);
		if ((w_count > w_count_max))
		{
			w_count_max = w_count;
			result_cc = csp->charset;
		}
	}

	if (!result_cc)
		return false;

	result_charset = result_cc;
	return true;
}

bool charset_radar::detact(const acl::string &data, acl::string &result_charset)
{
    return detact(data.c_str(), data.length(), result_charset);
}
