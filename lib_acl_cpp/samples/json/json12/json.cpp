#include "stdafx.h"

static void test_type(acl::json& json, const char* name)
{
	const std::vector<acl::json_node*>& nodes =
		json.getElementsByTagName(name);
	if (nodes.empty())
	{
		printf("%s not found\r\n", name);
		return;
	}

	std::vector<acl::json_node*>::const_iterator cit;
	for (cit = nodes.begin(); cit != nodes.end(); ++cit)
	{
		printf("%s: type is %s, value is %s\r\n", name,
			(*cit)->get_type(),
			(*cit)->get_text() ? (*cit)->get_text() : "NULL");
	}
}

static bool test(const char* in, bool once, const char* name)
{
	acl::json json;
	const char* ptr = NULL, *p1 = in;
	char  buf[2];
       
	if (once)
		ptr = json.update(p1);
	else
	{
		while (*p1)
		{
			buf[0] = *p1;
			buf[1] = 0;
			ptr = json.update(buf);
			p1++;
		}
	}

	printf("------------------  input json ------------------------\r\n");
	printf("%s\r\n", in);
	printf("-------------------------------------------------------\r\n");

	printf("json finish: %s, left: |%s|, len: %d\r\n",
		json.finish() ? "yes" : "no", ptr, (int) strlen(ptr));

	printf("------------------ rebuild json -----------------------\r\n");
	printf("%s\r\n", json.to_string().c_str());
	printf("-------------------------------------------------------\r\n");

	acl::string ibuf(in);
	ibuf.trim_space().trim_right_line();

	acl::string obuf = json.to_string();
	obuf.trim_space();

	if (ibuf == obuf)
		printf("\r\n-----OK----\r\n\r\n");
	else
		printf("\r\n-----Error----\r\n\r\n");

	if (name && *name)
		test_type(json, name);

	return true;
}

static void usage(const char* procname)
{
	printf("usage: %s -h [help]\r\n"
		"  -o [if parsing once]\r\n"
		"  -n name\r\n"
		"  -f json_file\r\n", procname);
}

int main(int argc, char* argv[])
{
	acl::string filepath, name;
	int  ch;
	bool once = false;

	while ((ch = getopt(argc, argv, "hf:on:")) > 0)
	{
		switch (ch)
		{
		case 'h':
			usage(argv[0]);
			return 0;
		case 'f':
			filepath = optarg;
			break;
		case 'o':
			once = true;
			break;
		case 'n':
			name = optarg;
			break;
		default:
			break;
		}
	}

	if (filepath.empty())
	{
		usage(argv[0]);
		return 0;
	}

	acl::string buf;

	if (acl::ifstream::load(filepath, &buf) == false)
	{
		printf("load %s error %s\r\n", filepath.c_str(),
			acl::last_serror());
		return 0;
	}

	if (test(buf, once, name) == false)
	{
		printf("test json %s error\r\n", filepath.c_str());
		return 1;
	}

	printf("test ALL OK\r\n");

	return 0;
}
