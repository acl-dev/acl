#include "stdafx.h"
#include "file_copy.h"
#include "http_creator.h"

static const char* src_path_ = "tmpl/http";
static const char* dst_path_ = "http_demo";

//////////////////////////////////////////////////////////////////////////

static void set_charset(tpl_t* tpl)
{
	const char* charset;
	printf("select charset: 1: utf-8; 2: gb2312; 3: gbk; 4: gb18030; 5: big5 ?[1]");
	fflush(stdout);

	int ch = getchar();
	switch (ch)
	{
	case '2':
		charset = "gb2312";
		break;
	case '3':
		charset = "gbk";
		break;
	case '4':
		charset = "gb18030";
		break;
	case '5':
		charset = "big5";
		break;
	case '1':
	default:
		charset = "utf-8";
		break;
	}

	tpl_set_field_fmt_global(tpl, "CHARSET", "%s", charset);
}

static void set_memcached(tpl_t* tpl)
{
	printf("Please enter memcached addr[127.0.0.1:11211]: ");
	fflush(stdout);

	char buf[256];
	int n = acl_vstream_gets_nonl(ACL_VSTREAM_IN, buf, sizeof(buf));
	if (n == ACL_VSTREAM_EOF || n == 0)
		snprintf(buf, sizeof(buf), "127.0.0.1:11211");
	tpl_set_field_fmt_global(tpl, "MEMCACHED_ADDR", "%s", buf);
}

static bool set_cookies(tpl_t* tpl)
{
	printf("Please enter cookie name: "); fflush(stdout);
	char line[256];
	int n = acl_vstream_gets_nonl(ACL_VSTREAM_IN, line, sizeof(line));
	if (n == ACL_VSTREAM_EOF)
	{
		printf("enter cookie name error!\r\n");
		return false;
	}

	string buf;
	buf.format("const char* %s = req.getCookieValue(\"%s\");\r\n", line, line);
	buf.format_append("\tif (%s == NULL)\r\n\t\tres.addCookie(\"%s\", \"{xxx}\");", line, line);

	tpl_set_field_fmt_global(tpl, "GET_COOKIES", "%s", buf.c_str());
	return true;
}

static tpl_t* open_tpl(const char* filename)
{
	tpl_t* tpl = tpl_alloc();
	string filepath;
	filepath.format("%s/%s", src_path_, filename);
	if (tpl_load(tpl, filepath.c_str()) != TPL_OK)
	{
		printf("load file %s error: %s\r\n",
			filepath.c_str(), last_serror());
		tpl_free(tpl);
		return NULL;
	}
	return tpl;
}

//////////////////////////////////////////////////////////////////////////

static bool create_http_request()
{
	return true;
}

static  bool create_http_response()
{
	return true;
}

static bool create_http_servlet()
{
	tpl_t* tpl = open_tpl("http_servlet.cpp");
	if (tpl == NULL)
		return false;

	set_charset(tpl);
	set_memcached(tpl);

	printf("Do you want add cookie? y[y/n]: "); fflush(stdout);
	int ch = getchar();
	if (ch != 'n')
		set_cookies(tpl);

	string filepath;
	filepath.format("%s/http_servlet.cpp", dst_path_);
	if (tpl_save_as(tpl, filepath.c_str()) != TPL_OK)
	{
		printf("save to %s error: %s\r\n", filepath.c_str(),
			last_serror());
		tpl_free(tpl);
		return false;
	}

	printf("create %s ok.\r\n", filepath.c_str());
	tpl_free(tpl);
	return true;
}

void http_creator()
{
	acl_make_dirs(dst_path_, 0700);

	while (true)
	{
		printf("please choose one http application type:\r\n");
		printf("c: for http request; r: for http response; s: http servlet\r\n");
		printf(">"); fflush(stdout);

		char buf[256];
		int  n = acl_vstream_gets_nonl(ACL_VSTREAM_IN, buf, sizeof(buf));
		if (n == ACL_VSTREAM_EOF)
			break;
		else if (strcasecmp(buf, "c") == 0)
			create_http_request();
		else if (strcasecmp(buf, "r") == 0)
			create_http_response();
		else if (strcasecmp(buf, "s") == 0)
			create_http_servlet();
		else
			printf("unknown flag: %s\r\n", buf);
		break;
	}
}
